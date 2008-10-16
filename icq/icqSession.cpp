/*
 * icqSession.h - ICQ user session.
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

#include "icqSession.h"
#include "icqSocket.h"

#include "managers/icqLoginManager.h"
#include "managers/icqMetaInfoManager.h"
#include "managers/icqMessageManager.h"
#include "managers/icqRateManager.h"
#include "managers/icqSsiManager.h"
#include "managers/icqUserInfoManager.h"

#include "types/icqContact.h"
#include "types/icqMessage.h"
#include "types/icqUserInfo.h"

#include <QHostAddress>
#include <QHostInfo>
#include <QStringList>
#include <QTimer>
#include <QTextCodec>

static const int LOOKUP_TIMEOUT = 15000;
static const int LOGIN_TIMEOUT = 30000;
static const int KEEP_ALIVE_INTERVAL = 60000;
static const int CONNECTION_TIMEOUT = 90000;

namespace ICQ
{


class Session::Private
{
	public:
		Private(Session *parent);
		~Private();

		void startLogin();

		OnlineStatus onlineStatus;
		ConnectionStatus connectionStatus;

		QString server;
		quint16 port;
		QString uin, password;
		QHostAddress peer;

		Socket *socket;

		LoginManager    *loginManager;
		MetaInfoManager *metaManager;
		MessageManager  *msgManager;
		RateManager     *rateManager;
		SSIManager      *ssiManager;
		UserInfoManager *userInfoManager;

		int lookupID;
		QTimer *lookupTimer;
		QTimer *connectTimer;
		QTimer *keepAliveTimer;

		QTextCodec *codec;
	private:
		Session *q;
};

Session::Private::Private(Session *parent)
{
	q = parent;

	onlineStatus = Offline;
	connectionStatus = Disconnected;

	loginManager    = 0;
	metaManager     = 0;
	msgManager      = 0;
	rateManager     = 0;
	ssiManager      = 0;
	userInfoManager = 0;

	lookupTimer = 0;
	connectTimer = 0;
	keepAliveTimer = 0;

	socket = 0;

	codec = 0;
}

Session::Private::~Private()
{
	/* It SHOULD be ensured that pointers are zero, or point a valid object */

	delete loginManager;
	delete metaManager;
	delete msgManager;
	delete rateManager;
	delete ssiManager;
	delete userInfoManager;

	delete lookupTimer;
	delete connectTimer;
	delete keepAliveTimer;

	delete socket;
}

void Session::Private::startLogin()
{
	loginManager = new LoginManager(q);

	QObject::connect( loginManager, SIGNAL( serverAvailable(QString,quint16) ), q, SLOT( processServerAvailable(QString,quint16) ) );
	QObject::connect( loginManager, SIGNAL( ratesRequest() ),                   q, SLOT( processRatesRequest() ) );
	QObject::connect( loginManager, SIGNAL( ssiRequest() ),                     q, SLOT( processSsiRequest() ) );
	QObject::connect( loginManager, SIGNAL( finished() ),                       q, SLOT( processLoginDone() ) );
	QObject::connect( loginManager, SIGNAL( error(QString) ),                   q, SIGNAL( error(QString) ) );

	loginManager->setUsername(uin);
	loginManager->setPassword(password);


	connectTimer = new QTimer(q);
	QObject::connect( connectTimer, SIGNAL( timeout() ), q, SLOT( processConnectionTimeout() ) );
	connectTimer->start(LOGIN_TIMEOUT);

	socket = new Socket(q);
	loginManager->setSocket(socket);
	socket->connectToHost(peer, port);
	qDebug() << "[ICQ:Session] connecting to" << peer.toString()+":"+QString::number(port,10);
}

Session::Session(QObject *parent)
	: QObject(parent)
{
	d = new Private(this);
}

Session::~Session()
{
	delete d;
}

void Session::connect()
{
	if ( d->connectionStatus != Disconnected ) {
		return;
	}

	d->connectionStatus = Connecting;

	if ( QHostAddress(d->server).isNull() ) {
		d->lookupTimer = new QTimer(this);
		QObject::connect( d->lookupTimer, SIGNAL( timeout() ), SLOT( processLookupTimeout() ) );
		d->lookupTimer->setSingleShot(true);
		d->lookupTimer->start(LOOKUP_TIMEOUT);

		d->lookupID = QHostInfo::lookupHost(d->server, this, SLOT( processLookupResult(QHostInfo) ) );
	} else {
		d->peer = QHostAddress(d->server);
		d->startLogin();
	}
}

void Session::disconnect()
{
	if ( d->connectionStatus == Disconnected ) {
		return;
	}

	if ( d->socket ) {
		d->socket->disconnectFromHost();
		d->socket->deleteLater();
		d->socket = 0;
	}

	delete d->loginManager; d->loginManager       = 0;
	delete d->metaManager; d->metaManager         = 0;
	delete d->msgManager; d->msgManager           = 0;
	delete d->rateManager; d->rateManager         = 0;
	delete d->ssiManager; d->ssiManager           = 0;
	delete d->userInfoManager; d->userInfoManager = 0;

	delete d->lookupTimer; d->lookupTimer       = 0;
	delete d->connectTimer; d->connectTimer     = 0;
	delete d->keepAliveTimer; d->keepAliveTimer = 0;

	d->connectionStatus = Disconnected;

	emit disconnected();
}

void Session::contactAdd(const QString& uin)
{
	if (!d->ssiManager) {
		return;
	}
	Contact c = d->ssiManager->contactByUin(uin);

	if ( !c.isValid() ) {
		d->ssiManager->addContact(uin);
	} else if ( c.awaitingAuth() ) {
		d->ssiManager->requestAuthorization(uin);
	} else {
		emit authGranted(uin);
	}
}

void Session::contactDel(const QString& uin)
{
	if (!d->ssiManager) {
		return;
	}

	Contact c = d->ssiManager->contactByUin(uin);

	if ( c.isValid() ) {
		d->ssiManager->delContact(uin);
	}
}

void Session::authGrant(const QString& toUin)
{
	if (!d->ssiManager) {
		return;
	}
	d->ssiManager->grantAuthorization(toUin);
}

void Session::authDeny(const QString& toUin)
{
	if (!d->ssiManager) {
		return;
	}
	d->ssiManager->denyAuthorization(toUin);
}

void Session::setCodecForMessages(QTextCodec *codec)
{
	d->codec = codec;
}

void Session::sendMessage(const QString& recipient, const QString& message)
{
	if (!d->msgManager || !d->userInfoManager) {
		return;
	}

	Message msg;

	if ( d->userInfoManager->getUserStatus(recipient) == UserInfo::Offline ) {
		qDebug() << "[ICQ:Connection]" << "sending offline message via channel 1";
		msg.setChannel(0x01);
	} else {
		qDebug() << "[ICQ:Connection]" << "sending message via channel 2";
		msg.setChannel(0x02);
	}

	msg.setSender(d->uin);
	msg.setReceiver(recipient);
	msg.setType(Message::PlainText);
	msg.setText( message.toUtf8() );
	d->msgManager->sendMessage(msg);
}

Session::ConnectionStatus Session::connectionStatus() const
{
	return d->connectionStatus;
}

QStringList Session::contactList() const
{
	if ( !d->ssiManager || d->connectionStatus != Connected ) {
		return QStringList();
	}

	QStringList contacts;

	QList<Contact> cl;
	QListIterator<Contact> ci(cl);
	while ( ci.hasNext() ) {
		contacts << ci.next().name();
	}
	return contacts;
}

Session::OnlineStatus Session::onlineStatus() const
{
	return d->onlineStatus;
}

QString Session::password() const
{
	return d->password;
}

QString Session::serverHost() const
{
	return d->server;
}

quint16 Session::serverPort() const
{
	return d->port;
}

QString Session::uin() const
{
	return d->uin;
}

void Session::setUin(const QString& uin)
{
	d->uin = uin;
}

void Session::setPassword(const QString& password)
{
	d->password = password;
}

void Session::setServerHost(const QString& server)
{
	d->server = server;
}

void Session::setServerPort(quint16 port)
{
	d->port = port;
}

void Session::setOnlineStatus(OnlineStatus status)
{
	if ( d->onlineStatus == status ) {
		return;
	}

	d->onlineStatus = status;

	if ( d->connectionStatus != Connected ) {
		return;
	}

	Word realStatus;
	switch ( d->onlineStatus ) {
		case FreeForChat:
			realStatus = UserInfo::FreeForChat;
			break;
		case Away:
			realStatus = UserInfo::Away;
			break;
		case NotAvailable:
			realStatus = UserInfo::Away | UserInfo::NotAvailable;
			break;
		case Occupied:
			realStatus = UserInfo::Away | UserInfo::Occupied;
			break;
		case DoNotDisturb:
			realStatus = UserInfo::Away | UserInfo::Occupied | UserInfo::DoNotDisturb;
			break;
		default:
			realStatus = UserInfo::Online;
			break;
	}

	SnacBuffer reqSetStatus(0x01, 0x1E);
	Word flags = flagDCDisabled;
	reqSetStatus.addTlv( (Tlv)Tlv(0x06).addWord(flags).addWord(realStatus) );
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

	d->socket->write(reqSetStatus);
}

void Session::processLookupTimeout()
{
	QHostInfo::abortHostLookup(d->lookupID);
	d->lookupTimer->deleteLater();
	d->lookupTimer = 0;

	emit error( tr("Host lookup timeout") );
	disconnect();
}

void Session::processLookupResult(const QHostInfo& result)
{
	delete d->lookupTimer;
	d->lookupTimer = 0;

	if ( result.error() != QHostInfo::NoError ) {
		qDebug() << "[ICQ:Connection] Lookup failed:" << result.errorString();
		emit error("Host lookup failed. " + result.errorString() );
		disconnect();
		return;
	}

	d->peer = result.addresses().value(0);
	qDebug() << "[ICQ:Connection] Found address:" << d->peer.toString();

	d->startLogin();
}

void Session::processConnectionTimeout()
{
	d->connectTimer->deleteLater();
	d->connectTimer = 0;

	emit error( tr("Connection timed out") );
	disconnect();
}

void Session::processServerAvailable(const QString& host, quint16 port)
{
	Q_ASSERT( !QHostAddress(host).isNull() );

	d->socket->disconnectFromHost();
	d->socket->connectToHost(QHostAddress(host), port);
	d->connectTimer->start(LOGIN_TIMEOUT);
}

void Session::processRatesRequest()
{
	d->rateManager = new RateManager(d->socket, this);
	d->rateManager->requestRates();
}

void Session::processSsiRequest()
{
	d->ssiManager = new SSIManager(this);
	d->ssiManager->setSocket(d->socket);

	QObject::connect( d->ssiManager, SIGNAL( authGranted(QString) ),    SIGNAL( authGranted(QString) ) );
	QObject::connect( d->ssiManager, SIGNAL( authDenied(QString) ),     SIGNAL( authDenied(QString) ) );

	d->ssiManager->requestParameters();
	d->ssiManager->requestContactList();
}

void Session::processLoginDone()
{
	/*d->loginManager->deleteLater();
	d->loginManager = 0;*/

	d->connectTimer->start(CONNECTION_TIMEOUT);
	d->keepAliveTimer = new QTimer(this);
	QObject::connect( d->keepAliveTimer, SIGNAL( timeout() ), SLOT( sendKeepAlive() ) );
	d->keepAliveTimer->start(KEEP_ALIVE_INTERVAL);

	d->userInfoManager = new UserInfoManager(d->socket, this);
	QObject::connect( d->userInfoManager, SIGNAL( statusChanged(int) ),      SLOT( processStatusChanged(int) ) );
	QObject::connect( d->userInfoManager, SIGNAL( userOnline(QString,int) ), SLOT( processUserStatus(QString,int) ) );
	QObject::connect( d->userInfoManager, SIGNAL( userOffline(QString) ),    SIGNAL( userOffline(QString) ) );

	d->metaManager = new MetaInfoManager(d->socket, this);
	d->metaManager->setUin(d->uin);
	d->socket->setMetaManager(d->metaManager);

	d->msgManager = new MessageManager(d->socket, this);
	QObject::connect( d->metaManager, SIGNAL( metaInfoAvailable(Word,Buffer&) ), d->msgManager, SLOT( incomingMetaInfo(Word,Buffer&) ) );
	QObject::connect( d->msgManager, SIGNAL( incomingMessage(Message) ), SLOT( processIncomingMessage(Message) ) );
	d->msgManager->setUin(d->uin);
	d->msgManager->requestOfflineMessages();

	d->connectionStatus = Connected;
	setOnlineStatus(d->onlineStatus);

	emit connected();
}

void Session::processSnac(SnacBuffer& snac)
{
	if ( d->connectionStatus == Connected ) {
		d->connectTimer->start(CONNECTION_TIMEOUT);
		d->keepAliveTimer->start(KEEP_ALIVE_INTERVAL);
	}

	if ( snac.subtype() == 0x01 ) {
		// TODO: handle err;
	}
}

void Session::processIncomingMessage(const Message& msg)
{
	if ( !d->codec ) {
		d->codec = QTextCodec::codecForName("Windows-1251");
	}
	Q_ASSERT(d->codec != 0);

	emit incomingMessage( msg.sender(), msg.text(d->codec) );
}

void Session::processUserStatus(const QString& uin, int status)
{
	/* hack: emit single status for userOnline signal instead a group of flags
	 * Anyways, it's a mystery why ICQ needs flags for online status. You can't be Occuppied and DND at the same time */
	int singleStatus = Online;
	if ( status & UserInfo::Away ) {
		singleStatus = Away;
		if ( status & UserInfo::NotAvailable ) {
			singleStatus = NotAvailable;
		} else if ( status & UserInfo::Occupied ) {
			singleStatus = Occupied;
			if ( status & UserInfo::DoNotDisturb ) {
				singleStatus = DoNotDisturb;
			}
		}
	} else if ( status & UserInfo::FreeForChat ) {
		singleStatus = FreeForChat;
	}

	emit userOnline(uin, singleStatus);
}

void Session::processStatusChanged(int status)
{
	int singleStatus = Online;
	if ( status & UserInfo::Away ) {
		singleStatus = Away;
		if ( status & UserInfo::NotAvailable ) {
			singleStatus = NotAvailable;
		} else if ( status & UserInfo::Occupied ) {
			singleStatus = Occupied;
			if ( status & UserInfo::DoNotDisturb ) {
				singleStatus = DoNotDisturb;
			}
		}
	} else if ( status & UserInfo::FreeForChat ) {
		singleStatus = FreeForChat;
	}
	emit statusChanged(singleStatus);
}

void Session::sendKeepAlive()
{
	d->socket->snacRequest(0x01, 0x0E);
}

} /* end of namespace ICQ */
