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
#include <QList>

namespace XMPP {
    class Jid;
    class RosterXItem;
    class vCard;
}

class QDateTime;

class GatewayTask : public QObject
{
    Q_OBJECT

    public:
        GatewayTask(QObject *parent = 0);
        virtual ~GatewayTask();

        void setIcqServer(const QString& host, quint16 port);
    public slots:
        void processRegister(const XMPP::Jid& user, const QString& uin, const QString& password);
        void processUnregister(const XMPP::Jid& user);
        void processUserOnline(const XMPP::Jid& user, int showStatus);
        void processUserOffline(const XMPP::Jid& user);
        void processUserStatusRequest(const XMPP::Jid& user);
        void processSubscribeRequest(const XMPP::Jid& user, const QString& uin);
        void processUnsubscribeRequest(const XMPP::Jid& user, const QString& uin);
        void processAuthGrant(const XMPP::Jid& user, const QString& uin);
        void processAuthDeny(const XMPP::Jid& user, const QString& uin);
        void processSendMessage(const XMPP::Jid& user, const QString& uin, const QString& message);

        void processVCardRequest(const XMPP::Jid& user, const QString& uin, const QString& requestID);

        void processCmd_RosterRequest(const XMPP::Jid& user);

        void processGatewayOnline();
        void processShutdown();

    signals:
        void subscriptionReceived(const XMPP::Jid& user, const QString& uin, const QString& nick);
        void subscriptionRemoved(const XMPP::Jid& user, const QString& uin);
        void subscriptionRequest(const XMPP::Jid& user, const QString& uin);

        void contactOnline(const XMPP::Jid& user, const QString& uin, int status, const QString& nick);
        void contactOffline(const XMPP::Jid& user, const QString& uin);

        void onlineNotifyFor(const XMPP::Jid& user, int show);
        void offlineNotifyFor(const XMPP::Jid& user);
        void probeRequest(const XMPP::Jid& user);

        void incomingVCard(const XMPP::Jid& user, const QString& uin, const QString& requestID, const XMPP::vCard& vcard);

        void incomingMessage(const XMPP::Jid& user, const QString& uin, const QString& text, const QString& nick);
        void incomingMessage(const XMPP::Jid& user, const QString& uin, const QString& text, const QString& nick, const QDateTime& timestamp);
        void gatewayMessage(const XMPP::Jid& user, const QString& text);

        void rosterAdd(const XMPP::Jid& user, const QList<XMPP::RosterXItem>& items);
    private slots:
        void processIcqError(const QString& desc);
        void processIcqSignOn();
        void processIcqSignOff();
        void processIcqStatus(int status);
        void processIcqFirstLogin();

        void processContactOnline(const QString& uin, int status);
        void processContactOffline(const QString& uin);
        void processIncomingMessage(const QString& senderUin, const QString& message);
        void processIncomingMessage(const QString& senderUin, const QString& message, const QDateTime& timestamp);

        void processAuthGranted(const QString& uin);
        void processAuthDenied(const QString& uin);
        void processAuthRequest(const QString& uin);

        void processShortUserDetails(const QString& uin);
    private:
        class Private;
        Private *d;
};

// vim:et:ts=4:sw=4:nowrap
#endif /* GATEWAYTASK_H_ */
