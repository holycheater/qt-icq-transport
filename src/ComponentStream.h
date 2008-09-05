/*
 * ComponentStream.h - Jabber component stream (jabber:component:accept)
 * Copyright (C) 2008  Alexander Saltykov
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

#ifndef X_COMPONENTSTREAM_H_
#define X_COMPONENTSTREAM_H_

#include "parser.h"
#include <QObject>
#include <QDomDocument>

#ifndef NS_ETHERX
#define NS_ETHERX "http://etherx.jabber.org/streams"
#endif
#define NS_COMPONENT "jabber:component:accept"

namespace XMPP {
	class AdvancedConnector;
	class Jid;
}

namespace X {


class Stanza;

class ComponentStream : public QObject
{
	Q_OBJECT

	enum ConnectionStatus { Disconnected, InitIncomingStream, RecvHandshakeReply, Connected };

	public:
		ComponentStream(XMPP::AdvancedConnector *connector, QObject *parent = 0);
		~ComponentStream();

		class Error;

		QString baseNS() const;

		void connectToServer(const XMPP::Jid& jid, quint16 port, const QString& secret);
		void close();

		void sendStanza(const Stanza& stanza);
	signals:
		void connected();
		void disconnected();
		void error(const Error& streamError);
	private:
		void handleStreamError(const XMPP::Parser::Event& event);
		void processEvent(const XMPP::Parser::Event& event);
		void processStanza(const XMPP::Parser::Event& event);
		void recv_stream_open(const XMPP::Parser::Event& event);
		void recv_handshake_reply(const XMPP::Parser::Event& event);
		void send_stream_open();
		void send_handshake();
		void write(const QByteArray& data);
	private slots:
		void bs_error(int errno);
		void bs_readyRead();
		void bs_closed();
		void cr_connected();
		void cr_error();
		void send_keepalive();
	private:
		class Private;
		Private *d;
};

class ComponentStream::Error
{
	public:
		enum Type { BadFormat, BadNamespacePrefix, Conflict, ConnectionTimeout, HostGone,
			HostUnknown, ImproperAddressing, InternalServerError, InvalidFrom, InvalidId,
			InvalidNamespace, InvalidXml, NotAuthorized, PolicyViolation, RemoteConnectionFailed,
			ResourceConstraint, RestrictedXml, SeeOtherHost, SystemShutdown, UndefinedCondition,
			UnsupportedEncoding, UnsupportedStanzaType, UnsupportedVersion, XmlNotWellFormed };

		Error();
		Error(const QDomDocument& document);
		Error(const QDomElement& element);
		~Error();

		QString appSpec() const;
		QString text() const;
		QString type() const;

		void setAppSpec(const QString& ns, const QString& spec);
		void setText(const QString& text);
		void setType(Type type);
	private:
		static QString typeToString(Type type);

		QDomDocument m_doc;
		QDomElement m_errorCondition;
		QDomElement m_appSpec;
};


} /* end of namespace X */

#endif /* X_COMPONENTSTREAM_H_ */
