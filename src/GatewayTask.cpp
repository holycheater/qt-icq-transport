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

void GatewayTask::processLogin(const Jid& user)
{
	if ( d->jidIcqTable.contains( user.bare() ) ) {
		qDebug() << "[GT] processLogin: user is already logged on, aborting login step";
		return;
	}
	if ( d->icqHost.isEmpty() || !d->icqPort ) {
		qDebug() << "[GT] processLogin: icq host and/or port values are not set. Aborting...";
		return;
	}

	QSqlQuery query;
	/* small hack with replace, because QSqlQuery somehow doesn't understand bindvalues there */
	query.exec( QString("SELECT uin, password FROM users WHERE jid = '?' ").replace( "?", user.bare() ) );

	if ( query.first() ) {
		QString uin = query.value(0).toString();
		QString password = query.value(1).toString();
		qDebug() << "[GT]" << "credentails for" << user.bare() << "are:" << uin << password;

		ICQ::Connection *conn = new ICQ::Connection(uin, password, d->icqHost, d->icqPort);

		QObject::connect( conn, SIGNAL( statusChanged(int) ), SLOT( processIcqStatus(int) ) );
		QObject::connect( conn, SIGNAL( userOnline(QString) ), SLOT( processContactOnline(QString) ) );
		QObject::connect( conn, SIGNAL( userOffline(QString) ), SLOT( processContactOffline(QString) ) );
		QObject::connect( conn, SIGNAL( authGranted(QString) ), SLOT( processAuthGranted(QString) ) );
		QObject::connect( conn, SIGNAL( authDenied(QString) ), SLOT( processAuthDenied(QString) ) );
		QObject::connect( conn, SIGNAL( authRequest(QString) ), SLOT( processAuthRequest(QString) ) );
		QObject::connect( conn, SIGNAL( incomingMessage(QString,QString) ), SLOT( processIncomingMessage(QString,QString) ) );

		d->jidIcqTable.insert( user.bare(), conn );
		d->icqJidTable.insert( conn, user.bare() );
		conn->setOnlineStatus(ICQ::UserInfo::Online);
	}
}

void GatewayTask::processLogout(const Jid& user)
{
	ICQ::Connection *conn = d->jidIcqTable.value( user.bare() );
	conn->signOff();
	d->jidIcqTable.remove( user.bare() );
	d->icqJidTable.remove(conn);

	// conn->deleteLater();
}

void GatewayTask::processContactAdd(const Jid& user, const QString& uin)
{
	qDebug() << "[GT]" << user << "request for adding contact:" << uin;
	ICQ::Connection *conn = d->jidIcqTable.value( user.bare() );
	conn->contactAdd(uin);
}

void GatewayTask::processContactDel(const Jid& user, const QString& uin)
{
	qDebug() << "[GT]" << user << "request for deleting contact:" << uin;
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
		ICQ::Connection *conn = d->jidIcqTable.value( user.bare() );
		QStringListIterator i( conn->contactList() );
		while ( i.hasNext() ) {
			emit contactOffline( user, i.next() );
		}
		emit offlineNotifyFor(user);
	}
	d->online = false;
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

void GatewayTask::processContactOnline(const QString& uin)
{
	ICQ::Connection *conn = qobject_cast<ICQ::Connection*>( sender() );
	Jid user = d->icqJidTable.value(conn);
	emit contactOnline(user, uin);
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
