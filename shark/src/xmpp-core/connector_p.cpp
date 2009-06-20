/*
 * connector_p.cpp - establish a connection to an XMPP server (Private implementation)
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

#include "connector_p.h"

using namespace XMPP;

Connector::Private::Private(Connector *parent)
    : QObject(parent)
{
    q = parent;

    lookupTimeout = LOOKUP_TIMEOUT;
    connectionTimeout = CONNECT_TIMEOUT;

    reset();
}

Connector::Private::~Private()
{
}

void Connector::Private::beginConnect()
{
    delete socket; socket = 0;

    connectTimer = new QTimer(q);
    QObject::connect(connectTimer, SIGNAL( timeout() ), SLOT( processConnectionTimeout() ) );
    connectTimer->setSingleShot(true);
    connectTimer->start(connectionTimeout);

    socket = new QTcpSocket(q);
    QObject::connect( socket, SIGNAL( connected() ), q, SIGNAL( connected() ) );
    QObject::connect( socket, SIGNAL( connected() ), connectTimer, SLOT( deleteLater() ) );
    QObject::connect( socket, SIGNAL( error(QAbstractSocket::SocketError) ), SLOT( processSocketError(QAbstractSocket::SocketError) ) );

    socket->connectToHost(addr, port);
}

void Connector::Private::reset()
{
    mode = Idle;
    addr.clear();

    delete socket; socket = 0;
}

void Connector::Private::processLookupResult(const QHostInfo& host)
{
    delete lookupTimer;

    if ( host.error() != QHostInfo::NoError ) {
        emit q->error(EHostLookupFailed);
        return;
    }
    addr = host.addresses().value(0);
    qWarning( "[XMPP::Connector] Found address: %s", qPrintable(addr.toString()) );

    beginConnect();
}

void Connector::Private::processLookupTimeout()
{
    reset();
    lookupTimer->deleteLater();
    QHostInfo::abortHostLookup(lookupID);
    emit q->error(EHostLookupTimeout);
}

void Connector::Private::processConnectionTimeout()
{
    reset();
    connectTimer->deleteLater();
    emit q->error(EConnectionTimeout);
}

void Connector::Private::processSocketError(QAbstractSocket::SocketError errcode)
{
    Q_UNUSED(errcode)
    emit q->error(ESocketError);
}

// vim:ts=4:sw=4:et:nowrap
