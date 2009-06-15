/*
 * icqUserDetails.h - ICQ User details.
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

#ifndef ICQ_TYPES_USER_DETAILS_H_
#define ICQ_TYPES_USER_DETAILS_H_

#include <QSharedDataPointer>

class QDate;
class QString;
class QStringList;

namespace ICQ
{


class UserDetails
{
	public:
		enum Gender { Male, Female, NoGender };

		UserDetails();
		UserDetails(const UserDetails& other);
		virtual ~UserDetails();
		UserDetails& operator=(const UserDetails& other);

		void clear();

		QString uin() const;

		QString nick() const;
		QString firstName() const;
		QString lastName() const;
		QString email() const;
		QString homeCity() const;
		QString homeState() const;
		QString homePhone() const;
		QString homeFax() const;
		QString homeAddress() const;
		QString cellPhone() const;
		QString homeZipCode() const;
		/* TODO: country-code */

		int age() const;
		Gender gender() const;
		QDate birthdate() const;
		/* TODO: lang1, lang2, lang3 */
		QString originalCity() const;
		QString originalState() const;
		/* TODO : country-code */

		QStringList emails() const;

		/* TODO: home-page category code */
		QString homepage() const;

		QString workCity() const;
		QString workState() const;
		QString workPhone() const;
		QString workFax() const;
		QString workAddress() const;
		QString workZipCode() const;
		/* TODO: work country-code */
		QString workCompany() const;
		QString workDepartment() const;
		QString workPosition() const;
		/* TODO: work occupation code */
		QString workWebpage() const;

		QString notes() const;

		/* TODO: interests and affiliations */

		void setUin(const QString& uin);

		void setNick(const QString& nick);
		void setFirstName(const QString& firstName);
		void setLastName(const QString& lastName);
		void setEmail(const QString& email);
		void setHomeCity(const QString& homeCity);
		void setHomeState(const QString& homeState);
		void setHomePhone(const QString& homePhone);
		void setHomeFax(const QString& homeFax);
		void setHomeAddress(const QString& homeAddress);
		void setCellPhone(const QString& cellPhone);
		void setHomeZipCode(const QString& homeZipCode);

		void setAge(int age);
		void setGender(Gender gender);
		void setBirthDate(const QDate& date);
		void setOriginalCity(const QString& city);
		void setOriginalState(const QString& state);

		void addEmail(const QString& email);
		void setEmails(const QStringList& emails);

		void setHomepage(const QString& homepage);

		void setWorkCity(const QString& workCity);
		void setWorkState(const QString& workState);
		void setWorkPhone(const QString& workPhone);
		void setWorkFax(const QString& workFax);
		void setWorkAddress(const QString& workAddress);
		void setWorkZipCode(const QString& workZipCode);
		void setWorkCompany(const QString& workCompany);
		void setWorkDepartment(const QString& workDepartment);
		void setWorkPosition(const QString& workPosition);
		void setWorkWebpage(const QString& workWebpage);

		void setNotes(const QString& notes);

		bool isEmpty();
	private:
		class Private;
		QSharedDataPointer<Private> d;
};


} /* end of namespace ICQ */

// vim:ts=4:sw=4:noet:nowrap
#endif /* ICQ_TYPES_USER_DETAILS_H_ */
