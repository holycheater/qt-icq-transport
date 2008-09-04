/*
 * Presence.h - XMPP Presence Stanza
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

#ifndef X_PRESENCE_H_
#define X_PRESENCE_H_

#include "Stanza.h"

namespace X
{


class Presence : public Stanza
{
	public:
		enum Type { Unavailable, Subscribe, Subscribed, Unsubscribe, Unsubscribed, Probe, Error };
		enum Show { Chat, Away, NotAvailable, DoNotDisturb };

		Presence();
		Presence(const QDomDocument& document);
		Presence(const QDomElement& element);
		~Presence();

		int priority() const;
		QString show() const;
		QString status() const;

		void setPriority(int priority);
		void setShow(Show showState);
		void setStatus(const QString& status);
		using Stanza::setType;
		void setType(Type type);
	private:
		static QString typeToString(Type type);
};

}

#endif /* X_PRESENCE_H_ */
