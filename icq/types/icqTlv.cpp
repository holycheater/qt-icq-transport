/*
 * icqTlv.cpp - ICQ TLV (type-length-value) buffer.
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

#include "icqTlv.h"

namespace ICQ
{


Tlv::Tlv()
{
    m_type = 0x0;
    m_Buffer.open(QBuffer::ReadWrite);
}

Tlv::Tlv(Word type)
{
    m_type = type;
    m_Buffer.open(QBuffer::ReadWrite);
}

Tlv::Tlv(Word type, const QByteArray& data)
{
    m_type = type;
    setData(data);
}

Tlv::Tlv(const QByteArray& data)
{
    operator=( Buffer(data) );
}

QByteArray Tlv::data() const
{
    Buffer tlvData;

    tlvData.addWord(m_type);
    tlvData.addWord( m_Buffer.size() );
    tlvData.addData( m_Buffer.data() );

    return tlvData;
}

Tlv Tlv::fromBuffer(Buffer& buffer)
{
    Word type = buffer.getWord();
    Word len = buffer.getWord();
    QByteArray data = buffer.read(len);
    return Tlv(type, data);
}

void Tlv::setType(Word type)
{
    m_type = type;
}

Word Tlv::type() const
{
    return m_type;
}

Tlv& Tlv::operator=(const Tlv& buffer)
{
    m_type = buffer.type();
    setData( buffer.m_Buffer.data() );

    return *this;
}

Tlv& Tlv::operator=(const Buffer& buffer)
{
    Buffer tmpBuf(buffer);
    m_type = tmpBuf.getWord();
    Word size = tmpBuf.getWord();

    setData( tmpBuf.read(size) );

    return *this;
}

Tlv& Tlv::operator=(const QByteArray& data)
{
    return *this = Buffer(data);
}


} /* end of namespace ICQ */

// vim:sw=4:ts=4:et:nowrap
