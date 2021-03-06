/*
 * icqRateManager.h - packet rate manager for an icq connection
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

#ifndef ICQRATEMANAGER_H_
#define ICQRATEMANAGER_H_

#include "types/icqTypes.h"
#include "types/icqRateClass.h"
#include "types/icqSnacBuffer.h"

#include <QList>
#include <QByteArray>
#include <QObject>

namespace ICQ
{

class Socket;


class RateManager: public QObject
{
    Q_OBJECT

    public:
        RateManager(Socket *socket, QObject *parent = 0);
        ~RateManager();

        /* reset the rate manager */
        void reset();

        /* add new rate class */
        void addClass(RateClass* rc);

        /* check if we can send the packet right now */
        bool canSend(const SnacBuffer& snac) const;

        /* enqueue the packet */
        void enqueue(const SnacBuffer& snac);

        void requestRates();
    public slots:
        /* this slot sends data to socket */
        void dataAvailable(SnacBuffer* snac);
    private slots:
        void incomingSnac(SnacBuffer& snac);
    private:
        class Private;
        Private* d;
};

}

// vim:ts=4:sw=4:et:nowrap
#endif /*ICQRATEMANAGER_H_*/
