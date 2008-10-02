/*
 * Presence.cpp - XMPP Presence Stanza
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

#include "Presence.h"

namespace XMPP {

/**
 * @class Presence
 * @brief Represents XMPP presence stanza object.
 */


/**
 * Constructs XMPP 'presence' stanza.
 */
Presence::Presence()
	: Stanza()
{
	QDomElement element = doc()->createElement("presence");
	doc()->appendChild(element);
}

/**
 * Constructs a deep copy of @a other presence stanza.
 */
Presence::Presence(const Presence& other)
	: Stanza(other)
{
}

/**
 * Constructs presence stanza from a DOM document. Constructor makes a deep copy of QDomDocument.
 *
 * @param document	DOM document
 */
Presence::Presence(const QDomDocument& document)
	: Stanza(document)
{
}

/**
 * Constructs presence stanza from a DOM element setting @a element as root-element.
 * Constructor makes a deep copy of QDomElement.
 */
Presence::Presence(const QDomElement& element)
	: Stanza(element)
{
}

/**
 * Destroys presence stanza.
 */
Presence::~Presence()
{
}

/**
 * Returns priority. Valid range is from -128 to 128. If no priority is set it returns 0.
 */
int Presence::priority() const
{
	QDomElement element = doc()->documentElement().firstChildElement("priority");
	if ( element.isNull() ) {
		return 0;
	}
	return element.text().toInt();
}

Presence::Show Presence::show() const
{
	return stringToShow( doc()->documentElement().firstChildElement("show").text() );
}

/**
 * Returns presence status text.
 */
QString Presence::status() const
{
	return doc()->documentElement().firstChildElement("show").text();
}

Presence::Type Presence::type() const
{
	return stringToType( doc()->documentElement().attribute("type") );
}

/**
 * Sets presence priority to @a priority. Valid range is from -128 to 128
 */
void Presence::setPriority(int priority)
{
	if (priority < -128) {
		priority = -128;
	} else if (priority > 128) {
		priority = 128;
	}

	setProperty( "priority", QString::number(priority) );
}

/**
 * Sets presence "show" value to @a showState which specifies particulary availability status.
 * The show value can only be set if presence type is "available" (no presence type set).
 *
 * @param showState		Presence show value
 * @sa Show
 */
void Presence::setShow(Show showState)
{
	QString str;
	switch (showState) {
		case Chat:
			str = "chat";
			break;
		case Away:
			str = "away";
			break;
		case NotAvailable:
			str = "xa";
			break;
		case DoNotDisturb:
			str = "dnd";
			break;
		default:
			return;
			break;
	}
	setProperty("show", str);
}

/**
 * Sets presence status text to @a status
 */
void Presence::setStatus(const QString& status)
{
	setProperty("status", status);
}

/**
 * Sets presence stanza type to @a type
 *
 * @sa Type
 */
void Presence::setType(Type type)
{
	setType( typeToString(type) );
}

/**
 * Returns string representation of @a type.
 *
 * @sa Type
 */
QString Presence::typeToString(Type type)
{
	switch (type) {
		case Unavailable:
			return "unavailable";
			break;
		case Subscribe:
			return "subscribe";
		case Subscribed:
			return "subscribed";
		case Unsubscribe:
			return "unsubscribe";
			break;
		case Unsubscribed:
			return "unsubscribed";
			break;
		case Probe:
			return "probe";
			break;
		case Error:
			return "error";
			break;
		default:
			return QString();
			break;
	}
}

Presence::Type Presence::stringToType(const QString& type)
{
	if ( type.isEmpty() ) {
		return Available;
	}
	if (type == "unavailable") {
		return Unavailable;
	}
	if (type == "subscribe") {
		return Subscribe;
	}
	if (type == "subscribed") {
		return Subscribed;
	}
	if (type == "unsubscribe") {
		return Unsubscribe;
	}
	if (type == "unsubscribed") {
		return Unsubscribed;
	}
	if (type == "probe") {
		return Probe;
	}
	if (type == "error") {
		return Error;
	}
	return Error;
}

Presence::Show Presence::stringToShow(const QString& show)
{
	if ( show.isEmpty() ) {
		return None;
	}
	if (show == "chat") {
		return Chat;
	}
	if (show == "away") {
		return Away;
	}
	if (show == "xa") {
		return NotAvailable;
	}
	if (show == "dnd") {
		return DoNotDisturb;
	}
	return None;
}

/**
 * @enum Presence::Type
 * Message stanza type.
 */

/**
 * @var Presence::Unavailable
 * Sender is unavailable.
 */

/**
 * @var Presence::Subscribe
 * Sender wishes to subscribe to recipient's presence.
 */

/**
 * @var Presence::Subscribed
 * Sender has allowed recipient to receive it's presence.
 */

/**
 * @var Presence::Unsubscribe
 * Sender is unsubscribing from another entity's presence.
 */

/**
 * @var Presence::Unsubscribed
 * Presence subscription has been denied or revoked.
 */

/**
 * @var Presence::Probe
 * Request for entity's current presence.
 * It SHOULD be generated only by server on behalf of a user.
 */

/**
 * @var Presence::Error
 * Error has occured during stanza processing or delivery
 */

/**
 * @enum Presence::Show
 * presence "show" type
 */

/**
 * @var Presence::Chat
 * Entity or resource is actively interested in chatting
 */

/**
 * @var Presence::Away
 * Entity or resource is temporarily away
 */

/**
 * @var Presence::NotAvailable
 * Entity or resource is away for an extended period
 */

/**
 * @var Presence::DoNotDisturb
 * Entity or resource is busy
 */

} /* end of namespace XMPP */
