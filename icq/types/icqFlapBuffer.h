/*
 * icqFlapBuffer.h - ICQ flap packet.
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

#ifndef ICQFLAPBUFFER_H_
#define ICQFLAPBUFFER_H_

#include "icqTypes.h"
#include "icqBuffer.h"
#include "icqTlv.h"
#include "icqTlvChain.h"

namespace ICQ
{


class FlapBuffer: public Buffer
{
    public:
        enum ChannelType { AuthChannel = 0x1, DataChannel, ErrorChannel, CloseChannel, KeepAliveChannel };
        /* data = flap data + flap header */
        FlapBuffer(const Buffer& data);
        FlapBuffer(Byte channel = DataChannel);
        /* data is FLAP data, it doesn't include a header */
        FlapBuffer(const QByteArray& data, Byte channel = DataChannel);

        /* append tlv to the flap */
        FlapBuffer& addTlv(Tlv tlv);
        FlapBuffer& addTlv(Buffer tlv);
        FlapBuffer& addTlv(Word type, const QByteArray& data);
        FlapBuffer& addTlv(Word type, const QString& data);

        /* append tlv chain to the flap */
        FlapBuffer& addTlvChain(TlvChain tlvChain);

        /* get flap channel */
        Byte channel() const;

        /* get flap packet (header + data) */
        virtual QByteArray data() const;

        /* return flap data size as described by header */
        Word flapDataSize() const;

        /* get header data (6 bytes) */
        QByteArray flapHeader() const;

        /* construct flap from raw data. This data includes a header (6 bytes from the beginning) */
        static FlapBuffer fromRawData(const QByteArray& data);
        static FlapBuffer fromRawData(const char *data, Word datalen);

        void setChannel(Byte channel);
        void setSequence(Word sequence);

        Word sequence() const;

        FlapBuffer& operator=(const Buffer& other);
        FlapBuffer& operator=(const QByteArray& data);
    private:
        Byte m_channel;
        Word m_sequence;
        /* flap size as described by length */
        Word m_flapSize;
};

}

// vim:ts=4:sw=4:et:nowrap
#endif /* ICQFLAPBUFFER_H_ */
