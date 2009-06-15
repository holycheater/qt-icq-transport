/*
 * Connector.cpp - establish a connection to an XMPP server
 * Copyright (C) 2003  Justin Karneges
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

#include "Connector.h"
#include "Connector_p.h"

using namespace XMPP;

Connector::Connector(QObject *parent)
	: QObject(parent)
{
	d = new Private(this);
}

Connector::~Connector()
{
	d->reset();
	delete d;
}

QTcpSocket* Connector::socket() const
{
	return d->socket;
}

void Connector::setOptHostPort(const QString& host, quint16 port)
{
	if (d->mode != Private::Idle) {
		return;
	}

	d->host = host;
	d->port = port;
}

void Connector::connectToServer(const QString& server)
{
	if (d->mode != Private::Idle) {
		qWarning("[XMPP::Connector] Already connected/connecting");
		return;
	}
	if ( server.isEmpty() ) {
		emit error(EHostLookupFailed);
		qWarning("[XMPP::Connector] Server address not specified");
		return;
	}
	d->mode = Private::Connecting;

	QString host;

	if ( !d->host.isEmpty() && d->port != 0 ) {
		host = d->host;
	} else {
		host = server;
	}

	if ( QHostAddress(host).isNull() ) {
		d->lookupTimer = new QTimer(this);
		QObject::connect( d->lookupTimer, SIGNAL( timeout() ), d, SLOT( processLookupTimeout() ) );
		d->lookupTimer->setSingleShot(true);
		d->lookupTimer->start(d->lookupTimeout);

		d->lookupID = QHostInfo::lookupHost(host, d, SLOT( processLookupResult(QHostInfo) ) );
	} else {
		d->addr.setAddress(host);
		d->beginConnect();
	}
}

// vim:ts=4:sw=4:noet:nowrap
