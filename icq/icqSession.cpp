/*
 * icqSession.cpp - ICQ user session.
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
#include "types/icqUserDetails.h"
#include "types/icqShortUserDetails.h"

#include <QDateTime>
#include <QHostAddress>
#include <QHostInfo>
#include <QPair>
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
        void processSnacError(SnacBuffer& snac);

        typedef QPair<Word, QString> IntStringPair;
        static IntStringPair subtypeOneErrors[];
        static QString errDescForCode(Word errorCode);

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
    connectTimer->setSingleShot(true);
    QObject::connect( connectTimer, SIGNAL( timeout() ), q, SLOT( processConnectionTimeout() ) );
    connectTimer->start(LOGIN_TIMEOUT);

    socket = new Socket(q);
    loginManager->setSocket(socket);
    socket->connectToHost(peer, port);
    qDebug() << "[ICQ:Session] connecting to" << peer.toString()+":"+QString::number(port,10);
}

void Session::Private::processSnacError(SnacBuffer& snac)
{
    Word code = snac.getWord();

    TlvChain chain = snac.readAll();
    QString text = "Error. SNAC family " + QString::number(snac.family(), 16) + ". Error Code: " + QString::number(code, 16) + "(" + errDescForCode(code) + ")";
    if ( chain.hasTlv(0x08) ) {
        text += ". Subcode: " + chain.getTlvData(0x08).toHex().toUpper();
    }
    emit q->error(text);
}

/**
 * @class Session
 * @brief Represents icq session
 *
 * First, you need to set UIN and password with @a setUin() and @a setPassword()
 * Then, you can all @a connect to establish a session with server.
 * After success session emits @a connected() signal.
 */

Session::Session(QObject *parent)
    : QObject(parent)
{
    d = new Private(this);
}

Session::~Session()
{
    delete d;
}

/**
 * Starts session with ICQ server.
 * @sa setUin(), setPassword()
 */
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

/**
 * Closes the session with ICQ server.
 */
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

    QStringListIterator ci(contactList());
    while ( ci.hasNext() ) {
        emit userOffline( ci.next() );
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
    d->onlineStatus = Offline;

    qDebug() << "[ICQ:Session]" << "Disconnected.";
    emit disconnected();
}

/**
 * Add @a uin to contact list.
 * @note If user requires authorization the request is sent automatically.
 */
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
        OnlineStatus status = onlineStatus(uin);
        if ( status == Offline ) {
            emit userOffline(uin);
        } else {
            emit userOnline(uin, status);
        }
    }
}

/**
 * Delete @a uin from contact list.
 */
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

/**
 * Grant authorization to @a toUin
 */
void Session::authGrant(const QString& toUin)
{
    if (!d->ssiManager) {
        return;
    }
    d->ssiManager->grantAuthorization(toUin);
}

/**
 * Deny authorization to @a toUin
 */
void Session::authDeny(const QString& toUin)
{
    if (!d->ssiManager) {
        return;
    }
    d->ssiManager->denyAuthorization(toUin);
}

/**
 * Get contact's display name from ssi-list
 */
QString Session::contactName(const QString& uin) const
{
    if ( !d->ssiManager ) {
        return QString();
    }
    Contact contact = d->ssiManager->contactByUin(uin);
    if ( !contact.isValid() ) {
        return QString();
    }
    return contact.displayName();
}

/**
 * Sets codec used for messages.
 */
void Session::setCodecForMessages(QTextCodec *codec)
{
    d->codec = codec;
}

/**
 * Sends @a message to @a recipient.
 */
void Session::sendMessage(const QString& recipient, const QString& message)
{
    if (!d->msgManager || !d->userInfoManager) {
        return;
    }

    Message msg;

    UserInfo ui = d->userInfoManager->getUserInfo(recipient);
    if ( d->userInfoManager->getUserStatus(recipient) == UserInfo::Offline || !ui.hasCapability( Capabilities[ccICQServerRelay] ) ) {
        // qDebug() << "[ICQ:Session]" << "sending offline message via channel 1";
        msg.setChannel(0x01);
    } else {
        // qDebug() << "[ICQ:Session]" << "sending message via channel 2";
        msg.setChannel(0x02);
    }

    msg.setSender(d->uin);
    msg.setReceiver(recipient);
    msg.setType(Message::PlainText);
    msg.setText( message.toUtf8() );
    d->msgManager->sendMessage(msg);
}

/**
 * Returns current connection status of the session.
 * @sa ConnectionStatus
 */
Session::ConnectionStatus Session::connectionStatus() const
{
    return d->connectionStatus;
}

/**
 * Returns list of UINs contained in contact list.
 */
QStringList Session::contactList() const
{
    if ( !d->ssiManager || d->connectionStatus != Connected ) {
        return QStringList();
    }

    QStringList contacts;

    QList<Contact> cl = d->ssiManager->contactList();
    QListIterator<Contact> ci(cl);
    while ( ci.hasNext() ) {
        contacts << ci.next().name();
    }
    return contacts;
}

/**
 * Returns user's online status.
 * @sa OnlineStatus
 */
Session::OnlineStatus Session::onlineStatus() const
{
    return d->onlineStatus;
}

/**
 * Request online status for selected @a uin.
 * @note UIN should be in server roster.
 * @overload
 */
Session::OnlineStatus Session::onlineStatus(const QString& uin) const
{
    if ( !d->userInfoManager ) {
        return Offline;
    }
    quint16 status = d->userInfoManager->getUserStatus(uin);

    if ( status == UserInfo::Online ) {
        return Online;
    }
    if ( status == UserInfo::Offline ) {
        return Offline;
    }

    OnlineStatus singleStatus = Online;
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
    return singleStatus;
}

/**
 * Returns server's hostname for this session.
 */
QString Session::serverHost() const
{
    return d->server;
}

/**
 * Returns server's port for this session.
 */
quint16 Session::serverPort() const
{
    return d->port;
}

/**
 * Returns user's UIN.
 */
QString Session::uin() const
{
    return d->uin;
}

/**
 * Sets user's UIN.
 * @note this method does nothing if user is connecting/connected.
 * @sa setPassword(), setServerHost(), setServerPort()
 */
void Session::setUin(const QString& uin)
{
    if ( connectionStatus() != Disconnected ) {
        return;
    }
    d->uin = uin;
}

/**
 * Sets user's password.
 * @note this method does nothing if user is connecting/connected.
 * @sa setUin(), setServerHost(), setServerPort()
 */
void Session::setPassword(const QString& password)
{
    if ( connectionStatus() != Disconnected ) {
        return;
    }
    d->password = password;
}

/**
 * Sets ICQ server hostname/IP.
 * @note this method does nothing if user is connecting/connected.
 * @sa setUin(), setPassword(), setServerPort()
 */
void Session::setServerHost(const QString& server)
{
    if ( connectionStatus() != Disconnected ) {
        return;
    }
    d->server = server;
}

/**
 * Sets ICQ server port.
 * @note this method does nothing if user is connecting/connected.
 * @sa setUin(), setPassword(), setServerHost()
 */
void Session::setServerPort(quint16 port)
{
    if ( connectionStatus() != Disconnected ) {
        return;
    }
    d->port = port;
}

/**
 * Sets user's online status.
 * @note this method does nothing if user is not connected.
 * @sa OnlineStatus
 */
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

/**
 * Sends request for own user details to the server.
 * @note this method does nothing if user is not connected to the server.
 */
void Session::requestOwnUserDetails()
{
    if ( !d->userInfoManager ) {
        return;
    }
    d->userInfoManager->requestOwnUserDetails(d->uin);
}

/**
 * Requests details for @a uin from server.
 * @note this method does nothing if user is not connected to the server.
 * @sa requestShortUserDetails(), shortUserDetails(), userDetails()
 */
void Session::requestUserDetails(const QString& uin)
{
    if ( !d->userInfoManager ) {
        return;
    }
    d->userInfoManager->requestUserDetails(uin);
}

/**
 * Request short user details for @a uin from server.
 * @note this method does nothing if user is not connected to the server
 * @sa requestUserDetails(), userDetails(), shortUserDetails()
 */
void Session::requestShortUserDetails(const QString& uin)
{
    if ( !d->userInfoManager ) {
        return;
    }
    d->userInfoManager->requestShortDetails(uin);
}

/**
 * Returns short user details for @a uin
 * @note this method returns empty details if user is not connected or
 * details were not previously requested with @a requestShortUserDetails()
 * @sa userDetails(), requestShortUserDetails(), requestUserDetails(), ShortUserDetails
 */
ShortUserDetails Session::shortUserDetails(const QString& uin) const
{
    if ( !d->userInfoManager ) {
        return ShortUserDetails();
    }
    return d->userInfoManager->shorUserDetails(uin);
}

/**
 * Returns user details for @a uin
 * @note this method returns empty details if user is not connected or
 * details were not previously requested with @a requestUserDetails()
 * @sa shortUserDetails(), requestShortUserDetails(), requestUserDetails(), UserDetails
 */
UserDetails Session::userDetails(const QString& uin) const
{
    if ( !d->userInfoManager ) {
        return UserDetails();
    }
    return d->userInfoManager->userDetails(uin);
}

/**
 * Return's connection information for @a uin.
 * @note this method returns empty UserInfo if user is not connected
 * or doesn't have @a uin in the contact list.
 */
UserInfo Session::userInfo(const QString& uin) const
{
    if ( !d->userInfoManager ) {
        return UserInfo();
    }
    return d->userInfoManager->getUserInfo(uin);
}

/**
 * Removes user details for @a uin from cache.
 * This is neccesary to request new user details for @a uin.
 */
void Session::removeUserDetails(const QString& uin)
{
    if ( !d->userInfoManager ) {
        return;
    }
    d->userInfoManager->clearUserDetails(uin);
}

/**
 * Removes short user details for @a uin from cache.
 * This is neccesary to request new short user details for @a uin.
 */
void Session::removeShortUserDetails(const QString& uin)
{
    if ( !d->userInfoManager ) {
        return;
    }
    d->userInfoManager->clearShortUserDetails(uin);
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
        qDebug() << "[ICQ:Session] Lookup failed:" << result.errorString();
        emit error("Host lookup failed. " + result.errorString() );
        disconnect();
        return;
    }

    d->peer = result.addresses().value(0);
    qDebug() << "[ICQ:Session] Found address:" << d->peer.toString();

    d->startLogin();
}

void Session::processConnectionTimeout()
{
    d->connectTimer->deleteLater();
    d->connectTimer = 0;

    emit error( tr("Connection timed out.") );
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

    QObject::connect( d->ssiManager, SIGNAL( authGranted(QString) ), SIGNAL( authGranted(QString) ) );
    QObject::connect( d->ssiManager, SIGNAL( authDenied(QString) ),  SIGNAL( authDenied(QString) ) );
    QObject::connect( d->ssiManager, SIGNAL( ssiActivated() ),       SIGNAL( rosterAvailable() ) );

    d->ssiManager->requestParameters();
    d->ssiManager->requestContactList();
}

void Session::processLoginDone()
{
    d->loginManager->deleteLater();
    d->loginManager = 0;

    d->connectTimer->start(CONNECTION_TIMEOUT);
    d->keepAliveTimer = new QTimer(this);
    QObject::connect( d->keepAliveTimer, SIGNAL( timeout() ), SLOT( sendKeepAlive() ) );
    d->keepAliveTimer->start(KEEP_ALIVE_INTERVAL);

    d->metaManager = new MetaInfoManager(d->socket, this);
    d->metaManager->setUin(d->uin);
    d->socket->setMetaManager(d->metaManager);

    d->userInfoManager = new UserInfoManager(d->socket, this);
    QObject::connect( d->userInfoManager, SIGNAL( statusChanged(int) ),      SLOT( processStatusChanged(int) ) );
    QObject::connect( d->userInfoManager, SIGNAL( userOnline(QString,int) ), SLOT( processUserStatus(QString,int) ) );
    QObject::connect( d->userInfoManager, SIGNAL( userOffline(QString) ),    SIGNAL( userOffline(QString) ) );
    QObject::connect( d->userInfoManager, SIGNAL( userDetailsAvailable(QString) ), SIGNAL( userDetailsAvailable(QString) ) );
    QObject::connect( d->userInfoManager, SIGNAL( shortUserDetailsAvailable(QString) ), SIGNAL( shortUserDetailsAvailable(QString) ) );
    QObject::connect( d->metaManager, SIGNAL( metaInfoAvailable(Word,Buffer&) ), d->userInfoManager, SLOT( incomingMetaInfo(Word,Buffer&) ) );
    d->userInfoManager->setTextCodec(d->codec);

    d->msgManager = new MessageManager(d->socket, this);
    QObject::connect( d->metaManager, SIGNAL( metaInfoAvailable(Word,Buffer&) ), d->msgManager, SLOT( incomingMetaInfo(Word,Buffer&) ) );
    QObject::connect( d->msgManager, SIGNAL( incomingMessage(Message) ), SLOT( processIncomingMessage(Message) ) );
    d->msgManager->setTextCodec(d->codec);
    d->msgManager->setUin(d->uin);
    d->msgManager->requestOfflineMessages();

    d->connectionStatus = Connected;

    /* hack to force setting of online status */
    OnlineStatus status = d->onlineStatus;
    d->onlineStatus = Offline;
    setOnlineStatus(status);

    QObject::connect( d->socket, SIGNAL( incomingFlap(FlapBuffer&) ), SLOT( processFlap(FlapBuffer&) ) );
    QObject::connect( d->socket, SIGNAL( incomingSnac(SnacBuffer&) ), SLOT( processSnac(SnacBuffer&) ) );

    qDebug() << "[ICQ:Session]" << "Connected.";
    emit connected();
}

void Session::processSnac(SnacBuffer& snac)
{
    if ( d->connectionStatus == Connected ) {
        d->keepAliveTimer->start(KEEP_ALIVE_INTERVAL);
        d->connectTimer->start(CONNECTION_TIMEOUT);
    }

    if ( snac.subtype() == 0x01 ) {
        d->processSnacError(snac);
    }
}

void Session::processIncomingMessage(const Message& msg)
{
    if ( !d->codec ) {
        d->codec = QTextCodec::codecForName("Windows-1251");
    }

    if ( msg.channel() == 0x04 ) {
        if ( msg.type() == Message::AuthRequest ) {
            emit authRequest( msg.sender() );
        }
        return;
    }

    if ( msg.type() == Message::PlainText ) {
        if ( msg.isOffline() ) {
            emit incomingMessage( msg.sender(), msg.text(d->codec), msg.timestamp() );
        } else {
            emit incomingMessage( msg.sender(), msg.text(d->codec) );
        }
    } else {
        qWarning( "[ICQ:Session] [User: %s] Ignoring message of type %s", qPrintable(d->uin), qPrintable(QString::number(msg.type(), 16)) );
    }
}

void Session::processFlap(FlapBuffer& flap)
{
    if ( flap.channel() == FlapBuffer::CloseChannel ) {
        qDebug() << "[ICQ:Session]" << "Flap stream was by server.";
        disconnect();
    }

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

Session::Private::IntStringPair Session::Private::subtypeOneErrors[] = {
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
    IntStringPair(0x11, tr("Sender is too \"evil\"") ),
    IntStringPair(0x12, tr("Receiver is too \"evil\"") ),
    IntStringPair(0x13, tr("User temporarily unavailable") ),
    IntStringPair(0x14, tr("No match") ),
    IntStringPair(0x15, tr("List overflow") ),
    IntStringPair(0x16, tr("Request ambiguous") ),
    IntStringPair(0x17, tr("Server queue full") ),
    IntStringPair(0x18, tr("Not while on AOL") )
};

QString Session::Private::errDescForCode(Word errorCode)
{
    for ( Word i = 0; !subtypeOneErrors[i].second.isEmpty(); ++i ) {
        if ( subtypeOneErrors[i].first == errorCode ) {
            return subtypeOneErrors[i].second;
        }
    }
    return tr("Unknown error");
}

/**
 * @fn void Session::connected()
 * This signal is emitted when user successfully logs on.
 */

/**
 * @fn void Session::disconnected()
 * This signal is emitted when user disconnects or the connection is lost.
 */

/**
 * @fn void Session::error(const QString& errorString)
 * This signal is emitted when session receives SNAC subtype 0x01 or error happens
 * during logging in.
 */

/**
 * @fn void Session::rosterAvailable()
 * This signal is emitted when session receives contact list from server during logging in.
 */

/**
 * @fn void Session::statusChanged(int onlineStatus)
 * This signal is emitted when server verifies that user's status changed.
 */

/**
 * @fn void Session::userOnline(const QString& uin, int status)
 * This signal is emitted when @a uin goes online or changes status.
 * @sa OnlineStatus
 */

/**
 * @fn void Session::userOffline(const QString& uin)
 * This signal is emitted when @a uin goes offline.
 */

/**
 * @fn void Session::authGranted(const QString& fromUin)
 * This signal is emitted when @a fromUin grants authorization to the user.
 */

/**
 * @fn void Session::authDenied(const QString& fromUin)
 * This signal is emitted when @a fromUin denies authorization to the user.
 */

/**
 * @fn void Session::authRequest(const QString& fromUin)
 * This signal is emitted when @a fromUin requests authorization from the user.
 */

/**
 * @fn void Session::incomingMessage(const QString& uin, const QString& msg)
 * This signal is emitted when the user receives a message from @a uin.
 */

/**
 * @fn void Session::incomingMessage(const QString& uin, const QString& msg, const QDateTime& timestamp)
 * This signal is emitted when the user receives an offline message from @a uin sent at @a timestamp
 */

/**
 * @fn void Session::shortUserDetailsAvailable(const QString& uin)
 * This signal is emitted when the user receives previously requested short user details
 * for @a uin
 * @sa shortUserDetails(), requestShortUserDetails(), ShortUserDetails
 */

/**
 * @fn void Session::userDetailsAvailable(const QString& uin)
 * This signal is emitted when the user receives previosly requested user details for @a uin.
 * @sa userDetails(), requestUserDetails(), UserDetails
 */


} /* end of namespace ICQ */

// vim:sw=4:ts=4:noet:nowrap
