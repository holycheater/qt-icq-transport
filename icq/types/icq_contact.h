/**
 * icq_contact.h - ICQ SSI list item.
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

#ifndef ICQCONTACT_H_
#define ICQCONTACT_H_

#include "icq_tlvchain.h"

#include <QString>

namespace ICQ
{


class Contact
{
	public:
		enum ItemType { Buddy = 0x0000, Group = 0x0001, Visible = 0x0002, Invisible = 0x0003, PermitDeny = 0x0004,
			Presence = 0x0005, Ignore = 0x000E, SelfIcon = 0x0013 };

		Contact();
		Contact(const QString& name, Word groupId, Word itemId, Word type, const TlvChain& data);
		~Contact();

		QString name() const;
		Word groupId() const;
		Word id() const;
		Word type() const;
		const TlvChain& tlvChain() const;

		void setName(const QString& name);
		void setGroupId(Word id);
		void setItemId(Word id);
		void setType(Word type);
		void setTlvChain(const TlvChain& chain);

		bool awaitingAuth() const;
		void setAwaitingAuth(bool awaitingAuth);

		/* get displayName for contact item (taken from tlv 0x0131 for buddy type item) */
		QString displayName() const;
		/* set display name. updated after ssi list change */
		void setDisplayName(const QString& name);

		bool operator==(const Contact& other) const;
		operator QByteArray() const;
	private:
		QString m_name;
		Word m_groupId;
		Word m_itemId;
		Word m_type;
		TlvChain m_data;
};

}

#endif /* ICQCONTACT_H_ */
