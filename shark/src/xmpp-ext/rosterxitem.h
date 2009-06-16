/*
 * rosterxitem.h - RosterX Item (XEP-0144)
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

#ifndef XMPP_ROSTERX_ITEM_H_
#define XMPP_ROSTERX_ITEM_H_

#include <QSharedDataPointer>
#include <QString>

class QStringList;

namespace XMPP
{

class RosterXItem
{
    public:
        enum Action { Add, Delete, Modify };

        RosterXItem();
        RosterXItem(const QString& jid, Action = Add, const QString& name = QString(), const QString& group = QString());
        RosterXItem(const RosterXItem& other);
        virtual ~RosterXItem();
        RosterXItem& operator=(const RosterXItem& other);

        Action action() const;
        QString actionString() const;
        QString jid() const;
        QString name() const;
        QStringList groups() const;

        void setAction(Action a);
        void setAction(const QString& action);
        void setJid(const QString& jid);
        void setName(const QString& name);
        void setGroups(const QStringList& groups);
        void setGroup(const QString& group);
    private:
        class Private;
        QSharedDataPointer<Private> d;
};


}

// vim:ts=4:sw=4:nowrap:et
#endif /* XMPP_ROSTERX_ITEM_H_ */
