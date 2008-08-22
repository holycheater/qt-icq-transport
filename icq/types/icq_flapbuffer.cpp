/**
 * icq_flapbuffer.cpp - ICQ flap packet.
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

#include "icq_flapbuffer.h"

ICQ::FlapBuffer::FlapBuffer(const Buffer& data)
{
	*this = data;
}

ICQ::FlapBuffer::FlapBuffer(Byte channel)
{
	m_channel = channel;
	m_Buffer.open(QBuffer::ReadWrite);
}

ICQ::FlapBuffer::FlapBuffer(const QByteArray& data, Byte channel)
{
	m_Buffer.setData(data);
	m_channel = channel;
	m_Buffer.open(QBuffer::ReadWrite);
}

ICQ::FlapBuffer& ICQ::FlapBuffer::addTlv(Tlv tlv)
{
	m_Buffer.write( tlv.data() );
	return *this;
}

ICQ::FlapBuffer& ICQ::FlapBuffer::addTlv(Buffer tlv)
{
	m_Buffer.write( tlv.data() );
	return *this;
}

ICQ::FlapBuffer& ICQ::FlapBuffer::addTlv(Word type, const QByteArray& data)
{
	addTlv( Tlv(type, data) );
	return *this;
}

ICQ::FlapBuffer& ICQ::FlapBuffer::addTlv(Word type, const QString& data)
{
	addTlv( Tlv( type, data.toLocal8Bit() ) );
	return *this;
}

ICQ::FlapBuffer& ICQ::FlapBuffer::addTlvChain(TlvChain tlvChain)
{
	m_Buffer.write( tlvChain.data() );
	return *this;
}

ICQ::Byte ICQ::FlapBuffer::channel() const
{
	return m_channel;
}

QByteArray ICQ::FlapBuffer::data() const
{
	QByteArray flap;
	flap += flapHeader();
	flap += m_Buffer.data();

	return flap;
}

ICQ::Word ICQ::FlapBuffer::flapDataSize() const
{
	return m_flapSize;
}

QByteArray ICQ::FlapBuffer::flapHeader() const
{
	Buffer header;
	header.addByte(0x2A);
	header.addByte(m_channel);
	header.addWord(m_sequence);
	header.addWord( size() );

	return header.data();
}

ICQ::FlapBuffer ICQ::FlapBuffer::fromRawData(const QByteArray& data)
{
	Buffer tmpBuf(data);
	tmpBuf.getByte(); // 0x2A

	FlapBuffer buf( tmpBuf.getByte() );
	buf.m_sequence = tmpBuf.getWord();
	buf.m_flapSize = tmpBuf.getWord();
	buf.setData( data.mid(ICQ::FLAP_HEADER_SIZE, buf.m_flapSize) );

	return buf;
}

ICQ::FlapBuffer ICQ::FlapBuffer::fromRawData(const char *data, Word datalen)
{
	return fromRawData( QByteArray::fromRawData(data, datalen) );
}

void ICQ::FlapBuffer::setChannel(Byte channel)
{
	m_channel = channel;
}

void ICQ::FlapBuffer::setSequence(Word sequence)
{
	m_sequence = sequence;
}

ICQ::Word ICQ::FlapBuffer::sequence() const
{
	return m_sequence;
}

ICQ::FlapBuffer& ICQ::FlapBuffer::operator=(const Buffer& other)
{
	Buffer tmpBuf = other;

	tmpBuf.getByte(); // 0x2A
	m_channel = tmpBuf.getByte();
	m_sequence = tmpBuf.getWord();
	Word size = tmpBuf.getWord();

	setData( tmpBuf.read(size) );

	return *this;
}

ICQ::FlapBuffer& ICQ::FlapBuffer::operator=(const QByteArray& data)
{
	return *this = Buffer(data);
}
