/*
 * icqContact.cpp - ICQ SSI list item.
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

#include "icqContact.h"

namespace ICQ
{


Contact::Contact()
{
	m_groupId = 0;
	m_itemId = 0;
	m_type = 0xFFFF;
}

Contact::Contact(const QString& name, Word groupId, Word itemId, Word type, const TlvChain& data)
{
	m_name = name;
	m_groupId = groupId;
	m_itemId = itemId;
	m_type = type;
	m_data = data;
}

Contact::~Contact()
{
}

QString Contact::name() const
{
	return m_name;
}

Word Contact::groupId() const
{
	return m_groupId;
}

Word Contact::id() const
{
	return m_itemId;
}

Word Contact::type() const
{
	return m_type;
}

const TlvChain& Contact::tlvChain() const
{
	return m_data;
}

void Contact::setName(const QString& name)
{
	m_name = name;
}

void Contact::setGroupId(Word id)
{
	m_groupId = id;
}

void Contact::setItemId(Word id)
{
	m_itemId = id;
}

void Contact::setType(Word type)
{
	m_type = type;
}

void Contact::setTlvChain(const TlvChain& chain)
{
	m_data = chain;
}

bool Contact::awaitingAuth() const
{
	return m_data.hasTlv(0x0066);
}

void Contact::setAwaitingAuth(bool awaitingAuth)
{
	if ( awaitingAuth == false ) {
		m_data.removeTlv(0x0066);
	} else if ( !m_data.hasTlv(0x0066) ) {
		m_data.addTlv(0x0066);
	}
}

QString Contact::displayName() const
{
	if ( m_type == 0 ) {
		return m_data.getTlvData(0x0131);
	}
	return m_name;
}

void Contact::setDisplayName(const QString& name)
{
	if ( m_type == 0 ) {
		m_data.addTlv(0x0131).addData(name);
	}
}

bool Contact::operator==(const Contact& other) const
{
	if ( m_name == other.m_name && m_groupId == other.m_groupId && m_itemId == other.m_itemId && m_type == other.m_type ) {
		return true;
	}
	return false;
}

Contact::operator QByteArray() const
{
	Buffer result;

	result.addWord( m_name.length() );
	result.addData(m_name);
	result.addWord(m_groupId);
	result.addWord(m_itemId);
	result.addWord(m_type);

	QByteArray data = m_data.data();
	result.addWord( data.length() );
	result.addData(data);

	return result;
}


} /* end of namespace ICQ */
