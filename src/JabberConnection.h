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

#include "ComponentStream.h"

namespace XMPP {
	class Jid;
	class Registration;
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
	typedef XMPP::Jid Jid;

	public:
		JabberConnection(QObject *parent = 0);
		~JabberConnection();

		void login();

		void setUsername(const QString& username);
		void setServer(const QString& host, quint16 port);
		void setPassword(const QString& password);
	public slots:
		void sendSubscribe(const Jid& user, const QString& node);
		void sendSubscribed(const Jid& user, const QString& node);
		void sendUnsubscribe(const Jid& user, const QString& node);
		void sendUnsubscribed(const Jid& user, const QString& node);

		// void sendMessage(const Jid& from, const Jid& to, const QString& message);
	signals:
		void userUnregistered(const QString& jid);
		void userRegistered(const QString& jid, const QString& uin, const QString& password);
		void userOnline(const Jid& jid);
		void userOffline(const Jid& jid);
		void userAdd(const Jid& jid, const QString& uin);
		void userDel(const Jid& jid, const QString& uin);
	private:
		void process_discoinfo(const IQ& iq);
		void process_discoitems(const IQ& iq);
		void process_register_request(const IQ& iq);
		void process_register_form(const Registration& iq);
	private slots:
		void stream_error(const ComponentStream::Error&);
		void stream_connected();
		void stream_iq(const IQ&);
		void stream_message(const Message&);
		void stream_presence(const Presence&);
	private:
		class Private;
		Private *d;
};

#endif /* JABBERCONNECTION_H_ */
