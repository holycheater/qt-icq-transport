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

#include <QList>
#include <QHash>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include <QtDebug>

class GatewayTask::Private
{
	public:
		QList<ICQ::Connection> icqLinks;

		/* Jabber-ID-to-ICQ-Connection hash-table. */
		QHash<QString, ICQ::Connection*> onlineUsers;

		QSqlDatabase db;
};

GatewayTask::GatewayTask(QObject *parent)
	: QObject(parent), d(new Private)
{
}

GatewayTask::~GatewayTask()
{
	d->db.close();
	delete d;
}

void GatewayTask::setDatabaseLink(const QSqlDatabase& sql)
{
	d->db = sql;
	if ( !d->db.open() ) {
		qDebug() << "[GT]" << "Database open failed" << d->db.lastError();
		return;
	}
	QSqlQuery query;
	query.exec("CREATE TABLE IF NOT EXISTS users ("
				"jid TEXT"
				"uin TEXT"
				"password TEXT"
				"PRIMARY KEY(jid)"
				")");
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
	if ( d->onlineUsers.contains( user.bare() ) ) {
		/* user is already signed on */
		return;
	}
	QSqlQuery query;
	query.prepare("SELECT uin, password FROM users"
				  "WHERE jid = '?'");
	query.addBindValue( user.bare() );
}

void GatewayTask::processLogout(const Jid& user)
{
	d->onlineUsers.value( user.bare() )->signOff();
	/* TODO: delete connection object */
}

void GatewayTask::processContactAdd(const Jid& user, const QString& uin)
{
	ICQ::Connection *conn = d->onlineUsers.value( user.bare() );
	conn->contactAdd(uin);

	/* TODO: do this on auth accept */
	emit contactAdded(user, uin);
}

void GatewayTask::processContactDel(const Jid& user, const QString& uin)
{
	ICQ::Connection *conn = d->onlineUsers.value( user.bare() );
	conn->contactDel(uin);

	emit contactDeleted(user, uin);
}

void GatewayTask::processSendMessage()
{
}
