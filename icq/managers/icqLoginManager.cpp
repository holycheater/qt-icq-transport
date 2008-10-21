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

#include "icqSession.h"
#include "icqSocket.h"

#include "icqRateManager.h"
#include "icqSsiManager.h"

#include "types/icqFlapBuffer.h"
#include "types/icqSnacBuffer.h"
#include "types/icqTlv.h"

#include <QCryptographicHash>
#include <QString>

#include <QtDebug>

namespace ICQ {


class LoginManager::Private
{
	public:
		static QString authErrorString(Word code);

		enum loginStage { stageAuth, stageProtocolNegotiation, stageServicesSetup, stageFinal };
		QByteArray authcookie;
		QString uin;
		QString password;

		Session *session;
		Socket *socket;
		loginStage loginStage;

		struct AuthErrorDesc {
			Word code;
			QString desc;
		};
		static AuthErrorDesc AuthErrorDescTable[];
};

LoginManager::Private::AuthErrorDesc LoginManager::Private::AuthErrorDescTable[] = {
	{ 0x0001, tr("Invalid nick or password") },
	{ 0x0002, tr("Service temporary unavailable") },
	{ 0x0003, tr("Generic error") },
	{ 0x0004, tr("Incorrect nickname or password") },
	{ 0x0005, tr("Nickname/password mismatch") },
	{ 0x0006, tr("Internal client error (bad input to authorizer)") },
	{ 0x0007, tr("Invalid account") },
	{ 0x0008, tr("Deleted account") },
	{ 0x0009, tr("Expired account") },
	{ 0x000A, tr("No access to database") },
	{ 0x000B, tr("No access to resolver") },
	{ 0x000C, tr("Invalid database fields") },
	{ 0x000D, tr("Bad database status") },
	{ 0x000E, tr("Bad resolver status") },
	{ 0x000F, tr("Internal error") },
	{ 0x0010, tr("Service temporarily offline") },
	{ 0x0011, tr("Suspended account") },
	{ 0x0012, tr("DB send error") },
	{ 0x0013, tr("DB link error") },
	{ 0x0014, tr("Reservation map error") },
	{ 0x0015, tr("Reservation link error") },
	{ 0x0016, tr("Number of users connected from this IP address has reached the maximum") },
	{ 0x0017, tr("Number of users connected from this IP address has reached the maximum (reservation)") },
	{ 0x0018, tr("Rate limit exceeded (reservation). Please try to reconnect in a few minutes") },
	{ 0x0019, tr("User too heavily warned") },
	{ 0x001A, tr("Reservation timeout") },
	{ 0x001B, tr("You are using an older version of ICQ. Upgrade required") },
	{ 0x001C, tr("You are using an older version of ICQ. Upgrade recommended") },
	{ 0x001D, tr("Rate limit exceeded. Please try to reconnect in a few minutes") },
	{ 0x001E, tr("Can't register on the ICQ network. Reconnect in a few minutes") },
	{ 0x0020, tr("Invalid SecurID") },
	{ 0x0022, tr("Account suspended because of your age (age < 13)") },
	{ 0, "" }
};

QString LoginManager::Private::authErrorString(Word code)
{
	for (int i = 0; !AuthErrorDescTable[i].desc.isEmpty(); ++i) {
		if (AuthErrorDescTable[i].code == code) {
			return AuthErrorDescTable[i].desc;
		}
	}
	return tr("Unknown error");
}

LoginManager::LoginManager(Session* sess)
	: QObject(sess)
{
	d = new Private;
	d->loginStage = Private::stageAuth;

	d->session = sess;
	d->socket = 0;
}

LoginManager::~LoginManager()
{
	delete d;
}

void LoginManager::setSocket(Socket *socket)
{
	d->socket = socket;
	QObject::connect( d->socket, SIGNAL( incomingFlap(FlapBuffer&) ), SLOT ( incomingFlap(FlapBuffer&) ) );
	QObject::connect( d->socket, SIGNAL( incomingSnac(SnacBuffer&) ), SLOT ( incomingSnac(SnacBuffer&) ) );
}

void LoginManager::setUsername(const QString& uin)
{
	d->uin = uin;
}

void LoginManager::setPassword(const QString& password)
{
	d->password = password;
}

void LoginManager::recv_flap_version(FlapBuffer& reply)
{
	DWord flapVersion = reply.getDWord();
	if ( flapVersion != 1 ) {
		qCritical() << "[Critical ERROR] not an ICQ";
		emit error( tr("Critical Error during authentication: The server you are trying to connect to seems to be not an ICQ server") );
		d->session->disconnect();
	}
}

void LoginManager::send_flap_version()
{
	FlapBuffer flap(FlapBuffer::AuthChannel);
	flap.addDWord(0x1);
	d->socket->write(flap);
}

/* >> SNAC (17,06) - CLI_AUTH_KEY_REQUEST */
void LoginManager::send_cli_auth_request()
{
	SnacBuffer snac(0x17, 0x06);
	snac.addTlv( (Tlv)Tlv(0x01).addData(d->uin) );
	d->socket->write(snac);
}

/* << SNAC (17,07) - SRV_AUTH_KEY_RESPONSE
 * >> SNAC (17,02) - CLI_MD5_LOGIN */
void LoginManager::recv_auth_key(SnacBuffer& reply)
{
	Word keylen = reply.getWord();
	QByteArray authkey = reply.read(keylen);

	SnacBuffer snac(0x17, 0x02);

	snac.addTlv( 0x01, d->uin );
	snac.addTlv( 0x25, md5password(authkey) );
	snac.addTlv( 0x03, QLatin1String("ICQBasic") );
	snac.addTlv( (Tlv)Tlv(0x16).addWord(0x010B) ); // client id
	snac.addTlv( (Tlv)Tlv(0x17).addWord(0x14) ); // client major version
	snac.addTlv( (Tlv)Tlv(0x18).addWord(0x22) ); // client minor version
	snac.addTlv( (Tlv)Tlv(0x19).addWord(0x01) ); // client lesser version
	snac.addTlv( (Tlv)Tlv(0x1A).addWord(0xFFFF) ); // client build number
	snac.addTlv( (Tlv)Tlv(0x14).addDWord(0x666) ); // distribution number
	snac.addTlv( 0x0F, QLatin1String("en") ); // client language
	snac.addTlv( 0x0E, QLatin1String("us") ); // client country
	snac.addTlv( (Tlv)Tlv(0x4A).addByte(0x1) ); // SSI use flag

	d->socket->write(snac);
}

/* << SNAC (17,03) - SRV_LOGIN_REPLY */
void LoginManager::recv_auth_reply(SnacBuffer& reply)
{
	TlvChain list = reply;
	reply.seekEnd();

	if ( list.hasTlv(0x08) ) {
		Word code = list.getTlvData(0x08).toHex().toUInt(NULL, 16);
		qCritical() << "[LoginManager] Error during authentication:" << list.getTlvData(0x08).toHex();
		QString desc = tr("Authentication failure.") + " " + tr("Code: ") + "0x" + QString::number(code, 16).rightJustified(4, '0') + ". " + Private::authErrorString(code);
		emit error(desc);
		d->session->disconnect();
		return;
	}

	d->authcookie = list.getTlvData(0x06); // tlv auth cookie

	QByteArray bosaddr = list.getTlvData(0x05); // tlv bos addr:port

	int pos = bosaddr.indexOf(':');
	QString server = bosaddr.left(pos);
	quint16 port = bosaddr.right(bosaddr.length()-pos-1).toUInt();

	d->loginStage = Private::stageProtocolNegotiation;
	emit serverAvailable(server, port);
}

void LoginManager::send_cli_auth_cookie()
{
	FlapBuffer flap(FlapBuffer::AuthChannel);
	flap.addDWord(0x1);
	flap.addTlv(0x06, d->authcookie);
	d->socket->write(flap);
}

/* << SNAC(01,03) - SRV_FAMILIES
 * >> SNAC(01,17) - CLI_FAMILIES_VERSIONS */
void LoginManager::recv_snac_list(SnacBuffer& reply)
{
	reply.seekEnd();

	/* send out snac(01,17) - CLI_FAMILIES_VERSIONS - client services and version */
	SnacBuffer snac(0x01, 0x17);

	snac.addWord(0x0001).addWord(0x0004);
	snac.addWord(0x0002).addWord(0x0001);
	snac.addWord(0x0003).addWord(0x0001);
	snac.addWord(0x0004).addWord(0x0001);
	snac.addWord(0x0009).addWord(0x0001);
	snac.addWord(0x0013).addWord(0x0005);
	snac.addWord(0x0015).addWord(0x0002);

	d->socket->write(snac);
}

/* << SNAC(01,18) - SRV_FAMILIES_VERSIONS
 * >> SNAC(01,06) - CLI_RATES_REQUEST
 * >> SNAC(02,02) - CLI_LOCATION_RIGHTS_REQ
 * >> SNAC(03,02) - CLI_BUDDYLIST_RIGHTS_REQ
 * >> SNAC(04,04) - CLI_ICBM_PARAM_REQ
 * >> SNAC(09,02) - CLI_PRIVACY_RIGHTS_REQ */
void LoginManager::recv_snac_versions(SnacBuffer& reply)
{
	reply.seekEnd(); // we've got it.

	/* send out snac(01,06) - CLI_RATES_REQUEST */
	emit ratesRequest();

	d->socket->snacRequest(0x02, 0x02);
	d->socket->snacRequest(0x03, 0x02);
	d->socket->snacRequest(0x04, 0x04);
	d->socket->snacRequest(0x09, 0x02);
}

/* << SNAC(02,03) - SRV_LOCATION_RIGHTS_REPLY
 * >> SNAC(02,04) - CLI_SET_LOCATION_INFO */
void LoginManager::recv_location_services_limits(SnacBuffer& reply)
{
	reply.seekEnd();

	SnacBuffer snac(0x02, 0x04);
	Tlv tlv(0x05);
	tlv.addData( Capabilities[ccICQDirectConnect] );
	tlv.addData( Capabilities[ccICQServerRelay] );
	tlv.addData( Capabilities[ccUTF8Messages] );
	tlv.addData( Capabilities[ccRTFMessages] );

	snac.addTlv(tlv); // 0x05 - clsid values

	/* send SNAC(02,04) */
	d->socket->write(snac);
}

/* << SNAC(03,03) - SRV_BUDDYLIST_RIGHTS_REPLY */
void LoginManager::recv_buddy_list_parameters(SnacBuffer& reply)
{
	reply.seekEnd();
}

/* << SNAC(04,05) - SRV_ICBM_PARAMS
 * >> SNAC(04,02) - CLI_SET_ICBM_PARAMS */
void LoginManager::recv_icbm_parameters(SnacBuffer& reply)
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
	SnacBuffer snac(0x04, 0x02);
	snac.addWord(channel);
	snac.addDWord(msgFlags);
	snac.addWord(maxMsgSnacSize);
	snac.addWord(maxSenderWarningLevel);
	snac.addWord(maxReceiverWarningLevel);
	snac.addWord(minMsgInterval);
	snac.addWord(unknown);

	d->socket->write(snac);
}

/* << SNAC(09,03) - SRV_PRIVACY_RIGHTS_REPLY */
void LoginManager::recv_privacy_parameters(SnacBuffer& reply)
{
	TlvChain list = reply;
	reply.seekEnd();

	Word maxVisibleList = list.getTlv(0x01).getWord();
	Word maxInvisibleList = list.getTlv(0x02).getWord();

	Q_UNUSED(maxVisibleList)
	Q_UNUSED(maxInvisibleList)

	emit ssiRequest();
	login_final_actions();
}

/* >> SNAC(01,1E) - CLI_SETxSTATUS
 * >> SNAC(01,02) - CLI_READY */
void LoginManager::login_final_actions()
{
	emit finished();

	SnacBuffer snac(0x01, 0x02);

	snac.addWord(0x0001).addWord(0x0004).addWord(0x0110).addWord(0x1246);
	snac.addWord(0x0002).addWord(0x0001).addWord(0x0110).addWord(0x1246);
	snac.addWord(0x0003).addWord(0x0001).addWord(0x0110).addWord(0x1246);
	snac.addWord(0x0004).addWord(0x0001).addWord(0x0110).addWord(0x1246);
	snac.addWord(0x0009).addWord(0x0001).addWord(0x0110).addWord(0x1246);
	snac.addWord(0x0013).addWord(0x0005).addWord(0x0110).addWord(0x1246);
	snac.addWord(0x0015).addWord(0x0001).addWord(0x0110).addWord(0x1246);

	d->socket->write(snac);
}

QByteArray LoginManager::md5password(const QByteArray& AuthKey)
{
	QCryptographicHash md5hash(QCryptographicHash::Md5);

	md5hash.addData(AuthKey);
	md5hash.addData( d->password.toLocal8Bit() );
	md5hash.addData( (char*)AIM_MD5_STRING, qstrlen(AIM_MD5_STRING) );
	return md5hash.result();
}

void LoginManager::incomingFlap(FlapBuffer& flap)
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

void LoginManager::incomingSnac(SnacBuffer& snac)
{
	if ( snac.family() == 0x17 && snac.subtype() == 0x07 ) {
		recv_auth_key(snac);
		return;
	}
	if ( snac.family() == 0x17 && snac.subtype() == 0x03 ) {
		recv_auth_reply(snac);
		return;
	}
	if ( snac.family() == 0x01 && snac.subtype() == 0x03 ) {
		recv_snac_list(snac);
		return;
	}
	if ( snac.family() == 0x01 && snac.subtype() == 0x18 ) {
		recv_snac_versions(snac);
		return;
	}
	if ( snac.family() == 0x02 && snac.subtype() == 0x03 ) {
		recv_location_services_limits(snac);
		return;
	}
	if ( snac.family() == 0x03 && snac.subtype() == 0x03 ) {
		recv_buddy_list_parameters(snac);
		return;
	}
	if ( snac.family() == 0x04 && snac.subtype() == 0x05 ) {
		recv_icbm_parameters(snac);
		return;
	}
	if ( snac.family() == 0x09 && snac.subtype() == 0x03 ) {
		recv_privacy_parameters(snac);
		return;
	}
}


} /* end of namespace ICQ */
