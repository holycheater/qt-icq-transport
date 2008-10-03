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
#include "icqConnection.h"
#include "types/icqUserInfo.h"

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
		QHash<QString, ICQ::Connection*> jidIcqTable;
		/* Connection & Jabber-ID hash-table */
		QHash<ICQ::Connection*, QString> icqJidTable;

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
	QSqlQuery query;
	query.prepare("REPLACE INTO users VALUES('?', '?', '?')");
	query.addBindValue(user);
	query.addBindValue(uin);
	query.addBindValue(password);
	query.exec();
}

void GatewayTask::processUnregister(const QString& user)
{
	QSqlQuery query;
	query.prepare("DELETE FROM users WHERE jid = '?'");
	query.addBindValue(user);
	query.exec();
}

void GatewayTask::processUserOnline(const Jid& user, int showStatus)
{
	if ( d->icqHost.isEmpty() || !d->icqPort ) {
		qDebug() << "[GT] processLogin: icq host and/or port values are not set. Aborting...";
		return;
	}
	if ( d->jidIcqTable.contains( user.bare() ) ) {
		ICQ::Connection *conn = d->jidIcqTable.value( user.bare() );
		int icqStatus;
		switch ( showStatus ) {
			case XMPP::Presence::None:
				icqStatus = ICQ::UserInfo::Online;
				break;
			case XMPP::Presence::Chat:
				icqStatus = ICQ::UserInfo::FreeForChat;
				break;
			case XMPP::Presence::Away:
				icqStatus = ICQ::UserInfo::Away;
				break;
			case XMPP::Presence::NotAvailable:
				icqStatus = ICQ::UserInfo::Away | ICQ::UserInfo::NotAvailable;
				break;
			case XMPP::Presence::DoNotDisturb:
				icqStatus = ICQ::UserInfo::Away | ICQ::UserInfo::Occupied | ICQ::UserInfo::DoNotDisturb;
				break;
		}
		conn->setOnlineStatus(icqStatus);
		return;
	}

	QSqlQuery query;
	/* small hack with replace, because QSqlQuery somehow doesn't understand bindvalues there */
	query.exec( QString("SELECT uin, password FROM users WHERE jid = '?' ").replace( "?", user.bare() ) );

	if ( query.first() ) {
		emit onlineNotifyFor(user);
		QString uin = query.value(0).toString();
		QString password = query.value(1).toString();
		qDebug() << "[GT]" << "credentails for" << user.bare() << "are:" << uin << password;

		ICQ::Connection *conn = new ICQ::Connection(uin, password, d->icqHost, d->icqPort);

		QObject::connect( conn, SIGNAL( statusChanged(int) ), SLOT( processIcqStatus(int) ) );
		QObject::connect( conn, SIGNAL( userOnline(QString,quint16) ), SLOT( processContactOnline(QString,quint16) ) );
		QObject::connect( conn, SIGNAL( userOffline(QString) ), SLOT( processContactOffline(QString) ) );
		QObject::connect( conn, SIGNAL( authGranted(QString) ), SLOT( processAuthGranted(QString) ) );
		QObject::connect( conn, SIGNAL( authDenied(QString) ), SLOT( processAuthDenied(QString) ) );
		QObject::connect( conn, SIGNAL( authRequest(QString) ), SLOT( processAuthRequest(QString) ) );
		QObject::connect( conn, SIGNAL( incomingMessage(QString,QString) ), SLOT( processIncomingMessage(QString,QString) ) );
		QObject::connect( conn, SIGNAL( signedOn() ), SLOT( processIcqSignOn() ) );
		QObject::connect( conn, SIGNAL( error(QString) ), SLOT( processIcqError(QString) ) );

		d->jidIcqTable.insert( user.bare(), conn );
		d->icqJidTable.insert( conn, user.bare() );
		conn->setOnlineStatus(ICQ::UserInfo::Online);
	}
}

/**
 * This slot is triggered when jabber user @a user goes offline.
 */
void GatewayTask::processUserOffline(const Jid& user)
{
	ICQ::Connection *conn = d->jidIcqTable.value( user.bare() );
	conn->signOff();
	d->jidIcqTable.remove( user.bare() );
	d->icqJidTable.remove(conn);
	conn->deleteLater();
}

/**
 * This slot is triggered when jabber user @a user requests authorization/add-to-contact from icq user @a uin
 */
void GatewayTask::processSubscribeRequest(const Jid& user, const QString& uin)
{
	qDebug() << "[GT]" << user << "sent subscribe request to" << uin;
	ICQ::Connection *conn = d->jidIcqTable.value( user.bare() );
	conn->contactAdd(uin);
}

/**
 * This slot is triggered when jabber user @a user requests to remove a contact @a uin from server.
 */
void GatewayTask::processUnsubscribeRequest(const Jid& user, const QString& uin)
{
	qDebug() << "[GT]" << user << "sent unsubscribe request to" << uin;
	ICQ::Connection *conn = d->jidIcqTable.value( user.bare() );
	conn->contactDel(uin);
}

void GatewayTask::processAuthGrant(const Jid& user, const QString& uin)
{
	ICQ::Connection *conn = d->jidIcqTable.value( user.bare() );
	conn->grantAuth(uin);
}

void GatewayTask::processAuthDeny(const Jid& user, const QString& uin)
{
	ICQ::Connection *conn = d->jidIcqTable.value( user.bare() );
	conn->denyAuth(uin);
}

/**
 * Sends @a message from jabber-user @a user to ICQ user with specified @a uin
 */
void GatewayTask::processSendMessage(const Jid& user, const QString& uin, const QString& message)
{
	ICQ::Connection *conn = d->jidIcqTable.value( user.bare() );
	conn->sendMessage(uin, message);
}

/**
 * Sends presence notification to all registered users.
 */
void GatewayTask::processGatewayOnline()
{
	QSqlQuery query;

	query.exec("SELECT jid FROM users");
	while ( query.next() ) {
		Jid user = query.value(0).toString();
		emit onlineNotifyFor(user);
	}
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
	while ( query.next() ) {
		Jid user = query.value(0).toString();
		if ( d->jidIcqTable.contains( user.bare() ) ) {
			ICQ::Connection *conn = d->jidIcqTable.value( user.bare() );
			QStringListIterator i( conn->contactList() );
			while ( i.hasNext() ) {
				emit contactOffline( user, i.next() );
			}

			d->jidIcqTable.remove( user.bare() );
			d->icqJidTable.remove(conn);

			conn->signOff();
			conn->deleteLater();
		}
		emit offlineNotifyFor(user);
	}
	d->online = false;
}

void GatewayTask::processIcqError(const QString& desc)
{
	ICQ::Connection *conn = qobject_cast<ICQ::Connection*>( sender() );
	Jid user = d->icqJidTable.value(conn);

	emit gatewayMessage(user, desc);
}

void GatewayTask::processIcqSignOn()
{
	ICQ::Connection *conn = qobject_cast<ICQ::Connection*>( sender() );
	Jid user = d->icqJidTable.value(conn);

	QStringList contacts = conn->contactList();
	QStringListIterator i(contacts);

	while ( i.hasNext() ) {
		emit contactOffline( user, i.next() );
	}
}

void GatewayTask::processIcqStatus(int status)
{
	ICQ::Connection *conn = qobject_cast<ICQ::Connection*>( sender() );
	qDebug() << "icq status for" << conn->userId() << "changed to" << status;
	Jid user = d->icqJidTable.value(conn);
	if ( status == ICQ::UserInfo::Offline ) {
		QStringList contacts = conn->contactList();
		QStringListIterator i(contacts);
		while ( i.hasNext() ) {
			emit contactOffline( user, i.next() );
		}
	}
}

void GatewayTask::processContactOnline(const QString& uin, quint16 status)
{
	ICQ::Connection *conn = qobject_cast<ICQ::Connection*>( sender() );
	Jid user = d->icqJidTable.value(conn);

	int showStatus;
	switch ( status ) {
		case ICQ::UserInfo::Online:
			showStatus = XMPP::Presence::None;
			break;
		case ICQ::UserInfo::Away:
			showStatus = XMPP::Presence::Away;
			break;
		case ICQ::UserInfo::NotAvailable:
			showStatus = XMPP::Presence::NotAvailable;
			break;
		case ICQ::UserInfo::DoNotDisturb:
			showStatus = XMPP::Presence::DoNotDisturb;
			break;
		case ICQ::UserInfo::FreeForChat:
			showStatus = XMPP::Presence::Chat;
			break;
		default:
			qDebug() << Q_FUNC_INFO << " - unknown contact status" << QString::number(status, 16) <<"for" << uin;
			showStatus = XMPP::Presence::None;
			break;
	}

	emit contactOnline(user, uin, showStatus);
}

void GatewayTask::processContactOffline(const QString& uin)
{
	ICQ::Connection *conn = qobject_cast<ICQ::Connection*>( sender() );
	Jid user = d->icqJidTable.value(conn);
	emit contactOffline(user, uin);
}

void GatewayTask::processIncomingMessage(const QString& senderUin, const QString& message)
{
	ICQ::Connection *conn = qobject_cast<ICQ::Connection*>( sender() );
	Jid user = d->icqJidTable.value(conn);
	emit incomingMessage(user, senderUin, message);
}

/**
 * This slot is triggered when user @a uin grants authorization to jabber user.
 */
void GatewayTask::processAuthGranted(const QString& uin)
{
	ICQ::Connection *conn = qobject_cast<ICQ::Connection*>( sender() );
	Jid user = d->icqJidTable.value(conn);

	qDebug() << "[GT]" << user << "granted auth to" << uin;
	emit subscriptionReceived(user, uin);
}

/**
 * This slot is triggered when user @a uin denies authorization to jabber user.
 */
void GatewayTask::processAuthDenied(const QString& uin)
{
	ICQ::Connection *conn = qobject_cast<ICQ::Connection*>( sender() );
	Jid user = d->icqJidTable.value(conn);

	qDebug() << "[GT]" << user << "denied auth to" << uin;
	emit subscriptionRemoved(user, uin);
}

/**
 * This slot is triggered when user @a uin sends an authorization request to jabber user.
 */
void GatewayTask::processAuthRequest(const QString& uin)
{
	ICQ::Connection *conn = qobject_cast<ICQ::Connection*>( sender() );
	Jid user = d->icqJidTable.value(conn);

	emit subscriptionRequest(user, uin);
}
