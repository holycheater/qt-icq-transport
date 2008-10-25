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

#include "xmpp-ext/AdHoc.h"
#include "xmpp-ext/DataForm.h"
#include "xmpp-ext/ServiceDiscovery.h"
#include "xmpp-ext/Registration.h"
#include "xmpp-ext/vCard.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QStringList>
#include <QSqlQuery>
#include <QTextCodec>
#include <QUrl>
#include <QVariant>
#include <qmath.h>

#include <QtDebug>

using namespace XMPP;

#define NS_IQ_GATEWAY "jabber:iq:gateway"

static const int SEC_MINUTE = 60;
static const int SEC_HOUR   = 3600;
static const int SEC_DAY    = 86400;
static const int SEC_WEEK   = 604800;

class JabberConnection::Private {

	public:
		bool checkRegistration(const Jid& user);
		QString getUserSetting(const Jid& user, const QString& setting);

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

		QDateTime startTime;

		/* list of adhoc commands */
		QHash<QString,DiscoItem> commands;
};

bool JabberConnection::Private::checkRegistration(const Jid& user)
{
	QSqlQuery query;
	query.exec( QString("SELECT jid FROM users WHERE jid = '?'").replace("?", user.bare()) );
	return query.first();
}

QString JabberConnection::Private::getUserSetting(const Jid& user, const QString& setting)
{
	QSqlQuery query;
	query.exec( QString("SELECT value FROM options WHERE jid = '_jid' AND option = '_option' ").replace("_jid", user.bare()).replace("_option",setting) );
	if ( !query.first() ) {
		return QString();
	}
	return query.value(0).toString();
}

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
	d->vcard.setUrl( QUrl("http://github.com/holycheater/qt-icq-transport") );

	d->commands.insert( "fetch-contacts", DiscoItem("icq.dragonfly", "fetch-contacts", "Fetch ICQ contacts") );
	d->commands.insert( "cmd-uptime",     DiscoItem("icq.dragonfly", "cmd-uptime",     "Report service uptime") );
	d->commands.insert( "set-options",    DiscoItem("icq.dragonfly", "set-options",    "Set service parameters") );

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

void JabberConnection::sendPresenceProbe(const Jid& user)
{
	Presence presence;
	presence.setFrom(d->jid);
	presence.setTo(user);
	presence.setType(Presence::Probe);

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
	AdHoc cmd = AdHoc::fromIQ(iq);
	qDebug() << "[JC]" << "Adhoc command from" << iq.from() << "command" << cmd.node();

	if ( cmd.action() == AdHoc::Cancel ) {
		qDebug() << "[JC]" << "Command" << cmd.node() << "from" << iq.from() << "was canceled";

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
		if ( !checkRegistration(iq.from()) ) {
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
		if ( !checkRegistration(iq.from()) ) {
			IQ err = IQ::createReply(iq);
			err.setError(Stanza::Error::NotAuthorized);

			stream->sendStanza(err);
			return;
		}

		if ( iq.childElement().firstChildElement("x").attribute("type") == "submit" ) {
			DataForm form = DataForm::fromDomElement( iq.childElement().firstChildElement("x") );
			DataForm::Field fai = form.fieldByName("auto-invite");
			QString auto_invite = fai.values().at(0);
			if ( auto_invite == "true" || auto_invite == "1" ) {
				QSqlQuery( QString("REPLACE INTO options (jid,option,value) VALUES('_jid','auto-invite','enabled')").replace("_jid", iq.from().bare()) ).exec();
			} else {
				QSqlQuery( QString("DELETE FROM options WHERE jid = '_jid' AND option='auto-invite'").replace("_jid", iq.from().bare()) ).exec();
			}
			QString auto_reconnect = form.fieldByName("auto-reconnect").values().at(0);
			if ( auto_reconnect == "true" || auto_invite == "1" ) {
				QSqlQuery( QString("REPLACE INTO options (jid,option,value) VALUES('_jid','auto-reconnect','enabled')").replace("_jid", iq.from().bare()) ).exec();
			} else {
				QSqlQuery( QString("DELETE FROM options WHERE jid = '_jid' AND option='auto-reconnect'").replace("_jid", iq.from().bare()) ).exec();
			}
			QString encoding = form.fieldByName("encoding").values().at(0);
			QSqlQuery( QString("REPLACE INTO options (jid,option,value) VALUES('_jid','encoding','_encoding')").replace("_jid", iq.from().bare()).replace("_encoding", encoding) ).exec();
		} else {
			cmd.setStatus(AdHoc::Executing);
			cmd.setSessionID( "set-options:"+QDateTime::currentDateTime().toString(Qt::ISODate) );

			DataForm form;
			form.setTitle("Service configuration");
			form.setInstructions("Please configure your settings");

			DataForm::Field fldAutoInvite("auto-invite", "Auto-Invite", DataForm::Field::Boolean);
			if ( getUserSetting(iq.from(),"auto-invite") == "enabled" ) {
				fldAutoInvite.addValue("true");
			}
			form.addField(fldAutoInvite);

			DataForm::Field fldAutoReconnect("auto-reconnect", "Automatically reconnect", DataForm::Field::Boolean);
			if ( getUserSetting(iq.from(),"auto-reconnect") == "enabled" ) {
				fldAutoReconnect.addValue("true");
			}
			form.addField(fldAutoReconnect);

			DataForm::Field fldEncoding("encoding", "Codepage", DataForm::Field::ListSingle);

			QListIterator<QByteArray> ci( QTextCodec::availableCodecs() );
			while ( ci.hasNext() ) {
				QString enc = ci.next();
				fldEncoding.addOption(enc,enc);
			}
			QString userEncoding = getUserSetting(iq.from(), "encoding");
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
	qDebug() << "disco-info query from" << iq.from().full() << "to" << iq.to().full();

	/* disco-info to command-node query handling */
	QString node = iq.childElement().attribute("node");
	if ( !node.isEmpty() & commands.contains(node) ) {
		qDebug() << "[JC]" << "disco-info to command node: " << node;

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
	Registration regForm = IQ::createReply(iq);

	regForm.setField(Registration::Instructions, QString("Enter UIN and password"));
	regForm.setField(Registration::Username);
	regForm.setField(Registration::Password);

	stream->sendStanza(regForm);
}

void JabberConnection::Private::processRegisterForm(const Registration& iq)
{
	if ( iq.to() != jid ) {
		Registration err = IQ::createReply(iq);
		err.setError( Stanza::Error(Stanza::Error::FeatureNotImplemented) );
		stream->sendStanza(err);
		return;
	}
	if ( iq.hasField(Registration::Remove) ) {
		if ( iq.fields().size() > 1 ) {
			/* error, <remove/> is not the only child element */
			Registration err = IQ::createReply(iq);
			err.setError( Stanza::Error(Stanza::Error::BadRequest) );
			stream->sendStanza(err);
			return;
		}
		if ( iq.from().isEmpty() ) {
			Registration err = IQ::createReply(iq);
			err.setError( Stanza::Error(Stanza::Error::UnexpectedRequest) );
			stream->sendStanza(err);
			return;
		}
		Registration reply = IQ::createReply(iq);
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
	IQ reply = IQ::createReply(iq);
	reply.clearChild();
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
	emit q->userOnline(iq.from().bare(), Presence::None, true);
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

		IQ reply = IQ::createReply(iq);
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
		emit userOnlineStatusRequest( presence.from() );
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
		emit userOnline(presence.from(), presence.show(), false);
		return;
	}
	if ( presence.type() == Presence::Unavailable ) {
		emit userOffline( presence.from() );
		return;
	}
}

void JabberConnection::stream_connected()
{
	d->startTime = QDateTime::currentDateTime();
	qDebug() << "[JC] Component signed on";
}

void JabberConnection::stream_error(const ComponentStream::Error& err)
{
	qDebug() << "[JC] Stream error! Condition:" << err.conditionString();
	exit(1);
}
