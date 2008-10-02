/*
 * icqUserInfoManager.h - ICQ User Info Manager.
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

#ifndef ICQUSERINFOMANAGER_H_
#define ICQUSERINFOMANAGER_H_

#include <QObject>
#include <QString>

namespace ICQ
{

class Connection;
class SnacBuffer;
class UserInfo;

class UserInfoManager : public QObject
{
	Q_OBJECT

	public:
		UserInfoManager(Connection *parent);
		~UserInfoManager();

		UserInfo getUserInfo(const QString& uin);
		quint16 getUserStatus(const QString& uin) const;
	signals:
		void statusChanged(int status);
		void userOnline(QString userId, quint16 status);
		void userOffline(QString userId);
	private:
		void handle_own_user_info(SnacBuffer& snac); // SNAC(01,0F)
		void handle_user_online_notification(SnacBuffer& snac); // SNAC(03,0B)
		void handle_user_offline_notification(SnacBuffer& snac); // SNAC(03,0C)
	private slots:
		void incomingSnac(SnacBuffer& snac);
	private:
		class Private;
		Private *d;
};

}

#endif /* ICQUSERINFOMANAGER_H_ */
