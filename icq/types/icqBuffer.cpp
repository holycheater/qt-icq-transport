/*
 * icqBuffer.cpp - ICQ data buffer (packet).
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

#include "icqBuffer.h"

#include <QtEndian>

ICQ::Buffer::Buffer()
{
	m_Buffer.open(QBuffer::ReadWrite);
}

ICQ::Buffer::Buffer(const QByteArray& data)
{
	m_Buffer.setData(data);
	m_Buffer.open(QBuffer::ReadWrite);
}

ICQ::Buffer::Buffer(const Buffer& buffer)
{
	m_Buffer.setData( buffer.m_Buffer.data() );
	m_Buffer.open(QBuffer::ReadWrite);
}

ICQ::Buffer::~Buffer()
{
	m_Buffer.close();
}

ICQ::Buffer& ICQ::Buffer::addByte(Byte data)
{
	m_Buffer.write( (char*)&data, sizeof(Byte) );
	return *this;
}

ICQ::Buffer& ICQ::Buffer::addWord(Word data)
{
	data = qToBigEndian(data);
	m_Buffer.write( (char*)&data, sizeof(Word) );
	return *this;
}

ICQ::Buffer& ICQ::Buffer::addDWord(DWord data)
{
	data = qToBigEndian(data);
	m_Buffer.write( (char*)&data, sizeof(DWord) );
	return *this;
}

ICQ::Buffer& ICQ::Buffer::addLEWord(Word data)
{
	m_Buffer.write( (char*)&data, sizeof(Word) );
	return *this;
}

ICQ::Buffer& ICQ::Buffer::addLEDWord(DWord data)
{
	m_Buffer.write( (char*)&data, sizeof(DWord) );
	return *this;
}

ICQ::Buffer& ICQ::Buffer::addData(const Buffer& buffer)
{
	m_Buffer.write( buffer.data() );
	return *this;
}

ICQ::Buffer& ICQ::Buffer::addData(const QByteArray& data)
{
	m_Buffer.write(data);
	return *this;
}

ICQ::Buffer& ICQ::Buffer::addData(const QString& data)
{
	m_Buffer.write( data.toLocal8Bit() );

	return *this;
}

bool ICQ::Buffer::atEnd() const
{
	return m_Buffer.atEnd();
}

ICQ::Word ICQ::Buffer::bytesAvailable() const
{
	return m_Buffer.bytesAvailable();
}

void ICQ::Buffer::close()
{
	m_Buffer.close();
}

QByteArray ICQ::Buffer::data() const
{
	return m_Buffer.data();
}

ICQ::Byte ICQ::Buffer::getByte()
{
	Byte data;
	m_Buffer.read( (char *)&data, sizeof(Byte) );

	return data;
}

QByteArray ICQ::Buffer::getBlock(Word blockSize)
{
	return m_Buffer.read(blockSize);
}

ICQ::Word ICQ::Buffer::getWord()
{
	Word data;
	m_Buffer.read( (char *)&data, sizeof(Word) );
	data = qFromBigEndian(data);

	return data;
}

ICQ::DWord ICQ::Buffer::getDWord()
{
	DWord data;
	m_Buffer.read( (char *)&data, sizeof(DWord) );
	data = qFromBigEndian(data);

	return data;
}

ICQ::Word ICQ::Buffer::getLEWord()
{
	Word data;
	m_Buffer.read( (char *)&data, sizeof(Word) );

	return data;
}

ICQ::DWord ICQ::Buffer::getLEDWord()
{
	DWord data;
	m_Buffer.read( (char *)&data, sizeof(DWord) );

	return data;
}

void ICQ::Buffer::open()
{
	if ( m_Buffer.isOpen() ) {
		m_Buffer.close();
	}
	m_Buffer.open(QBuffer::ReadWrite);
}

ICQ::Word ICQ::Buffer::pos() const
{
	return m_Buffer.pos();
}

QByteArray ICQ::Buffer::read(Word maxSize)
{
	return m_Buffer.read(maxSize);
}

QByteArray ICQ::Buffer::readAll()
{
	return m_Buffer.readAll();
}

bool ICQ::Buffer::seek(Word pos)
{
	return m_Buffer.seek(pos);
}

void ICQ::Buffer::seekEnd()
{
	m_Buffer.seek( m_Buffer.size() - 1 );
}

bool ICQ::Buffer::seekForward(Word count)
{
	return m_Buffer.seek( m_Buffer.pos() + count );
}

bool ICQ::Buffer::seekBackward(Word count)
{
	return m_Buffer.seek( m_Buffer.pos() - count );
}

void ICQ::Buffer::setData(const QByteArray& data)
{
	m_Buffer.close();
	m_Buffer.setData(data);
	m_Buffer.open(QBuffer::ReadWrite);
}

ICQ::Word ICQ::Buffer::size() const
{
	return m_Buffer.size();
}

ICQ::Buffer::operator QByteArray() const
{
	return data();
}

ICQ::Buffer& ICQ::Buffer::operator=(const Buffer& other)
{
	setData( other.m_Buffer.data() );
	return *this;
}

ICQ::Buffer& ICQ::Buffer::operator=(const QByteArray& data)
{
	setData(data);
	return *this;
}
