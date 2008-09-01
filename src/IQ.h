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

#include <QSharedDataPointer>

namespace XMPP {

class Jid;


class IQ
{
	public:
		enum Type { Get, Set, Result, Error };

		IQ();
		IQ(const IQ& other);
		~IQ();

		IQ& operator=(const IQ& other);

		bool isValid() const;

		Jid to() const;
		Jid from() const;
		QString type() const;
		QString id() const;

		void setTo(const Jid& toJid);
		void setFrom(const Jid& fromJid);
		void setType(const QString& type);
		void setType(Type type);
		void setId(int id);
	private:
		static QString typeToString(int type);
		static int stringToType(const QString& type);

		class Private;
		QSharedDataPointer<Private> d;

};


} /* end of namespace XMPP */

#endif /* XMPP_STANZA_IQ_H_ */
