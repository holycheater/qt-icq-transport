/*
 * icqLoginManager.h - login manager for an icq connection
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

#ifndef ICQLOGINMANAGER_H_
#define ICQLOGINMANAGER_H_

#include "icqConnection.h"

#include <QByteArray>
#include <QString>
#include <QObject>

namespace ICQ
{


class LoginManager: public QObject
{
	Q_OBJECT

	public:
		LoginManager(Connection *parent);
		~LoginManager();
		void login(QString& uin, QString& password, QString& server);
	signals:
		void loginFinished();
	private:
		void recv_flap_version(FlapBuffer& reply);
		void send_flap_version();
		void send_cli_auth_request();
		void recv_auth_key(SnacBuffer& reply);
		void recv_auth_reply(SnacBuffer& reply);
		void send_cli_auth_cookie();
		void recv_snac_list(SnacBuffer& reply);
		void recv_snac_versions(SnacBuffer& reply);
		void recv_location_services_limits(SnacBuffer& reply);
		void recv_buddy_list_parameters(SnacBuffer& reply);
		void recv_icbm_parameters(SnacBuffer& reply);
		void recv_privacy_parameters(SnacBuffer& reply);
		void login_final_actions();
		QByteArray md5password(const QByteArray& AuthKey);
	private slots:
		void incomingFlap(FlapBuffer& flap);
		void incomingSnac(SnacBuffer& snac);
	private:
		class Private;
		Private *d;
};

}

#endif /*ICQLOGINMANAGER_H_*/
