#include "icqConnection_p.h"

#include "types/icqUserInfo.h"

#include <QTextCodec>

namespace ICQ
{


Connection::Private::Private(Connection* parent)
	: QObject(parent)
{
	connectTimer = 0;
	keepAliveTimer = 0;
	lookupTimer = 0;

	codec = 0;

	ssiActivated = false;
	loginFinished = false;
	m_connectionStatus = Connection::Disconnected;

	rateManager = new RateManager(parent);

	ssiManager = new SSIManager(parent);
	QObject::connect( ssiManager, SIGNAL( ssiActivated() ), SLOT( processSsiActivated() ) );

	socket = new QTcpSocket(parent);

	onlineStatus = UserInfo::Online;

	m_flapSequence = 0;
	m_snacRequest = 0;
	q = parent;

	QObject::connect( socket, SIGNAL( readyRead() ), SLOT( incomingData() ) );
	QObject::connect( q, SIGNAL( readyRead() ), SLOT( incomingData() ) );
	QObject::connect( q, SIGNAL( signedOff() ), SLOT( processSignedOff() ) );
	QObject::connect( q, SIGNAL( incomingMessage(Message) ), SLOT( processIncomingMessage(Message) ) );

	QObject::connect( socket, SIGNAL( connected() ), SLOT( processConnected() ) );
	QObject::connect( socket, SIGNAL( disconnected() ), SLOT( processDisconnected() ) );
}

Connection::Private::~Private()
{
	delete rateManager;
	delete ssiManager;
	socket->deleteLater();
}

void Connection::Private::startSignOn()
{
	setConnectionStatus(Connecting);

	connectTimer = new QTimer(this);
	connectTimer->setSingleShot(true);
	QObject::connect( connectTimer, SIGNAL( timeout() ), SLOT( processConnectionTimeout() ) );

	loginManager = new LoginManager(q);
	QObject::connect( loginManager, SIGNAL( serverAvailable(QString,quint16) ), SLOT( processNewServer(QString,quint16) ) );
	QObject::connect( loginManager, SIGNAL( ratesRequest() ), SLOT( processRatesRequest() ) );
	QObject::connect( loginManager, SIGNAL( loginFinished() ), SLOT( processSignedOn() ) );
	QObject::connect( loginManager, SIGNAL( error(QString) ), q, SIGNAL( error(QString) ) );

	loginManager->setUsername(uin);
	loginManager->setPassword(password);

	qDebug() << "[ICQ:Connection] Looking up hostname" << server;

	lookupId = QHostInfo::lookupHost(server, this, SLOT( processLookupResult(QHostInfo) ) );

	lookupTimer = new QTimer(this);
	lookupTimer->setSingleShot(true);
	QObject::connect( lookupTimer, SIGNAL( timeout() ), SLOT( processLookupTimeout() ) );
	lookupTimer->start(10000);

	connectTimer->start(LOGIN_TIMEOUT);
}

Word Connection::Private::flapSequence()
{
	if ( m_flapSequence >= 0x8000 ) {
		m_flapSequence = 0;
	}
	return ++m_flapSequence;
}

Word Connection::Private::snacRequest()
{
	 return ++m_snacRequest;
}

int Connection::Private::connectionStatus() const
{
	return m_connectionStatus;
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

/* << SNAC(xx,01) - error handling */
void Connection::Private::handle_error(SnacBuffer& snac)
{
	Word errorCode = snac.getWord();
	TlvChain tlvs = snac.readAll();

	QString errorText = "Error. SNAC family " + QString::number(snac.family(), 16)
		+ ". Error code: " + QString::number(errorCode, 16) + " ("+ errDescForCode(errorCode) +")";
	if ( tlvs.hasTlv(0x08) ) {
		errorText += ". Subcode: " + tlvs.getTlvData(0x08).toHex().toUpper();
	}

	qDebug() << "[ICQ:Connection]" << errorText;
	emit q->error(errorText);
}

void Connection::Private::incomingData()
{
	if ( socket->bytesAvailable() < FLAP_HEADER_SIZE ) {
		/* we don't have a header at this point */
		return;
	}

	FlapBuffer flap = FlapBuffer::fromRawData( socket->peek(FLAP_HEADER_SIZE) );

	if (flap.flapDataSize() > (socket->bytesAvailable() - FLAP_HEADER_SIZE) ) {
		/* we don't need an incomplete packet */
		return;
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

	if ( connectionStatus() == Connected) {
		/* restart timers. one sends keep-alive, another kills connection if no data was received in a 90sec period. */
		keepAliveTimer->start();
		connectTimer->start();
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
		if ( snac.family() == 0x01 && snac.subtype() == 0x13 ) {
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
	q->snacRequest(0x01, 0x0E);
}

void Connection::Private::processConnectionTimeout()
{
	qWarning() << "[ICQ:Connection]" << "Connection timed out";
	connectTimer->deleteLater();
	socket->disconnectFromHost();
	setConnectionStatus(Disconnected);

	emit q->error( tr("Connection timed out") );

	emit q->statusChanged(UserInfo::Offline);
}

void Connection::Private::processConnected()
{
	qDebug() << "[ICQ:Connection] Connected to" << socket->peerName() << "port" << socket->peerPort();
}

void Connection::Private::processDisconnected()
{
	if ( connectTimer ) {
		delete connectTimer;
		connectTimer = 0;
	}

	socket->close();
	qDebug() << "[ICQ:Connection] Disconnected";

	if ( connectionStatus() == Connected ) {
		setConnectionStatus(Disconnected);
		emit q->signedOff();
	}
}

void Connection::Private::processLookupResult(const QHostInfo& host)
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

void Connection::Private::processLookupTimeout()
{
	QHostInfo::abortHostLookup(lookupId);
	lookupTimer->deleteLater();
	setConnectionStatus(Disconnected);

	qDebug() << "[ICQ:Connection] Lookup timeout";

	emit q->error( tr("Host lookup timeout") );

	emit q->statusChanged(UserInfo::Offline);
}

void Connection::Private::processIncomingMessage(const Message& msg)
{
	if ( codec == 0 ) {
		codec = QTextCodec::codecForName("Windows-1251");
	}
	Q_ASSERT(codec != 0);

	emit q->incomingMessage( msg.sender(), msg.text(codec) );
}

void Connection::Private::processNewServer(QString newHost, quint16 newPort)
{
	socket->disconnectFromHost();
	socket->connectToHost(QHostAddress(newHost), newPort);
	connectTimer->start(LOGIN_TIMEOUT);
}

void Connection::Private::processRatesRequest()
{
	rateManager->requestRates();
	ssiManager->requestParameters();
	ssiManager->requestContactList();
}

void Connection::Private::processSsiActivated()
{
	ssiActivated = true;

	if ( ssiActivated && loginFinished ) {
		emit q->signedOn();
	}
}

void Connection::Private::processSignedOn()
{
	loginFinished = true;

	connectTimer->stop();
	connectTimer->setInterval(CONNECTION_TIMEOUT); // connection dies if it doesn't receive anything in 90 sec.

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

	if ( ssiActivated && loginFinished ) {
		emit q->signedOn();
	}

	q->setOnlineStatus(onlineStatus);
}

void Connection::Private::processSignedOff()
{
	delete keepAliveTimer;
	delete userInfoManager;
	delete msgManager;
	delete metaManager;

	emit q->statusChanged(UserInfo::Offline);
}

Connection::Private::IntStringPair Connection::Private::subtypeOneErrors[] = {
		IntStringPair(0x01, tr("Invalid SNAC header") ),
		IntStringPair(0x02, tr("Server rate limit exceeded") ),
		IntStringPair(0x03, tr("Client rate limit exceeded") ),
		IntStringPair(0x04, tr("Recipient is not logged in") ),
		IntStringPair(0x05, tr("Requested service unavailable") ),
		IntStringPair(0x06, tr("Requested service is not defined") ),
		IntStringPair(0x07, tr("You sent obsolete snac") ),
		IntStringPair(0x08, tr("Not supported by server") ),
		IntStringPair(0x09, tr("Not supported by client") ),
		IntStringPair(0x0A, tr("Refused by client") ),
		IntStringPair(0x0B, tr("Reply too big") ),
		IntStringPair(0x0C, tr("Responses lost") ),
		IntStringPair(0x0D, tr("Request denied") ),
		IntStringPair(0x0E, tr("Incorrect SNAC format") ),
		IntStringPair(0x0F, tr("Insufficient rights") ),
		IntStringPair(0x10, tr("In local permit/deny (recipient blocked)") ),
		IntStringPair(0x11, tr("Sender is too evil") ),
		IntStringPair(0x12, tr("Receiver is too evil") ),
		IntStringPair(0x13, tr("User temporarily unavailable") ),
		IntStringPair(0x14, tr("No match") ),
		IntStringPair(0x15, tr("List overflow") ),
		IntStringPair(0x16, tr("Request ambiguous") ),
		IntStringPair(0x17, tr("Server queue full") ),
		IntStringPair(0x18, tr("Not while on AOL") )
};

QString Connection::Private::errDescForCode(Word errorCode)
{
	for ( Word i = 0; !subtypeOneErrors[i].second.isEmpty(); ++i ) {
		if ( subtypeOneErrors[i].first == errorCode ) {
			return subtypeOneErrors[i].second;
		}
	}
	return tr("Unknown error");
}


}
/* end of namespace ICQ */
