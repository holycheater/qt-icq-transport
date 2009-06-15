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
#include "icqTlvChain.h"

namespace ICQ
{


class Contact::Private : public QSharedData
{
	public:
		Private();
		Private(const Private& other);
		~Private();

		QString name;
		Word groupId;
		Word itemId;
		Word type;
		TlvChain data;
};

Contact::Private::Private()
	: QSharedData()
{
	groupId = 0;
	itemId = 0;
	type = 0xFFFF;
}
Contact::Private::Private(const Private& other)
	: QSharedData(other)
{
	name = other.name;
	groupId = other.groupId;
	itemId = other.itemId;
	type = other.type;
	data = other.data;
}

Contact::Private::~Private()
{
}

Contact::Contact()
	: d(new Private)
{
}

Contact::Contact(const Contact& other)
	: d(other.d)
{
}

Contact::Contact(const QString& name, Word groupId, Word itemId, Word type, const TlvChain& data)
	: d(new Private)
{
	d->name = name;
	d->groupId = groupId;
	d->itemId = itemId;
	d->type = type;
	d->data = data;
}

Contact::~Contact()
{
}

bool Contact::isValid() const
{
	return (d->type != 0xFFFF);
}

QString Contact::name() const
{
	return d->name;
}

Word Contact::groupId() const
{
	return d->groupId;
}

Word Contact::id() const
{
	return d->itemId;
}

Word Contact::type() const
{
	return d->type;
}

/**
 * Returns list of child item-ids for a group or child group-ids for master-group (TLV 0xC8).
 * @note Method works only for items which type is Group (0001), otherwise it does nothing.
 */
QList<Word> Contact::childs() const
{
	if ( d->type != Group ) {
		return QList<Word>();
	}
	QList<Word> childs;
	Tlv tlvChilds = d->data.getTlv(0xC8);
	while ( !tlvChilds.atEnd() ) {
		childs << tlvChilds.getWord();
	}
	return childs;
}

TlvChain Contact::tlvChain() const
{
	return d->data;
}

void Contact::setName(const QString& name)
{
	d->name = name;
}

void Contact::setGroupId(Word id)
{
	d->groupId = id;
}

void Contact::setItemId(Word id)
{
	d->itemId = id;
}

void Contact::setType(Word type)
{
	d->type = type;
}

/**
 * Sets list of child item-ids for a group or child group-ids for master-group (TLV 0xC8).
 * @note Method works only for items which type is Group (0001), otherwise it does nothing.
 */
void Contact::setChilds(const QList<Word>& childs)
{
	if ( d->type != Group ) {
		return;
	}
	Tlv tlv(0xC8);

	QListIterator<Word> i(childs);
	while ( i.hasNext() ) {
		tlv.addWord( i.next() );
	}

	d->data.addTlv(tlv);
}

void Contact::setTlvChain(const TlvChain& chain)
{
	d->data = chain;
}

bool Contact::awaitingAuth() const
{
	return d->data.hasTlv(0x0066);
}

void Contact::setAwaitingAuth(bool awaitingAuth)
{
	if ( awaitingAuth == false ) {
		d->data.removeTlv(0x0066);
	} else if ( !d->data.hasTlv(0x0066) ) {
		d->data.addTlv(0x0066);
	}
}

QString Contact::displayName() const
{
	if ( d->type == Buddy ) {
		return d->data.getTlvData(0x0131);
	}
	return d->name;
}

void Contact::setDisplayName(const QString& name)
{
	if ( d->type == 0 ) {
		d->data.addTlv(0x0131).addData(name);
	}
}

Contact& Contact::operator=(const Contact& other)
{
	d = other.d;
	return *this;
}

bool Contact::operator==(const Contact& other) const
{
	if ( d->name == other.d->name && d->groupId == other.d->groupId && d->itemId == other.d->itemId && d->type == other.d->type ) {
		return true;
	}
	return false;
}

Contact::operator QByteArray() const
{
	Buffer result;

	result.addWord( d->name.length() );
	result.addData(d->name);
	result.addWord(d->groupId);
	result.addWord(d->itemId);
	result.addWord(d->type);

	QByteArray data = d->data.data();
	result.addWord( data.length() );
	result.addData(data);

	return result;
}


} /* end of namespace ICQ */

// vim:sw=4:ts=4:noet:nowrap
