/*
 * Connector.h - establish a connection to an XMPP server
 * Copyright (C) 2003  Justin Karneges
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

#ifndef XMPP_CONNECTOR_H_
#define XMPP_CONNECTOR_H_

#include <QObject>

class QTcpSocket;
class QHostInfo;

namespace XMPP {


class Connector : public QObject
{
	Q_OBJECT

	public:
		static const int LOOKUP_TIMEOUT = 30000;
		static const int CONNECT_TIMEOUT = 30000;

		enum ErrorType { EHostLookupTimeout, EHostLookupFailed, EConnectionTimeout, ESocketError };

		Connector(QObject *parent = 0);
		virtual ~Connector();

		QTcpSocket* socket() const;

		void connectToServer(const QString& server);

		void setOptHostPort(const QString& host, quint16 port);

		void setLookupTimeout(int timeout);
		void setConnectionTimeout(int timeout);
	signals:
		void connected();
		void error(Connector::ErrorType errcode);
	private:
		class Private;
		Private *d;
};


} /* end of namespace XMPP */

#endif /* XMPP_CONNECTOR_H_ */
