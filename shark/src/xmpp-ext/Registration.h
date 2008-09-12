/*
 * Registration.h - In-Band Registration (XEP-0077)
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

#ifndef XMPP_EXT_REGISTRATION_H_
#define XMPP_EXT_REGISTRATION_H_

#define NS_IQ_REGISTER "jabber:iq:register"

#include "xmpp-core/IQ.h"

namespace XMPP {


class Registration : public IQ
{
	public:
		enum Field { Instructions, Username, Nick, Password, Name, First, Last, Email, Address, City, State, Zip, Phone, Url, Date, Misc, Text };
		Registration();
		Registration(const IQ& other);
		Registration(const Registration& other);
		Registration(const QDomElement& element);
		~Registration();

		QList<Field> fields() const;
		QString getField(Field name) const;
		bool hasField(Field name) const;
		void setField(Field name, const QString& value);
		void setField(Field name, bool present = true);

		void setRegistered(bool present = true);
		void setRemove(bool present = true);
	private:
		class Private;
};


} /* end of namespace XMPP */

#endif /* XMPP_EXT_REGISTRATION_H_ */
