/*
 * icqGuid.cpp - 16-byte UUID generator for ICQ capabilities.
 * Copyright (C) 2002-2005  Kopete developers <kopete-devel@kde.org>
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

#include "icqGuid.h"

#define GUID_SIZE 16

namespace ICQ
{


/**
 * Constructs an empty GUID object.
 */
Guid::Guid()
{
	m_data = QByteArray(GUID_SIZE, '\0');
}

/**
 * Constructs GUID from raw data. It takes first 16 bytes of @a raw.
 */
Guid Guid::fromRawData(const char* raw)
{
	QByteArray ba;
	ba.resize(GUID_SIZE);
	qMemCopy(ba.data(), raw, GUID_SIZE);

	Guid guid;
	guid.m_data = ba;
	return guid;
}

/**
 * Constructs GUID from raw data.
 */
Guid Guid::fromRawData(const QByteArray& raw)
{
	Guid guid;
	guid.m_data = raw;

	return guid;
}

/**
 * Constructs guid from GUID string.
 */
Guid Guid::fromString(const QString& guidstr)
{
	Guid guid;

	QString d(guidstr);
	/* strip dashes */
	d.remove('-');

	guid.m_data.clear();
	for ( int i = 0; i < 32; i += 2 ) {
		guid.m_data += d.mid(i, 2).toShort(NULL, GUID_SIZE);
	}

	return guid;
}

/**
 * Returns raw GUID data.
 */
QByteArray Guid::data() const
{
	return m_data;
}

/**
 * Returns true if first @a n bytes of this guid are equal with @a rhs.
 */
bool Guid::isEqual(const Guid& rhs, int n) const
{
	if( n > GUID_SIZE ) {
		n = GUID_SIZE;
	}
	return m_data.left(n) == rhs.m_data.left(n);
}

/**
 * Returns true if GUID is a valid identifier.
 */
bool Guid::isValid() const
{
	return m_data.size() == GUID_SIZE;
}

/**
 * Returns true if GUID is filled with zero-bytes.
 */
bool Guid::isZero() const
{
	for (int i = 0; i < m_data.length(); i++) {
		if ( m_data.at(i) != '\0' ) {
			return false;
		}
	}
	return true;
}

/**
 * Sets guid data to @a data.
 */
void Guid::setData(const QByteArray& data)
{
	m_data = data;
}

/**
 * Returns string representation of GUID.
 */
QString Guid::toString() const
{
	QString uuid = m_data.toHex().toUpper();

	uuid.insert(21, '-');
	uuid.insert(17, '-');
	uuid.insert(13, '-');
	uuid.insert(9, '-');

	return uuid;
}

/**
 * Sets GUID data to @a data.
 */
Guid& Guid::operator=(const QByteArray& data)
{
	m_data = data;
	return *this;
}

/**
 * Returns true if this guid and @a rhs are equal.
 */
bool Guid::operator==(const Guid& rhs) const
{
	return m_data == rhs.m_data;
}

/**
 * Returns GUID data.
 *
 * @sa data()
 */
Guid::operator QByteArray() const
{
	return m_data;
}


} /* end of namespace ICQ */
