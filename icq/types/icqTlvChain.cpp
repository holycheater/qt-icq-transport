/*
 * icqTlvChain.cpp - ICQ TLV (type-length-value) Chain.
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

#include "icqTlvChain.h"

namespace ICQ
{


TlvChain::TlvChain()
{
}

TlvChain::TlvChain(const Buffer& data)
{
	*this = data;
}

TlvChain::TlvChain(const QByteArray& data)
{
	*this = Buffer(data);
}

TlvChain& TlvChain::addTlv(const Tlv& tlv)
{
	if ( m_tlvList.contains( tlv.type() ) ) {
		m_tlvList.remove( tlv.type() );
	}
	m_tlvList.insert(tlv.type(), tlv);
	return *this;
}

TlvChain& TlvChain::addTlv(Word type, const QByteArray& data)
{
	if ( m_tlvList.contains(type) ) {
		m_tlvList.remove(type);
	}
	m_tlvList.insert(type, Tlv(type, data) );
	return *this;
}

Tlv& TlvChain::addTlv(Word type)
{
	if ( m_tlvList.contains(type) ) {
		m_tlvList.remove(type);
	}
	return *(m_tlvList.insert( type, Tlv(type) ));
}

QByteArray TlvChain::data() const
{
	QByteArray tlvChain;
	QHash<Word, Tlv>::const_iterator it = m_tlvList.constBegin();
	while ( it != m_tlvList.constEnd() ) {
		tlvChain += it->data();
		it++;
	}
	return tlvChain;
}

Tlv TlvChain::getTlv(Word type) const
{
	return m_tlvList.value(type);
}

QByteArray TlvChain::getTlvData(Word type) const
{
	return getTlv(type).m_Buffer.data();
}

bool TlvChain::hasTlv(Word type) const
{
	return m_tlvList.contains(type);
}

const QHash<Word, Tlv>& TlvChain::list() const
{
	return m_tlvList;
}

void TlvChain::removeTlv(Word type)
{
	m_tlvList.remove(type);
}

TlvChain& TlvChain::operator=(const Buffer& buffer)
{
	Buffer tmpBuffer = buffer;
	while ( tmpBuffer.bytesAvailable() > 0 ) {
		Word type = tmpBuffer.getWord();
		Word length = tmpBuffer.getWord();
		Tlv tlv( type, tmpBuffer.read(length) );
		m_tlvList.insert(type, tlv);
	}
	return *this;
}

TlvChain& TlvChain::operator=(const QByteArray& data)
{
	operator=( Buffer(data) );
	return *this;
}

TlvChain& TlvChain::operator<<(const Tlv& tlv)
{
	addTlv(tlv);
	return *this;
}

TlvChain& TlvChain::operator<<(const QByteArray& data)
{
	*this << Tlv(data);
	return *this;
}

TlvChain& TlvChain::operator<<(const TlvChain& other)
{
	QHash<Word, Tlv>::const_iterator it = other.m_tlvList.constBegin(), itEnd = other.m_tlvList.constEnd();
	while ( it != itEnd ) {
		*this << *it;
		++it;
	}
	return *this;
}


} /* end of namespace ICQ */
