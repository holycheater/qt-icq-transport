/*
 * IQ.h - XMPP Info/Query stanza
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

#ifndef XMPP_STANZA_IQ_H_
#define XMPP_STANZA_IQ_H_

#include "Stanza.h"

namespace XMPP {


class IQ : public Stanza
{
	public:
		enum Type { Get, Set, Result, Error };

		IQ();
		IQ(const IQ& other);
		IQ(const QDomDocument& document);
		IQ(const QDomElement& element);
		~IQ();

		QDomElement& childElement();
		const QDomElement& childElement() const;

		void clearChild();

		void setChildElement(const QString& name, const QString& ns);

		using Stanza::setType;
		void setType(Type type);
	private:
		/* XEP-0172: User Nickname */
		QString nick() const;
		void setNick(const QString& nick);

		static QString typeToString(Type type);
		static int stringToType(const QString& type);

		static int m_id;

		QDomElement m_element;
};


} /* end of namespace XMPP */

#endif /* XMPP_STANZA_IQ_H_ */
