/**
 * icqSnacBuffer.cpp - ICQ snac packet.
 * Copyright (C) 2008  Alexander Saltykov
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 **/

#include "icqSnacBuffer.h"

ICQ::SnacBuffer::SnacBuffer(Word family, Word subtype)
{
	setChannel(DataChannel);
	m_family = family;
	m_subtype = subtype;
	m_flags = 0;
	m_requestId = 0;

	m_Buffer.open(QBuffer::ReadWrite);
}

ICQ::SnacBuffer::SnacBuffer(Word family, Word subtype, const QByteArray& data)
{
	m_family = family;
	m_subtype = subtype;
	m_flags = 0x0;
	m_requestId = 0;
	setChannel(DataChannel);

	setData(data);
}

ICQ::SnacBuffer::SnacBuffer(const FlapBuffer& flap)
{
	*this = flap;
}

QByteArray ICQ::SnacBuffer::data() const
{
	Buffer tmpBuf;

	tmpBuf.addData( flapHeader() );

	tmpBuf.addWord(m_family);
	tmpBuf.addWord(m_subtype);
	tmpBuf.addWord(m_flags);
	tmpBuf.addDWord(m_requestId);

	tmpBuf.addData( m_Buffer.data() );

	return tmpBuf.data();
}

ICQ::Word ICQ::SnacBuffer::dataSize() const
{
	return m_Buffer.size();
}

ICQ::Word ICQ::SnacBuffer::family() const
{
	return m_family;
}

ICQ::Word ICQ::SnacBuffer::subtype() const
{
	return m_subtype;
}

ICQ::Word ICQ::SnacBuffer::flags() const
{
	return m_flags;
}

ICQ::DWord ICQ::SnacBuffer::requestId() const
{
	return m_requestId;
}

void ICQ::SnacBuffer::setFamily(Word family)
{
	m_family = family;
}

void ICQ::SnacBuffer::setSubtype(Word subtype)
{
	m_subtype = subtype;
}

void ICQ::SnacBuffer::setFlags(Word flags)
{
	m_flags = flags;
}

void ICQ::SnacBuffer::setRequestId(DWord requestId)
{
	m_requestId = requestId;
}

ICQ::Word ICQ::SnacBuffer::size() const
{
	return m_Buffer.size() + ICQ::SNAC_HEADER_SIZE;
}

ICQ::SnacBuffer& ICQ::SnacBuffer::operator=(const Buffer& other)
{
	Buffer tmpBuf = other;

	tmpBuf.getByte(); // 0x2A
	setChannel( tmpBuf.getByte() );
	setSequence( tmpBuf.getWord() );
	ICQ::Word size = tmpBuf.getWord();

	m_family = tmpBuf.getWord();
	m_subtype = tmpBuf.getWord();
	m_flags = tmpBuf.getWord();
	m_requestId = tmpBuf.getWord();

	setData( tmpBuf.read(size - ICQ::SNAC_HEADER_SIZE) );

	return *this;
}

ICQ::SnacBuffer& ICQ::SnacBuffer::operator=(const FlapBuffer& other)
{
	QByteArray data = other.data();
	data.remove(0, ICQ::FLAP_HEADER_SIZE);
	Buffer tmpBuf(data);

	setChannel( other.channel() );
	setSequence( other.sequence() );

	m_family = tmpBuf.getWord();
	m_subtype = tmpBuf.getWord();
	m_flags = tmpBuf.getWord();
	m_requestId = tmpBuf.getDWord();

	setData( tmpBuf.readAll() );

	return *this;
}

ICQ::SnacBuffer& ICQ::SnacBuffer::operator=(const SnacBuffer& other)
{

	m_family = other.m_family;
	m_subtype = other.m_subtype;
	m_flags = other.m_flags;
	m_requestId = other.m_requestId;

	setData( other.m_Buffer.data() );

	return *this;
}

ICQ::SnacBuffer& ICQ::SnacBuffer::operator=(const QByteArray& other)
{
	Buffer tmpBuf = other;

	tmpBuf.getByte(); // flap 0x2A;
	setChannel( tmpBuf.getByte() ); // flap channel
	setSequence( tmpBuf.getWord() );
	tmpBuf.getWord(); // flap size

	m_family = tmpBuf.getWord();
	m_subtype = tmpBuf.getWord();
	m_flags = tmpBuf.getWord();
	m_requestId = tmpBuf.getDWord();

	setData( tmpBuf.readAll() );

	return *this;
}

