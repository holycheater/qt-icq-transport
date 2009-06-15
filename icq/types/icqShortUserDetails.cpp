/*
 * icqShortUserDetails.cpp - ICQ Short User details.
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

#include "icqShortUserDetails.h"

#include <QSharedData>
#include <QString>

namespace ICQ
{


class ShortUserDetails::Private : public QSharedData
{
    public:
        Private();
        Private(const Private& other);
        virtual ~Private();

        QString uin;

        QString nick;
        QString firstName, lastName;
        QString email;
};

ShortUserDetails::Private::Private()
    : QSharedData()
{
}

ShortUserDetails::Private::Private(const Private& other)
    : QSharedData(other)
{
    uin = other.uin;

    nick = other.nick;
    firstName = other.firstName;
    lastName = other.lastName;
    email = other.email;
}

ShortUserDetails::Private::~Private()
{
}

ShortUserDetails::ShortUserDetails()
    : d(new Private)
{
}

ShortUserDetails::ShortUserDetails(const ShortUserDetails& other)
    : d(other.d)
{
}

ShortUserDetails::~ShortUserDetails()
{
}

ShortUserDetails& ShortUserDetails::operator=(const ShortUserDetails& other)
{
    d = other.d;
    return *this;
}

QString ShortUserDetails::uin() const
{
    return d->uin;
}

QString ShortUserDetails::nick() const
{
    return d->nick;
}
QString ShortUserDetails::firstName() const
{
    return d->firstName;
}

QString ShortUserDetails::lastName() const
{
    return d->lastName;
}

QString ShortUserDetails::email() const
{
    return d->email;
}

void ShortUserDetails::setUin(const QString& uin)
{
    d->uin = uin;
}

void ShortUserDetails::setNick(const QString& nick)
{
    d->nick = nick;
}

void ShortUserDetails::setFirstName(const QString& firstName)
{
    d->firstName = firstName;
}

void ShortUserDetails::setLastName(const QString& lastName)
{
    d->lastName = lastName;
}

void ShortUserDetails::setEmail(const QString& email)
{
    d->email = email;
}

bool ShortUserDetails::isEmpty()
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
