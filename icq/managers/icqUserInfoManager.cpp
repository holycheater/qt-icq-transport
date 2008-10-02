/*
 * icqUserInfoManager.cpp - ICQ User Info Manager.
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

#include "icqUserInfoManager.h"
#include "icqConnection.h"

#include "types/icqSnacBuffer.h"
#include "types/icqUserInfo.h"

#include <QHash>
#include <QtDebug>

namespace ICQ {


class UserInfoManager::Private {
	public:
		QHash<QString, UserInfo> userInfoList;
		QHash<QString, Word> statusList;
		UserInfo ownInfo;
		Connection *link;
};

UserInfoManager::UserInfoManager(Connection *parent)
	: QObject(parent)
{
	d = new Private;
	d->link = parent;

	QObject::connect( d->link, SIGNAL( incomingSnac(SnacBuffer&) ), SLOT( incomingSnac(SnacBuffer&) ) );

	QObject::connect( this, SIGNAL( statusChanged(int) ), d->link, SIGNAL( statusChanged(int) ) );
	QObject::connect( this, SIGNAL( userOnline(QString,quint16) ), d->link, SIGNAL( userOnline(QString,quint16) ) );
	QObject::connect( this, SIGNAL( userOffline(QString) ), d->link, SIGNAL( userOffline(QString) ) );
}

UserInfoManager::~UserInfoManager()
{
}

UserInfo UserInfoManager::getUserInfo(const QString& uin)
{
	return d->userInfoList.value(uin);
}

quint16 UserInfoManager::getUserStatus(const QString& uin) const
{
	if ( d->statusList.contains(uin) ) {
		return d->statusList.value(uin);
	}
	return UserInfo::Offline;
}

void UserInfoManager::handle_own_user_info(SnacBuffer& snac)
{
	UserInfo info = UserInfo::fromBuffer(snac);
	if ( info.hasTlv(0x06) && info.onlineStatus() != d->ownInfo.onlineStatus() ) {
		emit statusChanged( info.onlineStatus() );
	}
	d->ownInfo.mergeFrom(info);
}

/* << SNAC(03,0B) - SRV_USER_ONLINE */
void UserInfoManager::handle_user_online_notification(SnacBuffer& snac)
{
	while ( ! snac.atEnd() ) {
		UserInfo info = UserInfo::fromBuffer(snac);
		if ( d->userInfoList.contains( info.userId() ) ) {
			UserInfo existing = d->userInfoList.take( info.userId() );
			existing.mergeFrom(info);
			d->userInfoList.insert(existing.userId(), existing);
			d->statusList.insert( existing.userId(), existing.onlineStatus() );
		} else {
			d->userInfoList.insert(info.userId(), info);
			d->statusList.insert( info.userId(), info.onlineStatus() );
		}

		/* hack: emit single status for userOnline signal instead a group of flags
		 * Anyways, it's a mystery why ICQ needs flags for online status. You can't be Occuppied and DND at the same time */
		quint16 singleStatus;
		singleStatus = UserInfo::Online;
		if ( info.onlineStatus() & UserInfo::Invisible ) {
			singleStatus = UserInfo::Invisible;
		} else if ( info.onlineStatus() & UserInfo::Away ) {
			singleStatus = UserInfo::Away;
			if ( info.onlineStatus() & UserInfo::NotAvailable ) {
				singleStatus = UserInfo::NotAvailable;
			} else if ( info.onlineStatus() & UserInfo::Occupied ) {
				singleStatus = UserInfo::Occupied;
				if ( info.onlineStatus() & UserInfo::DoNotDisturb ) {
					singleStatus = UserInfo::DoNotDisturb;
				}
			}
		} else if ( info.onlineStatus() & UserInfo::FreeForChat ) {
			singleStatus = UserInfo::FreeForChat;
		} else if ( info.onlineStatus() & UserInfo::Evil ) {
			singleStatus = UserInfo::Evil;
		} else if ( info.onlineStatus() & UserInfo::Depression ) {
			singleStatus = UserInfo::Depression;
		}
		// qDebug() << info.onlineStatus() << "vs" << singleStatus;
		emit userOnline( info.userId(), singleStatus );
	}
}

/* << SNAC(03,0C) - SRV_USER_OFFLINE */
void UserInfoManager::handle_user_offline_notification(SnacBuffer& snac)
{
	while ( ! snac.atEnd() ) {
		UserInfo info = UserInfo::fromBuffer(snac);
		emit userOffline( info.userId() );
	}
}

void UserInfoManager::incomingSnac(SnacBuffer& snac)
{
	if ( snac.family() == 0x01 && snac.subtype() == 0x0F ) {
		handle_own_user_info(snac);
	} else if ( snac.family() == 0x03 ) {
		if ( snac.subtype() == 0x0B ) {
			handle_user_online_notification(snac);
		} else if ( snac.subtype() == 0x0C ) {
			handle_user_offline_notification(snac);
		}
	}
}


} /* end of namespace ICQ */
