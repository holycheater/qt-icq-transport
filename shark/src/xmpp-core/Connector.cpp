/*
 * Connector.cpp - establish a connection to an XMPP server
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

#include <QHostAddress>
#include <QPointer>
#include <QList>
#include <QUrl>

#include "Connector.h"

#include <QtCrypto>

#include "safedelete.h"
/* libidn */
#include <idna.h>

#include "ndns.h"
#include "srvresolver.h"

#include "cutestuff/bsocket.h"
#include "cutestuff/httpconnect.h"
#include "cutestuff/httppoll.h"
#include "cutestuff/socks.h"

using namespace XMPP;

enum { Idle, Connecting, Connected };

class Connector::Private
{
	public:
		/* from connector's private */
		bool useSSL;
		bool haveaddr;
		QHostAddress addr;
		quint16 port;

		int mode;
		ByteStream *bs;
		NDns dns;
		SrvResolver srv;

		QString server;
		QString opt_host;
		int opt_port;
		bool opt_probe, opt_ssl;
		Proxy proxy;

		QString host;
		QList<Q3Dns::Server> servers;
		int errorCode;

		bool multi, using_srv;
		bool will_be_ssl;
		int probe_mode;

		bool aaaa;
		SafeDelete sd;
};

//----------------------------------------------------------------------------
// Connector
//----------------------------------------------------------------------------
Connector::Connector(QObject *parent)
	: QObject(parent)
{
	d = new Private;

	setUseSSL(false);
	setPeerAddressNone();

	d->bs = 0;
	connect(&d->dns, SIGNAL(resultsReady()), SLOT(dns_done()));
	connect(&d->srv, SIGNAL(resultsReady()), SLOT(srv_done()));
	d->opt_probe = false;
	d->opt_ssl = false;
	cleanup();
	d->errorCode = 0;

}

Connector::~Connector()
{
	cleanup();
	delete d;
}

bool Connector::useSSL() const
{
	return d->useSSL;
}

bool Connector::havePeerAddress() const
{
	return d->haveaddr;
}

QHostAddress Connector::peerAddress() const
{
	return d->addr;
}

quint16 Connector::peerPort() const
{
	return d->port;
}

void Connector::setUseSSL(bool useSSL)
{
	d->useSSL = useSSL;
}

void Connector::setPeerAddressNone()
{
	d->haveaddr = false;
	d->addr = QHostAddress();
	d->port = 0;
}

void Connector::setPeerAddress(const QHostAddress& address, quint16 port)
{
	d->haveaddr = true;
	d->addr = address;
	d->port = port;
}


void Connector::cleanup()
{
	d->mode = Idle;

	// stop any dns
	if ( d->dns.isBusy() ) {
		d->dns.stop();
	}
	if ( d->srv.isBusy() ) {
		d->srv.stop();
	}

	// destroy the bytestream, if there is one
	delete d->bs;
	d->bs = 0;

	d->multi = false;
	d->using_srv = false;
	d->will_be_ssl = false;
	d->probe_mode = -1;

	setUseSSL(false);
	setPeerAddressNone();
}

void Connector::setProxy(const Proxy& proxy)
{
	if (d->mode != Idle) {
		return;
	}
	d->proxy = proxy;
}

void Connector::setOptHostPort(const QString& host, quint16 port)
{
	if (d->mode != Idle) {
		return;
	}
	d->opt_host = host;
	d->opt_port = port;
}

void Connector::setOptProbe(bool b)
{
	if(d->mode != Idle)
		return;
	d->opt_probe = b;
}

void Connector::setOptSSL(bool b)
{
	if(d->mode != Idle)
		return;
	d->opt_ssl = b;
}

void Connector::connectToServer(const QString &server)
{
	if(d->mode != Idle)
		return;
	if(server.isEmpty())
		return;

	d->errorCode = 0;
	d->mode = Connecting;
	d->aaaa = true;

	// Encode the servername
	d->server = QUrl::toAce(server);

	if(d->proxy.type() == Proxy::HttpPoll) {

		HttpPoll *s = new HttpPoll;
		d->bs = s;
		connect(s, SIGNAL( connected() ), SLOT( bs_connected() ));
		connect(s, SIGNAL( syncStarted() ), SIGNAL( httpSyncStarted() ));
		connect(s, SIGNAL( syncFinished() ), SIGNAL( httpSyncFinished() ));
		connect(s, SIGNAL( error(int) ), SLOT( bs_error(int) ));
		if(!d->proxy.user().isEmpty())
			s->setAuth(d->proxy.user(), d->proxy.pass());
		s->setPollInterval(d->proxy.pollInterval());

		if(d->proxy.host().isEmpty())
			s->connectToUrl(d->proxy.url());
		else
			s->connectToHost(d->proxy.host(), d->proxy.port(), d->proxy.url());
	}
	else if (d->proxy.type() == Proxy::HttpConnect) {
		if(!d->opt_host.isEmpty()) {
			d->host = d->opt_host;
			d->port = d->opt_port;
		}
		else {
			d->host = server;
			d->port = 5222;
		}
		do_connect();
	}
	else {
		if(!d->opt_host.isEmpty()) {
			d->host = d->opt_host;
			d->port = d->opt_port;
			do_resolve();
		}
		else {
			d->multi = true;

			QPointer<QObject> self = this;
			emit srvLookup(d->server);
			if(!self)
				return;

			d->srv.resolveSrvOnly(d->server, "xmpp-client", "tcp");
		}
	}
}

void Connector::changePollInterval(int secs)
{
	if(d->bs && (d->bs->inherits("XMPP::HttpPoll") || d->bs->inherits("HttpPoll"))) {
		HttpPoll *s = static_cast<HttpPoll*>(d->bs);
		s->setPollInterval(secs);
	}
}

ByteStream* Connector::stream() const
{
	if(d->mode == Connected)
		return d->bs;
	else
		return 0;
}

void Connector::done()
{
	cleanup();
}

int Connector::errorCode() const
{
	return d->errorCode;
}

void Connector::do_resolve()
{
	d->dns.resolve(d->host);
}

void Connector::dns_done()
{
	bool failed = false;
	QHostAddress addr;

	if(d->dns.result().isNull ())
		failed = true;
	else
		addr = QHostAddress(d->dns.result());

	if(failed) {
#ifdef XMPP_DEBUG
		printf("dns1\n");
#endif
		// using proxy?  then try the unresolved host through the proxy
		if(d->proxy.type() != Proxy::None) {
#ifdef XMPP_DEBUG
			printf("dns1.1\n");
#endif
			do_connect();
		}
		else if(d->using_srv) {
#ifdef XMPP_DEBUG
			printf("dns1.2\n");
#endif
			if(d->servers.isEmpty()) {
#ifdef XMPP_DEBUG
				printf("dns1.2.1\n");
#endif
				cleanup();
				d->errorCode = ErrConnectionRefused;
				emit error();
			}
			else {
#ifdef XMPP_DEBUG
				printf("dns1.2.2\n");
#endif
				tryNextSrv();
				return;
			}
		}
		else {
#ifdef XMPP_DEBUG
			printf("dns1.3\n");
#endif
			cleanup();
			d->errorCode = ErrHostNotFound;
			emit error();
		}
	}
	else {
#ifdef XMPP_DEBUG
		printf("dns2\n");
#endif
		d->host = addr.toString();
		do_connect();
	}
}

void Connector::do_connect()
{
#ifdef XMPP_DEBUG
	qDebug() << "trying host" << d->host << "port" << d->port;
#endif
	int t = d->proxy.type();
	if(t == Proxy::None) {
#ifdef XMPP_DEBUG
		printf("do_connect1\n");
#endif
		BSocket *s = new BSocket;
		d->bs = s;
		connect(s, SIGNAL(connected()), SLOT(bs_connected()));
		connect(s, SIGNAL(error(int)), SLOT(bs_error(int)));
		s->connectToHost(d->host, d->port);
	}
	else if(t == Proxy::HttpConnect) {
#ifdef XMPP_DEBUG
		printf("do_connect2\n");
#endif
		HttpConnect *s = new HttpConnect;
		d->bs = s;
		connect(s, SIGNAL(connected()), SLOT(bs_connected()));
		connect(s, SIGNAL(error(int)), SLOT(bs_error(int)));
		if(!d->proxy.user().isEmpty())
			s->setAuth(d->proxy.user(), d->proxy.pass());
		s->connectToHost(d->proxy.host(), d->proxy.port(), d->host, d->port);
	}
	else if(t == Proxy::Socks) {
#ifdef XMPP_DEBUG
		printf("do_connect3\n");
#endif
		SocksClient *s = new SocksClient;
		d->bs = s;
		connect(s, SIGNAL(connected()), SLOT(bs_connected()));
		connect(s, SIGNAL(error(int)), SLOT(bs_error(int)));
		if(!d->proxy.user().isEmpty())
			s->setAuth(d->proxy.user(), d->proxy.pass());
		s->connectToHost(d->proxy.host(), d->proxy.port(), d->host, d->port);
	}
}

void Connector::tryNextSrv()
{
#ifdef XMPP_DEBUG
	printf("trying next srv\n");
#endif
	Q_ASSERT(!d->servers.isEmpty());
	d->host = d->servers.first().name;
	d->port = d->servers.first().port;
	d->servers.takeFirst();
	do_resolve();
}

void Connector::srv_done()
{
	QPointer<QObject> self = this;
#ifdef XMPP_DEBUG
	printf("srv_done1\n");
#endif
	d->servers = d->srv.servers();
	if(d->servers.isEmpty()) {
		emit srvResult(false);
		if(!self)
			return;

#ifdef XMPP_DEBUG
		printf("srv_done1.1\n");
#endif
		// fall back to A record
		d->using_srv = false;
		d->host = d->server;
		if(d->opt_probe) {
#ifdef XMPP_DEBUG
			printf("srv_done1.1.1\n");
#endif
			d->probe_mode = 0;
			d->port = 5223;
			d->will_be_ssl = true;
		}
		else {
#ifdef XMPP_DEBUG
			printf("srv_done1.1.2\n");
#endif
			d->probe_mode = 1;
			d->port = 5222;
		}
		do_resolve();
		return;
	}

	emit srvResult(true);
	if(!self)
		return;

	d->using_srv = true;
	tryNextSrv();
}

void Connector::bs_connected()
{
	if(d->proxy.type() == Proxy::None) {
		QHostAddress h = (static_cast<BSocket*>(d->bs))->peerAddress();
		int p = (static_cast<BSocket*>(d->bs))->peerPort();
		setPeerAddress(h, p);
	}

	// only allow ssl override if proxy==poll or host:port
	if((d->proxy.type() == Proxy::HttpPoll || !d->opt_host.isEmpty()) && d->opt_ssl)
		setUseSSL(true);
	else if(d->will_be_ssl)
		setUseSSL(true);

	d->mode = Connected;
	emit connected();
}

void Connector::bs_error(int x)
{
	if(d->mode == Connected) {
		d->errorCode = ErrStream;
		emit error();
		return;
	}

	bool proxyError = false;
	int err = ErrConnectionRefused;
	int t = d->proxy.type();

#ifdef XMPP_DEBUG
	printf("bse1\n");
#endif

	// figure out the error
	if(t == Proxy::None) {
		if(x == BSocket::ErrHostNotFound)
			err = ErrHostNotFound;
		else
			err = ErrConnectionRefused;
	}
	else if(t == Proxy::HttpConnect) {
		if(x == HttpConnect::ErrConnectionRefused)
			err = ErrConnectionRefused;
		else if(x == HttpConnect::ErrHostNotFound)
			err = ErrHostNotFound;
		else {
			proxyError = true;
			if(x == HttpConnect::ErrProxyAuth)
				err = ErrProxyAuth;
			else if(x == HttpConnect::ErrProxyNeg)
				err = ErrProxyNeg;
			else
				err = ErrProxyConnect;
		}
	}
	else if(t == Proxy::HttpPoll) {
		if(x == HttpPoll::ErrConnectionRefused)
			err = ErrConnectionRefused;
		else if(x == HttpPoll::ErrHostNotFound)
			err = ErrHostNotFound;
		else {
			proxyError = true;
			if(x == HttpPoll::ErrProxyAuth)
				err = ErrProxyAuth;
			else if(x == HttpPoll::ErrProxyNeg)
				err = ErrProxyNeg;
			else
				err = ErrProxyConnect;
		}
	}
	else if(t == Proxy::Socks) {
		if(x == SocksClient::ErrConnectionRefused)
			err = ErrConnectionRefused;
		else if(x == SocksClient::ErrHostNotFound)
			err = ErrHostNotFound;
		else {
			proxyError = true;
			if(x == SocksClient::ErrProxyAuth)
				err = ErrProxyAuth;
			else if(x == SocksClient::ErrProxyNeg)
				err = ErrProxyNeg;
			else
				err = ErrProxyConnect;
		}
	}

	// no-multi or proxy error means we quit
	if(!d->multi || proxyError) {
		cleanup();
		d->errorCode = err;
		emit error();
		return;
	}

	if(d->using_srv && !d->servers.isEmpty()) {
#ifdef XMPP_DEBUG
		printf("bse1.1\n");
#endif
		tryNextSrv();
	}
	else if(!d->using_srv && d->opt_probe && d->probe_mode == 0) {
#ifdef XMPP_DEBUG
		printf("bse1.2\n");
#endif
		d->probe_mode = 1;
		d->port = 5222;
		d->will_be_ssl = false;
		do_connect();
	}
	else {
#ifdef XMPP_DEBUG
		printf("bse1.3\n");
#endif
		cleanup();
		d->errorCode = ErrConnectionRefused;
		emit error();
	}
}

//----------------------------------------------------------------------------
// Connector::Proxy
//----------------------------------------------------------------------------
Connector::Proxy::Proxy()
{
	m_type = None;
	m_poll = 30;
}

Connector::Proxy::~Proxy()
{
}

int Connector::Proxy::type() const
{
	return m_type;
}

QString Connector::Proxy::host() const
{
	return m_host;
}

quint16 Connector::Proxy::port() const
{
	return m_port;
}

QString Connector::Proxy::url() const
{
	return m_url;
}

QString Connector::Proxy::user() const
{
	return m_username;
}

QString Connector::Proxy::pass() const
{
	return m_password;
}

int Connector::Proxy::pollInterval() const
{
	return m_poll;
}

void Connector::Proxy::setHttpConnect(const QString &host, quint16 port)
{
	m_type = HttpConnect;
	m_host = host;
	m_port = port;
}

void Connector::Proxy::setHttpPoll(const QString &host, quint16 port, const QString &url)
{
	m_type = HttpPoll;
	m_host = host;
	m_port = port;
	m_url  = url;
}

void Connector::Proxy::setSocks(const QString &host, quint16 port)
{
	m_type = Socks;
	m_host = host;
	m_port = port;
}

void Connector::Proxy::setUserPass(const QString &user, const QString &pass)
{
	m_username = user;
	m_password = pass;
}

void Connector::Proxy::setPollInterval(int secs)
{
	m_poll = secs;
}

