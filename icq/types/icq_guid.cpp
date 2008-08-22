/**
 * icq_guid.cpp - 16-byte UUID generator for ICQ capabilities.
 * Copyright (C) 2002-2005  Kopete developers <kopete-devel@kde.org>
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

#include "icq_guid.h"

#include <QtDebug>

ICQ::Guid::Guid()
{
	m_data = QByteArray(16, '\0');
}

ICQ::Guid::Guid(const char* data)
{
	initData( QString(data) );
}

ICQ::Guid::Guid(const QByteArray& data)
	: m_data(data)
{
	initData( QString(data) );
}

ICQ::Guid::Guid(const QString& data)
{
	initData(data);
}

void ICQ::Guid::initData(const QString& data)
{
	//get rid of the const
	QString d(data);
	//strip out dashes
	d.remove('-');
	//get each of the 16 2-char bytes
	for ( int i = 0; i < 32; i += 2 ) {
		m_data += d.mid(i, 2).toShort(NULL, 16);
	}
}

const QByteArray ICQ::Guid::data() const
{
	return m_data;
}

void ICQ::Guid::setData(const QByteArray& data)
{
	m_data = data;
}

bool ICQ::Guid::isZero() const
{
	for (int i = 0; i < m_data.length(); i++) {
		if ( m_data.at(i) != '\0' ) {
			return false;
		}
	}
	return true;
}

bool ICQ::Guid::isValid() const
{
	return m_data.size() == 16;
}

bool ICQ::Guid::isEqual(const Guid& rhs, int n) const
{
	if( n > 16 ) {
		n = 16;
	}
	return m_data.left(n) == rhs.m_data.left(n);
}

const QString ICQ::Guid::toString() const
{
	QString uuid = m_data.toHex().toUpper();

	uuid.insert(21, '-');
	uuid.insert(17, '-');
	uuid.insert(13, '-');
	uuid.insert(9, '-');

	return uuid;
}

ICQ::Guid& ICQ::Guid::operator=(const QByteArray& data)
{
	m_data = data;
	return *this;
}

bool ICQ::Guid::operator==(const Guid& rhs) const
{
	return m_data == rhs.m_data;
}

ICQ::Guid::operator QByteArray() const
{
	return m_data;
}
