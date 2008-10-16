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
#include "icqSession.h"

#include <QHash>
#include <QStringList>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include <QtDebug>

class GatewayTask::Private
{
	public:
		Private(GatewayTask *parent);
		~Private();

		/* Jabber-ID-to-ICQ-Connection hash-table. */
		QHash<QString, ICQ::Session*> jidIcqTable;
		/* Connection & Jabber-ID hash-table */
		QHash<ICQ::Session*, QString> icqJidTable;

		QString icqHost;
		quint16 icqPort;

		QSqlDatabase db;

		GatewayTask *q;

		bool online;
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
		qDebug() << "[GT]" << "Database open failed" << d->db.lastError();
		return;
	}

	QSqlQuery query;
	query.exec("CREATE TABLE IF NOT EXISTS users ("
				"jid TEXT,"
				"uin TEXT,"
				"password TEXT,"
				"PRIMARY KEY(jid)"
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
	query.exec( QString("REPLACE INTO users VALUES('_user', '_uin', '_pass')").replace("_user", user).replace("_uin", uin).replace("_pass", password) );

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

	query.exec( QString("SELECT * FROM users WHERE jid = '_user'").replace("_user", user) );
	if ( !query.first() ) {
		return;
	}

	query.exec( QString("DELETE FROM users WHERE jid = '_user'").replace("_user", user) );
}

void GatewayTask::processUserOnline(const Jid& user, int showStatus)
{
	if ( d->icqHost.isEmpty() || !d->icqPort ) {
		qDebug() << "[GT] processLogin: icq host and/or port values are not set. Aborting...";
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
	query.exec( QString("SELECT uin, password FROM users WHERE jid = '?' ").replace( "?", user.bare() ) );

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

		d->jidIcqTable.insert( user.bare(), conn );
		d->icqJidTable.insert( conn, user.bare() );

		conn->connect();
	}
}

/**
 * This slot is triggered when jabber user @a user goes offline.
 */
void GatewayTask::processUserOffline(const Jid& user)
{
	ICQ::Session *conn = d->jidIcqTable.value( user.bare() );
	if ( !conn ) {
		return;
	}
	conn->disconnect();
	d->jidIcqTable.remove( user.bare() );
	d->icqJidTable.remove(conn);
	conn->deleteLater();
}

/**
 * This slot is triggered when jabber user @a user requests authorization/add-to-contact from icq user @a uin
 */
void GatewayTask::processSubscribeRequest(const Jid& user, const QString& uin)
{
	ICQ::Session *conn = d->jidIcqTable.value( user.bare() );
	if ( !conn ) {
		emit gatewayMessage( user, tr("Error. Contact add request: You are not logged on") );
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
		emit gatewayMessage( user, tr("Error. Contact delete request: You are not logged on") );
		return;
	}
	conn->contactDel(uin);
}

void GatewayTask::processAuthGrant(const Jid& user, const QString& uin)
{
	ICQ::Session *conn = d->jidIcqTable.value( user.bare() );
	if ( !conn ) {
		emit gatewayMessage( user, tr("Error. Auth Grant: You are not logged on") );
		return;
	}
	conn->authGrant(uin);
}

void GatewayTask::processAuthDeny(const Jid& user, const QString& uin)
{
	ICQ::Session *conn = d->jidIcqTable.value( user.bare() );
	if ( !conn ) {
		emit gatewayMessage( user, tr("Error. Auth Deny: You are not logged on") );
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
		emit gatewayMessage( user, tr("Error. Unable to send message: You are not logged on") );
		return;
	}
	conn->sendMessage(uin, message);
}

/**
 * Process legacy roster request from jabber user @a user.
 */
void GatewayTask::processCmd_RosterRequest(const Jid& user)
{
	qDebug() << "[GT]" << "Roster request from" << user;

	ICQ::Session *conn = d->jidIcqTable.value( user.bare() );
	if ( !conn ) {
		emit gatewayMessage( user, tr("Error. Unable to process roster-request command: You are not logged on") );
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

	emit onlineNotifyFor(user);
}

void GatewayTask::processIcqSignOff()
{
	if ( !d->online ) {
		return;
	}

	ICQ::Session *conn = qobject_cast<ICQ::Session*>( sender() );
	Jid user = d->icqJidTable.value(conn);

	emit offlineNotifyFor(user);

	QStringList contacts = conn->contactList();
	QStringListIterator i(contacts);
	while ( i.hasNext() ) {
		emit contactOffline( user, i.next() );
	}
}

void GatewayTask::processIcqStatus(int status)
{
	ICQ::Session *conn = qobject_cast<ICQ::Session*>( sender() );
	qDebug() << "icq status for" << conn->uin() << "changed to" << status;
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
}

void GatewayTask::processContactOnline(const QString& uin, int status)
{
	ICQ::Session *conn = qobject_cast<ICQ::Session*>( sender() );
	Jid user = d->icqJidTable.value(conn);

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

	emit contactOnline(user, uin, showStatus);
}

void GatewayTask::processContactOffline(const QString& uin)
{
	ICQ::Session *conn = qobject_cast<ICQ::Session*>( sender() );
	Jid user = d->icqJidTable.value(conn);
	emit contactOffline(user, uin);
}

void GatewayTask::processIncomingMessage(const QString& senderUin, const QString& message)
{
	ICQ::Session *conn = qobject_cast<ICQ::Session*>( sender() );
	Jid user = d->icqJidTable.value(conn);
	emit incomingMessage(user, senderUin, message);
}

/**
 * This slot is triggered when user @a uin grants authorization to jabber user.
 */
void GatewayTask::processAuthGranted(const QString& uin)
{
	ICQ::Session *conn = qobject_cast<ICQ::Session*>( sender() );
	Jid user = d->icqJidTable.value(conn);

	qDebug() << "[GT]" << user << "granted auth to" << uin;
	emit subscriptionReceived(user, uin);
}

/**
 * This slot is triggered when user @a uin denies authorization to jabber user.
 */
void GatewayTask::processAuthDenied(const QString& uin)
{
	ICQ::Session *conn = qobject_cast<ICQ::Session*>( sender() );
	Jid user = d->icqJidTable.value(conn);

	qDebug() << "[GT]" << user << "denied auth to" << uin;
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
