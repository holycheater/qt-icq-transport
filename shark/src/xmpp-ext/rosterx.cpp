/*
 * rosterx.cpp - Roster Item Exchange (XEP-0144)
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

#include "rosterx.h"
#include "rosterxitem.h"
#include "xmpp-core/IQ.h"
#include "xmpp-core/Message.h"

#include <QDomDocument>
#include <QDomElement>
#include <QSharedData>
#include <QStringList>

namespace XMPP
{


/**
 * @class RosterX
 * @brief Roster Item Exchange (XEP-0050).
 */

RosterX::RosterX()
{
}

RosterX::RosterX(const RosterX& other)
{
    m_items = other.m_items;
}

RosterX::~RosterX()
{
}

RosterX& RosterX::operator=(const RosterX& other)
{
    m_items = other.m_items;
    return *this;
}

void RosterX::toMessage(Message& msg)
{
    toStanza(msg);
}

void RosterX::toIQ(IQ& iq)
{
    toStanza(iq);
}

RosterX RosterX::fromMessage(const Message& msg)
{
    return fromStanza(msg);
}

RosterX RosterX::fromIQ(const IQ& iq)
{
    return fromStanza(iq);
}

void RosterX::addItem(const RosterXItem& item)
{
    m_items << item;
}

void RosterX::setItems(const QList<RosterXItem> items)
{
    m_items = items;
}

QList<RosterXItem> RosterX::items() const
{
    return m_items;
}

void RosterX::toStanza(Stanza& s)
{
    QDomElement e = s.doc()->documentElement();
    QDomNodeList childs = e.childNodes();
    QDomElement x;
    for (int i = 0; i < childs.count(); ++i) {
        QDomElement check = childs.item(i).toElement();
        if ( check.tagName() == "x" && check.namespaceURI() == NS_ROSTERX ) {
            x = check;
            break;
        }
    }
    if ( x.isNull() ) {
        x = s.doc()->createElementNS(NS_ROSTERX, "x");
        e.appendChild(x);
    }

    e.removeChild(e.firstChildElement("query"));

    QListIterator<RosterXItem> it(m_items);
    while (it.hasNext()) {
        RosterXItem item = it.next();
        QDomElement eItem = s.doc()->createElement("item");
        eItem.setAttribute("action", item.actionString());
        eItem.setAttribute("jid", item.jid());
        QString name = item.name();
        QStringList groups = item.groups();
        if (!name.isEmpty())
            eItem.setAttribute("name", name);
        if (!groups.isEmpty()) {
            QStringListIterator gi(groups);
            while (gi.hasNext()) {
                QDomElement eGroup = s.doc()->createElement("group");
                QDomText eText = s.doc()->createTextNode(gi.next());
                eGroup.appendChild(eText);
                eItem.appendChild(eGroup);
            }
        }
        x.appendChild(eItem);
    }
}

RosterX RosterX::fromStanza(const Stanza& s)
{
    RosterX x;
    QDomNodeList childs = s.doc()->documentElement().childNodes();
    QDomElement eX;
    for (int i = 0; i < childs.count(); ++i) {
        QDomElement e = childs.item(i).toElement();
        if ( e.tagName() == "x" && e.namespaceURI() == NS_ROSTERX ) {
            eX = e;
            break;
        }
    }
    if (eX.isNull())
        return x;
    QDomNodeList items = eX.childNodes();
    for (int i = 0; i < items.count(); ++i) {
        QDomElement eItem = items.item(i).toElement();
        if (eItem.isNull() || eItem.tagName() != "item")
            continue;
        QString action = eItem.attribute("action", "add");
        QString name = eItem.attribute("name");
        QString jid = eItem.attribute("jid");
        RosterXItem item(jid);
        item.setAction(action);
        item.setName(name);
        QDomNodeList groups = eItem.childNodes();
        QStringList list_groups;
        for (int j = 0; j < groups.count(); ++j) {
            QDomElement eGroup = groups.item(j).toElement();
            if (eGroup.isNull() || eGroup.tagName() != "group")
                continue;
            QString group = eGroup.text().trimmed();
            if (group.isEmpty())
                continue;
            list_groups << group;
        }
        item.setGroups(list_groups);
        x.addItem(item);
    }
    return x;
}


}

// vim:ts=4:sw=4:nowrap:et
