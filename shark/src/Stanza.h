/*
 * Stanza.h - XMPP Stanza
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

#ifndef XMPP_STANZA_H_
#define XMPP_STANZA_H_

#include <QDomDocument>

namespace XMPP {

class Jid;


class Stanza
{
	public:
		Stanza();
		Stanza(const Stanza& other);
		Stanza(const QDomDocument& document);
		Stanza(const QDomElement& element);
		virtual ~Stanza();

		Stanza& operator=(const Stanza& other);

		bool isValid() const;

		Jid to() const;
		Jid from() const;
		QString type() const;
		QString id() const;

		void setTo(const Jid& toJid);
		void setFrom(const Jid& fromJid);
		void setType(const QString& type);
		void setId(const QString& id);

		QString toString() const;
	protected:
		QDomDocument* doc();
		QDomDocument* doc() const;
		void setProperty(const QString& name, const QString& value);
	private:
		QDomDocument m_doc;
};


}

#endif /* XMPP_STANZA_H_ */
