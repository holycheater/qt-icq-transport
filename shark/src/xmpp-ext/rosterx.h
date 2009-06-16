/*
 * rosterx.h - Roster Item Exchange (XEP-0144)
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

#ifndef XMPP_ROSTERX_H_
#define XMPP_ROSTERX_H_

#define NS_ROSTERX "http://jabber.org/protocol/rosterx"

#include "rosterxitem.h"
#include <QList>

namespace XMPP
{

class Stanza;
class Message;
class IQ;

class RosterX
{
    public:
        RosterX();
        RosterX(const RosterX& other);
        virtual ~RosterX();
        RosterX& operator=(const RosterX& other);

        void toMessage(Message& msg);
        void toIQ(IQ& iq);

        static RosterX fromMessage(const Message& msg);
        static RosterX fromIQ(const IQ& iq);

        void addItem(const RosterXItem& item);
        void setItems(const QList<RosterXItem> items);
        QList<RosterXItem> items() const;
    private:
        static RosterX fromStanza(const Stanza& s);
        void toStanza(Stanza& s);

        QList<RosterXItem> m_items;
};


}

// vim:ts=4:sw=4:nowrap:et
#endif /* XMPP_ROSTERX_H_ */
