/*
 * componentstream.h - Jabber component stream (jabber:component:accept)
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

#ifndef XMPP_COMPONENTSTREAM_H_
#define XMPP_COMPONENTSTREAM_H_

#define NS_COMPONENT "jabber:component:accept"

#include "stream.h"

#include "xmpp-core/Parser.h"
#include "xmpp-core/Connector.h"

class QDomDocument;

namespace XMPP {

class Connector;
class Jid;

class ComponentStream : public Stream
{
    Q_OBJECT

    public:
        ComponentStream(Connector *connector, QObject *parent = 0);
        ~ComponentStream();

        QString baseNS() const;

        void connectToServer(const Jid& jid, const QString& secret);
    private:
        void handleStreamOpen(const Parser::Event& e);
        bool handleUnknownElement(const Parser::Event& e);
    private slots:
        void slotConnectorConnected();
    private:
        class Private;
        Private *d;
};


} /* end of namespace XMPP */

// vim:ts=4:sw=4:et:nowrap
#endif /* XMPP_COMPONENTSTREAM_H_ */
