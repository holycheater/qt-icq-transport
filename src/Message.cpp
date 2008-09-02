/*
 * Message.h - XMPP Message Stanza
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

#include "Message.h"

namespace X
{


/**
 * @class Message
 * @brief Represents Message stanza object.
 */


/**
 * Constructs message stanza.
 */
Message::Message()
	: Stanza()
{
	setType(Normal);
	QDomElement element = doc()->createElement("message");
	doc()->appendChild(element);
}

/**
 * Destroys message stanza.
 */
Message::~Message()
{
}

/**
 * Get message stanza body (message itself).
 *
 * @return message body string
 * @sa subject()
 */
QString Message::body() const
{
	if ( doc()->documentElement().elementsByTagName("body").isEmpty() ) {
		return QString();
	} else {
		QDomElement eBody = doc()->documentElement().elementsByTagName("body").item(0).toElement();
		return eBody.text();
	}
}

/**
 * Get message stanza subject.
 *
 * @return message subject string
 * @sa body()
 */
QString Message::subject() const
{
	if ( doc()->documentElement().elementsByTagName("subject").isEmpty() ) {
		return QString();
	} else {
		QDomElement eSubject = doc()->documentElement().elementsByTagName("subject").item(0).toElement();
		return eSubject.text();
	}
}

/**
 * Sets message stanza subject to @a subject.
 *
 * @sa setBody()
 */
void Message::setSubject(const QString& subject)
{
	QDomElement eSubject;
	if ( !doc()->documentElement().elementsByTagName("subject").isEmpty() ) {
		eSubject = doc()->documentElement().elementsByTagName("subject").item(0).toElement();
		doc()->documentElement().removeChild(eSubject);
	}
	eSubject = doc()->createElement("subject");
	doc()->documentElement().appendChild(eSubject);

	QDomText text = doc()->createTextNode(subject);
	eSubject.appendChild(text);
}

/**
 * Sets message stanza text to @a body.
 *
 * @sa setSubject()
 */
void Message::setBody(const QString& body)
{
	QDomElement eBody;
	if ( !doc()->documentElement().elementsByTagName("body").isEmpty() ) {
		eBody = doc()->documentElement().elementsByTagName("body").item(0).toElement();
		doc()->documentElement().removeChild(eBody);
	}
	eBody = doc()->createElement("body");
	doc()->documentElement().appendChild(eBody);

	QDomText text = doc()->createTextNode(body);
	eBody.appendChild(text);
}

/**
 * Sets message type to @a type.
 *
 * @overload
 * @sa Type
 */
void Message::setType(Type type)
{
	setType( typeToString(type) );
}

/**
 * Converts string type representation of @a type to @link #Type Type.
 * @param type	string representation of IQ stanza type
 * @return		Type enum value of the given type.
 */
QString Message::typeToString(Type type)
{
	switch (type) {
		case Normal:
			return "normal";
			break;
		case Chat:
			return "chat";
			break;
		case GroupChat:
			return "groupchat";
			break;
		case Headline:
			return "headline";
			break;
		case Error:
			return "error";
			break;
		default:
			return QString();
			break;
	}
}

/**
 * @enum Message::Type
 * @brief Message stanza type. The types are:
 *
 * @li Normal		(Default) Normal text message used in e-mail like interfaces.
 * @li Chat			Typical message used in chat interfaces.
 * @li GroupChat	Chat message sent to group-chat server for group chats.
 * @li Headline		Text message to be displayed in scrolling marquee displays.
 * @li Error		Indicates messaging error.
 */

}
