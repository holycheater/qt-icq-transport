/**
 * icqConnection.cpp - ICQ Connection class.
 * Copyright (C) 2008  Alexander Saltykov
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 **/

#include "icqConnection.h"

#include "icqLoginManager.h"
#include "icqRateManager.h"
#include "icqSsiManager.h"
#include "icqMessageManager.h"
#include "icqMetaInfoManager.h"
#include "icqUserInfoManager.h"

#include <QTcpSocket>
#include <QTimer>
#include <QtDebug>

class ICQ::Connection::Private
{
	public:
		Private(Connection* parent);
		~Private();

		Word flapSequence();
		Word snacRequest() { return ++m_snacRequest; };

		int connectionStatus() const { return m_connectionStatus; }
		void setConnectionStatus(int status);

		LoginManager 	*loginManager;
		RateManager 	*rateManager;
		SSIManager 		*ssiManager;
		UserInfoManager *userInfoManager;
		MessageManager 	*msgManager;
		MetaInfoManager *metaManager;

		QString uin;
		QString password;
		QString server;
		quint16 port;

		QTcpSocket *socket;

		Word onlineStatus;

		int lookupId;

		QTimer *connectTimer;
		QTimer *keepAliveTimer;
		QTimer *lookupTimer;
	private:
		Connection *q;
		Word m_snacRequest;
		Word m_flapSequence;
		int m_connectionStatus;
};

ICQ::Connection::Private::Private(Connection* parent)
{
	m_connectionStatus = Connection::Disconnected;

	rateManager = new RateManager(parent);
	ssiManager = new SSIManager(parent);

	socket = new QTcpSocket(parent);

	onlineStatus = ICQ::stOnline;

	m_flapSequence = 0;
	m_snacRequest = 0;
	q = parent;

	QObject::connect( socket, SIGNAL( readyRead() ), q, SLOT( incomingData() ) );
	QObject::connect( q, SIGNAL( readyRead() ), q, SLOT( incomingData() ) );
	QObject::connect( q, SIGNAL( signedOff() ), q, SLOT( slot_signedOff() ) );

	QObject::connect( socket, SIGNAL( connected() ), q, SLOT( slot_connected() ) );
	QObject::connect( socket, SIGNAL( disconnected() ), q, SLOT( slot_disconnected() ) );
}

ICQ::Connection::Private::~Private()
{
	delete rateManager;
	delete ssiManager;
	delete socket;
}

ICQ::Word ICQ::Connection::Private::flapSequence()
{
	if ( m_flapSequence >= 0x8000 ) {
		m_flapSequence = 0;
	}
	return ++m_flapSequence;
}

void ICQ::Connection::Private::setConnectionStatus(int status)
{
	if ( m_connectionStatus == status ) {
		return;
	}

	if ( m_connectionStatus == Connection::Connecting ) {
		loginManager->deleteLater(); // so, login manager is deleted if we go offline or online.
	}

	m_connectionStatus = status;
}

ICQ::Connection::Connection(QObject *parent = 0)
	: QObject(parent)
{
	d = new Private(this);
}

ICQ::Connection::Connection(const QString& uin, const QString& password, const QString& server, quint16 port, QObject *parent)
	: QObject(parent)
{
	d = new Private(this);

	d->uin = uin;
	d->password = password;
	d->server = server;
	d->port = port;
}

ICQ::Connection::~Connection()
{
	delete d;
}

int ICQ::Connection::connectionStatus() const
{
	return d->connectionStatus();
}

bool ICQ::Connection::isSignedOn() const
{
	if ( connectionStatus() == Connected ) {
		return true;
	}
	return false;
}

QString ICQ::Connection::userId() const
{
	return d->uin;
}

void ICQ::Connection::connectToHost(const QString& hostname, quint16 port)
{
	qDebug() << "[ICQ::Connection] Looking up hostname" << hostname;

	d->lookupId = QHostInfo::lookupHost(hostname, this, SLOT( connectToServer(QHostInfo) ) );

	d->lookupTimer = new QTimer(this);
	d->lookupTimer->setSingleShot(true);
	QObject::connect( d->lookupTimer, SIGNAL( timeout() ), this, SLOT( slot_lookupFailed() ) );
	d->lookupTimer->start(10000);

	d->port = port;
}

void ICQ::Connection::connectToHost(const QHostAddress& host, quint16 port)
{
	d->socket->connectToHost(host, port);
}

void ICQ::Connection::connectToServer(const QHostInfo& host)
{
	if ( host.error() != QHostInfo::NoError ) {
		qCritical() << "[ICQ::Connection] Lookup failed:" << host.errorString();
		return;
		// TODO: Throw error for GUI
	}
	QHostAddress address = host.addresses().value(0);
	qDebug() << "[ICQ::Connection] Found address:" << address.toString();
	d->socket->connectToHost(address, d->port);

	d->lookupTimer->stop();
	delete d->lookupTimer;
}

void ICQ::Connection::disconnectFromHost()
{
	d->socket->disconnectFromHost();
}

void ICQ::Connection::startConnectionTimer()
{
	d->connectTimer->stop();
	d->connectTimer->start(CONNECTION_TIMEOUT);
}

ICQ::Connection* ICQ::Connection::setUin(const QString& uin)
{
	d->uin = uin;
	return this;
}
ICQ::Connection* ICQ::Connection::setPassword(const QString& password)
{
	d->password = password;
	return this;
}

ICQ::Connection* ICQ::Connection::setServer(const QString& server)
{
	d->server = server;
	return this;
}

ICQ::Connection* ICQ::Connection::setServerPort(quint16 port)
{
	d->port = port;
	return this;
}

ICQ::Connection* ICQ::Connection::setOnlineStatus(Word onlineStatus)
{
	d->onlineStatus = onlineStatus;

	if ( onlineStatus == ICQ::stOffline ) {
		if ( connectionStatus() == Connecting ) {
			d->socket->disconnectFromHost();
		} else if ( connectionStatus() == Connected ) {
			signOff();
		}
		return this;
	}

	if ( connectionStatus() == Disconnected ) {
		signOn(d->uin, d->password, d->server);
		return this;
	}

	if ( connectionStatus() != Connected ) {
		return this;
	}

	SnacBuffer reqSetStatus(ICQ::sfGeneric, 0x1E);
	Word flags = ICQ::flagDCAuth;
	reqSetStatus.addTlv( (Tlv)Tlv(0x06).addWord(flags).addWord(d->onlineStatus) );

	reqSetStatus.addTlv( (Tlv)Tlv(0x08).addWord(0x0) ); // unknown. Don't know what is that;

	// dc info, some dummy data
	Tlv dcInfo(0x0C);
	dcInfo.addDWord(0x00000000); // internal ip
	dcInfo.addDWord(0x00000000); // internal port
	dcInfo.addByte(0x04); // dc type - DC_NORMAL
	dcInfo.addWord(0x000B); // dc protocol version
	dcInfo.addDWord(0x01020304); // dc auth cookie
	dcInfo.addDWord(0x00000050); // web front port?
	dcInfo.addDWord(0x00000001); // "client futures" ?!
	dcInfo.addDWord(0x0); // last info update time ?!
	dcInfo.addDWord(0x0); // last ext status?
	dcInfo.addWord(0x0000); // unknown
	dcInfo.addDWord(0x00000000); //unknown

	reqSetStatus.addTlv(dcInfo);

	write(reqSetStatus);

	return this;
}

ICQ::Connection* ICQ::Connection::setVisibility(int vis)
{
	Q_UNUSED(vis)
	return this;
}

void ICQ::Connection::signOn(QString& uin, QString& password, QString& server)
{
	d->setConnectionStatus(Connecting);

	d->connectTimer = new QTimer(this);
	d->connectTimer->setSingleShot(true);
	QObject::connect( d->connectTimer, SIGNAL( timeout() ), this, SLOT( slot_connectionTimeout() ) );

	d->loginManager = new LoginManager(this);
	d->loginManager->login(uin, password, server);
	startConnectionTimer();
}

void ICQ::Connection::snacRequest(Word family, Word subtype)
{
	write( SnacBuffer(family, subtype) );
}

void ICQ::Connection::write(const FlapBuffer& flap)
{
	writeForced( const_cast<FlapBuffer*>(&flap) );
}

void ICQ::Connection::write(const SnacBuffer& snac)
{
	if ( d->rateManager && !d->rateManager->canSend(snac) ) {
		d->rateManager->enqueue(snac);
	} else {
		writeForced( const_cast<SnacBuffer*>(&snac) );
	}
}

void ICQ::Connection::writeForced(FlapBuffer* flap)
{
	flap->setSequence( d->flapSequence() );

	// qDebug() << "[ICQ::Connection] >> flap channel" << flap->channel() << "len" << flap->size() << "sequence" << QByteArray::number(flap->sequence(), 16);
	// qDebug() << "[ICQ::Connection] >> flap data" << flap->data().toHex().toUpper();
	d->socket->write( flap->data() );
}

void ICQ::Connection::writeForced(SnacBuffer* snac)
{
	snac->setRequestId( d->snacRequest() );

	writeForced( dynamic_cast<FlapBuffer*>(snac) );

	qDebug() << "[ICQ::Connection] >>"
		<< "snac head: family"
		<< QByteArray::number(snac->family(),16)
		<< "subtype" << QByteArray::number(snac->subtype(),16)
		<< "flags" << QByteArray::number(snac->flags(), 16)
		<< "requestid" << QByteArray::number(snac->requestId(), 16);
}

void ICQ::Connection::signOff()
{
	qDebug() << "[ICQ::Connection] Signing off";
	write( FlapBuffer(FlapBuffer::CloseChannel) );

	d->socket->disconnectFromHost();
}

ICQ::RateManager* ICQ::Connection::rateManager() const
{
	return d->rateManager;
}

ICQ::SSIManager* ICQ::Connection::ssiManager() const
{
	return d->ssiManager;
}

void ICQ::Connection::sendMetaRequest(Word type)
{
	d->metaManager->sendMetaRequest(type);
}

void ICQ::Connection::sendMetaRequest(Word type, Buffer& data)
{
	d->metaManager->sendMetaRequest(type, data);
}

/* << SNAC(xx,01) - error handling */
void ICQ::Connection::handle_error(SnacBuffer& snac)
{
	QString errmsg = "errCode " + QString::number(snac.getWord(), 16);
	TlvChain tlvs = snac.readAll();
	if ( tlvs.hasTlv(0x08) ) {
		errmsg += " subcode" + tlvs.getTlvData(0x08).toHex();
	}
	qDebug() << "[ICQ::Connection] ERROR!!" << "family" << snac.family() << errmsg;
}

void ICQ::Connection::incomingData()
{
	if ( d->socket->bytesAvailable() < ICQ::FLAP_HEADER_SIZE ) {
		qDebug() << "[ICQ::Connection] Not enough data for a header in the socket";
		return; // we don't have a header at this point
	}

	FlapBuffer flap = FlapBuffer::fromRawData( d->socket->peek(ICQ::FLAP_HEADER_SIZE) );

	if (flap.flapDataSize() > (d->socket->bytesAvailable() - ICQ::FLAP_HEADER_SIZE) ) {
		qDebug() << "[ICQ::Connection] Not enough data for a packet" << flap.flapDataSize() << d->socket->bytesAvailable();
		return; // we don't need an incomplete packet
	}

	d->socket->seek(ICQ::FLAP_HEADER_SIZE);

	flap.setData( d->socket->read( flap.flapDataSize() ) );
/*
	qDebug()
		<< "[ICQ::Connection] << flap"
		<< "channel" << flap.channel()
		<< "sequence" << QByteArray::number(flap.sequence(), 16)
		<< "size" << flap.size();
	qDebug() << "[ICQ::Connection] << flap data" << flap.data().remove(0, ICQ::FLAP_HEADER_SIZE).toHex();*/

	if ( flap.channel() == FlapBuffer::CloseChannel ) {
		d->socket->disconnectFromHost();
	}

	if ( flap.channel() == FlapBuffer::KeepAliveChannel ) {
		qDebug() << "[ICQ::Connection] Keep-alive received";
	}
	if ( connectionStatus() == Connected) {
		d->keepAliveTimer->stop();
		d->keepAliveTimer->start();
	}

	/* now we emit an incoming flap signal, which will be catched by various
	 * services (login manager, rate manager, etc) */
	emit incomingFlap(flap);

	if ( flap.channel() == FlapBuffer::DataChannel && flap.pos() == 0 ) { // pos == 0 means that flap wasn't touched by flap handlers.
		SnacBuffer snac = flap;

		qDebug()
			<< "[ICQ::Connection] << snac head: family" << QByteArray::number(snac.family(), 16)
			<< "subtype" << QByteArray::number(snac.subtype(), 16)
			<< "flags" << QByteArray::number(snac.flags(), 16)
			<< "requestid" << QByteArray::number(snac.requestId(), 16)
			<< "len" << snac.size();

		if ( snac.flags() & 0x8000 ) {
			Word unknownSize = snac.getWord();
			snac.seek(sizeof(Word)+unknownSize);
		}

		// read out motd
		if ( snac.family() == ICQ::sfGeneric && snac.subtype() == 0x13 ) {
			snac.seekEnd();
		}

		/* ignore SNAC(01,21) */
		if ( snac.family() == 0x01 && snac.subtype() == 0x21 ) {
			snac.seekEnd();
		}

		if ( snac.subtype() == 0x01 ) {
			handle_error(snac);
		}

		emit incomingSnac(snac);

		if ( (snac.pos() + 1) < snac.dataSize() ) {
			qDebug() << "[ICQ::Connection]" << (snac.pos() + 1) << snac.dataSize() << "unhandled snac" << QByteArray::number(snac.family(), 16) << QByteArray::number(snac.subtype(), 16);
		}
	}

	if ( d->socket->bytesAvailable() > 0 ) {
		emit readyRead();
	}
}

void ICQ::Connection::sendKeepAlive()
{
	qDebug() << "[ICQ::Connection] Keep-alive sent";
	write ( FlapBuffer(FlapBuffer::KeepAliveChannel) );
}

void ICQ::Connection::slot_disconnected()
{
	d->socket->close();
	qDebug() << "[ICQ::Connection] Disconnected";

	if ( connectionStatus() == Connected ) {
		d->setConnectionStatus(Disconnected);
		emit signedOff();
	}
}

void ICQ::Connection::slot_connected()
{
	qDebug() << "[ICQ::Connection] Connected to" << d->socket->peerName() << "port" << d->socket->peerPort();
}

void ICQ::Connection::slot_lookupFailed()
{
	QHostInfo::abortHostLookup(d->lookupId);
	d->lookupTimer->deleteLater();
	d->setConnectionStatus(Disconnected);

	qDebug() << "[Error] Host lookup timeout";

	emit statusChanged(ICQ::stOffline);
}

void ICQ::Connection::slot_connectionTimeout()
{
	qWarning() << "[error]" << "connection timed out";
	d->connectTimer->deleteLater();
	d->socket->disconnectFromHost();
	d->setConnectionStatus(Disconnected);
	emit statusChanged(ICQ::stOffline);
}

void ICQ::Connection::slot_signedOn()
{
	delete d->connectTimer;

	d->keepAliveTimer = new QTimer(this);
	d->keepAliveTimer->setInterval(KEEP_ALIVE_INTERVAL);

	QObject::connect( d->keepAliveTimer, SIGNAL( timeout() ), this, SLOT( sendKeepAlive() ) );
	d->keepAliveTimer->start();

	d->userInfoManager = new UserInfoManager(this);
	d->msgManager = new MessageManager(this);
	d->metaManager = new MetaInfoManager(this);
	QObject::connect(d->metaManager, SIGNAL( metaInfoAvailable(ICQ::Word,ICQ::Buffer&) ), d->msgManager, SLOT( incomingMetaInfo(ICQ::Word,ICQ::Buffer&) ) );

	d->msgManager->requestOfflineMessages();

	d->setConnectionStatus(Connected);
	setOnlineStatus(d->onlineStatus);
}

void ICQ::Connection::slot_signedOff()
{
	delete d->keepAliveTimer;
	emit statusChanged(ICQ::stOffline);

	delete d->userInfoManager;
	delete d->msgManager;
	delete d->metaManager;
}
