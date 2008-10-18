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

#include "xmpp-core/Connector.h"
#include "xmpp-core/IQ.h"
#include "xmpp-core/Jid.h"
#include "xmpp-core/Message.h"
#include "xmpp-core/Presence.h"

#include "xmpp-ext/DataForm.h"
#include "xmpp-ext/ServiceDiscovery.h"
#include "xmpp-ext/Registration.h"
#include "xmpp-ext/vCard.h"

#include <QCoreApplication>
#include <QStringList>
#include <QUrl>

#include <QtDebug>

using namespace XMPP;

#define NS_QUERY_ADHOC "http://jabber.org/protocol/commands"
#define NS_IQ_GATEWAY "jabber:iq:gateway"

class JabberConnection::Private {

	public:
		void processAdHoc(const IQ& iq);
		void processDiscoInfo(const IQ& iq);
		void processDiscoItems(const IQ& iq);
		void processRegisterRequest(const IQ& iq);
		void processRegisterForm(const Registration& iq);
		void processPromptRequest(const IQ& iq);
		void processPrompt(const IQ& iq);

		JabberConnection *q;

		Connector* connector;
		ComponentStream* stream;
		Jid jid;
		vCard vcard;
		DiscoInfo disco;
		QString secret;

		/* list of adhoc commands */
		QHash<QString,DiscoItem> commands;
};

/**
 * Constructs jabber-connection object.
 */
JabberConnection::JabberConnection(QObject *parent)
	: QObject(parent)
{
	d = new Private;
	d->q = this;

	d->connector = new Connector;
	d->stream = new ComponentStream(d->connector);

	d->disco << DiscoInfo::Identity("gateway", "icq", "ICQ Transport");
	d->disco << NS_IQ_REGISTER << NS_QUERY_ADHOC << NS_VCARD_TEMP << NS_IQ_GATEWAY;

	d->vcard.setFullName("ICQ Transport");
	d->vcard.setDescription("Qt ICQ Transport");
	d->vcard.setUrl( QUrl("http://github.com/holycheater") );

	d->commands.insert( "fetch-contacts", DiscoItem("icq.dragonfly", "fetch-contacts", "Fetch ICQ contacts") );
	d->commands.insert( "say-cheese", DiscoItem("icq.dragonfly", "say-cheese", "Say 'cheese'") );

	QObject::connect( d->stream, SIGNAL( stanzaIQ(IQ) ), SLOT( stream_iq(IQ) ) );
	QObject::connect( d->stream, SIGNAL( stanzaMessage(Message) ), SLOT( stream_message(Message) ) );
	QObject::connect( d->stream, SIGNAL( stanzaPresence(Presence) ), SLOT( stream_presence(Presence) ) );
	QObject::connect( d->stream, SIGNAL( error(ComponentStream::Error) ), SLOT( stream_error(ComponentStream::Error) ) );
	QObject::connect( d->stream, SIGNAL( connected() ), SLOT( stream_connected() ) );
	QObject::connect( d->stream, SIGNAL( connected() ), SIGNAL( connected() ) );
}

/**
 * Destroys jabber-connection object.
 */
JabberConnection::~JabberConnection()
{
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

/**
 * Message send slot from legacy user to jabber-user.
 * @param senderUin		ICQ message sender's uin.
 * @param recipient		Jabber user recipient's jabber-id.
 * @param message		Message itself.
 */
void JabberConnection::sendMessage(const Jid& recipient, const QString& uin, const QString& message, const QString& nick)
{
	Message msg;
	msg.setFrom( d->jid.withNode(uin) );
	msg.setTo(recipient);
	msg.setBody(message);
	msg.setNick(nick);

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

void JabberConnection::Private::processAdHoc(const IQ& iq)
{
	if ( iq.childElement().tagName() != "command" ) {
		qDebug() << "[JC] error.. not an adhoc command";
		return;
	}

	QString command = iq.childElement().attribute("node");
	qDebug() << "[JC]" << "Adhoc command from" << iq.from() << "command" << command;

	if ( !commands.contains(command) ) {
		IQ reply(iq);
		reply.swapFromTo();
		reply.setError(Stanza::Error::ItemNotFound);

		stream->sendStanza(reply);
		return;
	}

	if ( command == "say-cheese" ) {
		Message msg;
		msg.setTo( iq.from() );
		msg.setFrom(jid);
		msg.setBody("Cheese!.. n00b..");

		stream->sendStanza(msg);
	} else if ( command == "fetch-contacts" ) {
		emit q->cmd_RosterRequest( iq.from() );
	}

	IQ completedNotify(iq);
	completedNotify.swapFromTo();
	completedNotify.setType(IQ::Result);
	completedNotify.childElement().setAttribute("status", "completed");

	stream->sendStanza(completedNotify);
}

void JabberConnection::Private::processDiscoInfo(const IQ& iq)
{
	qDebug() << "disco-info query from" << iq.from().full() << "to" << iq.to().full();

	/* disco-info to command-node query handling */
	QString node = iq.childElement().attribute("node");
	if ( !node.isEmpty() & commands.contains(node) ) {
		qDebug() << "[JC]" << "disco-info to command node: " << node;

		IQ adhoc_info(iq);
		adhoc_info.swapFromTo();
		adhoc_info.setType(IQ::Result);

		DiscoInfo info;
		info << DiscoInfo::Identity("automation", "command-node", commands.value(node).name() );
		info << NS_DATA_FORMS;
		info << NS_QUERY_ADHOC;
		info.pushToDomElement( adhoc_info.childElement() );

		stream->sendStanza(adhoc_info);
		return;
	}

	IQ reply(iq);
	reply.swapFromTo();
	reply.setType(IQ::Result);

	disco.pushToDomElement( reply.childElement() );
	stream->sendStanza(reply);
}

void JabberConnection::Private::processDiscoItems(const IQ& iq)
{
	IQ reply(iq);
	reply.swapFromTo();
	reply.setType(IQ::Result);

	/* process disco-items to the service itself */
	if ( iq.childElement().attribute("node").isEmpty() || iq.childElement().attribute("node") == NS_QUERY_ADHOC ) {
		DiscoItems items;

		QHashIterator<QString,DiscoItem> ci(commands);
		while ( ci.hasNext() ) {
			ci.next();
			items << ci.value();
		}
		items.pushToDomElement( reply.childElement() );
	}

	stream->sendStanza(reply);
}

void JabberConnection::Private::processRegisterRequest(const IQ& iq)
{
	Registration regForm(iq);

	regForm.swapFromTo();
	regForm.setType(IQ::Result);

	regForm.setField(Registration::Instructions, QString("Enter UIN and password"));
	regForm.setField(Registration::Username);
	regForm.setField(Registration::Password);

	stream->sendStanza(regForm);
}

void JabberConnection::Private::processRegisterForm(const Registration& iq)
{
	if ( iq.to() != jid ) {
		Registration err(iq);
		err.swapFromTo();
		err.setError( Stanza::Error(Stanza::Error::FeatureNotImplemented) );
		stream->sendStanza(err);
		return;
	}
	if ( iq.hasField(Registration::Remove) ) {
		if ( iq.fields().size() > 1 ) {
			/* error, <remove/> is not the only child element */
			Registration err(iq);
			err.swapFromTo();
			err.setError( Stanza::Error(Stanza::Error::BadRequest) );
			stream->sendStanza(err);
			return;
		}
		if ( iq.from().isEmpty() ) {
			Registration err(iq);
			err.swapFromTo();
			err.setError( Stanza::Error(Stanza::Error::UnexpectedRequest) );
			stream->sendStanza(err);
			return;
		}
		Registration reply(iq);
		reply.swapFromTo();
		reply.clearChild();
		reply.setType(IQ::Result);
		stream->sendStanza(reply);

		Presence removeSubscription;
		removeSubscription.setTo( iq.from().bare() );
		removeSubscription.setType(Presence::Unsubscribe);
		stream->sendStanza(removeSubscription);

		Presence removeAuth;
		removeAuth.setTo( iq.from().bare() );
		removeAuth.setType(Presence::Unsubscribed);
		stream->sendStanza(removeAuth);

		Presence logout;
		logout.setTo( iq.from().bare() );
		logout.setType(Presence::Unavailable);
		stream->sendStanza(logout);

		/* send unregister signal, slot should remove the user from the database */
		emit q->userUnregistered( iq.from().bare() );
		return;
	}
	if ( iq.getField(Registration::Username).isEmpty() || iq.getField(Registration::Password).isEmpty() ) {
		Registration err(iq);
		err.swapFromTo();
		err.setError( Stanza::Error(Stanza::Error::NotAcceptable) );
		stream->sendStanza(err);
		return;
	}

	/* registration success */
	IQ reply(iq);
	reply.swapFromTo();
	reply.clearChild();
	reply.setType(IQ::Result);
	stream->sendStanza(reply);

	/* subscribe for user presence */
	Presence subscribe;
	subscribe.setFrom(jid);
	subscribe.setTo( iq.from().bare() );
	subscribe.setType(Presence::Subscribe);
	stream->sendStanza(subscribe);

	emit q->userRegistered( iq.from().bare(), iq.getField(Registration::Username), iq.getField(Registration::Password) );

	Presence presence;
	presence.setFrom(jid);
	presence.setTo( iq.from().bare() );
	stream->sendStanza(presence);

	/* execute log-in case */
	emit q->userOnline(iq.from().bare(), Presence::None);
}

void JabberConnection::Private::processPromptRequest(const IQ& iq)
{
	IQ prompt(iq);
	prompt.swapFromTo();
	prompt.setType(IQ::Result);

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

	IQ reply(iq);
	reply.swapFromTo();
	reply.setType(IQ::Result);

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

void JabberConnection::stream_iq(const IQ& iq)
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
		if ( iq.childElement().namespaceURI() == NS_IQ_REGISTER ) {
			d->processRegisterRequest(iq);
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

		IQ reply(iq);
		reply.swapFromTo();
		reply.setType(IQ::Result);
		d->vcard.toIQ(reply);

		d->stream->sendStanza(reply);
		return;
	}
	if ( iq.childElement().tagName() == "query" && iq.type() == "set" ) {
		if ( iq.childElement().namespaceURI() == NS_IQ_REGISTER ) {
			d->processRegisterForm(iq);
			return;
		}
		if ( iq.childElement().namespaceURI() == NS_IQ_GATEWAY ) {
			d->processPrompt(iq);
			return;
		}
	}
	if ( iq.childElement().tagName() == "command" && iq.type() == "set" && iq.childElement().namespaceURI() == NS_QUERY_ADHOC ) {
		d->processAdHoc(iq);
		return;
	}

	if ( iq.type() == "error" ) {
		/* TODO: Error logging? */
		return;
	}

	qDebug() << "[JC]" << "unhandled IQ type" << iq.type() << "tag" << iq.childElement().tagName() << "nsuri" << iq.childElement().namespaceURI();
}

void JabberConnection::stream_message(const Message& msg)
{
	qDebug() << "message from" << msg.from() << "to" << msg.to() << "subject" << msg.subject();
	if ( msg.to().domain() != d->jid.domain() ) {
		qDebug() << "We shouldn't receive this message";
		return;
	}
	/* message for legacy user */
	if ( !msg.to().node().isEmpty() ) {
		emit outgoingMessage( msg.from(), msg.to().node(), msg.body() );
	}
}

void JabberConnection::stream_presence(const Presence& presence)
{
	/* approve subscription to gateway */
	if (presence.type() == Presence::Subscribe && presence.to() == d->jid) {
		Presence approve;
		approve.setType(Presence::Subscribed);
		approve.setTo( presence.from() );
		approve.setFrom(d->jid);

		d->stream->sendStanza(approve);
		return;
	}
	if ( presence.type() == Presence::Subscribe && !presence.to().node().isEmpty() ) {
		emit userAdd( presence.from(), presence.to().node() );
	}
	if ( presence.type() == Presence::Unsubscribe && !presence.to().node().isEmpty() ) {
		qDebug() << "[JC]" << "User" << presence.from() << "requested contact delete of" << presence.to().node();
		emit userDel( presence.from(), presence.to().node() );
	}
	if ( presence.type() == Presence::Unsubscribed && !presence.to().node().isEmpty() ) {
		emit userAuthDeny( presence.from(), presence.to().node() );
	}
	if ( presence.type() == Presence::Subscribed && !presence.to().node().isEmpty() ) {
		emit userAuthGrant( presence.from(), presence.to().node() );
	}

	if ( presence.type() == Presence::Available ) {
		emit userOnline( presence.from(), presence.show() );
		return;
	}
	if ( presence.type() == Presence::Unavailable ) {
		emit userOffline( presence.from() );
		return;
	}
}

void JabberConnection::stream_connected()
{
	qDebug() << "[JC] Component signed on";
}

void JabberConnection::stream_error(const ComponentStream::Error& err)
{
	qDebug() << "[JC] Stream error! Condition:" << err.conditionString();
	qApp->quit();
}
