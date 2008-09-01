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

#ifndef X_STANZA_H_
#define X_STANZA_H_

#include <QSharedDataPointer>

class QDomDocument;

namespace XMPP {
	class Jid;
}

namespace X
{


class Stanza
{
	public:
		Stanza();
		Stanza(const Stanza& other);
		virtual ~Stanza();

		Stanza& operator=(const Stanza& other);

		bool isValid() const;

		XMPP::Jid to() const;
		XMPP::Jid from() const;
		QString type() const;
		QString id() const;

		void setTo(const XMPP::Jid& toJid);
		void setFrom(const XMPP::Jid& fromJid);
		void setType(const QString& type);
		void setId(const QString& id);

		QString toString() const;
	protected:
		QDomDocument* doc();
	private:
		class Private;
		QSharedDataPointer<Private> d;
};


}

#endif /* X_STANZA_H_ */
