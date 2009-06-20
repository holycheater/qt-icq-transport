/*
 * componentstream.cpp - Jabber component stream (jabber:component:accept)
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
#include <QTcpSocket>
#include <QXmlAttributes>

#include "componentstream.h"
#include "streamerror.h"

#include "xmpp-core/Jid.h"

namespace XMPP {


class ComponentStream::Private
{
    public:
        /* server connector (dns-lookups, connect) */
        Connector *connector;

        /* component Jabber-ID */
        Jid jid;

        /* server component secret (password) */
        QByteArray secret;
};

/**
 * Constructs stream object with @a connector to be used for initating
 * connection to the server. @a parent will be passed to the QObject constructor.
 */
ComponentStream::ComponentStream(Connector *connector, QObject *parent)
    : Stream(parent), d(new Private)
{
    d->connector = connector;
    QObject::connect( d->connector, SIGNAL(connected()),
            SLOT(slotConnectorConnected()) );
}

/**
 * Destroys the stream.
 */
ComponentStream::~ComponentStream()
{
    delete d;
}

QString ComponentStream::baseNS() const
{
    return NS_COMPONENT;
}

/**
 * Connects to the JID domain host to the specified port with secret.
 *
 * @param jid       jabber-id
 * @param secret    server secret for component
 */
void ComponentStream::connectToServer(const Jid& jid, const QString& secret)
{
    d->jid = jid;
    d->secret = secret.toUtf8();

    d->connector->connectToServer(jid.domain());
}

void ComponentStream::handleStreamOpen(const Parser::Event& e)
{
    QByteArray sid = e.attributes().value("id").toUtf8();
    QByteArray hashable = sid+d->secret;

    QByteArray hash = QCryptographicHash::hash(hashable, QCryptographicHash::Sha1).toHex();

    write("<handshake>" + hash + "</handshake>");
}

bool ComponentStream::handleUnknownElement(const Parser::Event& e)
{
    if ( e.qualifiedName() != "handshake" )
        return false;
    emit streamReady();
    return true;
}

void ComponentStream::slotConnectorConnected()
{
    setByteStream( d->connector->socket() );
    setRemoteEntity( d->jid.domain() );
    sendStreamOpen();
}


} /* end of namespace XMPP */

// vim:ts=4:sw=4:et:nowrap
