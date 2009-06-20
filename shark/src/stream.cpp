/*
 * stream.cpp - Abstract XMPP stream class.
 * Copyright (C) 2009  Alexander Saltykov
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
#include <QIODevice>
#include <QXmlAttributes>

#include "stream.h"
#include "stream_p.h"
#include "streamerror.h"

#include "xmpp-core/jid.h"
#include "xmpp-core/iq.h"
#include "xmpp-core/message.h"
#include "xmpp-core/presence.h"

namespace XMPP {


Stream::Stream(QObject *parent)
    : QObject(parent), d(new Private)
{
    d->bytestream = 0;
    d->state = Closed;
}

/**
 * Destroys the stream.
 */
Stream::~Stream()
{
    delete d;
}

/**
 * Returns last (and the only possible) stream error (if any).
 */
StreamError Stream::lastStreamError() const
{
    return d->lastStreamError;
}

/**
 * @fn QString ComponentStream::baseNS() const
 * @return base stream namespace.
 */

/**
 * Sends @a stanza to the outgoing stream.
 */
void Stream::sendStanza(const Stanza& stanza)
{
    write ( stanza.toString().toUtf8() );
}

void Stream::sendStreamOpen()
{
    d->state = Open;
    write("<?xml version='1.0'?>");
    write("<stream:stream xmlns:stream='"
            + QByteArray(NS_ETHERX)
            + "' xmlns='"
            + baseNS().toUtf8()
            + "' to='" + d->remoteEntity.full().toUtf8()
            + "'>");
}

void Stream::sendStreamClose()
{
    write("</stream:stream>");
}

/**
 * Sets bytestream to @a bs
 * @note This method does nothing if the stream is already open.
 * @param bs QIODevice based stream.
 */
void Stream::setByteStream(QIODevice *bs)
{
    if ( d->state == Open )
        return;
    if ( d->bytestream )
        d->bytestream->disconnect();

    QObject::connect( bs, SIGNAL(readyRead()), SLOT(bsReadyRead()) );
    QObject::connect( bs, SIGNAL(aboutToClose()), SLOT(bsClosing()) );
    d->bytestream = bs;
}

/**
 * Sets the remote entity of the stream
 */
void Stream::setRemoteEntity(const Jid& entity)
{
    d->remoteEntity = entity;
}

/**
 * @fn void Stream::handleStreamOpen(const Parser::Event& e)
 * This method is called when DocumentOpen event is received from
 * the remote entity.
 */

/**
 * @fn bool Stream::handleUnknownElement(const Parser::Event& e)
 *
 * This method handles first-level elements which are not
 * defined in RFC-3920.
 *
 * @return Method must return true if element was handled, false otherwise.
 */

/**
 * Handles stream erorrs (\<stream:error/\> element).
 */
void Stream::handleStreamError(const Parser::Event& event)
{
    d->lastStreamError = StreamError( event.element() );
    emit streamError();
}

/**
 * Process events from incoming stream.
 */
void Stream::processEvent(const Parser::Event& event)
{
    switch ( event.type() ) {
        case Parser::Event::DocumentOpen:
            {
                qDebug("[XMPP::Stream] Remote entity has opened the stream");
                handleStreamOpen(event);
                emit streamOpened();
            }
            break;
        case Parser::Event::DocumentClose:
            {
                qWarning("[XMPP::Stream] Remote entity has closed the stream");
                emit streamClosed();
            }
            break;
        case Parser::Event::Element:
            processStanza(event);
            break;
        case Parser::Event::Error:
            qCritical("[XMPP::Stream] Parser error");
            break;
    }
}

/**
 * Process incoming stanza (first-level element).
 */
void Stream::processStanza(const Parser::Event& event)
{
    if ( event.qualifiedName() == "stream:error" ) {
        handleStreamError(event);
    } else if ( event.qualifiedName() == "message" ) {
        emit stanzaMessage(event.element());
    } else if ( event.qualifiedName() == "iq" ) {
        emit stanzaIQ(event.element());
    } else if ( event.qualifiedName() == "presence" ) {
        emit stanzaPresence(event.element());
    } else {
        if (!handleUnknownElement(event))
            qDebug("[XMPP::Stream] Unhandled first-level element: %s",
                    qPrintable(event.qualifiedName()));
    }
}

/**
 * Write data to the outgoing stream.
 */
void Stream::write(const QByteArray& data)
{
    if (d->state != Open)
        return;
    // qDebug("[XMPP:Stream] -send-: %s", qPrintable(QString::fromUtf8(data)));
    d->bytestream->write(data);
}

void Stream::bsReadyRead()
{
    QByteArray data = d->bytestream->readAll();
    // qDebug("[XMPP:Stream] -recv-: %s", qPrintable(QString::fromUtf8(data)));

    d->parser.appendData(data);
    Parser::Event event = d->parser.readNext();
    while ( !event.isNull() ) {
        processEvent(event);
        event = d->parser.readNext();
    }
}

void Stream::bsClosing()
{
    qDebug("[XMPP::Stream] Bytestream is about to close");
    d->state = Closed;
    emit streamClosed();
}

/**
 * @fn void Stream::streamOpened()
 *
 * This signal is emitted when the stream receives \<stream:stream\> tag
 * from the remote entity
 */

/**
 * @fn void Stream::streamClosed()
 *
 * This signal is emitted when the stream receives stream closing tag or
 * when the stream IODevice is about to close
 */

/**
 * @fn void Stream::streamError()
 *
 * This signal is emitted when there's an incoming \<stream:error\> element.
 *
 * @sa lastStreamError()
 */

/**
 * @fn void Stream::streamReady()
 *
 * This signal should be emitted when the stream is ready to accept stanzas
 */


} /* end of namespace XMPP */

// vim:ts=4:sw=4:et:nowrap
