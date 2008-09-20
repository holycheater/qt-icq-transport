/*
 * vCard.cpp - vcard-temp (XEP-0054)
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

#include "vCard.h"
#include "xmpp-core/Jid.h"
#include "xmpp-core/IQ.h"

#include <QDate>
#include <QDomElement>
#include <QUrl>

namespace XMPP
{


class vCard::Private : public QSharedData
{
	public:
		Private();
		Private(const Private& other);
		~Private();

		void toDomElement(QDomElement& element) const;

		inline bool isEmpty() const;

		QString fullname;
		QString nGiven, nMiddle, nFamily, nPrefix, nSuffix;
		QString nickname;

		QDate birthday;

		QString aCountry, aRegion, aCity, aPostal, aStreet;
		QString phone;
		QString email;

		QString tzoffset;

		QString lat, lon;
		QString title, role, orgName, orgUnit;

		QUrl url;

		QString desc;

		Jid jid;
};

vCard::Private::Private()
	: QSharedData()
{
}
vCard::Private::Private(const Private& other)
	: QSharedData(other)
{
	fullname = other.fullname;

	nGiven = other.nGiven;
	nMiddle = other.nMiddle;
	nFamily = other.nFamily;
	nPrefix = other.nPrefix;
	nSuffix = other.nSuffix;

	nickname = other.nickname;

	birthday = other.birthday;

	aCountry = other.aCountry;
	aRegion = other.aRegion;
	aCity = other.aCity;
	aPostal = other.aPostal;
	aStreet = other.aStreet;

	phone = other.phone;
	email = other.email;

	tzoffset = other.tzoffset;

	lat = other.lat;
	lon = other.lon;

	title = other.title;
	role = other.role;
	orgName = other.orgName;
	orgUnit = other.orgUnit;

	url = other.url;

	desc = other.desc;

	jid = other.jid;
}

vCard::Private::~Private()
{
}

void vCard::Private::toDomElement(QDomElement& element) const
{
	element.setAttribute("version", "2.0");
	element.setAttribute("prodid", "-//HandGen//NONSGML vGen v1.0//EN");

	if ( !fullname.isEmpty() ) {
		QDomElement fn = element.ownerDocument().createElement("FN");
		QDomText fnText = element.ownerDocument().createTextNode(fullname);

		element.appendChild(fn);
		fn.appendChild(fnText);
	}

	if ( !( nGiven.isEmpty() && nMiddle.isEmpty() && nFamily.isEmpty() && nPrefix.isEmpty() && nSuffix.isEmpty() ) ) {
		QDomElement eName = element.ownerDocument().createElement("N");
		element.appendChild(eName);

		if ( !nGiven.isEmpty() ) {
			QDomElement e = element.ownerDocument().createElement("GIVEN");
			QDomText text = element.ownerDocument().createTextNode(nGiven);

			eName.appendChild(e);
			e.appendChild(text);
		}

		if ( !nMiddle.isEmpty() ) {
			QDomElement e = element.ownerDocument().createElement("MIDDLE");
			QDomText text = element.ownerDocument().createTextNode(nMiddle);

			eName.appendChild(e);
			e.appendChild(text);
		}

		if ( !nFamily.isEmpty() ) {
			QDomElement e = element.ownerDocument().createElement("FAMILY");
			QDomText text = element.ownerDocument().createTextNode(nFamily);

			eName.appendChild(e);
			e.appendChild(text);
		}

		if ( !nPrefix.isEmpty() ) {
			QDomElement e = element.ownerDocument().createElement("PREFIX");
			QDomText text = element.ownerDocument().createTextNode(nPrefix);

			eName.appendChild(e);
			e.appendChild(text);
		}

		if ( !nSuffix.isEmpty() ) {
			QDomElement e = element.ownerDocument().createElement("SUFFIX");
			QDomText text = element.ownerDocument().createTextNode(nSuffix);

			eName.appendChild(e);
			e.appendChild(text);
		}
	}

	if ( !nickname.isEmpty() ) {
		QDomElement e = element.ownerDocument().createElement("NICKNAME");
		QDomText text = element.ownerDocument().createTextNode(nickname);

		element.appendChild(e);
		e.appendChild(text);
	}

	if ( birthday.isValid() ) {
		QDomElement e = element.ownerDocument().createElement("BDAY");
		QDomText text = element.ownerDocument().createTextNode( birthday.toString(Qt::ISODate) );

		element.appendChild(e);
		e.appendChild(text);
	}

	if ( !( aCountry.isEmpty() && aRegion.isEmpty() && aPostal.isEmpty() && aCity.isEmpty() && aStreet.isEmpty() ) ) {
		QDomElement eAddr = element.ownerDocument().createElement("ADR");
		element.appendChild(eAddr);
		eAddr.appendChild( eAddr.ownerDocument().createElement("HOME") );

		if ( !aCountry.isEmpty() ) {
			QDomElement e = element.ownerDocument().createElement("CTRY");
			QDomText text = element.ownerDocument().createTextNode(aCountry);

			eAddr.appendChild(e);
			e.appendChild(text);
		}

		if ( !aRegion.isEmpty() ) {
			QDomElement e = element.ownerDocument().createElement("REGION");
			QDomText text = element.ownerDocument().createTextNode(aRegion);

			eAddr.appendChild(e);
			e.appendChild(text);
		}

		if ( !aPostal.isEmpty() ) {
			QDomElement e = element.ownerDocument().createElement("PCODE");
			QDomText text = element.ownerDocument().createTextNode(aPostal);

			eAddr.appendChild(e);
			e.appendChild(text);
		}

		if ( !aCity.isEmpty() ) {
			QDomElement e = element.ownerDocument().createElement("LOCALITY");
			QDomText text = element.ownerDocument().createTextNode(aCity);

			eAddr.appendChild(e);
			e.appendChild(text);
		}

		if ( !aStreet.isEmpty() ) {
			QDomElement e = element.ownerDocument().createElement("STREET");
			QDomText text = element.ownerDocument().createTextNode(aStreet);

			eAddr.appendChild(e);
			e.appendChild(text);
		}
	}

	if ( !phone.isEmpty() ) {
		QDomElement e = element.ownerDocument().createElement("TEL");
		element.appendChild(e);

		e.appendChild( e.ownerDocument().createElement("HOME") );
		e.appendChild( e.ownerDocument().createElement("VOICE") );

		QDomElement eNumber = e.ownerDocument().createElement("NUMBER");
		QDomText text = e.ownerDocument().createTextNode(phone);
		eNumber.appendChild(text);
	}

	if ( !email.isEmpty() ) {
		QDomElement e = element.ownerDocument().createElement("EMAIL");

		e.appendChild( e.ownerDocument().createElement("INTERNET") );

		QDomElement eUserID = e.ownerDocument().createElement("USERID");
		QDomText text = element.ownerDocument().createTextNode(email);
		eUserID.appendChild(text);

		element.appendChild(e);
	}

	if ( !tzoffset.isEmpty() ) {
		QDomElement e = element.ownerDocument().createElement("TZ");
		QDomText text = element.ownerDocument().createTextNode(tzoffset);

		element.appendChild(e);
		e.appendChild(text);
	}

	if ( !lat.isEmpty() && !lon.isEmpty() ) {
		QDomElement eGeo = element.ownerDocument().createElement("GEO");

		QDomElement eLat = eGeo.ownerDocument().createElement("LAT");
		QDomElement eLon = eGeo.ownerDocument().createElement("LON");

		QDomText tLat = eGeo.ownerDocument().createTextNode(lat);
		QDomText tLon = eGeo.ownerDocument().createTextNode(lon);

		eLat.appendChild(tLat);
		eLon.appendChild(tLon);

		eGeo.appendChild(eLat);
		eGeo.appendChild(eLon);
	}

	if ( !title.isEmpty() ) {
		QDomElement e = element.ownerDocument().createElement("TITLE");
		QDomText text = element.ownerDocument().createTextNode(title);

		element.appendChild(e);
		e.appendChild(text);
	}

	if ( !role.isEmpty() ) {
		QDomElement e = element.ownerDocument().createElement("ROLE");
		QDomText text = element.ownerDocument().createTextNode(role);

		element.appendChild(e);
		e.appendChild(text);
	}

	if ( !( orgName.isEmpty() && orgUnit.isEmpty() ) ) {
		QDomElement eOrg = element.ownerDocument().createElement("ORG");
		element.appendChild(eOrg);

		if ( !orgName.isEmpty() ) {
			QDomElement e = element.ownerDocument().createElement("ORGNAME");
			QDomText text = element.ownerDocument().createTextNode(orgName);

			eOrg.appendChild(e);
			e.appendChild(text);
		}

		if ( !orgUnit.isEmpty() ) {
			QDomElement e = element.ownerDocument().createElement("ORGUNIT");
			QDomText text = element.ownerDocument().createTextNode(orgUnit);

			eOrg.appendChild(e);
			e.appendChild(text);
		}
	}

	if ( !url.isEmpty() ) {
		QDomElement e = element.ownerDocument().createElement("URL");
		QDomText text = element.ownerDocument().createTextNode( url.toString() );

		element.appendChild(e);
		e.appendChild(text);
	}

	if ( !desc.isEmpty() ) {
		QDomElement e = element.ownerDocument().createElement("DESC");
		QDomText text = element.ownerDocument().createTextNode(desc);

		element.appendChild(e);
		e.appendChild(text);
	}

	if ( jid.isValid() ) {
		QDomElement e = element.ownerDocument().createElement("JABBERID");
		QDomText text = element.ownerDocument().createTextNode(jid);

		element.appendChild(e);
		e.appendChild(text);
	}
}

inline bool vCard::Private::isEmpty() const
{
	if ( !fullname.isEmpty() || !nGiven.isEmpty() || !nMiddle.isEmpty() || !nFamily.isEmpty() || !nPrefix.isEmpty() || !nSuffix.isEmpty() ) {
		return false;
	}

	if ( !nickname.isEmpty() || !birthday.isNull() || !aCountry.isEmpty() || !aRegion.isEmpty() || !aCity.isEmpty() || !aPostal.isEmpty() || !aStreet.isEmpty() ) {
		return false;
	}

	if ( !phone.isEmpty() || !email.isEmpty() || !tzoffset.isEmpty() ) {
		return false;
	}

	if ( !lat.isEmpty() && !lon.isEmpty() ) {
		return false;
	}
	if ( !title.isEmpty() || !role.isEmpty() || !orgName.isEmpty() || !orgUnit.isEmpty() ) {
		return false;
	}
	if ( !url.isEmpty() || !desc.isEmpty() || !jid.isEmpty() ) {
		return false;
	}

	return true;
}

vCard::vCard()
	: d(new Private)
{
}

vCard::vCard(const vCard& other)
	: d(other.d)
{
}

vCard::~vCard()
{
}

vCard vCard::fromIQ(const IQ& iq)
{
	/* TODO: Creating vCard from info-query */
}

IQ vCard::toIQ() const
{
	IQ iq;
	iq.setChildElement("vCard", NS_VCARD_TEMP);
	d->toDomElement( iq.childElement() );

	return iq;
}

void vCard::toIQ(IQ& iq) const
{
	iq.setChildElement("vCard", NS_VCARD_TEMP);

	d->toDomElement( iq.childElement() );
}

void vCard::toDomElement(QDomElement& element) const
{
	d->toDomElement(element);
}

vCard::operator IQ() const
{
	return toIQ();
}

bool vCard::isEmpty() const
{
	return d->isEmpty();
}

QString vCard::fullName() const
{
	return d->fullname;
}

QString vCard::givenName() const
{
	return d->nGiven;
}

QString vCard::middleName() const
{
	return d->nMiddle;
}

QString vCard::familyName() const
{
	return d->nFamily;
}

QString vCard::namePrefix() const
{
	return d->nPrefix;
}

QString vCard::nameSuffix() const
{
	return d->nSuffix;
}

QString vCard::nickname() const
{
	return d->nickname;
}

QDate vCard::birthday() const
{
	return d->birthday;
}

QString vCard::addrCountry() const
{
	return d->aCountry;
}

QString vCard::addrRegion() const
{
	return d->aRegion;
}

QString vCard::addrPostal() const
{
	return d->aPostal;
}

QString vCard::addrCity() const
{
	return d->aCity;
}

QString vCard::addrStreet() const
{
	return d->aStreet;
}

QString vCard::phone() const
{
	return d->phone;
}

QString vCard::email() const
{
	return d->email;
}

QString vCard::timezone() const
{
	return d->tzoffset;
}

QString vCard::geoLatitude() const
{
	return d->lat;
}

QString vCard::geoLongitude() const
{
	return d->lon;
}

QString vCard::title() const
{
	return d->title;
}

QString vCard::role() const
{
	return d->role;
}

QString vCard::orgName() const
{
	return d->orgName;
}

QString vCard::orgUnit() const
{
	return d->orgUnit;
}

QUrl vCard::url() const
{
	return d->url;
}

QString vCard::description() const
{
	return d->desc;
}

Jid vCard::jid() const
{
	return d->jid;
}

void vCard::setFullName(const QString& fn)
{
	d->fullname = fn;
}

void vCard::setGivenName(const QString& given)
{
	d->nGiven = given;
}

void vCard::setMiddleName(const QString& middle)
{
	d->nMiddle = middle;
}

void vCard::setFamilyName(const QString& family)
{
	d->nFamily = family;
}

void vCard::setNamePrefix(const QString& prefix)
{
	d->nPrefix = prefix;
}

void vCard::setNameSuffix(const QString& suffix)
{
	d->nSuffix = suffix;
}

void vCard::setNickname(const QString& nick)
{
	d->nickname = nick;
}

void vCard::setBirthday(const QDate& birthdate)
{
	d->birthday = birthdate;
}

void vCard::setAddrCountry(const QString& country)
{
	d->aCountry = country;
}

void vCard::setAddrRegion(const QString& region)
{
	d->aRegion = region;
}

void vCard::setAddrCity(const QString& city)
{
	d->aCity = city;
}

void vCard::setAddrPostal(const QString& postal)
{
	d->aPostal = postal;
}

void vCard::setAddrStreet(const QString& street)
{
	d->aStreet = street;
}

void vCard::setAddress(const QString& country, const QString& region, const QString& city, const QString& postal, const QString& street)
{
	d->aCountry = country;
	d->aRegion = region;
	d->aCity = city;
	d->aPostal = postal;
	d->aStreet = street;
}

void vCard::setPhone(const QString& number)
{
	d->phone = number;
}

void vCard::setEmail(const QString& email)
{
	d->email = email;
}

void vCard::setTimezone(const QString& tz)
{
	d->tzoffset = tz;
}

void vCard::setGeo(const QString& lat, const QString& lon)
{
	d->lat = lat;
	d->lon = lon;
}

void vCard::setTitle(const QString& title)
{
	d->title = title;
}

void vCard::setRole(const QString& role)
{
	d->role = role;
}

void vCard::setOrgName(const QString& name)
{
	d->orgName = name;
}

void vCard::setOrgUnit(const QString& unit)
{
	d->orgUnit = unit;
}

void vCard::setUrl(const QUrl& url)
{
	d->url = url;
}

void vCard::setDescription(const QString& desc)
{
	d->desc = desc;
}

void vCard::setJid(const Jid& jid)
{
	d->jid = jid;
}


} /* end of namespace XMPP */
