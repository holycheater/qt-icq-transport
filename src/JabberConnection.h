/**
 * JabberConnection.h - Jabber connection handler class
 * Copyright (C) 2008  Alexander Saltykov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 **/

#ifndef JABBERCONNECTION_H_
#define JABBERCONNECTION_H_

#include <QObject>

class JabberConnection : public QObject
{
	Q_OBJECT

	public:
		JabberConnection(QObject *parent = 0);
		~JabberConnection();

		void login();

		void setUsername(const QString& username);
		void setServer(const QString& host, quint16 port = 5222);
		void setPassword(const QString& password);
	private slots:
		void stream_error(int err);
		void stream_authenticated();
		void stream_needAuthParams(bool user, bool passwd, bool);
	private:
		class Private;
		Private *d;
};

#endif /* JABBERCONNECTION_H_ */
