/*
 * gatewaytask.cpp - Gateway interaction task (XEP-0100)
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

#include "gatewaytask.h"

#include "componentstream.h"

#include "xmpp-core/jid.h"
#include "xmpp-core/message.h"
#include "xmpp-core/iq.h"
#include "xmpp-core/presence.h"

#include "xmpp-ext/registration.h"

#include <QDomElement>

namespace XMPP {


class GatewayTask::Private {
    public:
        Registration reg;
        ComponentStream *stream;
};

static void send_presence(ComponentStream *cs, const XMPP::Jid& user, const QString& legacyName, Presence::Type t)
{
    Jid from = cs->serviceName().withNode(legacyName);
    Presence p = Presence(t, from, user);
    cs->sendStanza(p);
}

GatewayTask::GatewayTask(ComponentStream *stream)
    : QObject(stream), d(new Private)
{
    d->stream = stream;
    QObject::connect( stream, SIGNAL(stanzaIQ(XMPP::IQ)),
                      SLOT(slotRegister(XMPP::IQ)) );
    QObject::connect( stream, SIGNAL(stanzaMessage(XMPP::Message)),
                      SLOT(slotMessage(XMPP::Message)) );
    QObject::connect( stream, SIGNAL(stanzaPresence(XMPP::Presence)),
                      SLOT(slotGatewaySubscribe(XMPP::Presence)) );
    QObject::connect( stream, SIGNAL(stanzaPresence(XMPP::Presence)),
                      SLOT(slotSubscription(XMPP::Presence)) );
    QObject::connect( stream, SIGNAL(stanzaPresence(XMPP::Presence)),
                      SLOT(slotPresence(XMPP::Presence)) );
}

GatewayTask::~GatewayTask()
{
}

void GatewayTask::setRegistrationForm(const Registration& reg)
{
    d->reg = reg;
}

void GatewayTask::notifyOnline(const XMPP::Jid& user, const QString& legacyName, int presence_show)
{
    Jid from = d->stream->serviceName().withNode(legacyName);
    Presence p = Presence(Presence::Available, from, user, (Presence::Show)presence_show);
    d->stream->sendStanza(p);
}

void GatewayTask::notifyOffline(const XMPP::Jid& user, const QString& legacyName)
{
    send_presence(d->stream, user, legacyName, Presence::Unavailable);
}

void GatewayTask::notifySubscribe(const XMPP::Jid& user, const QString& legacyName)
{
    send_presence(d->stream, user, legacyName, Presence::Subscribe);
}

void GatewayTask::notifyUnsubscribe(const XMPP::Jid& user, const QString& legacyName)
{
    send_presence(d->stream, user, legacyName, Presence::Unsubscribe);
}

void GatewayTask::notifySubscribed(const XMPP::Jid& user, const QString& legacyName)
{
    send_presence(d->stream, user, legacyName, Presence::Subscribed);
}

void GatewayTask::notifyUnsubscribed(const XMPP::Jid& user, const QString& legacyName)
{
    send_presence(d->stream, user, legacyName, Presence::Unsubscribed);
}

void GatewayTask::slotMessage(const XMPP::Message& msg)
{
    Jid user = msg.from();
    QString legacyNode = msg.to().node();
    QString text = msg.body();

    if ( !legacyNode.isEmpty() )
        emit messageToLegacyNode(user, legacyNode, text);
}

void GatewayTask::slotRegister(const XMPP::IQ& iq)
{
    QString tag = iq.childElement().tagName();
    QString ns = iq.childElement().namespaceURI();

    if ( tag == "query" && ns == NS_IQ_REGISTER && iq.type() == "get" ) {
        Registration form(d->reg);
        form.setTo(iq.from());
        form.setFrom(iq.to());
        form.setId(iq.id());
        form.setType(IQ::Result);

        d->stream->sendStanza(form);
        return;
    }
    if ( tag == "query" && ns == NS_IQ_REGISTER && iq.type() == "set" ) {
        Registration request(iq);

        if ( request.from().isEmpty() ) {
            Registration err = IQ::createReply(request);
            err.setError( Stanza::Error(Stanza::Error::UnexpectedRequest) );
            d->stream->sendStanza(err);
            return;
        }

        if ( request.hasField(Registration::Remove) ) {
            Registration reply = IQ::createReply(iq);
            reply.clearChild();
            reply.setType(IQ::Result);
            d->stream->sendStanza(reply);

            Presence removeSubscription;
            removeSubscription.setTo( iq.from().bare() );
            removeSubscription.setType(Presence::Unsubscribe);
            d->stream->sendStanza(removeSubscription);

            Presence removeAuth;
            removeAuth.setTo( iq.from().bare() );
            removeAuth.setType(Presence::Unsubscribed);
            d->stream->sendStanza(removeAuth);

            Presence logout;
            logout.setTo( iq.from() );
            logout.setType(Presence::Unavailable);
            d->stream->sendStanza(logout);

            emit userUnregister(iq.from());
            return;
        }

        if ( request.getField(Registration::Username).isEmpty() || request.getField(Registration::Password).isEmpty() ) {
            Registration err(request);
            err.swapFromTo();
            err.setError( Stanza::Error(Stanza::Error::NotAcceptable) );
            d->stream->sendStanza(err);
            return;
        }

        /* registration success */
        IQ reply = IQ::createReply(iq);
        reply.clearChild();
        d->stream->sendStanza(reply);

        /* subscribe for user presence */
        Presence subscribe;
        subscribe.setFrom(iq.to());
        subscribe.setTo( iq.from().bare() );
        subscribe.setType(Presence::Subscribe);
        d->stream->sendStanza(subscribe);

        emit userRegister( request.from(), request.getField(Registration::Username), request.getField(Registration::Password) );

        Presence presence;
        presence.setFrom(iq.to());
        presence.setTo( iq.from().bare() );
        d->stream->sendStanza(presence);

        /* execute log-in case */
        emit userLogIn(iq.from(), Presence::None);
        return;
    }
}

void GatewayTask::slotGatewaySubscribe(const XMPP::Presence& p)
{
    if ( p.type() == Presence::Subscribe && p.to().node().isEmpty() ) {
        Presence approve;
        approve.setType(Presence::Subscribed);
        approve.setTo( p.from() );
        approve.setFrom( p.to() );

        d->stream->sendStanza(approve);
    }
}

void GatewayTask::slotSubscription(const XMPP::Presence& p)
{
    QString legacyNode = p.to().node();
    if ( !p.to().node().isEmpty() ) {
        Jid user = p.from();
        switch ( p.type() ) {
        case Presence::Subscribe:
            emit addContact(user, legacyNode);
            break;
        case Presence::Unsubscribe:
            emit deleteContact(user, legacyNode);
            break;
        case Presence::Subscribed:
            emit grantAuth(user, legacyNode);
            break;
        case Presence::Unsubscribed:
            emit denyAuth(user, legacyNode);
            break;
        default:
            break;
        }
    }
}

void GatewayTask::slotPresence(const XMPP::Presence& p)
{
    if ( p.type() == Presence::Available ) {
        emit userLogIn(p.from(), p.show());
    } else if ( p.type() == Presence::Unavailable ) {
        emit userLogOut(p.from());
    }
}


} /* end of namespace XMPP */


// vim:ts=4:sw=4:nowrap:et

