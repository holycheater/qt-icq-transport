/*
 * gatewaytask.h - Gateway interaction task (XEP-0100)
 * Copyright (C) 2009  Alexander Saltykov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef XMPP_GATEWAY_TASK_H_
#define XMPP_GATEWAY_TASK_H_

#include <QObject>

namespace XMPP {

class ComponentStream;
class Jid;
class Message;
class IQ;
class Presence;
class Registration;


class GatewayTask : public QObject
{
    Q_OBJECT

    public:
        GatewayTask(ComponentStream *stream);
        virtual ~GatewayTask();

        void setRegistrationForm(const Registration& reg);
    public slots:
        void notifyOnline(const XMPP::Jid& user, const QString& legacyName, int presence_show);
        void notifyOffline(const XMPP::Jid& user, const QString& legacyName);

        void notifySubscribe(const XMPP::Jid& user, const QString& legacyName);
        void notifyUnsubscribe(const XMPP::Jid& user, const QString& legacyName);

        void notifySubscribed(const XMPP::Jid& user, const QString& legacyName);
        void notifyUnsubscribed(const XMPP::Jid& user, const QString& legacyName);
    signals:
        void userRegister(const XMPP::Jid& user, const QString& legacyName, const QString& legacyPassword);
        void userUnregister(const XMPP::Jid& user);

        void userLogIn(const XMPP::Jid& user, int presence_show);
        void userLogOut(const XMPP::Jid& user);

        void addContact(const XMPP::Jid& user, const QString& legacyNode);
        void deleteContact(const XMPP::Jid& user, const QString& legacyNode);

        void grantAuth(const XMPP::Jid& user, const QString& legacyNode);
        void denyAuth(const XMPP::Jid& user, const QString& legacyNode);

        void messageToLegacyNode(const XMPP::Jid& user, const QString& legacyNode, const QString& text);
    private slots:
        void slotMessage(const XMPP::Message& msg);
        void slotRegister(const XMPP::IQ& iq);

        void slotGatewaySubscribe(const XMPP::Presence& p);
        void slotSubscription(const XMPP::Presence& p);
        void slotPresence(const XMPP::Presence& p);
    private:
        Q_DISABLE_COPY(GatewayTask);
        class Private;
        Private *d;
};


} /* end of namespace XMPP */

// vim:ts=4:sw=4:nowrap:et
#endif /* XMPP_GATEWAY_TASK_H_ */
