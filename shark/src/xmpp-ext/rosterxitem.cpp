/*
 * rosterxitem.cpp - RosterX Item (XEP-0144)
 * Copyright (C) 2009  Alexander Saltykov
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

#include "rosterxitem.h"
#include <QSharedData>
#include <QString>
#include <QStringList>

namespace XMPP {


class RosterXItem::Private : public QSharedData {
    public:
        Private();
        Private(const Private& other);
        ~Private();

        Action action;
        QString jid;
        QString name;
        QStringList groups;
};

RosterXItem::Private::Private()
    : QSharedData()
{
    action = Add;
}

RosterXItem::Private::Private(const Private& other)
    : QSharedData(other)
{
    action = other.action;
    jid = other.jid;
    name = other.name;
    groups = other.groups;
}

RosterXItem::Private::~Private()
{
}

RosterXItem::RosterXItem()
    : d(new Private)
{
}

RosterXItem::RosterXItem(const QString& jid, Action a, const QString& name, const QString& group)
    : d(new Private)
{
    d->action = a;
    d->jid = jid;
    d->name = name;
    if (!group.isEmpty())
        d->groups << group;
}

RosterXItem::RosterXItem(const RosterXItem& other)
    : d(other.d)
{
}

RosterXItem::~RosterXItem()
{
}

RosterXItem& RosterXItem::operator=(const RosterXItem& other)
{
    d = other.d;
    return *this;
}

RosterXItem::Action RosterXItem::action() const
{
    return d->action;
}

QString RosterXItem::actionString() const
{
    QString a;
    switch ( d->action ) {
        case Add:
            a = "add";
            break;
        case Delete:
            a = "delete";
            break;
        case Modify:
            a = "modify";
            break;
    }
    return a;
}

QString RosterXItem::jid() const
{
    return d->jid;
}

QString RosterXItem::name() const
{
    return d->name;
}

QStringList RosterXItem::groups() const
{
    return d->groups;
}

void RosterXItem::setAction(Action a)
{
    d->action = a;
}

void RosterXItem::setAction(const QString& action)
{
    Action a = Add;
    if (action == "delete") {
        a = Delete;
    } else if (action == "modify") {
        a = Modify;
    }
    d->action = a;
}

void RosterXItem::setJid(const QString& jid)
{
    d->jid = jid;
}

void RosterXItem::setName(const QString& name)
{
    d->name = name;
}

void RosterXItem::setGroups(const QStringList& g)
{
    d->groups = g;
}

void RosterXItem::setGroup(const QString& group)
{
    if (!group.isEmpty()) {
        d->groups.clear();
        d->groups << group;
    }
}

} /* end of namespace XMPP */
