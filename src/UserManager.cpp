/*
 * UserManager.cpp - User Database Manager
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

#include "UserManager.h"

#include <QMutex>
#include <QSqlQuery>
#include <QString>
#include <QVariant>

UserManager::UserManager()
{
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

UserManager::~UserManager()
{
}

UserManager* UserManager::instance()
{
	static QMutex mutex;
	if ( !m_instance ) {
		mutex.lock();
		if ( !m_instance ) {
			m_instance = new UserManager;
		}
		mutex.unlock();
	}
	return m_instance;
}

void UserManager::add(const QString& user, const QString& uin, const QString& passwd)
{
	clearOptions(user);
	QSqlQuery query;
	/* prepare + bindvalue doesn't work... at least on sqlite */
	query.exec( QString("REPLACE INTO users VALUES('%1', '%2', '%3')").arg(user,uin,passwd) );
}

void UserManager::del(const QString& user)
{
	QSqlQuery query;

	query.exec( QString("SELECT * FROM users WHERE jid = '%1'").arg(user) );
	if ( !query.first() ) {
		return;
	}

	query.exec( QString("DELETE FROM users WHERE jid = '%1'").arg(user) );
	query.exec( QString("DELETE FROM options WHERE jid = '%1'").arg(user) );
}

bool UserManager::isRegistered(const QString& user) const
{
	QSqlQuery query;
	query.exec( QString("SELECT jid FROM users WHERE jid = '%1'").arg(user) );
	return query.first();
}

QVariant UserManager::getOption(const QString& user, const QString& option) const
{
	QSqlQuery query;
	query.exec( QString("SELECT value from options WHERE jid = '%1' AND option='%2'").arg(user,option) );

	if ( query.first() ) {
		QVariant value = query.value(0);
		return value;
	}
	return QVariant();
}

void UserManager::setOption(const QString& user, const QString& option, const QVariant& value)
{
	QSqlQuery query;
	query.exec( QString("REPLACE INTO options (jid,option,value) VALUES('%1', '%2', '%3')").arg(user,option, value.toString()) );
}

bool UserManager::hasOption(const QString& user, const QString& option) const
{
	QSqlQuery query;
	query.exec( QString("SELECT value from options WHERE jid = '%1' AND option='%2'").arg(user,option) );
	return query.first();
}

QHash<QString,QVariant> UserManager::options(const QString& user) const
{
	QSqlQuery query;
	query.exec( QString("SELECT option, value from options WHERE jid = '%1'").arg(user) );

	QHash<QString,QVariant> list;
	while ( query.next() ) {
		list.insert(query.value(0).toString(), query.value(1));
	}
	return list;
}

void UserManager::setOptions(const QString& user, const QHash<QString,QVariant>& list)
{
	QHashIterator<QString,QVariant> i(list);
	while ( i.hasNext() ) {
		i.next();
		setOption(user, i.key(), i.value());
	}
}

void UserManager::clearOptions(const QString& user)
{
	QSqlQuery query;
	query.exec( QString("DELETE FROM options WHERE jid='%1'").arg(user) );
}

UserManager* UserManager::m_instance = 0;
