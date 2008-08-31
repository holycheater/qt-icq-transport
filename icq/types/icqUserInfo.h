/*
 * icqUserInfo.h - ICQ User Info block.
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

#ifndef ICQUSERINFO_H_
#define ICQUSERINFO_H_

#include "icqTypes.h"
#include "icqBuffer.h"
#include "icqTlv.h"
#include "icqGuid.h"

#include <QDateTime>
#include <QList>
#include <QString>
#include <QSharedDataPointer>

namespace ICQ
{


class UserInfo
{
	public:
		UserInfo();
		UserInfo(const UserInfo& other);
		UserInfo& operator=(const UserInfo& other);
		~UserInfo();

		/* read UserInfo from the part of a packet buffer */
		static UserInfo fromBuffer(Buffer& buffer);

		/* merge from another UserInfo */
		void mergeFrom(const UserInfo& info);

		/* update user info from TLV block */
		void updateFromTlv(Tlv& tlv);

		QString userId() const;
		DWord classFlags() const;
		QDateTime signOnTime() const;
		QDateTime registerTime() const;

		Word onlineStatus() const;
		Word statusFlags() const;

		QString externalIP() const;
		QString internalIP() const;

		DWord dcPort() const;
		Byte dcType() const;
		Word dcVersion() const;
		DWord dcAuthCookie() const;

		DWord clientFeatures() const;
		QDateTime lastInfoUpdateTime() const;

		Word idleTime() const;

		const QList<Guid> capabilities() const;

		bool hasCapability(Guid capability) const;
		/* check for capability by Id (as defined in ICQ.h ICQ::Capability enum) */
		bool hasCapability(int capId) const;

		bool hasTlv(Word tlvType) const;
	private:
		class Private;
		QSharedDataPointer<Private> d;
};

}

#endif /* ICQUSERINFO_H_ */
