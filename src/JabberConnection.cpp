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
#include "UserManager.h"

#include "xmpp-core/connector.h"
#include "xmpp-core/iq.h"
#include "xmpp-core/jid.h"
#include "xmpp-core/message.h"
#include "xmpp-core/presence.h"

#include "streamerror.h"
#include "xmpp-ext/adhoc.h"
#include "xmpp-ext/dataform.h"
#include "xmpp-ext/gatewaytask.h"
#include "xmpp-ext/registration.h"
#include "xmpp-ext/servicediscovery.h"
#include "xmpp-ext/vcard.h"
#include "xmpp-ext/rosterx.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QHash>
#include <QStringList>
#include <QTextCodec>
#include <QUrl>
#include <QVariant>
#include <qmath.h>

#include <stdlib.h>

using namespace XMPP;

#define NS_IQ_GATEWAY "jabber:iq:gateway"

static const int SEC_MINUTE = 60;
static const int SEC_HOUR   = 3600;
static const int SEC_DAY    = 86400;
static const int SEC_WEEK   = 604800;

class JabberConnection::Private {

    public:
        void processAdHoc(const IQ& iq);
        void processDiscoInfo(const IQ& iq);
        void processDiscoItems(const IQ& iq);
        void processPromptRequest(const IQ& iq);
        void processPrompt(const IQ& iq);

        void initCommands();

        JabberConnection *q;

        Connector* connector;
        ComponentStream* stream;
        Jid jid;
        vCard vcard;
        DiscoInfo disco;
        QString secret;

        QDateTime startTime;

        /* list of adhoc commands */
        QHash<QString,DiscoItem> commands;

        XMPP::GatewayTask *gwtask;
};

void JabberConnection::Private::initCommands()
{
    commands.clear();

    commands.insert( "fetch-contacts", DiscoItem(jid, "fetch-contacts", "Fetch ICQ contacts") );
    commands.insert( "cmd-uptime",     DiscoItem(jid, "cmd-uptime",     "Report service uptime") );
    commands.insert( "set-options",    DiscoItem(jid, "set-options",    "Set service parameters") );
}

static XMPP::GatewayTask* init_gateway_task(JabberConnection *jc, XMPP::ComponentStream *stream)
{
    XMPP::Registration regform;
    regform.setField(XMPP::Registration::Instructions, QString("Enter UIN and password"));
    regform.setField(XMPP::Registration::Username);
    regform.setField(XMPP::Registration::Password);

    XMPP::GatewayTask *gw_task = new XMPP::GatewayTask(stream);
    gw_task->setRegistrationForm(regform);
    QObject::connect( gw_task, SIGNAL(userRegister(XMPP::Jid,QString,QString)),
                      jc, SIGNAL(userRegistered(XMPP::Jid,QString,QString)) );
    QObject::connect( gw_task, SIGNAL(userUnregister(XMPP::Jid)),
                      jc, SIGNAL(userUnregistered(XMPP::Jid)) );
    QObject::connect( gw_task, SIGNAL(userLogIn(XMPP::Jid,int)),
                      jc, SIGNAL(userOnline(XMPP::Jid,int)) );
    QObject::connect( gw_task, SIGNAL(userLogOut(XMPP::Jid)),
                      jc, SIGNAL(userOffline(XMPP::Jid)) );
    QObject::connect( gw_task, SIGNAL(addContact(XMPP::Jid,QString)),
                      jc, SIGNAL(userAdd(XMPP::Jid,QString)) );
    QObject::connect( gw_task, SIGNAL(deleteContact(XMPP::Jid,QString)),
                      jc, SIGNAL(userDel(XMPP::Jid,QString)) );
    QObject::connect( gw_task, SIGNAL(grantAuth(XMPP::Jid,QString)),
                      jc, SIGNAL(userAuthGrant(XMPP::Jid,QString)) );
    QObject::connect( gw_task, SIGNAL(denyAuth(XMPP::Jid,QString)),
                      jc, SIGNAL(userAuthDeny(XMPP::Jid,QString)) );
    QObject::connect( gw_task, SIGNAL(messageToLegacyNode(XMPP::Jid,QString,QString)),
                      jc, SIGNAL(outgoingMessage(XMPP::Jid,QString,QString)) );
    return gw_task;
}

/**
 * Constructs jabber-connection object.
 */
JabberConnection::JabberConnection(QObject *parent)
    : QObject(parent)
{
    d = new Private;
    d->q = this;

    d->connector = new XMPP::Connector;
    d->stream = new XMPP::ComponentStream(d->connector);

    d->disco << DiscoInfo::Identity("gateway", "icq", "ICQ Transport");
    d->disco << NS_IQ_REGISTER << NS_QUERY_ADHOC << NS_VCARD_TEMP << NS_IQ_GATEWAY
            << NS_ROSTERX;

    d->vcard.setFullName("ICQ Transport");
    d->vcard.setDescription("Qt ICQ Transport");
    d->vcard.setUrl( QUrl("http://github.com/holycheater/qt-icq-transport") );

    QObject::connect( d->stream, SIGNAL(stanzaIQ(XMPP::IQ)),
            SLOT(stream_iq(XMPP::IQ)) );

    QObject::connect( d->stream, SIGNAL(streamReady()),
            SLOT(slotStreamReady()) );
    QObject::connect( d->stream, SIGNAL(streamReady()),
            SIGNAL(connected()) );
    QObject::connect( d->stream, SIGNAL(streamClosed()),
            SLOT(slotStreamClosed()) );
    QObject::connect( d->stream, SIGNAL(streamError()),
            SLOT(slotStreamError()) );

    d->gwtask = init_gateway_task(this, d->stream);
}

/**
 * Destroys jabber-connection object.
 */
JabberConnection::~JabberConnection()
{
    delete d->gwtask;
    delete d->stream;
    delete d->connector;
}

/**
 * Start connecting to jabber-server.
 */
void JabberConnection::login()
{
    d->stream->connectToServer(d->jid, d->secret);
}

/**
 * Sets jabber-id to @a username (it should be equal to domain name which the component will serve)
 */
void JabberConnection::setUsername(const QString& username)
{
    d->jid = username;
    d->initCommands();
}

/**
 * Sets jabber server host and port to connect to.
 */
void JabberConnection::setServer(const QString& host, quint16 port)
{
    d->connector->setOptHostPort(host, port);
}

/**
 * Sets secret keyword for jabber server to authorize component.
 */
void JabberConnection::setPassword(const QString& password)
{
    d->secret = password;
}

/**
 * Sends 'subscribe' presence to @a toUser on behalf uin\@component.domain
 */
void JabberConnection::sendSubscribe(const Jid& toUser, const QString& uin)
{
    Presence subscribe;

    subscribe.setType(Presence::Subscribe);
    subscribe.setFrom( d->jid.withNode(uin) );
    subscribe.setTo(toUser);

    d->stream->sendStanza(subscribe);
}

/**
 * Sends 'subscribed' presence to @a toUser on behalf uin\@component.domain
 */
void JabberConnection::sendSubscribed(const Jid& toUser, const QString& fromUin, const QString& nick)
{
    Presence subscribed;

    subscribed.setType(Presence::Subscribed);
    subscribed.setFrom( d->jid.withNode(fromUin) );
    subscribed.setTo(toUser);
    subscribed.setNick(nick);

    d->stream->sendStanza(subscribed);
}

/**
 * Sends 'unsubscribe' presence to @a toUser on behalf uin\@component.domain
 */
void JabberConnection::sendUnsubscribe(const Jid& toUser, const QString& fromUin)
{
    Presence unsubscribe;

    unsubscribe.setType(Presence::Unsubscribe);
    unsubscribe.setFrom( d->jid.withNode(fromUin) );
    unsubscribe.setTo(toUser);

    d->stream->sendStanza(unsubscribe);
}

/**
 * Sends 'unsubscribed' presence to @a toUser on behalf uin\@component.domain
 */
void JabberConnection::sendUnsubscribed(const Jid& toUser, const QString& fromUin)
{
    Presence unsubscribed;

    unsubscribed.setType(Presence::Unsubscribed);
    unsubscribed.setFrom( d->jid.withNode(fromUin) );
    unsubscribed.setTo(toUser);

    d->stream->sendStanza(unsubscribed);
}

/**
 * Sends 'available' presence to @a toUser on behalf of '@a fromUin [at] component.domain'
 */
void JabberConnection::sendOnlinePresence(const Jid& toUser, const QString& fromUin, int showStatus, const QString& nick)
{
    Presence presence;
    presence.setFrom( d->jid.withNode(fromUin) );
    presence.setTo(toUser);
    presence.setShow( Presence::Show(showStatus) );
    presence.setNick(nick);

    d->stream->sendStanza(presence);
}

/**
 * Sends 'unavailable' presence to @a toUser on behalf of '@a fromUin [at] component.domain'
 */
void JabberConnection::sendOfflinePresence(const Jid& toUser, const QString& fromUin)
{
    Presence presence;
    presence.setFrom( d->jid.withNode(fromUin) );
    presence.setTo(toUser);
    presence.setType(Presence::Unavailable);

    d->stream->sendStanza(presence);
}

/**
 * Sends 'available' presence to @a toUser on behalf of component.
 */
void JabberConnection::sendOnlinePresence(const Jid& recipient, int showStatus)
{
    Presence presence;
    presence.setFrom(d->jid);
    presence.setTo(recipient);
    presence.setShow( Presence::Show(showStatus) );

    d->stream->sendStanza(presence);
}

/**
 * Sends 'unavailable' presence to @a toUser on behalf of component.
 */
void JabberConnection::sendOfflinePresence(const Jid& recipient)
{
    Presence presence;
    presence.setFrom(d->jid);
    presence.setTo(recipient);
    presence.setType(Presence::Unavailable);

    d->stream->sendStanza(presence);
}

void JabberConnection::sendPresenceProbe(const Jid& user)
{
    Presence presence;
    presence.setFrom(d->jid);
    presence.setTo(user);
    presence.setType(Presence::Probe);

    d->stream->sendStanza(presence);
}

/**
 * Send message from legacy user to jabber user.
 * @param recipient jabber-user
 * @param uin       ICQ sender's UIN.
 * @param message   Message itself.
 * @param nick      ICQ user nick.
 * @param timestamp message timestamp.
 */
void JabberConnection::sendMessage(const Jid& recipient, const QString& uin, const QString& message, const QString& nick, const QDateTime& timestamp)
{
    Message msg;
    msg.setFrom( d->jid.withNode(uin) );
    msg.setTo(recipient);
    msg.setBody(message);
    msg.setNick(nick);
    msg.setType(Message::Chat);
    msg.setTimestamp(timestamp);

    d->stream->sendStanza(msg);
}

void JabberConnection::sendMessage(const Jid& recipient, const QString& uin, const QString& message, const QString& nick)
{
    Message msg;
    msg.setFrom( d->jid.withNode(uin) );
    msg.setTo(recipient);
    msg.setBody(message);
    msg.setNick(nick);
    msg.setType(Message::Chat);

    d->stream->sendStanza(msg);
}

/**
 * Send a @a message to @a recipient on behalf of this service.
 */
void JabberConnection::sendMessage(const Jid& recipient, const QString& message)
{
    Message msg;
    msg.setFrom(d->jid);
    msg.setTo(recipient);
    msg.setBody(message);
    msg.setType(Message::Chat);

    d->stream->sendStanza(msg);
}

void JabberConnection::sendVCard(const Jid& recipient, const QString& uin, const QString& requestID, const vCard& vcard)
{
    IQ reply;
    reply.setFrom( d->jid.withNode(uin) );
    reply.setTo(recipient);
    reply.setId(requestID);
    reply.setType(IQ::Result);

    if ( vcard.isEmpty() ) {
        reply.setError(Stanza::Error::ItemNotFound);
        d->stream->sendStanza(reply);
    }
    vcard.toIQ(reply);
    d->stream->sendStanza(reply);
}

void JabberConnection::slotRosterAdd(const Jid& user, const QList<XMPP::RosterXItem>& items)
{
    QList<RosterXItem> copy = items;
    QList<RosterXItem>::iterator it;
    QList<RosterXItem>::iterator itEnd = copy.end();
    for ( it = copy.begin(); it != itEnd; ++it ) {
        (*it).setJid( d->jid.withNode((*it).jid()) );
    }

    IQ contacts;
    contacts.setFrom(d->jid);
    contacts.setTo(user);
    contacts.setType(IQ::Set);

    RosterX x;
    x.setItems(copy);
    x.toIQ(contacts);

    d->stream->sendStanza(contacts);
}

void JabberConnection::Private::processAdHoc(const IQ& iq)
{
    AdHoc cmd = AdHoc::fromIQ(iq);
    // qDebug() << "[JC]" << "Adhoc command from" << iq.from() << "command" << cmd.node();

    if ( cmd.action() == AdHoc::Cancel ) {
        // qDebug() << "[JC]" << "Command" << cmd.node() << "from" << iq.from() << "was canceled";

        IQ reply = IQ::createReply(iq);
        cmd.setStatus(AdHoc::Canceled);
        cmd.setAction(AdHoc::ActionNone);
        cmd.toIQ(reply);

        stream->sendStanza(reply);
        return;
    }
    if ( cmd.action() != AdHoc::Execute ) {
        return;
    }

    if ( !commands.contains(cmd.node()) ) {
        IQ reply = IQ::createReply(iq);
        reply.setError(Stanza::Error::ItemNotFound);

        stream->sendStanza(reply);
        return;
    }

    if ( cmd.node() == "fetch-contacts" ) {
        if ( !UserManager::instance()->isRegistered(iq.from().bare()) ) {
            IQ err = IQ::createReply(iq);
            err.setError(Stanza::Error::NotAuthorized);

            stream->sendStanza(err);
            return;
        }
        emit q->cmd_RosterRequest( iq.from() );
    } else if ( cmd.node() == "cmd-uptime" ) {
        uint uptime_t = QDateTime::currentDateTime().toTime_t() - startTime.toTime_t();
        int weeks = qCeil(uptime_t / SEC_WEEK);
        uptime_t -= weeks*SEC_WEEK;
        int days = qCeil(uptime_t / SEC_DAY);
        uptime_t -= days*SEC_DAY;
        int hours = qCeil(uptime_t / SEC_HOUR);
        uptime_t -= hours*SEC_HOUR;
        int minutes = qCeil(uptime_t / SEC_MINUTE);
        uptime_t -= minutes*SEC_MINUTE;
        int seconds = uptime_t;

        QString uptimeText = QString::number(weeks) + "w " + QString::number(days) + "d " + QString::number(hours) + "h " + QString::number(minutes) + "m " + QString::number(seconds)+"s.";

        Message msg;
        msg.setTo( iq.from() );
        msg.setFrom(jid);
        msg.setBody("Uptime: "+uptimeText);
        stream->sendStanza(msg);
    } else if ( cmd.node() == "set-options" ) {
        if ( !UserManager::instance()->isRegistered(iq.from().bare()) ) {
            IQ err = IQ::createReply(iq);
            err.setError(Stanza::Error::NotAuthorized);

            stream->sendStanza(err);
            return;
        }

        if ( iq.childElement().firstChildElement("x").attribute("type") == "submit" ) {
            DataForm form = DataForm::fromDomElement( iq.childElement().firstChildElement("x") );

            DataForm::Field fai = form.fieldByName("auto-invite");
            QString auto_invite = fai.values().at(0);
            bool o_auto_invite;
            if ( auto_invite == "true" || auto_invite == "1" ) {
                o_auto_invite = true;
            } else {
                o_auto_invite = false;
            }
            UserManager::instance()->setOption(iq.from().bare(), "auto-invite", QVariant(o_auto_invite));

            QString auto_reconnect = form.fieldByName("auto-reconnect").values().at(0);
            bool o_auto_reconnect;
            if ( auto_reconnect == "true" || auto_invite == "1" ) {
                o_auto_reconnect = true;
            } else {
                o_auto_reconnect = false;
            }
            UserManager::instance()->setOption(iq.from().bare(),
                                               "auto-reconnect", QVariant(o_auto_reconnect));
            QString encoding = form.fieldByName("encoding").values().at(0);
            UserManager::instance()->setOption(iq.from().bare(), "encoding", encoding);
        } else {
            cmd.setStatus(AdHoc::Executing);
            cmd.setSessionID( "set-options:"+QDateTime::currentDateTime().toString(Qt::ISODate) );

            DataForm form;
            form.setTitle("Service configuration");
            form.setInstructions("Please configure your settings");

            DataForm::Field fldAutoInvite("auto-invite", "Auto-Invite", DataForm::Field::Boolean);
            if ( UserManager::instance()->getOption(iq.from().bare(),"auto-invite").toBool() == true ) {
                fldAutoInvite.addValue("true");
            }
            form.addField(fldAutoInvite);

            DataForm::Field fldAutoReconnect("auto-reconnect", "Automatically reconnect",
                                             DataForm::Field::Boolean);
            if ( UserManager::instance()->getOption(iq.from().bare(),"auto-reconnect").toBool() == true ) {
                fldAutoReconnect.addValue("true");
            }
            form.addField(fldAutoReconnect);

            DataForm::Field fldEncoding("encoding", "Codepage", DataForm::Field::ListSingle);

            QListIterator<QByteArray> ci( QTextCodec::availableCodecs() );
            while ( ci.hasNext() ) {
                QString enc = ci.next();
                fldEncoding.addOption(enc,enc);
            }
            QString userEncoding = UserManager::instance()->getOption(iq.from().bare(), "encoding").toString();
            if ( !userEncoding.isEmpty() ) {
                fldEncoding.addValue(userEncoding);
            } else {
                fldEncoding.addValue("windows-1251");
            }
            fldEncoding.setDesc( tr("This option is used to set encoding for retrieving/sending offline messages and user details since ICQ service doesn't fully support UTF-8") );
            form.addField(fldEncoding);

            cmd.setForm(form);

            IQ reply = IQ::createReply(iq);
            cmd.toIQ(reply);
            stream->sendStanza(reply);
            return;
        }
    }

    IQ completedNotify = IQ::createReply(iq);
    cmd.setStatus(AdHoc::Completed);
    cmd.setAction(AdHoc::ActionNone);
    cmd.setForm( DataForm() );
    cmd.toIQ(completedNotify);

    stream->sendStanza(completedNotify);
}

void JabberConnection::Private::processDiscoInfo(const IQ& iq)
{
    // qDebug() << "disco-info query from" << iq.from().full() << "to" << iq.to().full();

    /* disco-info to command-node query handling */
    QString node = iq.childElement().attribute("node");
    if ( !node.isEmpty() & commands.contains(node) ) {
        // qDebug() << "[JC]" << "disco-info to command node: " << node;

        IQ adhoc_info = IQ::createReply(iq);

        DiscoInfo info;
        info << DiscoInfo::Identity("automation", "command-node", commands.value(node).name() );
        info << NS_DATA_FORMS;
        info << NS_QUERY_ADHOC;
        info.pushToDomElement( adhoc_info.childElement() );

        stream->sendStanza(adhoc_info);
        return;
    }

    IQ reply = IQ::createReply(iq);

    disco.pushToDomElement( reply.childElement() );
    stream->sendStanza(reply);
}

void JabberConnection::Private::processDiscoItems(const IQ& iq)
{
    IQ reply = IQ::createReply(iq);

    /* process disco-items to the service itself */
    if ( iq.childElement().attribute("node").isEmpty() || iq.childElement().attribute("node") == NS_QUERY_ADHOC ) {
        DiscoItems items;

        if ( UserManager::instance()->isRegistered(iq.from().bare()) ) {
            QHashIterator<QString,DiscoItem> ci(commands);
            while ( ci.hasNext() ) {
                ci.next();
                items << ci.value();
            }
            items.pushToDomElement( reply.childElement() );
        }
    }

    stream->sendStanza(reply);
}

void JabberConnection::Private::processPromptRequest(const IQ& iq)
{
    IQ prompt = IQ::createReply(iq);

    QDomDocument doc = prompt.childElement().ownerDocument();

    QDomElement eDesc = doc.createElement("desc");
    QDomText eDescText = doc.createTextNode("Please enter the ICQ Number of the person you would like to contact.");
    prompt.childElement().appendChild(eDesc);
    eDesc.appendChild(eDescText);

    QDomElement ePrompt = doc.createElement("prompt");
    QDomText ePromptText = doc.createTextNode("Contact ID");
    prompt.childElement().appendChild(ePrompt);
    ePrompt.appendChild(ePromptText);

    stream->sendStanza(prompt);
}

void JabberConnection::Private::processPrompt(const IQ& iq)
{
    QString uin = iq.childElement().firstChildElement("prompt").text();

    IQ reply = IQ::createReply(iq);

    bool ok;
    int u = uin.toInt(&ok, 10);
    if ( !ok && u <= 0 ) {
        reply.setError(Stanza::Error::ItemNotFound);
        stream->sendStanza(reply);
        return;
    }

    reply.clearChild();
    QDomDocument doc = reply.childElement().ownerDocument();
    QDomElement eJid = doc.createElement("jid");
    reply.childElement().appendChild(eJid);
    QDomText eJidText = doc.createTextNode( jid.withNode(uin) );
    eJid.appendChild(eJidText);

    stream->sendStanza(reply);
}

void JabberConnection::stream_iq(const XMPP::IQ& iq)
{
    if ( iq.childElement().tagName() == "query" && iq.type() == "get" ) {
        if ( iq.childElement().namespaceURI() == NS_QUERY_DISCO_INFO ) {
            d->processDiscoInfo(iq);
            return;
        }
        if ( iq.childElement().namespaceURI() == NS_QUERY_DISCO_ITEMS ) {
            d->processDiscoItems(iq);
            return;
        }
        if ( iq.childElement().namespaceURI() == NS_IQ_GATEWAY ) {
            d->processPromptRequest(iq);
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
        if ( !iq.to().node().isEmpty() ) {
            emit vCardRequest( iq.from(), iq.to().node(), iq.id() );
            return;
        }

        if ( d->vcard.isEmpty() ) {
            IQ reply(iq);
            reply.swapFromTo();
            reply.setError(Stanza::Error::ItemNotFound);

            d->stream->sendStanza(reply);
            return;
        }

        IQ reply = IQ::createReply(iq);
        d->vcard.toIQ(reply);

        d->stream->sendStanza(reply);
        return;
    }
    if ( iq.childElement().tagName() == "query" && iq.type() == "set" ) {
        if ( iq.childElement().namespaceURI() == NS_IQ_GATEWAY ) {
            d->processPrompt(iq);
            return;
        }
    }
    if ( iq.childElement().tagName() == "command" && iq.type() == "set" && iq.childElement().namespaceURI() == NS_QUERY_ADHOC ) {
        d->processAdHoc(iq);
        return;
    }
    if ( iq.childElement().tagName() == "x" && iq.type() == "result" && iq.childElement().namespaceURI() == NS_ROSTERX ) {
        qDebug("Roster update success for '%s'", qPrintable(QString(iq.from())));
    }

    if ( iq.type() == "error" ) {
        /* TODO: Error logging? */
        return;
    }
    qWarning("[JC] Unhandled IQ from %s type:%s, tag:%s, nsuri:%s",
             qPrintable(iq.from().full()),
             qPrintable(iq.type()),
             qPrintable(iq.childElement().tagName()),
             qPrintable(iq.childElement().namespaceURI()) );
}

void JabberConnection::slotStreamReady()
{
    d->startTime = QDateTime::currentDateTime();
    qDebug("[JC] Component signed on");
}

void JabberConnection::slotStreamError()
{
    qCritical("[JC] Stream error: %s",
            qPrintable(d->stream->lastStreamError().conditionString()) );
    exit(1);
}

void JabberConnection::slotStreamClosed()
{
    qDebug("[JC] Stream closed");
    exit(0);
}

// vim:et:ts=4:sw=4:nowrap
