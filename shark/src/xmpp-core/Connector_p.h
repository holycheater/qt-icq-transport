/*
 * Connector_p.h - establish a connection to an XMPP server (Private implementation)
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

#ifndef XMPP_CORE_CONNECTOR_P_H_
#define XMPP_CORE_CONNECTOR_P_H_

#include "Connector.h"

#include <QObject>
#include <QHostAddress>
#include <QHostInfo>
#include <QPointer>
#include <QTcpSocket>
#include <QTimer>

namespace XMPP {


class Connector::Private : public QObject
{
	Q_OBJECT

	public:
		enum Mode { Idle, Connecting, Connected };

		Private(Connector *parent = 0);
		~Private();
		void beginConnect();
		void reset();
	public slots:
		void processLookupResult(const QHostInfo& host);
		void processLookupTimeout();
		void processConnectionTimeout();
		void processSocketError(QAbstractSocket::SocketError errcode);
	public:
		Connector *q;

		int mode;

		QString host;
		quint16 port;

		QHostAddress addr;
		QPointer<QTcpSocket> socket;
		QTimer *lookupTimer, *connectTimer;

		int lookupTimeout, connectionTimeout;
		int lookupID;
};


} /* end of namespace XMPP */

// vim:ts=4:sw=4:noet:nowrap
#endif /* XMPP_CORE_CONNECTOR_P_H_ */
