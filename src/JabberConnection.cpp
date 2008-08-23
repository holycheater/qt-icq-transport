/**
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
 **/

#include "JabberConnection.h"

#include <im.h>
#include <xmpp.h>
#include <QCoreApplication>

class JabberConnection::Private {

	public:
		XMPP::AdvancedConnector* connector;
		XMPP::ClientStream* stream;
		XMPP::Client* client;
		XMPP::Jid jid;
		QString password;
};

JabberConnection::JabberConnection(QObject *parent)
	: QObject(parent)
{
	d = new Private;

	d->connector = new XMPP::AdvancedConnector;
	d->stream = new XMPP::ClientStream(d->connector);
	d->client = new XMPP::Client(d->stream);

	QObject::connect( d->stream, SIGNAL( error( int ) ), SLOT( stream_error( int ) ) );
	QObject::connect( d->stream, SIGNAL( needAuthParams( bool, bool, bool ) ), SLOT( stream_needAuthParams( bool, bool, bool ) ) );
	QObject::connect( d->stream, SIGNAL( authenticated() ), SLOT( stream_authenticated() ) );
}

JabberConnection::~JabberConnection()
{
	delete d->client;
	delete d->stream;
	delete d->connector;
}

void JabberConnection::login()
{
	d->client->connectToServer(d->stream, d->jid);
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
	d->password = password;
}

void JabberConnection::stream_error(int err)
{
	qDebug() << "D'oh - a stream error occurred! Code: " << err;

	qApp->quit();
}

void JabberConnection::stream_authenticated()
{
	d->client->start( d->jid.host(), d->jid.user(), d->password, d->jid.resource() );

	qDebug() << "Authenticated on server - sweet!";

	// our work here is done...
	qApp->quit();
}

void JabberConnection::stream_needAuthParams(bool user, bool passwd, bool)
{

	if (user) {
		d->stream->setUsername( d->jid.node() );
	}

	if (passwd) {
		d->stream->setPassword(d->password);
	}

	qDebug() << "Sending auth params ...";
	d->stream->continueAfterParams();

}
