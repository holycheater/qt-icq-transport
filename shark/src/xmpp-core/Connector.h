/*
 * Connector.h - establish a connection to an XMPP server
 * Copyright (C) 2003  Justin Karneges
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

class ByteStream;
class QHostAddress;

namespace XMPP {


class Connector : public QObject
{
	Q_OBJECT

	public:
		enum Error { ErrConnectionRefused, ErrHostNotFound, ErrProxyConnect, ErrProxyNeg, ErrProxyAuth, ErrStream };

		Connector(QObject *parent = 0);
		virtual ~Connector();

		class Proxy;

		void connectToServer(const QString& server);
		ByteStream *stream() const;
		void done();


		bool useSSL() const;
		bool havePeerAddress() const;
		QHostAddress peerAddress() const;
		quint16 peerPort() const;

		void setProxy(const Proxy& proxy);
		void setOptHostPort(const QString& host, quint16 port);
		void setOptProbe(bool);
		void setOptSSL(bool);

		void changePollInterval(int secs);

		int errorCode() const;

	signals:
		void connected();
		void error();
		void srvLookup(const QString &server);
		void srvResult(bool success);
		void httpSyncStarted();
		void httpSyncFinished();

	protected:
		void setUseSSL(bool b);
		void setPeerAddressNone();
		void setPeerAddress(const QHostAddress& addr, quint16 port);
	private:
		void cleanup();
		void do_resolve();
		void do_connect();
		void tryNextSrv();
	private slots:
		void dns_done();
		void srv_done();
		void bs_connected();
		void bs_error(int);
	private:
		class Private;
		Private *d;
};

class Connector::Proxy
{
	public:
		enum { None, Socks };
		Proxy();
		~Proxy();

		int type() const;
		QString host() const;
		quint16 port() const;
		QString url() const;
		QString user() const;
		QString pass() const;
		int pollInterval() const;

		void setSocks(const QString& host, quint16 port);
		void setUserPass(const QString& user, const QString& pass);
	private:
		int m_type;
		QString m_host, m_url;
		quint16 m_port;
		QString m_username, m_password;
		int m_poll;
};




} /* end of namespace XMPP */

#endif /* XMPP_CONNECTOR_H_ */
