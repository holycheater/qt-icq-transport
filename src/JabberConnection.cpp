/*
 * JabberConnection.cpp - Jabber connection handler class
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

#include "JabberConnection.h"

#include "xmpp-core/Message.h"
#include "xmpp-core/IQ.h"
#include "xmpp-core/Connector.h"
#include "xmpp-core/Jid.h"

#include "xmpp-ext/ServiceDiscovery.h"
#include "xmpp-ext/Registration.h"
#include "xmpp-ext/vCard.h"

#include <QCoreApplication>
#include <QStringList>
#include <QUrl>

#include <QtDebug>

#define NS_QUERY_ADHOC "http://jabber.org/protocol/commands"

using namespace XMPP;

class JabberConnection::Private {

	public:
		Connector* connector;
		ComponentStream* stream;
		Jid jid;
		vCard vcard;
		DiscoInfo disco;
		QString secret;
};

JabberConnection::JabberConnection(QObject *parent)
	: QObject(parent)
{
	d = new Private;

	d->connector = new Connector;
	d->stream = new ComponentStream(d->connector);

	d->disco << DiscoInfo::Identity("gateway", "icq", "ICQ Transport");
	d->disco << NS_IQ_REGISTER << NS_QUERY_ADHOC << NS_VCARD_TEMP;

	d->vcard.setFullName("ICQ Transport");
	d->vcard.setDescription("Qt ICQ Transport");
	d->vcard.setUrl( QUrl("http://github.com/holycheater") );

	QObject::connect(d->stream, SIGNAL( stanzaIQ(IQ) ), SLOT( stream_iq(IQ) ) );
	QObject::connect(d->stream, SIGNAL( stanzaMessage(Message) ), SLOT( stream_message(Message) ) );
	QObject::connect(d->stream, SIGNAL( stanzaPresence(Presence) ), SLOT( stream_presence(Presence) ) );
	QObject::connect(d->stream, SIGNAL( error(ComponentStream::Error) ), SLOT( stream_error(ComponentStream::Error) ) );
	QObject::connect(d->stream, SIGNAL( connected() ), SLOT( stream_connected() ) );
}

JabberConnection::~JabberConnection()
{
	delete d->stream;
	delete d->connector;
}

void JabberConnection::login()
{
	d->stream->connectToServer(d->jid, d->secret);
}

void JabberConnection::setUsername(const QString& username)
{
	d->jid = username;
}

void JabberConnection::setServer(const QString& host, quint16 port)
{
	d->connector->setOptHostPort(host, port);
}

void JabberConnection::setPassword(const QString& password)
{
	d->secret = password;
}

void JabberConnection::process_discoinfo(const IQ& iq)
{
	qDebug() << "disco-info query from" << iq.from().full() << "to" << iq.to().full();

	IQ reply(iq);
	reply.swapFromTo();
	reply.setType(IQ::Result);

	d->disco.pushToDomElement( reply.childElement() );
	d->stream->sendStanza(reply);
}

void JabberConnection::process_discoitems(const IQ& iq)
{
	IQ reply(iq);
	reply.swapFromTo();
	reply.setType(IQ::Result);

	d->stream->sendStanza(reply);
}

void JabberConnection::process_register_request(const IQ& iq)
{
	Registration regForm(iq);

	regForm.swapFromTo();
	regForm.setType(IQ::Result);

	regForm.setField(Registration::Instructions, QString("Enter UIN and password"));
	regForm.setField(Registration::Username);
	regForm.setField(Registration::Password);

	d->stream->sendStanza(regForm);
}

void JabberConnection::process_register_form(const Registration& iq)
{
	if ( iq.to() != d->jid ) {
		Registration err(iq);
		err.swapFromTo();
		err.setError( Stanza::Error(Stanza::Error::FeatureNotImplemented) );
		d->stream->sendStanza(err);
		return;
	}
	if ( iq.hasField(Registration::Remove) ) {
		if ( iq.fields().size() > 1 ) {
			/* error, <remove/> is not the only child element */
			Registration err(iq);
			err.swapFromTo();
			err.setError( Stanza::Error(Stanza::Error::BadRequest) );
			d->stream->sendStanza(err);
			return;
		}
		if ( iq.from().isEmpty() ) {
			Registration err(iq);
			err.swapFromTo();
			err.setError( Stanza::Error(Stanza::Error::UnexpectedRequest) );
			d->stream->sendStanza(err);
			return;
		}
		Registration reply(iq);
		reply.swapFromTo();
		reply.clearChild();
		reply.setType(IQ::Result);
		d->stream->sendStanza(reply);
		/* send unregister signal, slot should remove the user from the database */
		emit userUnregistered( iq.from().bare() );
		return;
	}
	if ( iq.getField(Registration::Username).isEmpty() || iq.getField(Registration::Password).isEmpty() ) {
		Registration err(iq);
		err.swapFromTo();
		err.setError( Stanza::Error(Stanza::Error::NotAcceptable) );
		d->stream->sendStanza(err);
		return;
	}

	/* registration success */
	IQ reply(iq);
	reply.swapFromTo();
	reply.clearChild();
	reply.setType(IQ::Result);
	d->stream->sendStanza(reply);
	emit userRegistered( iq.from().bare(), iq.getField(Registration::Username), iq.getField(Registration::Password) );
}

void JabberConnection::stream_iq(const IQ& iq)
{
	if ( iq.childElement().tagName() == "query" && iq.type() == "get" ) {
		if ( iq.childElement().namespaceURI() == NS_QUERY_DISCO_INFO ) {
			process_discoinfo(iq);
			return;
		}
		if ( iq.childElement().namespaceURI() == NS_QUERY_DISCO_ITEMS ) {
			process_discoitems(iq);
			return;
		}
		if ( iq.childElement().namespaceURI() == NS_IQ_REGISTER ) {
			process_register_request(iq);
			return;
		}
	}
	if ( iq.childElement().tagName() == "vCard" && iq.type() == "get" ) {
		if ( iq.childElement().namespaceURI() != NS_VCARD_TEMP ) {
			IQ reply(iq);
			reply.swapFromTo();
			reply.setError(Stanza::Error::BadRequest);

			d->stream->sendStanza(reply);
			return;
		}
		if ( d->vcard.isEmpty() ) {
			IQ reply(iq);
			reply.swapFromTo();
			reply.setError(Stanza::Error::ItemNotFound);

			d->stream->sendStanza(reply);
			return;
		}

		IQ reply(iq);
		reply.swapFromTo();
		reply.setType(IQ::Result);
		d->vcard.toIQ(reply);

		d->stream->sendStanza(reply);
		return;
	}
	if ( iq.childElement().tagName() == "query" && iq.type() == "set" ) {
		if ( iq.childElement().namespaceURI() == NS_IQ_REGISTER ) {
			process_register_form(iq);
			return;
		}
	}
}

void JabberConnection::stream_message(const Message& msg)
{
	qDebug() << "message from" << msg.from() << "to" << msg.to() << "subject" << msg.subject();
}

void JabberConnection::stream_presence(const Presence& presence)
{
}

void JabberConnection::stream_connected()
{
	qDebug() << "signed on";
}

void JabberConnection::stream_error(const ComponentStream::Error& err)
{
	qDebug() << "Stream error! Condition:" << err.conditionString();
	qApp->quit();
}
