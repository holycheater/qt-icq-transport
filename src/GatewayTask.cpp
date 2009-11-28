/*
 * GatewayTask.cpp - Handles tasks from jabber clients (register/status/message/etc)
 * Copyright (C) 2008  Alexander Saltykov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "GatewayTask.h"
#include "UserManager.h"

#include "xmpp-core/jid.h"
#include "xmpp-core/presence.h"
#include "xmpp-ext/rosterxitem.h"
#include "xmpp-ext/vcard.h"

#include "icqSession.h"
#include "types/icqShortUserDetails.h"
#include "types/icqUserInfo.h"

#include <QDateTime>
#include <QHash>
#include <QList>
#include <QStringList>
#include <QSqlError>
#include <QTextCodec>
#include <QVariant>

#include <stdlib.h>


#define GET_JID_BY_SENDER(_str_bare, _jid_user) \
    ICQ::Session *session = qobject_cast<ICQ::Session*>( sender() ); \
    QString _str_bare = d->icqJidTable[session]; \
    XMPP::Jid _jid_user = d->jidResources[user_bare];

class GatewayTask::Private
{
    public:
        Private(GatewayTask *parent);
        ~Private();

        typedef QHash<QString, XMPP::Jid> HashJidBareFull;
        typedef QHash<QString, ICQ::Session*> HashJidIcq;
        typedef QHash<ICQ::Session*, QString> HashIcqJid;

        /* Jabber-ID-to-ICQ-Connection hash-table. (JID is bare) */
        HashJidIcq jidIcqTable;
        /* Connection & Jabber-ID hash-table (JID is bare) */
        HashIcqJid icqJidTable;
        HashJidBareFull jidResources;


        struct vCardRequestInfo {
            QString requestID;
            QString resource;
        };
        /* Queue of vcard requests. Key is "<jid>-<uin>", value - requestID */
        QHash<QString,vCardRequestInfo> vCardRequests;

        QString icqHost;
        quint16 icqPort;

        GatewayTask *q;

        bool online;

        QHash<QString, int> reconnects;
};

static ICQ::Session::OnlineStatus xmmpToIcqStatus(XMPP::Presence::Show status)
{
    ICQ::Session::OnlineStatus icqStatus;
    switch ( status ) {
    case XMPP::Presence::None:
        icqStatus = ICQ::Session::Online;
        break;
    case XMPP::Presence::Chat:
        icqStatus = ICQ::Session::FreeForChat;
        break;
    case XMPP::Presence::Away:
        icqStatus = ICQ::Session::Away;
        break;
    case XMPP::Presence::NotAvailable:
        icqStatus = ICQ::Session::NotAvailable;
        break;
    case XMPP::Presence::DoNotDisturb:
        icqStatus = ICQ::Session::DoNotDisturb;
        break;
    }
    return icqStatus;
}

GatewayTask::Private::Private(GatewayTask *parent)
{
    q = parent;
}

GatewayTask::Private::~Private()
{
    qDeleteAll(jidIcqTable);
    icqJidTable.clear();
}

GatewayTask::GatewayTask(QObject *parent)
    : QObject(parent)
{
    d = new Private(this);
}

GatewayTask::~GatewayTask()
{
    delete d;
}

void GatewayTask::setIcqServer(const QString& host, quint16 port)
{
    d->icqHost = host;
    d->icqPort = port;
}

void GatewayTask::processRegister(const XMPP::Jid& user, const QString& uin, const QString& password)
{
    if ( d->jidIcqTable.contains(user.bare()) ) {
        ICQ::Session *conn = d->jidIcqTable.value(user.bare());
        d->jidIcqTable.remove(user.bare());
        d->jidResources.remove(user.bare());
        d->icqJidTable.remove(conn);

        delete conn;
    }

    UserManager::instance()->add(user.bare(), uin, password);
    UserManager::instance()->setOption(user.bare(), "first_login", QVariant(true));
    emit gatewayMessage( user, tr("You have been successfully registered") );
}

/**
 * Process unregister actions with database and close the legacy connection.
 */
void GatewayTask::processUnregister(const XMPP::Jid& user)
{
    ICQ::Session *conn = d->jidIcqTable.value(user.bare());
    if ( conn ) {
        d->icqJidTable.remove(conn);
        d->jidIcqTable.remove(user.bare());
        d->jidResources.remove(user.bare());
        delete conn;
    }
    UserManager::instance()->del(user);
}

void GatewayTask::processUserOnline(const XMPP::Jid& user, int showStatus)
{
    bool first_login = UserManager::instance()->getOption(user.bare(), "first_login").toBool();

    if ( d->icqHost.isEmpty() || !d->icqPort ) {
        qCritical("[GT] processLogin: icq host and/or port values are not set. Aborting...");
        return;
    }
    ICQ::Session::OnlineStatus icqStatus = xmmpToIcqStatus(XMPP::Presence::Show(showStatus));

    if ( d->jidIcqTable.contains( user.bare() ) ) {
        ICQ::Session *conn = d->jidIcqTable.value( user.bare() );
        conn->setOnlineStatus(icqStatus);
        d->jidResources.insert(user.bare(), user);
        return;
    }

    if ( UserManager::instance()->isRegistered(user.bare()) ) {
        QString uin = UserManager::instance()->getUin(user.bare());
        QString password = UserManager::instance()->getPassword(user.bare());

        ICQ::Session *conn = new ICQ::Session(this);
        conn->setUin(uin);
        conn->setPassword(password);
        conn->setServerHost(d->icqHost);
        conn->setServerPort(d->icqPort);
        conn->setOnlineStatus(ICQ::Session::Online);

        QObject::connect( conn, SIGNAL( statusChanged(int) ),
                          SLOT( processIcqStatus(int) ) );
        QObject::connect( conn, SIGNAL( userOnline(QString,int) ),
                          SLOT( processContactOnline(QString,int) ) );
        QObject::connect( conn, SIGNAL( userOffline(QString) ),
                          SLOT( processContactOffline(QString) ) );
        QObject::connect( conn, SIGNAL( authGranted(QString) ),
                          SLOT( processAuthGranted(QString) ) );
        QObject::connect( conn, SIGNAL( authDenied(QString) ),
                          SLOT( processAuthDenied(QString) ) );
        QObject::connect( conn, SIGNAL( authRequest(QString) ),
                          SLOT( processAuthRequest(QString) ) );
        QObject::connect( conn, SIGNAL( incomingMessage(QString,QString) ),
                          SLOT( processIncomingMessage(QString,QString) ) );
        QObject::connect( conn, SIGNAL( incomingMessage(QString,QString,QDateTime) ),
                          SLOT( processIncomingMessage(QString,QString,QDateTime) ) );
        QObject::connect( conn, SIGNAL( connected() ),
                          SLOT( processIcqSignOn() ) );
        QObject::connect( conn, SIGNAL( disconnected() ),
                          SLOT( processIcqSignOff() ) );
        QObject::connect( conn, SIGNAL( error(QString) ),
                          SLOT( processIcqError(QString) ) );
        QObject::connect( conn, SIGNAL( shortUserDetailsAvailable(QString) ),
                          SLOT( processShortUserDetails(QString) ) );

        if ( first_login ) {
            QObject::connect( conn, SIGNAL( rosterAvailable() ), SLOT( processIcqFirstLogin() ) );
        }

        d->jidIcqTable.insert(user.bare(), conn);
        d->icqJidTable.insert(conn, user.bare());
        d->jidResources.insert(user.bare(), user);

        QTextCodec *codec;
        if ( UserManager::instance()->hasOption(user.bare(), "encoding") ) {
            codec = QTextCodec::codecForName( UserManager::instance()->getOption(user.bare(), "encoding").toByteArray() );
            if ( codec == 0 ) {
                codec = QTextCodec::codecForName("windows-1251");
            }
        } else {
            codec = QTextCodec::codecForName("windows-1251");
        }
        Q_ASSERT( codec != 0 );
        conn->setCodecForMessages(codec);
        conn->connect();
    }
}

/**
 * This slot is triggered when jabber user @a user goes offline.
 */
void GatewayTask::processUserOffline(const XMPP::Jid& user)
{
    ICQ::Session *conn = d->jidIcqTable.value( user.bare() );
    emit offlineNotifyFor(user);
    if ( !conn ) {
        return;
    }

    QStringListIterator ci(conn->contactList());
    while ( ci.hasNext() ) {
        emit contactOffline( user, ci.next() );
    }

    d->jidIcqTable.remove( user.bare() );
    d->icqJidTable.remove(conn);
    d->jidResources.remove( user.bare() );
    conn->disconnect();
    conn->deleteLater();
}

void GatewayTask::processUserStatusRequest(const XMPP::Jid& user)
{
    ICQ::Session *session = d->jidIcqTable.value( user.bare() );
    if ( !session ) {
        return;
    }
    if ( session->onlineStatus() == ICQ::Session::Offline ) {
        emit offlineNotifyFor(user);
    } else {
        int show;
        switch ( session->onlineStatus() ) {
            case ICQ::Session::Away:
                show = XMPP::Presence::Away;
                break;
            case ICQ::Session::NotAvailable:
                show = XMPP::Presence::NotAvailable;
                break;
            case ICQ::Session::FreeForChat:
                show = XMPP::Presence::Chat;
                break;
            case ICQ::Session::DoNotDisturb:
                show = XMPP::Presence::DoNotDisturb;
                break;
            default:
                show = XMPP::Presence::None;
                break;
        }
        emit onlineNotifyFor(user, show);
    }
}

/**
 * This slot is triggered when jabber user @a user requests authorization/add-to-contact from icq user @a uin
 */
void GatewayTask::processSubscribeRequest(const XMPP::Jid& user, const QString& uin)
{
    ICQ::Session *conn = d->jidIcqTable.value( user.bare() );
    if ( !conn ) {
        return;
    }
    conn->contactAdd(uin);
}

/**
 * This slot is triggered when jabber user @a user requests to remove a contact @a uin from server.
 */
void GatewayTask::processUnsubscribeRequest(const XMPP::Jid& user, const QString& uin)
{
    ICQ::Session *conn = d->jidIcqTable.value( user.bare() );
    if ( !conn ) {
        return;
    }
    conn->contactDel(uin);
}

void GatewayTask::processAuthGrant(const XMPP::Jid& user, const QString& uin)
{
    ICQ::Session *conn = d->jidIcqTable.value( user.bare() );
    if ( !conn ) {
        return;
    }
    conn->authGrant(uin);
}

void GatewayTask::processAuthDeny(const XMPP::Jid& user, const QString& uin)
{
    ICQ::Session *conn = d->jidIcqTable.value( user.bare() );
    if ( !conn ) {
        return;
    }
    conn->authDeny(uin);
}

/**
 * Sends @a message from jabber-user @a user to ICQ user with specified @a uin
 */
void GatewayTask::processSendMessage(const XMPP::Jid& user, const QString& uin, const QString& message)
{
    ICQ::Session *conn = d->jidIcqTable.value( user.bare() );
    if ( !conn ) {
        return;
    }
    conn->sendMessage(uin, message);
}

void GatewayTask::processVCardRequest(const XMPP::Jid& user, const QString& uin, const QString& requestID)
{
    ICQ::Session *session = d->jidIcqTable.value( user.bare() );
    if ( !session ) {
        emit incomingVCard(user, uin, requestID, XMPP::vCard() );
        return;
    }
    QString key = user.bare()+"-"+uin;
    Private::vCardRequestInfo info;
    info.requestID = requestID;
    info.resource = user.resource();
    d->vCardRequests.insert(key, info);
    session->requestShortUserDetails(uin);
}

/**
 * Process legacy roster request from jabber user @a user.
 */
void GatewayTask::processCmd_RosterRequest(const XMPP::Jid& user)
{
    ICQ::Session *session = d->jidIcqTable.value( user.bare() );
    if ( !session ) {
        return;
    }

    QStringList contacts = session->contactList();
    QStringListIterator i(contacts);
    QList<XMPP::RosterXItem> items;
    while ( i.hasNext() ) {
        QString uin = i.next();
        QString name = session->contactName(uin);
        XMPP::RosterXItem item(uin, XMPP::RosterXItem::Add, name);
        items << item;
    }
    emit rosterAdd(user, items);
}

/**
 * Sends presence notification to all registered users.
 */
void GatewayTask::processGatewayOnline()
{
    d->online = true;

    QStringListIterator ri(UserManager::instance()->getUserList());
    while ( ri.hasNext() ) {
        emit offlineNotifyFor( XMPP::Jid(ri.next()) );
    }

    QStringListIterator auto_invite(UserManager::instance()->getUserListByOptVal("auto-invite", QVariant(true)));
    while ( auto_invite.hasNext() ) {
        emit probeRequest( XMPP::Jid(auto_invite.next()) );
    }
}

/**
 * Sends offline presence notifications to all registered users.
 */
void GatewayTask::processShutdown()
{
    if ( !d->online ) {
        return;
    }
    d->online = false;

    QStringListIterator ui(UserManager::instance()->getUserList());
    while ( ui.hasNext() ) {
        QString user = ui.next();
        if ( d->jidIcqTable.contains(user) ) {
            ICQ::Session *session = d->jidIcqTable.value(user);

            QStringListIterator ci( session->contactList() );
            while ( ci.hasNext() ) {
                emit contactOffline( user, ci.next() );
            }

            d->jidIcqTable.remove(user);
            d->icqJidTable.remove(session);
            d->jidResources.remove(user);

            session->disconnect();
            session->deleteLater();
        }
        emit offlineNotifyFor(user);
    }
}

void GatewayTask::processIcqError(const QString& desc)
{
    GET_JID_BY_SENDER(user_bare,user);
    emit gatewayMessage(user, desc);
}

void GatewayTask::processIcqSignOn()
{
    GET_JID_BY_SENDER(user_bare,user);
    emit onlineNotifyFor(user, XMPP::Presence::None);
    d->reconnects.remove(user_bare);
}

void GatewayTask::processIcqSignOff()
{
    if ( !d->online ) {
        return;
    }

    ICQ::Session *conn = qobject_cast<ICQ::Session*>( sender() );
    if ( !d->icqJidTable.contains(conn) ) {
        return;
    }

    QString user_bare = d->icqJidTable[conn];
    XMPP::Jid user = d->jidResources[user_bare];
    emit offlineNotifyFor(user);

    d->icqJidTable.remove(conn);
    d->jidIcqTable.remove(user_bare);
    d->jidResources.remove(user_bare);
    conn->deleteLater();

    bool reconnect = UserManager::instance()->getOption(user_bare, "auto-reconnect").toBool();
    if ( reconnect ) {
        int rCount = d->reconnects.value(user_bare);
        if ( rCount >= 3 ) { // limit number of reconnects to 3.
            emit gatewayMessage(user, "Tried to reconnect 3 times, but no result. Stopping reconnects.");
            return;
        }
        d->reconnects.insert(user_bare, ++rCount);
        // qDebug() << "[GT]" << "Processing auto-reconnect for user" << user;
        emit probeRequest(user);
    }
}

void GatewayTask::processIcqStatus(int status)
{
    GET_JID_BY_SENDER(user_bare,user);

    int show;
    switch ( status ) {
        case ICQ::Session::Away:
            show = XMPP::Presence::Away;
            break;
        case ICQ::Session::NotAvailable:
            show = XMPP::Presence::NotAvailable;
            break;
        case ICQ::Session::FreeForChat:
            show = XMPP::Presence::Chat;
            break;
        case ICQ::Session::DoNotDisturb:
            show = XMPP::Presence::DoNotDisturb;
            break;
        default:
            show = XMPP::Presence::None;
            break;
    }
    emit onlineNotifyFor(user, show);
}

void GatewayTask::processIcqFirstLogin()
{
    GET_JID_BY_SENDER(user_bare,user);

    QStringList contacts = session->contactList();
    QStringListIterator i(contacts);
    QList<XMPP::RosterXItem> items;
    while ( i.hasNext() ) {
        QString uin = i.next();
        QString name = session->contactName(uin);
        XMPP::RosterXItem item(uin, XMPP::RosterXItem::Add, name);
        items << item;
    }
    emit rosterAdd(user, items);
    UserManager::instance()->setOption(user_bare, "first_login", QVariant(false));
}

void GatewayTask::processContactOnline(const QString& uin, int status)
{
    GET_JID_BY_SENDER(user_bare,user);

    int showStatus;
    switch ( status ) {
        case ICQ::Session::Away:
            showStatus = XMPP::Presence::Away;
            break;
        case ICQ::Session::NotAvailable:
            showStatus = XMPP::Presence::NotAvailable;
            break;
        case ICQ::Session::DoNotDisturb:
            showStatus = XMPP::Presence::DoNotDisturb;
            break;
        case ICQ::Session::FreeForChat:
            showStatus = XMPP::Presence::Chat;
            break;
        default:
            showStatus = XMPP::Presence::None;
            break;
    }

    emit contactOnline( user, uin, showStatus, session->contactName(uin) );
}

void GatewayTask::processContactOffline(const QString& uin)
{
    ICQ::Session *conn = qobject_cast<ICQ::Session*>( sender() );
    if ( !conn || !d->icqJidTable.contains(conn) ) {
        return;
    }
    QString user_bare = d->icqJidTable[conn];
    XMPP::Jid user = d->jidResources[user_bare];
    emit contactOffline(user, uin);
}

void GatewayTask::processIncomingMessage(const QString& senderUin, const QString& message)
{
    GET_JID_BY_SENDER(user_bare,user);
    QString msg = QString(message).replace('\r', "");
    emit incomingMessage(user, senderUin, msg, session->contactName(senderUin));
}

void GatewayTask::processIncomingMessage(const QString& senderUin, const QString& message, const QDateTime& timestamp)
{
    GET_JID_BY_SENDER(user_bare,user);
    QString msg = QString(message).replace('\r', "");
    emit incomingMessage(user, senderUin, msg, session->contactName(senderUin), timestamp.toUTC());
}

/**
 * This slot is triggered when user @a uin grants authorization to jabber user.
 */
void GatewayTask::processAuthGranted(const QString& uin)
{
    ICQ::Session *session = qobject_cast<ICQ::Session*>( sender() );
    XMPP::Jid user = d->icqJidTable[session];

    // qDebug() << "[GT]" << user << "granted auth to" << uin << "nick" << session->contactName(uin);
    emit subscriptionReceived( user, uin, session->contactName(uin) );
}

/**
 * This slot is triggered when user @a uin denies authorization to jabber user.
 */
void GatewayTask::processAuthDenied(const QString& uin)
{
    ICQ::Session *session = qobject_cast<ICQ::Session*>( sender() );
    XMPP::Jid user = d->icqJidTable[session];

    // qDebug() << "[GT]" << user << "denied auth to" << uin;
    emit subscriptionRemoved(user, uin);
}

/**
 * This slot is triggered when user @a uin sends an authorization request to jabber user.
 */
void GatewayTask::processAuthRequest(const QString& uin)
{
    ICQ::Session *session = qobject_cast<ICQ::Session*>( sender() );
    XMPP::Jid user = d->icqJidTable[session];

    emit subscriptionRequest(user, uin);
}

void GatewayTask::processShortUserDetails(const QString& uin)
{
    GET_JID_BY_SENDER(user_bare,user);
    QString key = QString(user_bare)+"-"+uin;

    if ( !d->vCardRequests.contains(key) ) {
        // qDebug() << "[GT]" << "Request was not logged";
        return;
    }

    Private::vCardRequestInfo info = d->vCardRequests.take(key);

    ICQ::ShortUserDetails details = session->shortUserDetails(uin);
    XMPP::vCard vcard;
    vcard.setNickname( details.nick() );
    vcard.setFullName( QString( details.firstName() + " " + details.lastName() ).trimmed() );
    vcard.setFamilyName( details.lastName() );
    vcard.setGivenName( details.firstName() );

    ICQ::UserInfo ui = session->userInfo(uin);
    QList<ICQ::Guid> guids = ui.capabilities();
    if ( guids.size() > 0  ) {
        QString notes = QString("Capabilities:") + QChar(QChar::LineSeparator);
        QListIterator<ICQ::Guid> i(guids);
        while ( i.hasNext() ) {
            notes += i.next().toString() + QChar(QChar::LineSeparator);
        }
        vcard.setDescription(notes);
    }

    if ( !info.resource.isEmpty() ) {
        user.setResource(info.resource);
    }

    emit incomingVCard(user, uin, info.requestID, vcard);
}

// vim:et:ts=4:sw=4:nowrap
