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

#include <QDomDocument>
#include <QCryptographicHash>
#include <QXmlAttributes>

#include "ComponentStream.h"

#include "xmpp-core/IQ.h"
#include "xmpp-core/Jid.h"
#include "xmpp-core/Message.h"
#include "xmpp-core/Presence.h"

#include "xmpp-core/Connector.h"
#include "cutestuff/bytestream.h"

#include <QtDebug>

namespace XMPP {


class ComponentStream::Private
{
	public:
		/* server connector (dns-lookups, connect) */
		Connector *connector;

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
 * connection to the server. @a parent will be passed to the QObject constructor.
 */
ComponentStream::ComponentStream(Connector *connector, QObject *parent)
	: QObject(parent)
{
	d = new Private;
	d->connectionStatus = Disconnected;

	d->connector = connector;
	QObject::connect( d->connector, SIGNAL( connected() ), SLOT( cr_connected() ) );
	QObject::connect( d->connector, SIGNAL( error() ), SLOT( cr_error() ) );
}

/**
 * Destroys the stream.
 */
ComponentStream::~ComponentStream()
{
	delete d;
}

/**
 *
 * @return base stream namespace.
 */
QString ComponentStream::baseNS() const
{
	return NS_COMPONENT;
}

/**
 * Connects to the JID domain host to the specified port with secret.
 *
 * @param jid		jabber-id
 * @param secret	server secret for component
 */
void ComponentStream::connectToServer(const Jid& jid, const QString& secret)
{
	d->jid = jid;
	d->secret = secret.toUtf8();

	d->connector->connectToServer( jid.full() );
}

/**
 * Close the stream and connection.
 */
void ComponentStream::close()
{
	write("</stream:stream>");
}

/**
 * Sends @a stanza to the outgoing stream.
 */
void ComponentStream::sendStanza(const Stanza& stanza)
{
	write ( stanza.toString().toUtf8() );
}

/**
 * Handles stream erorrs (<stream:error/> element).
 *
 * @param event		incoming stream event containing an error
 */
void ComponentStream::handleStreamError(const Parser::Event& event)
{
	emit error( Error( event.element() ) );
}

/**
 * Process events from incoming stream.
 *
 * @param event		incoming event
 */
void ComponentStream::processEvent(const Parser::Event& event)
{
	qDebug() << "[CS]" << "processEvent()" << "type" << event.typeString();
	switch ( event.type() ) {
		case Parser::Event::DocumentOpen:
			if (d->connectionStatus == InitIncomingStream) {
				recv_stream_open(event);
				return;
			}
			break;

		case Parser::Event::DocumentClose:
			qDebug() << "[CS] we are kicked off";
			close();
			break;

		case Parser::Event::Element:
			qDebug() << "[CS] incoming element" << event.qualifiedName();
			if ( event.qualifiedName() == "stream:error" ) {
				handleStreamError(event);
			}
			if (d->connectionStatus == RecvHandshakeReply) {
				recv_handshake_reply(event);
				return;
			}
			processStanza(event);
			break;

		case Parser::Event::Error:
			qDebug() << "[CS] whoops.. error occured";
			close();
			break;
	}
}

/**
 * Process incoming stanza (first-level element).
 *
 * @param event		parser event
 */
void ComponentStream::processStanza(const Parser::Event& event)
{
	if ( event.qualifiedName() == "message" ) {
		qDebug() << "[CS]" << "-message-";
		emit stanzaMessage( event.element() );
	} else if ( event.qualifiedName() == "iq" ) {
		qDebug() << "[CS]" << "-iq-";
		emit stanzaIQ( event.element() );
	} else if ( event.qualifiedName() == "presence" ) {
		qDebug() << "[CS]" << "-presence-";
		emit stanzaPresence( event.element() );
	}
}

/**
 * Handle incoming stream initiation event @a event.
 */
void ComponentStream::recv_stream_open(const Parser::Event& event)
{
	d->sessionId = event.attributes().value("id").toUtf8();

	/* stream accepted, next step - send the handshake */
	send_handshake();
}

/**
 * Handle handshake reply event @a event.
 */
void ComponentStream::recv_handshake_reply(const Parser::Event& event)
{
	if ( event.qualifiedName()=="handshake" && !event.attributes().count() ) {
		qDebug() << "[CS]" << "Handshaking success";
	} else {
		qDebug() << "[CS]" << "Error!" << "Handshake failed";
		close();
	}

	/* We have no errors */
	d->connectionStatus = Connected;
	emit connected();
}

/**
 * Initiate outgoing stream.
 */
void ComponentStream::send_stream_open()
{
	write("<?xml version='1.0'?>");
	write("<stream:stream xmlns:stream='" + QByteArray(NS_ETHERX) + "' xmlns='"+QByteArray(NS_COMPONENT)+"' to='" + d->jid.domain().toUtf8() + "'>");

	/* we sent stream open instruction, so server need to open the incoming stream */
	d->connectionStatus = InitIncomingStream;
}

/**
 * Send a handshake to the server: sha1(sessionID + secret).
 */
void ComponentStream::send_handshake()
{
	QByteArray hash = QCryptographicHash::hash(d->sessionId + d->secret, QCryptographicHash::Sha1).toHex();

	write("<handshake>" + hash + "</handshake>");

	/* we've sent handshake element, we need to get ack/error reply from the server */
	d->connectionStatus = RecvHandshakeReply;
}

/**
 * Write data to the outgoing stream.
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

	Parser::Event event = d->parser.readNext();
	while ( !event.isNull() ) {
		processEvent(event);
		event = d->parser.readNext();
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
 * This signal is emitted when the stream is closed
 *
 * @sa connected()
 */

/**
 * @fn void ComponentStream::error(const Error& streamError)
 *
 * This signal is emitted when there is an error on the stream(<stream:error/> element).
 *
 * @param streamError	Stream error object, containing error condition and (optionally) description.
 */

/**
 * @fn void ComponentStream::stanzaIQ(const IQ& iq)
 *
 * This signal is emitted when there is an incoming info/query stanza on the stream.
 *
 * @param iq			Info/query stanza.
 */

/**
 * @fn void ComponentStream::stanzaMessage(const Message& message);
 *
 * This signal is emitted when there is an incoming message stanza on the stream.
 *
 * @param message		Message stanza.
 */

/**
 * @fn void ComponentStream::stanzaPresence(const Presence& presence)
 *
 * This signal is emitted when there is an incoming presence stanza on the stream.
 *
 * @param presence		Presence stanza.
 */

/**
 * @enum ComponentStream::ConnectionStatus
 *
 * This enum describes the next incoming step we are waiting for.
 */

} /* end of namespace XMPP */
