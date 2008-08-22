/**
 * icq_tlvchain.cpp - ICQ TLV (type-length-value) Chain.
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

#include "icq_tlvchain.h"

ICQ::TlvChain::TlvChain()
{
}

ICQ::TlvChain::TlvChain(const Buffer& data)
{
	*this = data;
}

ICQ::TlvChain::TlvChain(const QByteArray& data)
{
	*this = Buffer(data);
}

ICQ::TlvChain& ICQ::TlvChain::addTlv(const Tlv& tlv)
{
	if ( m_tlvList.contains( tlv.type() ) ) {
		m_tlvList.remove( tlv.type() );
	}
	m_tlvList.insert(tlv.type(), tlv);
	return *this;
}

ICQ::TlvChain& ICQ::TlvChain::addTlv(Word type, const QByteArray& data)
{
	if ( m_tlvList.contains(type) ) {
		m_tlvList.remove(type);
	}
	m_tlvList.insert(type, Tlv(type, data) );
	return *this;
}

ICQ::Tlv& ICQ::TlvChain::addTlv(Word type)
{
	if ( m_tlvList.contains(type) ) {
		m_tlvList.remove(type);
	}
	return *(m_tlvList.insert( type, Tlv(type) ));
}

QByteArray ICQ::TlvChain::data() const
{
	QByteArray tlvChain;
	QHash<Word, Tlv>::const_iterator it = m_tlvList.constBegin();
	while ( it != m_tlvList.constEnd() ) {
		tlvChain += it->data();
		it++;
	}
	return tlvChain;
}

ICQ::Tlv ICQ::TlvChain::getTlv(Word type) const
{
	return m_tlvList.value(type);
}

QByteArray ICQ::TlvChain::getTlvData(Word type) const
{
	return getTlv(type).m_Buffer.data();
}

bool ICQ::TlvChain::hasTlv(Word type) const
{
	return m_tlvList.contains(type);
}

const QHash<ICQ::Word, ICQ::Tlv>& ICQ::TlvChain::list() const
{
	return m_tlvList;
}

void ICQ::TlvChain::removeTlv(Word type)
{
	m_tlvList.remove(type);
}

ICQ::TlvChain& ICQ::TlvChain::operator=(const Buffer& buffer)
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

ICQ::TlvChain& ICQ::TlvChain::operator=(const QByteArray& data)
{
	operator=( ICQ::Buffer(data) );
	return *this;
}

ICQ::TlvChain& ICQ::TlvChain::operator<<(const Tlv& tlv)
{
	addTlv(tlv);
	return *this;
}

ICQ::TlvChain& ICQ::TlvChain::operator<<(const QByteArray& data)
{
	*this << Tlv(data);
	return *this;
}

ICQ::TlvChain& ICQ::TlvChain::operator<<(const TlvChain& other)
{
	QHash<Word, Tlv>::const_iterator it = other.m_tlvList.constBegin(), itEnd = other.m_tlvList.constEnd();
	while ( it != itEnd ) {
		*this << *it;
		++it;
	}
	return *this;
}
