/*
 * GatewayTask.h - Handles tasks from jabber clients (register/status/message/etc)
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

#ifndef GATEWAYTASK_H_
#define GATEWAYTASK_H_

#include <QObject>

namespace XMPP {
	class Jid;
}

class QSqlDatabase;

class GatewayTask : public QObject
{
	Q_OBJECT

	typedef XMPP::Jid Jid;

	public:
		GatewayTask(QObject *parent = 0);
		virtual ~GatewayTask();

		void setDatabaseLink(const QSqlDatabase& sql);
	public slots:
		void processRegister(const QString& user, const QString& uin, const QString& password);
		void processUnregister(const QString& user);
		void processLogin(const Jid& user);
		void processLogout(const Jid& user);
		void processContactAdd(const Jid& user, const QString& uin);
		void processContactDel(const Jid& user, const QString& uin);
		void processSendMessage();
	signals:
		void contactAdded(const Jid& user, const QString& uin);
		void contactDeleted(const Jid& user, const QString& uin);
	private:
		class Private;
		Private *d;
};

#endif /* GATEWAYTASK_H_ */
