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

#include "Message.h"

#include <QDomDocument>

namespace X
{

/**
 * Constructs message stanza.
 */
Message::Message()
	: Stanza()
{
	setType(Normal);
}

/**
 * Destroys message stanza.
 */
Message::~Message()
{
}

/**
 * Sets message stanza subject to @a subject.
 *
 * @sa setBody()
 */
void Message::setSubject(const QString& subject)
{
	/* TODO: set message subject */
}

/**
 * Sets message stanza text to @a body.
 *
 * @sa setSubject()
 */
void Message::setBody(const QString& body)
{
	/* TODO: set message text */
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
 * Converts string type representation of @a type to @link enum-type.
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
 * @brief Message stanza type.
 *
 * @var Normal		(Default) Normal text message used in e-mail like interfaces.
 * @var Chat		Typical message used in chat interfaces.
 * @var GroupChat	Chat message sent to group-chat server for group chats.
 * @var Headline	Text message to be displayed in scrolling marquee displays.
 * @var Error		Indicates messaging error.
 */

}
