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

namespace XMPP {

/**
 * @class Message
 * @brief Represents XMPP Message stanza object.
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
 * Constructs a deep copy of @a other message stanza.
 */
Message::Message(const Message& other)
	: Stanza(other)
{
}

/**
 * Constructs message stanza from a DOM document. Constructor makes a deep copy of QDomDocument.
 *
 * @param document	DOM document
 */
Message::Message(const QDomDocument& document)
	: Stanza(document)
{
}

/**
 * Constructs message stanza from a DOM element setting @a element as root-element.
 * Constructor makes a deep copy of QDomElement.
 */
Message::Message(const QDomElement& element)
	: Stanza(element)
{
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
 * @sa subject(), thread()
 */
QString Message::body() const
{
	return doc()->documentElement().firstChildElement("body").text();
}

/**
 * Get message stanza subject.
 *
 * @return message subject string
 * @sa body(), thread()
 */
QString Message::subject() const
{
	return doc()->documentElement().firstChildElement("subject").text();
}

/**
 * Get message thread.
 *
 * @return	message thread string
 * @sa body(), subject()
 */
QString Message::thread() const
{
	return doc()->documentElement().firstChildElement("thread").text();
}

/**
 * Sets message stanza text to @a body.
 *
 * @sa setSubject(), setThread()
 */
void Message::setBody(const QString& body)
{
	setProperty("body", body);
}

/**
 * Sets message stanza subject to @a subject.
 *
 * @sa setBody(), setThread()
 */
void Message::setSubject(const QString& subject)
{
	setProperty("subject", subject);
}

/**
 * Sets message thread to @a thread.
 *
 * @sa setBody(), setSubject()
 */
void Message::setThread(const QString& thread)
{
	setProperty("thread", thread);
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

} /* end of namespace XMPP */
