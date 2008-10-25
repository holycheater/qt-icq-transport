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

#include "xmpp-core/Jid.h"
#include "xmpp-core/Presence.h"
#include "xmpp-ext/vCard.h"

#include "icqSession.h"
#include "types/icqShortUserDetails.h"
#include "types/icqUserInfo.h"

#include <QHash>
#include <QStringList>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTextCodec>
#include <QVariant>

class GatewayTask::Private
{
	public:
		Private(GatewayTask *parent);
		~Private();

		/* Jabber-ID-to-ICQ-Connection hash-table. */
		QHash<QString, ICQ::Session*> jidIcqTable;
		/* Connection & Jabber-ID hash-table */
		QHash<ICQ::Session*, QString> icqJidTable;


		struct vCardRequestInfo {
			QString requestID;
			QString resource;
		};
		/* Queue of vcard requests. Key is "<jid>-<uin>", value - requestID */
		QHash<QString,vCardRequestInfo> vCardRequests;

		QString icqHost;
		quint16 icqPort;

		QSqlDatabase db;

		GatewayTask *q;

		bool online;

		QHash<QString, int> reconnects;
};

GatewayTask::Private::Private(GatewayTask *parent)
{
	q = parent;
}

GatewayTask::Private::~Private()
{
	db.close();

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

void GatewayTask::setDatabaseLink(const QSqlDatabase& sql)
{
	d->db = sql;
	if ( !d->db.isOpen() && !d->db.open() ) {
		qCritical( "[GT] Database open failed: %s", qPrintable(d->db.lastError().text()) );
		exit(1);
		return;
	}

	QSqlQuery query;
	query.exec("CREATE TABLE IF NOT EXISTS users ("
				"jid TEXT,"
				"uin TEXT,"
				"password TEXT,"
				"PRIMARY KEY(jid)"
				")");
	query.exec("CREATE TABLE IF NOT EXISTS options ("
				"jid TEXT,"
				"option TEXT,"
				"value TEXT,"
				"PRIMARY KEY(jid,option)"
				")");
}

void GatewayTask::setIcqServer(const QString& host, quint16 port)
{
	d->icqHost = host;
	d->icqPort = port;
}

void GatewayTask::processRegister(const QString& user, const QString& uin, const QString& password)
{
	if ( d->jidIcqTable.contains(user) ) {
		ICQ::Session *conn = d->jidIcqTable.value(user);
		d->jidIcqTable.remove(user);
		d->icqJidTable.remove(conn);

		delete conn;
	}

	QSqlQuery query;
	/* prepare + bindvalue doesn't work... */
	query.exec( QString("REPLACE INTO users VALUES('%1', '%2', '%3')").arg(user,uin,password) );

	emit gatewayMessage( user, tr("You have been successfully registered") );
}

/**
 * Process unregister actions with database and close the legacy connection.
 */
void GatewayTask::processUnregister(const QString& user)
{
	ICQ::Session *conn = d->jidIcqTable.value(user);
	if ( conn ) {
		d->icqJidTable.remove(conn);
		d->jidIcqTable.remove(user);
		delete conn;
	}

	QSqlQuery query;

	query.exec( QString("SELECT * FROM users WHERE jid = '%1'").arg(user) );
	if ( !query.first() ) {
		return;
	}

	query.exec( QString("DELETE FROM users WHERE jid = '%1'").arg(user) );
	query.exec( QString("DELETE FROM options WHERE jid = '%1'").arg(user) );
}

void GatewayTask::processUserOnline(const Jid& user, int showStatus, bool first_login)
{
	if ( d->icqHost.isEmpty() || !d->icqPort ) {
		qCritical("[GT] processLogin: icq host and/or port values are not set. Aborting...");
		return;
	}
	ICQ::Session::OnlineStatus icqStatus;
	switch ( showStatus ) {
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

	if ( d->jidIcqTable.contains( user.bare() ) ) {
		ICQ::Session *conn = d->jidIcqTable.value( user.bare() );
		conn->setOnlineStatus(icqStatus);
		return;
	}

	QSqlQuery query;
	/* small hack with replace, because QSqlQuery somehow doesn't understand bindvalues there */
	query.exec( QString("SELECT uin, password FROM users WHERE jid = '%1' ").arg(user.bare()) );

	if ( query.first() ) {
		QString uin = query.value(0).toString();
		QString password = query.value(1).toString();

		ICQ::Session *conn = new ICQ::Session(this);
		conn->setUin(uin);
		conn->setPassword(password);
		conn->setServerHost(d->icqHost);
		conn->setServerPort(d->icqPort);
		conn->setOnlineStatus(ICQ::Session::Online);

		QObject::connect( conn, SIGNAL( statusChanged(int) ),               SLOT( processIcqStatus(int) ) );
		QObject::connect( conn, SIGNAL( userOnline(QString,int) ),          SLOT( processContactOnline(QString,int) ) );
		QObject::connect( conn, SIGNAL( userOffline(QString) ),             SLOT( processContactOffline(QString) ) );
		QObject::connect( conn, SIGNAL( authGranted(QString) ),             SLOT( processAuthGranted(QString) ) );
		QObject::connect( conn, SIGNAL( authDenied(QString) ),              SLOT( processAuthDenied(QString) ) );
		QObject::connect( conn, SIGNAL( authRequest(QString) ),             SLOT( processAuthRequest(QString) ) );
		QObject::connect( conn, SIGNAL( incomingMessage(QString,QString) ), SLOT( processIncomingMessage(QString,QString) ) );
		QObject::connect( conn, SIGNAL( connected() ),                      SLOT( processIcqSignOn() ) );
		QObject::connect( conn, SIGNAL( disconnected() ),                   SLOT( processIcqSignOff() ) );
		QObject::connect( conn, SIGNAL( error(QString) ),                   SLOT( processIcqError(QString) ) );
		QObject::connect( conn, SIGNAL( shortUserDetailsAvailable(QString) ), SLOT( processShortUserDetails(QString) ) );

		if ( first_login ) {
			QObject::connect( conn, SIGNAL( rosterAvailable() ), SLOT( processIcqFirstLogin() ) );
		}

		d->jidIcqTable.insert( user.bare(), conn );
		d->icqJidTable.insert( conn, user.bare() );

		query.exec( QString("SELECT value FROM options WHERE jid='%1' AND option='encoding'").arg(user.bare()) );
		QTextCodec *codec;
		if ( query.first() ) {
			codec = QTextCodec::codecForName( query.value(0).toByteArray() );
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
void GatewayTask::processUserOffline(const Jid& user)
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
	conn->disconnect();
	conn->deleteLater();
}

void GatewayTask::processUserStatusRequest(const Jid& user)
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
void GatewayTask::processSubscribeRequest(const Jid& user, const QString& uin)
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
void GatewayTask::processUnsubscribeRequest(const Jid& user, const QString& uin)
{
	ICQ::Session *conn = d->jidIcqTable.value( user.bare() );
	if ( !conn ) {
		return;
	}
	conn->contactDel(uin);
}

void GatewayTask::processAuthGrant(const Jid& user, const QString& uin)
{
	ICQ::Session *conn = d->jidIcqTable.value( user.bare() );
	if ( !conn ) {
		return;
	}
	conn->authGrant(uin);
}

void GatewayTask::processAuthDeny(const Jid& user, const QString& uin)
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
void GatewayTask::processSendMessage(const Jid& user, const QString& uin, const QString& message)
{
	ICQ::Session *conn = d->jidIcqTable.value( user.bare() );
	if ( !conn ) {
		return;
	}
	conn->sendMessage(uin, message);
}

void GatewayTask::processVCardRequest(const Jid& user, const QString& uin, const QString& requestID)
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
void GatewayTask::processCmd_RosterRequest(const Jid& user)
{
	ICQ::Session *conn = d->jidIcqTable.value( user.bare() );
	if ( !conn ) {
		return;
	}

	QStringList contacts = conn->contactList();
	QStringListIterator ci(contacts);
	while ( ci.hasNext() ) {
		emit subscriptionRequest( user, ci.next() );
	}
}

/**
 * Sends presence notification to all registered users.
 */
void GatewayTask::processGatewayOnline()
{
	d->online = true;
	QSqlQuery query;
	query.exec("SELECT jid FROM options WHERE option = 'auto-invite'");
	while ( query.next() ) {
		Jid jid = query.value(0).toString();
		emit probeRequest(jid);
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

	QSqlQuery query;
	query.exec("SELECT jid FROM users");

	d->online = false;

	while ( query.next() ) {
		Jid user = query.value(0).toString();
		if ( d->jidIcqTable.contains( user.bare() ) ) {
			ICQ::Session *session = d->jidIcqTable.value( user.bare() );

			QStringListIterator i( session->contactList() );
			while ( i.hasNext() ) {
				emit contactOffline( user, i.next() );
			}

			d->jidIcqTable.remove( user.bare() );
			d->icqJidTable.remove(session);

			session->disconnect();
			session->deleteLater();
		}
		emit offlineNotifyFor(user);
	}
}

void GatewayTask::processIcqError(const QString& desc)
{
	ICQ::Session *conn = qobject_cast<ICQ::Session*>( sender() );
	Jid user = d->icqJidTable.value(conn);

	emit gatewayMessage(user, desc);
}

void GatewayTask::processIcqSignOn()
{
	ICQ::Session *conn = qobject_cast<ICQ::Session*>( sender() );
	Jid user = d->icqJidTable.value(conn);

	emit onlineNotifyFor(user, XMPP::Presence::None);

	d->reconnects.remove( user.bare() );
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

	Jid user = d->icqJidTable.value(conn);
	emit offlineNotifyFor(user);

	d->icqJidTable.remove(conn);
	d->jidIcqTable.remove(user);
	conn->deleteLater();

	QSqlQuery query;
	query.exec( QString("SELECT value FROM options WHERE jid='%1' AND option='auto-reconnect'").arg(user) );
	if ( query.first() && query.value(0).toString() == "enabled" ) {
		int rCount = d->reconnects.value( user.bare() );
		if ( rCount >= 3 ) { // limit number of reconnects to 3.
			emit gatewayMessage(user, "Tried to reconnect 3 times, but no result. Stopping reconnects.");
			return;
		}
		d->reconnects.insert(user.bare(), ++rCount);
		// qDebug() << "[GT]" << "Processing auto-reconnect for user" << user;
		emit probeRequest(user);
	}
}

void GatewayTask::processIcqStatus(int status)
{
	ICQ::Session *conn = qobject_cast<ICQ::Session*>( sender() );
	Jid user = d->icqJidTable.value(conn);

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
	ICQ::Session *session = qobject_cast<ICQ::Session*>( sender() );
	Jid user = d->icqJidTable.value(session);

	QStringList contacts = session->contactList();
	QStringListIterator i(contacts);
	while ( i.hasNext() ) {
		emit subscriptionRequest( user, i.next() );
	}
}

void GatewayTask::processContactOnline(const QString& uin, int status)
{
	ICQ::Session *session = qobject_cast<ICQ::Session*>( sender() );
	Jid user = d->icqJidTable.value(session);

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
	Jid user = d->icqJidTable.value(conn);
	emit contactOffline(user, uin);
}

void GatewayTask::processIncomingMessage(const QString& senderUin, const QString& message)
{
	ICQ::Session *session = qobject_cast<ICQ::Session*>( sender() );
	Jid user = d->icqJidTable.value(session);
	QString msg = QString(message).replace('\r', "");
	emit incomingMessage(user, senderUin, msg, session->contactName(senderUin) );
}

/**
 * This slot is triggered when user @a uin grants authorization to jabber user.
 */
void GatewayTask::processAuthGranted(const QString& uin)
{
	ICQ::Session *session = qobject_cast<ICQ::Session*>( sender() );
	Jid user = d->icqJidTable.value(session);

	// qDebug() << "[GT]" << user << "granted auth to" << uin << "nick" << session->contactName(uin);
	emit subscriptionReceived( user, uin, session->contactName(uin) );
}

/**
 * This slot is triggered when user @a uin denies authorization to jabber user.
 */
void GatewayTask::processAuthDenied(const QString& uin)
{
	ICQ::Session *conn = qobject_cast<ICQ::Session*>( sender() );
	Jid user = d->icqJidTable.value(conn);

	// qDebug() << "[GT]" << user << "denied auth to" << uin;
	emit subscriptionRemoved(user, uin);
}

/**
 * This slot is triggered when user @a uin sends an authorization request to jabber user.
 */
void GatewayTask::processAuthRequest(const QString& uin)
{
	ICQ::Session *conn = qobject_cast<ICQ::Session*>( sender() );
	Jid user = d->icqJidTable.value(conn);

	emit subscriptionRequest(user, uin);
}

void GatewayTask::processShortUserDetails(const QString& uin)
{
	ICQ::Session *session = qobject_cast<ICQ::Session*>( sender() );
	Jid user = d->icqJidTable.value(session);

	QString key = QString(user)+"-"+uin;

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
