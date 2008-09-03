/*
 * icqLoginManager.cpp - login manager for an icq connection
 * Copyright (C) 2008  Alexander Saltykov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "icqLoginManager.h"
#include "icqRateManager.h"
#include "icqSsiManager.h"
#include "icqTlv.h"

#include <QCryptographicHash>
#include <QString>

#include <QtDebug>

class ICQ::LoginManager::Private
{
	public:
		enum loginStage { stageAuth, stageProtocolNegotiation, stageServicesSetup, stageFinal };
		QByteArray authcookie;
		QString uin;
		QString password;

		ICQ::Connection *link;
		loginStage loginStage;
};

ICQ::LoginManager::LoginManager(Connection* parent)
	: QObject(parent)
{
	d = new Private;
	d->link = parent;

	QObject::connect( d->link, SIGNAL( incomingFlap(ICQ::FlapBuffer&) ), this, SLOT ( incomingFlap(ICQ::FlapBuffer&) ) );
	QObject::connect( d->link, SIGNAL( incomingSnac(ICQ::SnacBuffer&) ), this, SLOT ( incomingSnac(ICQ::SnacBuffer&) ) );
	QObject::connect( this, SIGNAL( loginFinished() ), d->link, SLOT( slot_signedOn() ) );
}

ICQ::LoginManager::~LoginManager()
{
	delete d;
}

void ICQ::LoginManager::login(QString& uin, QString& password, QString& server)
{
	quint16 port = 5190;
	d->uin = uin;
	d->password = password;

	d->link->connectToHost(server, port);
	d->loginStage = Private::stageAuth;
}

void ICQ::LoginManager::recv_flap_version(FlapBuffer& reply)
{
	DWord flapVersion = reply.getDWord();
	if ( flapVersion != 1 ) {
		qCritical() << "[Critical ERROR] not an ICQ";
	}
}

void ICQ::LoginManager::send_flap_version()
{
	FlapBuffer flap(FlapBuffer::AuthChannel);
	flap.addDWord(0x1);
	d->link->write(flap);
}

/* >> SNAC (17,06) - CLI_AUTH_KEY_REQUEST */
void ICQ::LoginManager::send_cli_auth_request()
{
	SnacBuffer snac(ICQ::sfAuth, 0x06);
	snac.addTlv( (Tlv)Tlv(0x01).addData(d->uin) );
	d->link->write(snac);
}

/* << SNAC (17,07) - SRV_AUTH_KEY_RESPONSE
 * >> SNAC (17,02) - CLI_MD5_LOGIN */
void ICQ::LoginManager::recv_auth_key(SnacBuffer& reply)
{
	Word keylen = reply.getWord();
	QByteArray authkey = reply.read(keylen);

	SnacBuffer snac(ICQ::sfAuth, 0x02);

	snac.addTlv(0x01, d->uin);
	snac.addTlv( 0x03, QLatin1String("ICQBasic") );
	snac.addTlv( 0x25, md5password(authkey) );
	snac.addTlv( (Tlv)Tlv(0x16).addWord(0x010B) );

	d->link->write(snac);
}

/* << SNAC (17,03) - SRV_LOGIN_REPLY */
void ICQ::LoginManager::recv_auth_reply(SnacBuffer& reply)
{
	/* TODO: Check for errors (TLV 0x08) */
	TlvChain list = reply;
	reply.seekEnd();

	if ( list.hasTlv(0x08) ) {
		qCritical() << "[ICQ::LoginManager] Error during authentication:" << list.getTlvData(0x08).toHex();
		d->link->signOff();
		return;
	}

	d->authcookie = list.getTlvData(0x06); // tlv auth cookie

	QByteArray bosaddr = list.getTlvData(0x05); // tlv bos addr:port

	int pos = bosaddr.indexOf(':');
	QString server = bosaddr.left(pos);
	int port = bosaddr.right(bosaddr.length()-pos-1).toUInt();

	d->link->disconnectFromHost();
	d->loginStage = Private::stageProtocolNegotiation;
	d->link->connectToHost(QHostAddress(server), port);
	d->link->startConnectionTimer();
}

void ICQ::LoginManager::send_cli_auth_cookie()
{
	FlapBuffer flap(FlapBuffer::AuthChannel);
	flap.addDWord(0x1);
	flap.addTlv(0x06, d->authcookie);
	d->link->write(flap);
}

/* << SNAC(01,03) - SRV_FAMILIES
 * >> SNAC(01,17) - CLI_FAMILIES_VERSIONS */
void ICQ::LoginManager::recv_snac_list(SnacBuffer& reply)
{
	reply.seekEnd();

	/* send out snac(01,17) - CLI_FAMILIES_VERSIONS - client services and version */
	SnacBuffer snac(ICQ::sfGeneric, 0x17);

	snac.addWord(0x0001).addWord(0x0004);
	snac.addWord(0x0002).addWord(0x0001);
	snac.addWord(0x0003).addWord(0x0001);
	snac.addWord(0x0004).addWord(0x0001);
	snac.addWord(0x0009).addWord(0x0001);
	snac.addWord(0x0013).addWord(0x0005);
	snac.addWord(0x0015).addWord(0x0002);

	d->link->write(snac);
}

/* << SNAC(01,18) - SRV_FAMILIES_VERSIONS
 * >> SNAC(01,06) - CLI_RATES_REQUEST
 * >> SNAC(02,02) - CLI_LOCATION_RIGHTS_REQ
 * >> SNAC(03,02) - CLI_BUDDYLIST_RIGHTS_REQ
 * >> SNAC(04,04) - CLI_ICBM_PARAM_REQ
 * >> SNAC(09,02) - CLI_PRIVACY_RIGHTS_REQ */
void ICQ::LoginManager::recv_snac_versions(SnacBuffer& reply)
{
	reply.seekEnd(); // we've got it.

	/* send out snac(01,06) - CLI_RATES_REQUEST */
	d->link->rateManager()->requestRates();

	d->link->snacRequest(0x02, 0x02);
	d->link->snacRequest(0x03, 0x02);
	d->link->snacRequest(0x04, 0x04);
	d->link->snacRequest(0x09, 0x02);

	d->link->ssiManager()->requestParameters();
	d->link->ssiManager()->checkContactList();
}

/* << SNAC(02,03) - SRV_LOCATION_RIGHTS_REPLY
 * >> SNAC(02,04) - CLI_SET_LOCATION_INFO */
void ICQ::LoginManager::recv_location_services_limits(SnacBuffer& reply)
{
	reply.seekEnd();

	SnacBuffer snac(ICQ::sfLocation, 0x04);
	Tlv tlv(0x05);
	tlv.addData( ICQ::Capabilities[ICQ::ccICQDirectConnect] );
	tlv.addData( ICQ::Capabilities[ICQ::ccICQServerRelay] );
	tlv.addData( ICQ::Capabilities[ICQ::ccUTF8Messages] );
	tlv.addData( ICQ::Capabilities[ICQ::ccRTFMessages] );

	snac.addTlv(tlv); // 0x05 - clsid values

	/* send SNAC(02,04) */
	d->link->write(snac);

	login_final_actions();
}

/* << SNAC(03,03) - SRV_BUDDYLIST_RIGHTS_REPLY */
void ICQ::LoginManager::recv_buddy_list_parameters(SnacBuffer& reply)
{
	reply.seekEnd();
}

/* << SNAC(04,05) - SRV_ICBM_PARAMS
 * >> SNAC(04,02) - CLI_SET_ICBM_PARAMS */
void ICQ::LoginManager::recv_icbm_parameters(SnacBuffer& reply)
{
	// TODO: SNAC (04,01) - SRV_ICBM_ERROR handling
	Word channel = reply.getWord();
	DWord msgFlags = reply.getDWord();

	Word maxMsgSnacSize = reply.getWord();
	Word maxSenderWarningLevel = reply.getWord();
	Word maxReceiverWarningLevel = reply.getWord();
	Word minMsgInterval = reply.getWord();
	Word unknown = reply.getWord();

	channel = 0;
	msgFlags = 0x3;
	maxMsgSnacSize = 8000;
	maxSenderWarningLevel = 999;
	maxReceiverWarningLevel = 999;
	minMsgInterval = 0;
	unknown = 0;

	/* Send snac(04,02) */
	SnacBuffer snac(ICQ::sfICBM, 0x02);
	snac.addWord(channel);
	snac.addDWord(msgFlags);
	snac.addWord(maxMsgSnacSize);
	snac.addWord(maxSenderWarningLevel);
	snac.addWord(maxReceiverWarningLevel);
	snac.addWord(minMsgInterval);
	snac.addWord(unknown);

	d->link->write(snac);
}

/* << SNAC(09,03) - SRV_PRIVACY_RIGHTS_REPLY */
void ICQ::LoginManager::recv_privacy_parameters(SnacBuffer& reply)
{
	TlvChain list = reply;
	reply.seekEnd();

	Word maxVisibleList = list.getTlv(0x01).getWord();
	Word maxInvisibleList = list.getTlv(0x02).getWord();

	Q_UNUSED(maxVisibleList)
	Q_UNUSED(maxInvisibleList)
}

/* >> SNAC(01,1E) - CLI_SETxSTATUS
 * >> SNAC(01,02) - CLI_READY */
void ICQ::LoginManager::login_final_actions()
{
	emit loginFinished();

	SnacBuffer snac(ICQ::sfGeneric, 0x02);

	snac.addWord(0x0001).addWord(0x0004).addWord(0x0110).addWord(0x1246);
	snac.addWord(0x0002).addWord(0x0001).addWord(0x0110).addWord(0x1246);
	snac.addWord(0x0003).addWord(0x0001).addWord(0x0110).addWord(0x1246);
	snac.addWord(0x0004).addWord(0x0001).addWord(0x0110).addWord(0x1246);
	snac.addWord(0x0009).addWord(0x0001).addWord(0x0110).addWord(0x1246);
	snac.addWord(0x0013).addWord(0x0005).addWord(0x0110).addWord(0x1246);
	snac.addWord(0x0015).addWord(0x0001).addWord(0x0110).addWord(0x1246);

	d->link->write(snac);
}

QByteArray ICQ::LoginManager::md5password(const QByteArray& AuthKey)
{
	QCryptographicHash md5hash(QCryptographicHash::Md5);

	md5hash.addData(AuthKey);
	md5hash.addData( d->password.toLocal8Bit() );
	md5hash.addData( (char*)ICQ::AIM_MD5_STRING, qstrlen(ICQ::AIM_MD5_STRING) );
	return md5hash.result();
}

void ICQ::LoginManager::incomingFlap(ICQ::FlapBuffer& flap)
{
	if ( flap.channel() == FlapBuffer::AuthChannel ) {
		recv_flap_version(flap);
		if ( d->loginStage == Private::stageAuth ) {
			send_flap_version();
			send_cli_auth_request();
		} else if ( d->loginStage == Private::stageProtocolNegotiation ) {
			send_cli_auth_cookie();
		}
		return;
	}
}

void ICQ::LoginManager::incomingSnac(ICQ::SnacBuffer& snac)
{
	if ( snac.family() == ICQ::sfAuth && snac.subtype() == 0x07 ) {
		recv_auth_key(snac);
		return;
	}
	if ( snac.family() == ICQ::sfAuth && snac.subtype() == 0x03 ) {
		recv_auth_reply(snac);
		return;
	}
	if ( snac.family() == ICQ::sfGeneric && snac.subtype() == 0x03 ) {
		recv_snac_list(snac);
		return;
	}
	if ( snac.family() == ICQ::sfGeneric && snac.subtype() == 0x18 ) {
		recv_snac_versions(snac);
		return;
	}
	if ( snac.family() == ICQ::sfLocation && snac.subtype() == 0x03 ) {
		recv_location_services_limits(snac);
		return;
	}
	if ( snac.family() == ICQ::sfBLM && snac.subtype() == 0x03 ) {
		recv_buddy_list_parameters(snac);
		return;
	}
	if ( snac.family() == ICQ::sfICBM && snac.subtype() == 0x05 ) {
		recv_icbm_parameters(snac);
		return;
	}
	if ( snac.family() == ICQ::sfPrivacyManagement && snac.subtype() == 0x03 ) {
		recv_privacy_parameters(snac);
		return;
	}
}
