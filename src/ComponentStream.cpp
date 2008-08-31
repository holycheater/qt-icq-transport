/*
 * ComponentStream.cpp - Jabber component stream (jabber:component:accept)
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

#include "ComponentStream.h"

#include "xmpp.h"
#include "bytestream.h"
#include "parser.h"

#include <QCryptographicHash>
#include <QXmlStreamWriter>

namespace XMPP {

class ComponentStream::Private
{
	public:
		/* server connector (dns-lookups, connect) */
		AdvancedConnector *connector;

		/* Connection socket stream */
		ByteStream *bs;

		/* component Jabber-ID */
		Jid jid;

		/* incoming data parser object */
		Parser parser;

		/* server component secret (password) */
		QByteArray secret;

		/* session Id, this will be told by server in the opening stream */
		QByteArray sessionId;

		/* connection status from ComponentStream::ConnectionStatus */
		int connectionStatus;
};

/**
 * Constructs stream object with @a connector to be used for initating
 * connection to the server. @a parent will be passed to the QObject constructor
 */
ComponentStream::ComponentStream(AdvancedConnector *connector, QObject *parent)
	: QObject(parent)
{
	d = new Private;
	d->connectionStatus = Disconnected;

	d->connector = connector;
	QObject::connect( d->connector, SIGNAL( connected() ), SLOT( cr_connected() ) );
	QObject::connect( d->connector, SIGNAL( error() ), SLOT( cr_error() ) );
}

/**
 * Destroys the stream
 */
ComponentStream::~ComponentStream()
{
	delete d;
}

/**
 * Connects to the JID domain host to the specified port with secret
 *
 * @param jid		jabber-id
 * @param port		server port
 * @param secret	server secret for component
 */
void ComponentStream::connectToServer(const Jid& jid, quint16 port, const QString& secret)
{
	d->jid = jid;
	d->secret = secret.toUtf8();

	d->connector->setOptHostPort(jid.domain(), port);
	d->connector->connectToServer( jid.domain() );
}

/**
 *
 * @return base stream namespace
 */
QString ComponentStream::baseNS() const
{
	return NS_COMPONENT;
}

/**
 * Close the stream and connection
 */
void ComponentStream::close()
{
	write("</stream:stream>");
}

/**
 * Process events from incoming stream
 *
 * @param event		incoming event
 */
void ComponentStream::processEvent(const Parser::Event& event)
{
	qDebug() << "[CS]" << "processEvent()" << "type" << event.type();
	switch ( event.type() ) {
		case Parser::Event::DocumentClose:
			qDebug() << "[CS] we are kicked off";
			close();
			break;
		case Parser::Event::Element:
			qDebug() << "[CS] we've got an element (stanza) here";
			/* TODO: process stream:error */
			processStanza(event);
			break;
		case Parser::Event::Error:
			qDebug() << "[CS] whoops.. error occured";
			close();
			break;
	}
}

/**
 * Process next step for the server authentication process
 */
void ComponentStream::processNext()
{
	switch ( d->connectionStatus ) {
		case Disconnected:
			qDebug() << "[CS]" << "processNext() in 'Disconnected' state. This should not happen";
			break;
		case InitIncomingStream: /* we need to process incoming stream initiation */
			recv_stream_open();
			break;
		case RecvHandshakeReply: /* we need to receive handshake reply */
			recv_handshake_reply();
		case Connected: /* we need to process various events while we're connected. */
		{
			Parser::Event event = d->parser.readNext();
			while ( !event.isNull() ) {
				processEvent(event);
				event = d->parser.readNext();
			}
			break;
		}
		default:
			break;
	}
}

/**
 * Process incoming stanza (first-level element)
 * @param event		parser event
 */
void ComponentStream::processStanza(const Parser::Event& event)
{
	if ( event.qualifiedName() == "message" ) {
		qDebug() << "[CS]" << "incoming message";
	} else if ( event.qualifiedName() == "iq" ) {
		qDebug() << "[CS]" << "incoming info/query";
	} else if ( event.qualifiedName() == "presence" ) {
		qDebug() << "[CS]" << "presence info";
	}
}

/**
 * Handle incoming stream initiation
 */
void ComponentStream::recv_stream_open()
{
	Parser::Event event = d->parser.readNext();
	d->sessionId = event.attributes().value("id").toUtf8();

	/* stream accepted, next step - send the handshake */
	send_handshake();
}

/**
 * Handle handshake reply
 */
void ComponentStream::recv_handshake_reply()
{
	Parser::Event event = d->parser.readNext();
	if ( event.qualifiedName()=="handshake" && !event.attributes().count() ) {
		qDebug() << "[CS]" << "Handshaking success";
	} else {
		qDebug() << "[CS]" << "Error!" << "Handshake failed";
		close();
	}

	/* We have no errors */
	d->connectionStatus = Connected;
	emit connected();
	processNext();
}

/**
 * Initiate outgoing stream
 */
void ComponentStream::send_stream_open()
{
	QByteArray data;
	QXmlStreamWriter xmlStream(&data);
	xmlStream.writeStartDocument();
	xmlStream.writeStartElement("stream:stream");
	xmlStream.writeAttribute("xmlns:stream", NS_ETHERX);
	xmlStream.writeDefaultNamespace(NS_COMPONENT);
	xmlStream.writeAttribute( "to", d->jid.domain() );

	/* dirty hack, cause QXmlStreamWriter can't close an opening element without appending closing tag */
	data += ">";

	write(data);
	/* we sent stream open instruction, so server need to open the incoming stream */
	d->connectionStatus = InitIncomingStream;
}

/**
 * Send a handshake to the server: sha1(sessionID + secret)
 */
void ComponentStream::send_handshake()
{
	QByteArray hash = QCryptographicHash::hash(d->sessionId + d->secret, QCryptographicHash::Sha1).toHex();

	QByteArray data;
	QXmlStreamWriter writer(&data);
	writer.writeStartElement("handshake");
	writer.writeCharacters(hash);
	writer.writeEndElement();
	write(data);

	/* we've sent handshake element, we need to get ack/error reply from the server */
	d->connectionStatus = RecvHandshakeReply;
}

/**
 * Write data to the outgoing stream
 *
 * @param data		XML data
 */
void ComponentStream::write(const QByteArray& data)
{
	qDebug() << "[CS]" << "-send-" << data;
	d->bs->write(data);
}

void ComponentStream::bs_closed()
{
	qDebug() << "[CS]" << "Bytestream closed";
}

void ComponentStream::bs_error(int errno)
{
	qDebug() << "[CS]" << "Bytestream error. Errno:" << errno;
}

void ComponentStream::bs_readyRead()
{
	QByteArray data = d->bs->read();
	qDebug() << "[CS]" << "-recv-" << data;

	d->parser.appendData(data);
	while ( d->parser.unprocessed().size() > 0 ) {
		processNext();
	}
}

void ComponentStream::cr_connected()
{
	d->bs = d->connector->stream();

	QObject::connect( d->bs, SIGNAL( connectionClosed() ), SIGNAL( disconnected() ) );
	QObject::connect( d->bs, SIGNAL( connectionClosed() ), SLOT( bs_closed() ) );
	QObject::connect( d->bs, SIGNAL( error(int) ), SLOT( bs_error(int) ) );
	QObject::connect( d->bs, SIGNAL( readyRead() ), SLOT( bs_readyRead() ) );

	/* we've connected to the server, so we need to initiate communications */
	send_stream_open();
}

void ComponentStream::cr_error()
{
	qDebug() << "[CS]" << "Connector error";
}

/**
 * Send keep-alive data (whitespace)
 */
void ComponentStream::send_keepalive()
{
	write(" ");
}

/**
 * @fn void ComponentStream::connected()
 *
 * This signal is emitted when the authentication process is finished
 *
 * @sa disconnected()
 */

/**
 * @fn void ComponentStream::disconnected()
 *
 * This signal is emmited when the stream is closed
 *
 * @sa connected()
 */


/**
 * @enum ComponentStream::ConnectionStatus
 *
 * This enum describes the next incoming step we are waiting for.
 */

} /* end of namespace XMPP */
