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

#include "core/IQ.h"
#include "core/Connector.h"
#include "core/Jid.h"

#include <QCoreApplication>
#include <QtDebug>

class JabberConnection::Private {

	public:
		XMPP::Connector* connector;
		XMPP::ComponentStream* stream;
		XMPP::Jid jid;
		QString secret;
};

using namespace XMPP;

JabberConnection::JabberConnection(QObject *parent)
	: QObject(parent)
{
	d = new Private;

	d->connector = new Connector;
	d->stream = new ComponentStream(d->connector);

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

void JabberConnection::stream_iq(const IQ& iq)
{
}

void JabberConnection::stream_message(const Message& msg)
{
}

void JabberConnection::stream_presence(const Presence& presence)
{
}

void JabberConnection::stream_connected()
{
	qDebug() << "signed on";
	IQ query;
	query.setType(IQ::Get);
	query.setFrom("icq.dragonfly");
	query.setTo("dragonfly");
	query.setChildElement("query", "http://jabber.org/protocol/disco#info");
	//d->stream->sendStanza(query);
}

void JabberConnection::stream_error(const ComponentStream::Error& err)
{
	qDebug() << "Stream error! Condition:" << err.type();
	qApp->quit();
}
