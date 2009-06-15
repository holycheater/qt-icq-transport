/*
 * icqUserDetails.cpp - ICQ User details.
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

#include "icqUserDetails.h"

#include <QDate>
#include <QSharedData>
#include <QString>
#include <QStringList>

namespace ICQ
{


class UserDetails::Private : public QSharedData
{
    public:
        Private();
        Private(const Private& other);
        virtual ~Private();

        QString uin;

        QString nick;
        QString firstName, lastName;
        QString email;

        QString homeCity;
        QString homeState;
        QString homePhone;
        QString homeFax;
        QString homeAddress;
        QString cellPhone;
        QString homeZipCode;

        int age;
        Gender gender;
        QDate birthdate;
        QString originalCity;
        QString originalState;

        QStringList emails;

        QString homepage;

        QString workCity;
        QString workState;
        QString workPhone;
        QString workFax;
        QString workAddress;
        QString workZipCode;
        QString workCompany;
        QString workDepartment;
        QString workPosition;
        QString workWebpage;

        QString notes;
};

UserDetails::Private::Private()
    : QSharedData()
{
    age = 0;
    gender = NoGender;
}

UserDetails::Private::Private(const Private& other)
    : QSharedData(other)
{
    uin = other.uin;

    nick = other.nick;
    firstName = other.firstName;
    lastName = other.lastName;
    email = other.email;

    homeCity = other.homeCity;
    homeState = other.homeState;
    homePhone = other.homePhone;
    homeFax = other.homeFax;
    homeAddress = other.homeAddress;
    cellPhone = other.cellPhone;
    homeZipCode = other.homeZipCode;

    age = other.age;
    gender = other.gender;
    birthdate = other.birthdate;
    originalCity = originalCity;
    originalState = other.originalState;

    QStringList emails;

    homepage = other.homepage;

    workCity = other.workCity;
    workState = other.workState;
    workPhone = other.workPhone;
    workFax = other.workFax;
    workAddress = other.workAddress;
    workZipCode = other.workZipCode;
    workCompany = other.workCompany;
    workDepartment = other.workDepartment;
    workPosition = other.workPosition;
    workWebpage = other.workWebpage;

    notes = other.notes;
}

UserDetails::Private::~Private()
{
}

UserDetails::UserDetails()
    : d(new Private)
{
}

UserDetails::UserDetails(const UserDetails& other)
    : d(other.d)
{
}

UserDetails::~UserDetails()
{
}

UserDetails& UserDetails::operator=(const UserDetails& other)
{
    d = other.d;
    return *this;
}

void UserDetails::clear()
{
    d = new Private;
}

QString UserDetails::uin() const
{
    return d->uin;
}

QString UserDetails::nick() const
{
    return d->nick;
}
QString UserDetails::firstName() const
{
    return d->firstName;
}

QString UserDetails::lastName() const
{
    return d->lastName;
}

QString UserDetails::email() const
{
    return d->email;
}

QString UserDetails::homeCity() const
{
    return d->homeCity;
}

QString UserDetails::homeState() const
{
    return d->homeState;
}

QString UserDetails::homePhone() const
{
    return d->homePhone;
}

QString UserDetails::homeFax() const
{
    return d->homeFax;
}

QString UserDetails::homeAddress() const
{
    return d->homeAddress;
}

QString UserDetails::cellPhone() const
{
    return d->cellPhone;
}

QString UserDetails::homeZipCode() const
{
    return d->homeZipCode;
}

int UserDetails::age() const
{
    return d->age;
}

UserDetails::Gender UserDetails::gender() const
{
    return d->gender;
}

QDate UserDetails::birthdate() const
{
    return d->birthdate;
}

QString UserDetails::originalCity() const
{
    return d->originalCity;
}

QString UserDetails::originalState() const
{
    return d->originalState;
}

QStringList UserDetails::emails() const
{
    return d->emails;
}

QString UserDetails::homepage() const
{
    return d->homepage;
}

QString UserDetails::workCity() const
{
    return d->workCity;
}

QString UserDetails::workState() const
{
    return d->workState;
}

QString UserDetails::workPhone() const
{
    return d->workPhone;
}

QString UserDetails::workFax() const
{
    return d->workFax;
}

QString UserDetails::workAddress() const
{
    return d->workAddress;
}

QString UserDetails::workZipCode() const
{
    return d->workZipCode;
}

QString UserDetails::workCompany() const
{
    return d->workCompany;
}

QString UserDetails::workDepartment() const
{
    return d->workDepartment;
}

QString UserDetails::workPosition() const
{
    return d->workPosition;
}

QString UserDetails::workWebpage() const
{
    return d->workWebpage;
}

QString UserDetails::notes() const
{
    return d->notes;
}

void UserDetails::setUin(const QString& uin)
{
    d->uin = uin;
}

void UserDetails::setNick(const QString& nick)
{
    d->nick = nick;
}

void UserDetails::setFirstName(const QString& firstName)
{
    d->firstName = firstName;
}

void UserDetails::setLastName(const QString& lastName)
{
    d->lastName = lastName;
}

void UserDetails::setEmail(const QString& email)
{
    d->email = email;
}

void UserDetails::setHomeCity(const QString& homeCity)
{
    d->homeCity = homeCity;
}

void UserDetails::setHomeState(const QString& homeState)
{
    d->homeState = homeState;
}

void UserDetails::setHomePhone(const QString& homePhone)
{
    d->homePhone = homePhone;
}

void UserDetails::setHomeFax(const QString& homeFax)
{
    d->homeFax = homeFax;
}

void UserDetails::setHomeAddress(const QString& homeAddress)
{
    d->homeAddress = homeAddress;
}

void UserDetails::setCellPhone(const QString& cellPhone)
{
    d->cellPhone = cellPhone;
}

void UserDetails::setHomeZipCode(const QString& homeZipCode)
{
    d->homeZipCode = homeZipCode;
}

void UserDetails::setAge(int age)
{
    d->age = age;
}

void UserDetails::setGender(Gender gender)
{
    d-> gender = gender;
}

void UserDetails::setBirthDate(const QDate& date)
{
    d->birthdate = date;
}

void UserDetails::setOriginalCity(const QString& city)
{
    d->originalCity = city;
}

void UserDetails::setOriginalState(const QString& state)
{
    d->originalState = state;
}

void UserDetails::addEmail(const QString& email)
{
    d->emails << email;
}

void UserDetails::setEmails(const QStringList& emails)
{
    d->emails = emails;
}

void UserDetails::setHomepage(const QString& homepage)
{
    d->homepage = homepage;
}

void UserDetails::setWorkCity(const QString& workCity)
{
    d->workCity = workCity;
}

void UserDetails::setWorkState(const QString& workState)
{
    d->workState = workState;
}

void UserDetails::setWorkPhone(const QString& workPhone)
{
    d->workPhone = workPhone;
}

void UserDetails::setWorkFax(const QString& workFax)
{
    d->workFax = workFax;
}

void UserDetails::setWorkAddress(const QString& workAddress)
{
    d->workAddress = workAddress;
}

void UserDetails::setWorkZipCode(const QString& workZipCode)
{
    d->workZipCode = workZipCode;
}

void UserDetails::setWorkCompany(const QString& workCompany)
{
    d->workCompany = workCompany;
}

void UserDetails::setWorkDepartment(const QString& workDepartment)
{
    d->workDepartment = workDepartment;
}

void UserDetails::setWorkPosition(const QString& workPosition)
{
    d->workPosition = workPosition;
}

void UserDetails::setWorkWebpage(const QString& workWebpage)
{
    d->workWebpage = workWebpage;
}

void UserDetails::setNotes(const QString& notes)
{
    d->notes = notes;
}

bool UserDetails::isEmpty()
{
    if ( d->uin.isEmpty() ) {
        return true;
    }
    if ( d->nick.isEmpty() && d->firstName.isEmpty() && d->lastName.isEmpty() && d->email.isEmpty() ) {
        return true;
    }
    return false;
}


} /* end of namespace ICQ */

// vim:sw=4:ts=4:et:nowrap
