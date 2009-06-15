/*
 * icqShortUserDetails.h - ICQ Short User details.
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

#ifndef ICQ_TYPES_SHORT_USER_DETAILS_H_
#define ICQ_TYPES_SHORT_USER_DETAILS_H_

#include <QSharedDataPointer>

class QString;

namespace ICQ
{

class ShortUserDetails
{
	public:
		ShortUserDetails();
		ShortUserDetails(const ShortUserDetails& other);
		virtual ~ShortUserDetails();
		ShortUserDetails& operator=(const ShortUserDetails& other);

		QString uin() const;
		QString nick() const;
		QString firstName() const;
		QString lastName() const;
		QString email() const;

		void setUin(const QString& uin);
		void setNick(const QString& nick);
		void setFirstName(const QString& firstName);
		void setLastName(const QString& lastName);
		void setEmail(const QString& email);

		bool isEmpty();
	private:
		class Private;
		QSharedDataPointer<Private> d;
};


} /* end of namespace ICQ */

// vim:ts=4:sw=4:noet:nowrap
#endif /* ICQ_TYPES_SHORT_USER_DETAILS_H_ */
