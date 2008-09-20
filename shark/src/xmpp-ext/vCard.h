/*
 * vCard.h - vcard-temp (XEP-0054)
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

#ifndef XMPP_EXT_VCARD_H_
#define XMPP_EXT_VCARD_H_

#include <QSharedDataPointer>

class QDate;
class QDomElement;
class QUrl;

#define NS_VCARD_TEMP "vcard-temp"

namespace XMPP {

class Jid;
class IQ;


class vCard
{
	public:
		vCard();
		vCard(const vCard& other);
		virtual ~vCard();

		static vCard fromIQ(const IQ& iq);
		IQ toIQ() const;
		void toIQ(IQ& iq) const;
		void toDomElement(QDomElement& element) const;

		operator IQ() const;

		bool isEmpty() const;

		QString fullName() const;

		QString givenName() const;
		QString middleName() const;
		QString familyName() const;
		QString namePrefix() const;
		QString nameSuffix() const;

		QString nickname() const;

		QDate birthday() const;

		QString addrCountry() const;
		QString addrRegion() const;
		QString addrPostal() const;
		QString addrCity() const;
		QString addrStreet() const;

		QString phone() const;
		QString email() const;

		QString timezone() const;

		QString geoLatitude() const;
		QString geoLongitude() const;

		QString title() const;
		QString role() const;
		QString orgName() const;
		QString orgUnit() const;

		QUrl url() const;

		QString description() const;

		Jid jid() const;

		void setFullName(const QString& fn);

		void setGivenName(const QString& given);
		void setMiddleName(const QString& middle);
		void setFamilyName(const QString& family);
		void setNamePrefix(const QString& prefix);
		void setNameSuffix(const QString& suffix);

		void setNickname(const QString& nick);

		void setBirthday(const QDate& birthdate);

		void setAddrCountry(const QString& country);
		void setAddrRegion(const QString& region);
		void setAddrCity(const QString& city);
		void setAddrPostal(const QString& postal);
		void setAddrStreet(const QString& street);
		void setAddress(const QString& country, const QString& region, const QString& city, const QString& postal, const QString& street);

		void setPhone(const QString& number);
		void setEmail(const QString& email);

		void setTimezone(const QString& tz);

		void setGeo(const QString& lat, const QString& lon);

		void setTitle(const QString& title);
		void setRole(const QString& role);
		void setOrgName(const QString& name);
		void setOrgUnit(const QString& unit);

		void setUrl(const QUrl& url);

		void setDescription(const QString& desc);

		void setJid(const Jid& jid);
	private:
		class Private;
		QSharedDataPointer<Private> d;
};


} /* end of namespace XMPP */

#endif /* XMPP_EXT_VCARD_H_ */
