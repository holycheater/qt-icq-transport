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

#ifndef COMPONENTSTREAM_H_
#define COMPONENTSTREAM_H_

#include "parser.h"
#include <QObject>

#ifndef NS_ETHERX
#define NS_ETHERX "http://etherx.jabber.org/streams"
#endif
#define NS_COMPONENT "jabber:component:accept"

namespace XMPP {

class AdvancedConnector;
class Jid;

class ComponentStream : public QObject
{
	Q_OBJECT

	enum ConnectionStatus { Disconnected, InitIncomingStream, RecvHandshakeReply, Connected };

	public:
		ComponentStream(AdvancedConnector *connector, QObject *parent = 0);
		~ComponentStream();

		void connectToServer(const Jid& jid, quint16 port, const QString& secret);
		void close();

		QString baseNS() const;
	signals:
		void connected();
		void disconnected();
	private:
		void processEvent(const Parser::Event& event);
		void processNext();
		void processStanza(const Parser::Event& event);
		void recv_stream_open();
		void recv_handshake_reply();
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


} /* end of namespace XMPP */

#endif /* COMPONENTSTREAM_H_ */
