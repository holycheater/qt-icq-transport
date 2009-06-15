/*
 * icqSocket.h - ICQ connection socket.
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

#ifndef ICQ_SOCKET_H_
#define ICQ_SOCKET_H_

#include <QObject>

#include "types/icqTypes.h"

class QHostAddress;

namespace ICQ
{

class Buffer;
class FlapBuffer;
class SnacBuffer;
class RateManager;
class MetaInfoManager;

class Socket : public QObject
{
    Q_OBJECT

    public:
        Socket(QObject *parent = 0);
        virtual ~Socket();

        int connectionStatus() const;

        void connectToHost(const QHostAddress& host, quint16 port);
        void disconnectFromHost();

        void setRateManager(RateManager *ptr);
        void setMetaManager(MetaInfoManager *ptr);

        void snacRequest(Word family, Word subtype);

        void sendMetaRequest(Word type);
        void sendMetaRequest(Word type, Buffer& data);

        void write(const FlapBuffer& flap);
        void write(const SnacBuffer& snac);

        void writeForced(FlapBuffer* flap);
        void writeForced(SnacBuffer* snac);
    signals:
        void incomingFlap(FlapBuffer& flap);
        void incomingSnac(SnacBuffer& snac);

        void readyRead();
    private slots:
        void processIncomingData();
    private:
        Q_DISABLE_COPY(Socket)
        class Private;
        Private *d;
};


} /* end of namespace ICQ */

// vim:ts=4:sw=4:et:nowrap
#endif /* ICQ_SOCKET_H_ */
