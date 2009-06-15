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

#ifndef XMPP_STANZA_MESSAGE_H_
#define XMPP_STANZA_MESSAGE_H_

#include "Stanza.h"

class QDateTime;

namespace XMPP {


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
		QString thread() const;

		void setBody(const QString& body);
		void setSubject(const QString& subject);
		void setThread(const QString& thread);

		/* XEP-0091: jabber:x:delay
		 * this XEP is deprecated, so the code is temporarily.
		 */
		QDateTime timestamp() const;
		void setTimestamp(const QDateTime& t);

		using Stanza::setType;
		void setType(Type type);
	private:
		static QString typeToString(Type type);
};


}

// vim:ts=4:sw=4:noet:nowrap
#endif /* XMPP_STANZA_MESSAGE_H_ */
