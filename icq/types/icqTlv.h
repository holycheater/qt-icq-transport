/*
 * icqTlv.h - ICQ TLV (type-length-value) buffer.
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

#ifndef ICQTLV_H_
#define ICQTLV_H_

#include "icqBuffer.h"

#include <QString>

namespace ICQ
{


class Tlv : public Buffer
{
    public:
        Tlv();
        Tlv(Word type);
        Tlv(Word type, const QByteArray& data);
        Tlv(const QByteArray& data);

        QByteArray data() const;

        /* read tlv from the part of the buffer */
        static Tlv fromBuffer(Buffer& buffer);

        void setType(Word type);
        Word type() const;

        Tlv& operator=(const Tlv& buffer);
        Tlv& operator=(const Buffer& buffer);
        Tlv& operator=(const QByteArray& data);
    private:
        friend class TlvChain;
        Word m_type;
};

}

// vim:ts=4:sw=4:et:nowrap
#endif /* ICQTLV_H_ */
