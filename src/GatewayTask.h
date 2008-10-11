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
		void setIcqServer(const QString& host, quint16 port);
	public slots:
		void processRegister(const QString& user, const QString& uin, const QString& password);
		void processUnregister(const QString& user);
		void processUserOnline(const Jid& user, int showStatus);
		void processUserOffline(const Jid& user);
		void processSubscribeRequest(const Jid& user, const QString& uin);
		void processUnsubscribeRequest(const Jid& user, const QString& uin);
		void processAuthGrant(const Jid& user, const QString& uin);
		void processAuthDeny(const Jid& user, const QString& uin);
		void processSendMessage(const Jid& user, const QString& uin, const QString& message);

		void processCmd_RosterRequest(const Jid& user);

		void processGatewayOnline();
		void processShutdown();
	signals:
		void subscriptionReceived(const Jid& user, const QString& uin);
		void subscriptionRemoved(const Jid& user, const QString& uin);
		void subscriptionRequest(const Jid& user, const QString& uin);

		void contactOnline(const Jid& user, const QString& uin, int status);
		void contactOffline(const Jid& user, const QString& uin);

		void onlineNotifyFor(const Jid& user);
		void offlineNotifyFor(const Jid& user);

		void incomingMessage(const Jid& user, const QString& uin, const QString& text);
		void gatewayMessage(const Jid& user, const QString& text);
	private slots:
		void processIcqError(const QString& desc);
		void processIcqSignOn();
		void processIcqStatus(int status);

		void processContactOnline(const QString& uin, quint16 status);
		void processContactOffline(const QString& uin);
		void processIncomingMessage(const QString& senderUin, const QString& message);

		void processAuthGranted(const QString& uin);
		void processAuthDenied(const QString& uin);
		void processAuthRequest(const QString& uin);
	private:
		class Private;
		Private *d;
};

#endif /* GATEWAYTASK_H_ */
