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

#include "IQ.h"

#include <im.h>
#include <xmpp.h>
#include <QCoreApplication>

class JabberConnection::Private {

	public:
		XMPP::AdvancedConnector* connector;
		X::ComponentStream* stream;
		XMPP::Jid jid;
		QString secret;
};



JabberConnection::JabberConnection(QObject *parent)
	: QObject(parent)
{
	d = new Private;

	d->connector = new XMPP::AdvancedConnector;
	d->stream = new X::ComponentStream(d->connector);

	QObject::connect(d->stream, SIGNAL( error(const X::Stream::Error&) ), SLOT( stream_error(const X::Stream::Error&) ) );
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

void JabberConnection::stream_connected()
{
	qDebug() << "signed on";
	X::IQ query;
	query.setType(X::IQ::Get);
	query.setFrom("icq.dragonfly");
	query.setTo("dragonfly");
	query.setChildElement("query", "http://jabber.org/protocol/disco#info");
	d->stream->sendStanza(query);
}

void JabberConnection::stream_error(X::ComponentStream::Error& err)
{
	qDebug() << "Stream error! Condition:" << err.type();
	qApp->quit();
}
