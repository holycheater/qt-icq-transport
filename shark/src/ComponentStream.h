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

#ifndef XMPP_COMPONENTSTREAM_H_
#define XMPP_COMPONENTSTREAM_H_

#define NS_ETHERX "http://etherx.jabber.org/streams"
#define NS_COMPONENT "jabber:component:accept"
#define NS_STREAMS "urn:ietf:params:xml:ns:xmpp-streams"

#include <QObject>

#include "xmpp-core/Parser.h"
#include "xmpp-core/Connector.h"

class QDomDocument;

namespace XMPP {

class Connector;
class Jid;
class Stanza;
class IQ;
class Message;
class Presence;


class ComponentStream : public QObject
{
	Q_OBJECT

	enum ConnectionStatus { Disconnected, InitIncomingStream, RecvHandshakeReply, Connected };

	public:
		enum ErrorType { EStreamError, EHandshakeFailed, EConnectorError };

		ComponentStream(Connector *connector, QObject *parent = 0);
		~ComponentStream();

		class Error;

		QString lastErrorString() const;
		Error lastStreamError() const;

		QString baseNS() const;

		void connectToServer(const Jid& jid, const QString& secret);
		void close();

		void sendStanza(const Stanza& stanza);
	signals:
		void connected();
		void disconnected();
		void error(ComponentStream::ErrorType errType);
		void stanzaIQ(const IQ&);
		void stanzaMessage(const Message&);
		void stanzaPresence(const Presence&);
	private:
		void handleStreamError(const Parser::Event& event);
		void processEvent(const Parser::Event& event);
		void processStanza(const Parser::Event& event);
		void recv_stream_open(const Parser::Event& event);
		void recv_handshake_reply(const Parser::Event& event);
		void send_stream_open();
		void send_handshake();
		void write(const QByteArray& data);
	private slots:
		void bs_readyRead();
		void bs_closed();
		void cr_connected();
		void cr_error(Connector::ErrorType errcode);
		void send_keepalive();
	private:
		class Private;
		Private *d;
};

class ComponentStream::Error
{
	public:
		enum Condition {
			BadFormat, BadNamespacePrefix, Conflict, ConnectionTimeout, HostGone,
			HostUnknown, ImproperAddressing, InternalServerError, InvalidFrom,
			InvalidId, InvalidNamespace, InvalidXml, NotAuthorized, PolicyViolation,
			RemoteConnectionFailed, ResourceConstraint, RestrictedXml, SeeOtherHost,
			SystemShutdown, UndefinedCondition, UnsupportedEncoding, UnsupportedStanzaType,
			UnsupportedVersion, XmlNotWellFormed
		};

		Error();
		Error(const Error& other);
		Error(const QDomDocument& document);
		Error(const QDomElement& element);
		~Error();
		Error& operator=(const Error& other);

		QString appSpec() const;
		QString appSpecNS() const;
		Condition condition() const;
		QString conditionString() const;
		QString text() const;

		void setAppSpec(const QString& ns, const QString& spec);
		void setCondition(Condition type);
		void setText(const QString& text);
	private:
		class Private;
		QSharedDataPointer<Private> d;
};


} /* end of namespace XMPP */

#endif /* XMPP_COMPONENTSTREAM_H_ */
