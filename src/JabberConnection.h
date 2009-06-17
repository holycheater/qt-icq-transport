/*
 * JabberConnection.h - Jabber connection handler class
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

#ifndef JABBERCONNECTION_H_
#define JABBERCONNECTION_H_

#include <QObject>
#include <QList>

#include "ComponentStream.h"

class QDateTime;

namespace XMPP {
    class Jid;
    class Registration;
    class RosterXItem;
    class vCard;
}

class JabberConnection : public QObject
{
    Q_OBJECT

    /* typedefs for slots */
    typedef XMPP::IQ IQ;
    typedef XMPP::Message Message;
    typedef XMPP::Presence Presence;
    typedef XMPP::ComponentStream ComponentStream;
    typedef XMPP::Registration Registration;
    typedef XMPP::RosterXItem RosterXItem;
    typedef XMPP::Jid Jid;
    typedef XMPP::vCard vCard;

    public:
        JabberConnection(QObject *parent = 0);
        ~JabberConnection();

        void login();

        void setUsername(const QString& username);
        void setServer(const QString& host, quint16 port);
        void setPassword(const QString& password);
    public slots:
        void sendSubscribe(const Jid& toUser, const QString& fromUin);
        void sendSubscribed(const Jid& toUser, const QString& fromUin, const QString& nick);
        void sendUnsubscribe(const Jid& toUser, const QString& fromUin);
        void sendUnsubscribed(const Jid& toUser, const QString& fromUin);

        void sendOnlinePresence(const Jid& toUser, const QString& fromUin, int showStatus, const QString& nick);
        void sendOfflinePresence(const Jid& toUser, const QString& fromUin);

        void sendOnlinePresence(const Jid& recipient, int showStatus);
        void sendOfflinePresence(const Jid& recipient);
        void sendPresenceProbe(const Jid& user);

        void sendMessage(const Jid& recipient, const QString& uin, const QString& message, const QString& nick, const QDateTime& timestamp);
        void sendMessage(const Jid& recipient, const QString& uin, const QString& message, const QString& nick);
        void sendMessage(const Jid& recipient, const QString& message);

        void sendVCard(const Jid& recipient, const QString& uin, const QString& requestID, const vCard& vcard);

        void slotRosterAdd(const Jid& user, const QList<RosterXItem>& items);
    signals:
        void userUnregistered(const QString& jid);
        void userRegistered(const QString& jid, const QString& uin, const QString& password);
        void userOnline(const Jid& jid, int showStatus, bool first_login);
        void userOffline(const Jid& jid);
        void userOnlineStatusRequest(const Jid& jid);
        void userAdd(const Jid& jid, const QString& uin);
        void userDel(const Jid& jid, const QString& uin);
        void userAuthGrant(const Jid& jid, const QString& uin);
        void userAuthDeny(const Jid& jid, const QString& uin);

        void vCardRequest(const Jid& jid, const QString& uin, const QString& requestID);

        void outgoingMessage(const Jid& fromUser, const QString& toUin, const QString& message);

        void connected();

        void cmd_RosterRequest(const Jid& user);
    private slots:
        void stream_error(ComponentStream::ErrorType);
        void stream_connected();
        void stream_iq(const IQ&);
        void stream_message(const Message&);
        void stream_presence(const Presence&);
    private:
        class Private;
        Private *d;
};

// vim:et:ts=4:sw=4:nowrap
#endif /* JABBERCONNECTION_H_ */
