/**
 * main.cpp - application entry point
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

#include <QCoreApplication>
#include <QString>
#include <QtCrypto>
#include <QtDebug>

#include "im.h"
#include "xmpp.h"

class IrisExample: public QObject
{
	Q_OBJECT

	public:
		IrisExample(QObject *parent = 0);
		~IrisExample();

		void goAuthenticate();

	private:
		XMPP::Jid m_jid;
		XMPP::AdvancedConnector* m_pConn;
		XMPP::ClientStream* m_pStream;
		XMPP::Client* m_pClient;
		QString m_pass;
	private slots:
		void stream_warning(int warn);
		void stream_error(int err);
		void stream_authenticated();
		void stream_needAuthParams(bool user, bool passwd, bool);
};

IrisExample::IrisExample(QObject *parent)
	: QObject(parent)
{
	m_jid = "icq.dragonfly/dragon";
	m_pass = "jaba";

	// Connector
	m_pConn = new XMPP::AdvancedConnector;

	// Stream
	m_pStream = new XMPP::ClientStream(m_pConn);

	// Use XMPP .9
	//m_pStream->setOldOnly( true );

	QObject::connect( m_pStream, SIGNAL( warning( int ) ), SLOT( stream_warning( int ) ) );

	QObject::connect( m_pStream, SIGNAL( error( int ) ), SLOT( stream_error( int ) ) );

	QObject::connect( m_pStream, SIGNAL( needAuthParams( bool, bool, bool ) ), SLOT( stream_needAuthParams( bool, bool, bool ) ) );

	QObject::connect( m_pStream, SIGNAL( authenticated() ), SLOT( stream_authenticated() ) );

	// Client
	m_pClient = new XMPP::Client( m_pStream );
}

IrisExample::~IrisExample()
{
	delete m_pClient;
	delete m_pStream;
	delete m_pConn;
}

void IrisExample::goAuthenticate()
{
	m_pClient->connectToServer( m_pStream, m_jid );
}

void IrisExample::stream_warning(int warn)
{
	Q_UNUSED(warn)
	// We will get two warnings ... no TLS and pre-1.0 XMPP stream
	// neither of which we care about...
	m_pStream->continueAfterWarning();
}

void IrisExample::stream_error( int err )
{
	qDebug() << "D'oh - a stream error occurred! Code: " << err;

	QCoreApplication::instance()->quit();
}

void IrisExample::stream_authenticated()
{
	m_pClient->start( m_jid.host(), m_jid.user(), m_pass, m_jid.resource() );

	qDebug() << "Authenticated on server - sweet!";

	// our work here is done...
	QCoreApplication::instance()->quit();
}

void IrisExample::stream_needAuthParams( bool user, bool passwd, bool )
{

	if (user) {
		m_pStream->setUsername( m_jid.node() );
	}

	if (passwd) {
		m_pStream->setPassword( m_pass );
	}

	qDebug() << "Sending auth params ...";
	m_pStream->continueAfterParams();

}

int main(int argc, char **argv)
{
	QCA::Initializer init;

	QCoreApplication app( argc, argv );

	IrisExample irisExample;

	irisExample.goAuthenticate();

	app.exec();

	return 0;
}

// dirty trick allows us to have Q_OBJECT definitions in our .cpp's...
#include "main.moc"
