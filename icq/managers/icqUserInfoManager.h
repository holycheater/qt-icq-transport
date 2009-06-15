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

#include "types/icqTypes.h"

class QTextCodec;

namespace ICQ
{

class Buffer;
class SnacBuffer;
class Socket;
class UserInfo;
class ShortUserDetails;
class UserDetails;

class UserInfoManager : public QObject
{
	Q_OBJECT

	public:
		UserInfoManager(Socket *socket, QObject *parent = 0);
		~UserInfoManager();

		void setTextCodec(QTextCodec *codec);

		UserInfo getUserInfo(const QString& uin);
		quint16 getUserStatus(const QString& uin) const;

		void requestOwnUserDetails(const QString& uin);
		void requestUserDetails(const QString& uin);
		void requestShortDetails(const QString& uin);

		ShortUserDetails shorUserDetails(const QString& uin) const;
		UserDetails userDetails(const QString& uin) const;

		void clearShortUserDetails(const QString& uin);
		void clearUserDetails(const QString& uin);
	signals:
		void statusChanged(int status);
		void userOnline(QString userId, int status);
		void userOffline(QString userId);

		void shortUserDetailsAvailable(const QString& uin);
		void userDetailsAvailable(const QString& uin);
	private slots:
		void incomingMetaInfo(Word type, Buffer& data);
		void incomingSnac(SnacBuffer& snac);
	private:
		class Private;
		Private *d;
};

}

// vim:ts=4:sw=4:noet:nowrap
#endif /* ICQUSERINFOMANAGER_H_ */
