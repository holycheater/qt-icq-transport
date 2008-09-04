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

#ifndef X_MESSAGE_H_
#define X_MESSAGE_H_

#include "Stanza.h"

namespace X
{


class Message : public Stanza
{
	public:
		enum Type { Normal, Chat, GroupChat, Headline, Error };

		Message();
		Message(const Message& other);
		Message(const QDomDocument& document);
		Message(const QDomElement& element);
		~Message();

		QString body() const;
		QString subject() const;

		void setBody(const QString& body);
		void setSubject(const QString& subject);

		using Stanza::setType;
		void setType(Type type);
	private:
		static QString typeToString(Type type);
};


}

#endif /* X_MESSAGE_H_ */
