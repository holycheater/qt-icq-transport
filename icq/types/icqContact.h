/*
 * icqContact.h - ICQ SSI list item.
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

#ifndef ICQCONTACT_H_
#define ICQCONTACT_H_

#include "icqTypes.h"

#include <QString>
#include <QSharedDataPointer>

namespace ICQ
{


class TlvChain;

class Contact
{
	public:
		enum ItemType { Buddy = 0x0000, Group = 0x0001, Visible = 0x0002, Invisible = 0x0003, PermitDeny = 0x0004,
			Presence = 0x0005, Ignore = 0x000E, SelfIcon = 0x0013, Deleted = 0x0019 };

		Contact();
		Contact(const Contact& other);
		Contact(const QString& name, Word groupId, Word itemId, Word type, const TlvChain& data);
		~Contact();

		bool isValid() const;

		QString name() const;
		Word groupId() const;
		Word id() const;
		Word type() const;
		QList<Word> childs() const;
		TlvChain tlvChain() const;

		void setName(const QString& name);
		void setGroupId(Word id);
		void setItemId(Word id);
		void setType(Word type);
		void setChilds(const QList<Word>& childs);
		void setTlvChain(const TlvChain& chain);

		bool awaitingAuth() const;
		void setAwaitingAuth(bool awaitingAuth);

		/* get displayName for contact item (taken from tlv 0x0131 for buddy type item) */
		QString displayName() const;
		/* set display name. updated after ssi list change */
		void setDisplayName(const QString& name);

		Contact& operator=(const Contact& other);
		bool operator==(const Contact& other) const;
		operator QByteArray() const;
	private:
		class Private;
		QSharedDataPointer<Private> d;
};

}

#endif /* ICQCONTACT_H_ */
