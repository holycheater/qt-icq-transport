#include "icqConnection_p.h"

namespace ICQ
{


Connection::Private::Private(Connection* parent)
	: QObject(parent)
{
	m_connectionStatus = Connection::Disconnected;

	rateManager = new RateManager(parent);
	ssiManager = new SSIManager(parent);

	socket = new QTcpSocket(parent);

	onlineStatus = Online;

	m_flapSequence = 0;
	m_snacRequest = 0;
	q = parent;

	QObject::connect( socket, SIGNAL( readyRead() ), SLOT( incomingData() ) );
	QObject::connect( q, SIGNAL( readyRead() ), SLOT( incomingData() ) );
	QObject::connect( q, SIGNAL( signedOff() ), SLOT( slot_signedOff() ) );
	QObject::connect( q, SIGNAL( incomingMessage(Message) ), SLOT( processIncomingMessage(Message) ) );

	QObject::connect( socket, SIGNAL( connected() ), SLOT( slot_connected() ) );
	QObject::connect( socket, SIGNAL( disconnected() ), SLOT( slot_disconnected() ) );
}

Connection::Private::~Private()
{
	delete rateManager;
	delete ssiManager;
	socket->deleteLater();
}

Word Connection::Private::flapSequence()
{
	if ( m_flapSequence >= 0x8000 ) {
		m_flapSequence = 0;
	}
	return ++m_flapSequence;
}

void Connection::Private::setConnectionStatus(int status)
{
	if ( m_connectionStatus == status ) {
		return;
	}

	if ( m_connectionStatus == Connection::Connecting ) {
		loginManager->deleteLater(); // so, login manager is deleted if we go offline or online.
	}

	m_connectionStatus = status;
}

void Connection::Private::connectToServer(const QHostInfo& host)
{
	if ( host.error() != QHostInfo::NoError ) {
		qCritical() << "[ICQ:Connection] Lookup failed:" << host.errorString();
		return;
	}
	QHostAddress address = host.addresses().value(0);
	qDebug() << "[ICQ:Connection] Found address:" << address.toString();
	socket->connectToHost(address, port);

	lookupTimer->stop();
	delete lookupTimer;
}

/* << SNAC(xx,01) - error handling */
void Connection::Private::handle_error(SnacBuffer& snac)
{
	QString errmsg = "errCode " + QString::number(snac.getWord(), 16);
	TlvChain tlvs = snac.readAll();
	if ( tlvs.hasTlv(0x08) ) {
		errmsg += " subcode" + tlvs.getTlvData(0x08).toHex();
	}
	qDebug() << "[ICQ:Connection] ERROR!!" << "family" << snac.family() << errmsg;
}

void Connection::Private::incomingData()
{
	if ( socket->bytesAvailable() < FLAP_HEADER_SIZE ) {
		qDebug() << "[ICQ:Connection] Not enough data for a header in the socket";
		return; // we don't have a header at this point
	}

	FlapBuffer flap = FlapBuffer::fromRawData( socket->peek(FLAP_HEADER_SIZE) );

	if (flap.flapDataSize() > (socket->bytesAvailable() - FLAP_HEADER_SIZE) ) {
		qDebug() << "[ICQ:Connection] Not enough data for a packet" << flap.flapDataSize() << socket->bytesAvailable();
		return; // we don't need an incomplete packet
	}

	socket->seek(FLAP_HEADER_SIZE);

	flap.setData( socket->read( flap.flapDataSize() ) );
/*
	qDebug()
		<< "[ICQ:Connection] << flap"
		<< "channel" << flap.channel()
		<< "sequence" << QByteArray::number(flap.sequence(), 16)
		<< "size" << flap.size();
	qDebug() << "[ICQ:Connection] << flap data" << flap.data().remove(0, FLAP_HEADER_SIZE).toHex();*/

	if ( flap.channel() == FlapBuffer::CloseChannel ) {
		socket->disconnectFromHost();
	}

	if ( flap.channel() == FlapBuffer::KeepAliveChannel ) {
		qDebug() << "[ICQ:Connection] Keep-alive received";
	}
	if ( connectionStatus() == Connected) {
		keepAliveTimer->stop();
		keepAliveTimer->start();
	}

	/* now we emit an incoming flap signal, which will be catched by various
	 * services (login manager, rate manager, etc) */
	emit q->incomingFlap(flap);

	if ( flap.channel() == FlapBuffer::DataChannel && flap.pos() == 0 ) { // pos == 0 means that flap wasn't touched by flap handlers.
		SnacBuffer snac = flap;

		qDebug()
			<< "[ICQ:Connection] << snac head: family" << QByteArray::number(snac.family(), 16)
			<< "subtype" << QByteArray::number(snac.subtype(), 16)
			<< "flags" << QByteArray::number(snac.flags(), 16)
			<< "requestid" << QByteArray::number(snac.requestId(), 16)
			<< "len" << snac.size();

		if ( snac.flags() & 0x8000 ) {
			Word unknownSize = snac.getWord();
			snac.seek(sizeof(Word)+unknownSize);
		}

		// read out motd
		if ( snac.family() == sfGeneric && snac.subtype() == 0x13 ) {
			snac.seekEnd();
		}

		/* ignore SNAC(01,21) */
		if ( snac.family() == 0x01 && snac.subtype() == 0x21 ) {
			snac.seekEnd();
		}

		if ( snac.subtype() == 0x01 ) {
			handle_error(snac);
		}

		emit q->incomingSnac(snac);

		if ( (snac.pos() + 1) < snac.dataSize() ) {
			qDebug() << "[ICQ:Connection]" << (snac.pos() + 1) << snac.dataSize() << "unhandled snac" << QByteArray::number(snac.family(), 16) << QByteArray::number(snac.subtype(), 16);
		}
	}

	if ( socket->bytesAvailable() > 0 ) {
		emit q->readyRead();
	}
}

void Connection::Private::sendKeepAlive()
{
	qDebug() << "[ICQ:Connection] Keep-alive sent";
	q->write ( FlapBuffer(FlapBuffer::KeepAliveChannel) );
}

void Connection::Private::processIncomingMessage(const Message& msg)
{
	emit q->incomingMessage( msg.sender(), msg.text() );
}

void Connection::Private::slot_disconnected()
{
	socket->close();
	qDebug() << "[ICQ:Connection] Disconnected";

	if ( connectionStatus() == Connected ) {
		setConnectionStatus(Disconnected);
		emit q->signedOff();
	}
}

void Connection::Private::slot_connected()
{
	qDebug() << "[ICQ:Connection] Connected to" << socket->peerName() << "port" << socket->peerPort();
}

void Connection::Private::slot_lookupFailed()
{
	QHostInfo::abortHostLookup(lookupId);
	lookupTimer->deleteLater();
	setConnectionStatus(Disconnected);

	qDebug() << "[Error] Host lookup timeout";

	emit q->statusChanged(Offline);
}

void Connection::Private::slot_connectionTimeout()
{
	qWarning() << "[error]" << "connection timed out";
	connectTimer->deleteLater();
	socket->disconnectFromHost();
	setConnectionStatus(Disconnected);

	emit q->statusChanged(Offline);
}

void Connection::Private::slot_signedOn()
{
	delete connectTimer;

	keepAliveTimer = new QTimer(q);
	keepAliveTimer->setInterval(KEEP_ALIVE_INTERVAL);

	QObject::connect( keepAliveTimer, SIGNAL( timeout() ), SLOT( sendKeepAlive() ) );
	keepAliveTimer->start();

	userInfoManager = new UserInfoManager(q);
	msgManager = new MessageManager(q);
	metaManager = new MetaInfoManager(q);
	QObject::connect(metaManager, SIGNAL( metaInfoAvailable(Word,Buffer&) ), msgManager, SLOT( incomingMetaInfo(Word,Buffer&) ) );

	msgManager->requestOfflineMessages();

	setConnectionStatus(Connected);
	q->setOnlineStatus(onlineStatus);
}

void Connection::Private::slot_signedOff()
{
	delete keepAliveTimer;
	emit q->statusChanged(Offline);

	delete userInfoManager;
	delete msgManager;
	delete metaManager;
}


}
/* end of namespace ICQ */
