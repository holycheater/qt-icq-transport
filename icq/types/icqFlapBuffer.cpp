/*
 * icqFlapBuffer.cpp - ICQ flap packet.
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

#include "icqFlapBuffer.h"

namespace ICQ
{


FlapBuffer::FlapBuffer(const Buffer& data)
{
	*this = data;
}

FlapBuffer::FlapBuffer(Byte channel)
{
	m_channel = channel;
	m_Buffer.open(QBuffer::ReadWrite);
}

FlapBuffer::FlapBuffer(const QByteArray& data, Byte channel)
{
	m_Buffer.setData(data);
	m_channel = channel;
	m_Buffer.open(QBuffer::ReadWrite);
}

FlapBuffer& FlapBuffer::addTlv(Tlv tlv)
{
	m_Buffer.write( tlv.data() );
	return *this;
}

FlapBuffer& FlapBuffer::addTlv(Buffer tlv)
{
	m_Buffer.write( tlv.data() );
	return *this;
}

FlapBuffer& FlapBuffer::addTlv(Word type, const QByteArray& data)
{
	addTlv( Tlv(type, data) );
	return *this;
}

FlapBuffer& FlapBuffer::addTlv(Word type, const QString& data)
{
	addTlv( Tlv( type, data.toLocal8Bit() ) );
	return *this;
}

FlapBuffer& FlapBuffer::addTlvChain(TlvChain tlvChain)
{
	m_Buffer.write( tlvChain.data() );
	return *this;
}

Byte FlapBuffer::channel() const
{
	return m_channel;
}

QByteArray FlapBuffer::data() const
{
	QByteArray flap;
	flap += flapHeader();
	flap += m_Buffer.data();

	return flap;
}

Word FlapBuffer::flapDataSize() const
{
	return m_flapSize;
}

QByteArray FlapBuffer::flapHeader() const
{
	Buffer header;
	header.addByte(0x2A);
	header.addByte(m_channel);
	header.addWord(m_sequence);
	header.addWord( size() );

	return header.data();
}

FlapBuffer FlapBuffer::fromRawData(const QByteArray& data)
{
	Buffer tmpBuf(data);
	tmpBuf.getByte(); // 0x2A

	FlapBuffer buf( tmpBuf.getByte() );
	buf.m_sequence = tmpBuf.getWord();
	buf.m_flapSize = tmpBuf.getWord();
	buf.setData( data.mid(FLAP_HEADER_SIZE, buf.m_flapSize) );

	return buf;
}

FlapBuffer FlapBuffer::fromRawData(const char *data, Word datalen)
{
	return fromRawData( QByteArray::fromRawData(data, datalen) );
}

void FlapBuffer::setChannel(Byte channel)
{
	m_channel = channel;
}

void FlapBuffer::setSequence(Word sequence)
{
	m_sequence = sequence;
}

Word FlapBuffer::sequence() const
{
	return m_sequence;
}

FlapBuffer& FlapBuffer::operator=(const Buffer& other)
{
	Buffer tmpBuf = other;

	tmpBuf.getByte(); // 0x2A
	m_channel = tmpBuf.getByte();
	m_sequence = tmpBuf.getWord();
	Word size = tmpBuf.getWord();

	setData( tmpBuf.read(size) );

	return *this;
}

FlapBuffer& FlapBuffer::operator=(const QByteArray& data)
{
	return *this = Buffer(data);
}


} /* end of namespace ICQ */

// vim:sw=4:ts=4:noet:nowrap
