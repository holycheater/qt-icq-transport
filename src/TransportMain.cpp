/*
 * TransportMain.cpp - Application class
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
 */

#include "TransportMain.h"
#include "GatewayTask.h"
#include "JabberConnection.h"
#include "Options.h"

#include <signal.h>
#include <stdlib.h>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSqlDatabase>
#include <QStringList>
#include <QTextStream>
#include <QTimer>

TransportMain::TransportMain(int& argc, char **argv)
	: QCoreApplication(argc, argv)
{
	m_options = new Options;
	m_options->parseCommandLine();

	m_gateway = 0;
	m_connection = 0;
	m_logfile = 0;

	bool isFork = false;
	if ( m_options->getOption("fork") == "yes" ) {
		isFork = true;
	}
	if ( isFork ) {
		// qDebug() << "This is a fork. Start as transport.";
		setup_transport();
	} else {
		// qDebug() << "This is not a fork. Start as sandbox.";
		setup_sandbox();
	}

	/* try to catch terminate signals to send offline presence to users before quit */
	if ( signal(SIGTERM, sighandler) == SIG_IGN ) {
		signal(SIGTERM, SIG_IGN);
	}
	if ( signal(SIGINT, sighandler) == SIG_IGN ) {
		signal(SIGINT, SIG_IGN);
	}
}

TransportMain::~TransportMain()
{
	delete m_options;
	delete m_gateway;
	delete m_connection;
	delete m_logfile;
}

void TransportMain::shutdown()
{
	if ( !m_gateway ) {
		return;
	}
	m_gateway->processShutdown();
}

void TransportMain::setup_sandbox()
{
	QString logfile = m_options->getOption("log-file");
	QFileInfo info(logfile);

	if ( info.exists() ) {
		if ( info.isDir() ) {
			fprintf( stderr, "Log-file \"%s\" is a directory.\n", qPrintable(logfile) );
			abort();
		}
		if ( !info.isReadable() || !info.isWritable() ) {
			fprintf( stderr, "Process doesn't have permissions to read/write log-file: %s.\n", qPrintable(logfile) );
			abort();
		}
	} else {
		QFileInfo parent = QFileInfo(info.absolutePath());
		if ( !parent.isExecutable() || !parent.isReadable() || !parent.isWritable() ) {
			fprintf( stderr, "Process doesn't have permissions to read/write/execute on log-directory: %s.\n", qPrintable(parent.absoluteFilePath()) );
			abort();
		}
	}
	// qDebug() << "[Sandbox] Log-file check passed.";

	if ( !QSqlDatabase::drivers().contains("QSQLITE") ) {
		fprintf( stderr, "Your Qt installation doesn't have the sqlite database driver" );
		abort();
	}
	// qDebug() << "[Sandbox] Sqlite driver check passed.";

	QString dbfile = m_options->getOption("database");
	if ( dbfile.isEmpty() ) {
		fprintf( stderr, "Database file not specified.\n" );
		abort();
	}
	QFileInfo fi(dbfile);
	if ( fi.exists() ) {
		if ( fi.isDir() ) {
			fprintf( stderr, "Database file \"%s\" is a directory.\n", qPrintable(dbfile) );
			abort();
		}
		if ( !fi.isReadable() || !fi.isWritable() ) {
			fprintf( stderr, "Process doesn't have permissions to read/write database file: %s.\n", qPrintable(dbfile) );
			abort();
		}
	} else {
		QFileInfo parent = QFileInfo(fi.absolutePath());
		if ( !parent.isExecutable() || !parent.isReadable() || !parent.isWritable() ) {
			fprintf( stderr, "Process doesn't have permissions to read/write/execute on database directory: %s.\n", qPrintable(parent.absoluteFilePath()) );
			abort();
		}
	}
	// qDebug() << "[Sandbox] Database file check passed.";

	m_logfile = new QFile(logfile);
	m_logfile->open(QIODevice::Append);

	/* TODO: config variables check */
	launchTransport();
}

void TransportMain::launchTransport()
{
	QString appFile = applicationFilePath();
	QStringList args = arguments();
	args.removeFirst(); // app name
	args << "-fork";

	QProcess *transport = new QProcess(this);
	QObject::connect( transport, SIGNAL( error(QProcess::ProcessError) ), SLOT( processTransportError(QProcess::ProcessError) ) );
	QObject::connect( transport, SIGNAL( finished(int,QProcess::ExitStatus) ), SLOT( processTransportFinished(int,QProcess::ExitStatus) ) );
	QObject::connect( transport, SIGNAL( started() ), SLOT( processTransportStarted() ) );

	transport->start(appFile, args);
}

void TransportMain::setup_transport()
{
	m_logfile = new QFile( m_options->getOption("log-file") );
	m_logfile->open(QIODevice::Append);
	qInstallMsgHandler(loghandler);

	m_gateway = new GatewayTask(this);
	m_connection = new JabberConnection(this);

	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
	db.setDatabaseName( m_options->getOption("database") );
	m_gateway->setDatabaseLink(db);

	m_gateway->setIcqServer( m_options->getOption("icq-server"), m_options->getOption("icq-port").toUInt() );

	m_connection->setUsername( m_options->getOption("jabber-domain") );
	m_connection->setServer( m_options->getOption("jabber-server"), m_options->getOption("jabber-port").toUInt() );
	m_connection->setPassword( m_options->getOption("jabber-secret") );

	connect_signals();
	m_connection->login();
}

void TransportMain::connect_signals()
{
	QObject::connect( m_connection, SIGNAL( connected() ),                             m_gateway, SLOT( processGatewayOnline() ) );
	QObject::connect( m_connection, SIGNAL( userRegistered(QString,QString,QString) ), m_gateway, SLOT( processRegister(QString,QString,QString) ) );
	QObject::connect( m_connection, SIGNAL( userUnregistered(QString) ),               m_gateway, SLOT( processUnregister(QString) ) );
	QObject::connect( m_connection, SIGNAL( userOnline(Jid,int,bool) ),                m_gateway, SLOT( processUserOnline(Jid,int,bool) ) );
	QObject::connect( m_connection, SIGNAL( userOffline(Jid) ),                        m_gateway, SLOT( processUserOffline(Jid) ) );
	QObject::connect( m_connection, SIGNAL( userOnlineStatusRequest(Jid) ),            m_gateway, SLOT( processUserStatusRequest(Jid) ) );
	QObject::connect( m_connection, SIGNAL( userAdd(Jid,QString) ),                    m_gateway, SLOT( processSubscribeRequest(Jid,QString) ) );
	QObject::connect( m_connection, SIGNAL( userDel(Jid,QString) ),                    m_gateway, SLOT( processUnsubscribeRequest(Jid,QString) ) );
	QObject::connect( m_connection, SIGNAL( userAuthGrant(Jid,QString) ),              m_gateway, SLOT( processAuthGrant(Jid,QString) ) );
	QObject::connect( m_connection, SIGNAL( userAuthDeny(Jid,QString) ),               m_gateway, SLOT( processAuthDeny(Jid,QString) ) );
	QObject::connect( m_connection, SIGNAL( vCardRequest(Jid,QString,QString) ),       m_gateway, SLOT( processVCardRequest(Jid,QString,QString) ) );
	QObject::connect( m_connection, SIGNAL( outgoingMessage(Jid,QString,QString) ),    m_gateway, SLOT( processSendMessage(Jid,QString,QString) ) );
	QObject::connect( m_connection, SIGNAL( cmd_RosterRequest(Jid) ),                  m_gateway, SLOT( processCmd_RosterRequest(Jid) ) );

	QObject::connect( m_gateway, SIGNAL( subscriptionReceived(Jid,QString,QString) ),    m_connection, SLOT( sendSubscribed(Jid,QString,QString) ) );
	QObject::connect( m_gateway, SIGNAL( subscriptionRemoved(Jid,QString) ),             m_connection, SLOT( sendUnsubscribed(Jid,QString) ) );
	QObject::connect( m_gateway, SIGNAL( subscriptionRequest(Jid,QString) ),             m_connection, SLOT( sendSubscribe(Jid,QString) ) );
	QObject::connect( m_gateway, SIGNAL( contactOnline(Jid,QString,int,QString) ),       m_connection, SLOT( sendOnlinePresence(Jid,QString, int,QString) ) );
	QObject::connect( m_gateway, SIGNAL( contactOffline(Jid,QString) ),                  m_connection, SLOT( sendOfflinePresence(Jid,QString) ) );
	QObject::connect( m_gateway, SIGNAL( onlineNotifyFor(Jid,int) ),                     m_connection, SLOT( sendOnlinePresence(Jid,int) ) );
	QObject::connect( m_gateway, SIGNAL( offlineNotifyFor(Jid) ),                        m_connection, SLOT( sendOfflinePresence(Jid) ) );
	QObject::connect( m_gateway, SIGNAL( probeRequest(Jid) ),                            m_connection, SLOT( sendPresenceProbe(Jid) )  );
	QObject::connect( m_gateway, SIGNAL( incomingVCard(Jid,QString,QString,vCard) ),     m_connection, SLOT( sendVCard(Jid,QString,QString,vCard) ) );
	QObject::connect( m_gateway, SIGNAL( incomingMessage(Jid,QString,QString,QString) ), m_connection, SLOT( sendMessage(Jid,QString,QString,QString) ) );
	QObject::connect( m_gateway, SIGNAL( gatewayMessage(Jid,QString) ),                  m_connection, SLOT( sendMessage(Jid,QString) ) );
}

void TransportMain::sighandler(int param)
{
	Q_UNUSED(param)

	TransportMain *app = qobject_cast<TransportMain*>(instance());
	Q_ASSERT( app != 0 );

	QTimer::singleShot( 0, app, SLOT( shutdown() ) );
	processEvents(QEventLoop::AllEvents, 60000);
	::exit(0);
}

void TransportMain::loghandler(QtMsgType type, const char *msg)
{
	QString msgType;
	switch ( type ) {
		case QtDebugMsg:
			msgType = "[DEBUG]";
			break;
		case QtWarningMsg:
			msgType = "[WARN]";
			break;
		case QtCriticalMsg:
			msgType = "[CRIT]";
			break;
		case QtFatalMsg:
			msgType = "[FATAL]";
		default:
			break;
	}

	TransportMain *app = qobject_cast<TransportMain*>(instance());
	QFile *logfile = app->m_logfile;
	Q_CHECK_PTR(logfile);

	QTextStream(logfile) << "[" << QDateTime::currentDateTime().toString(Qt::ISODate) << "] " << msgType << " " << msg << "\n";
	if ( type == QtFatalMsg ) {
		abort();
	}
}

void TransportMain::processTransportError(QProcess::ProcessError error)
{
	QTextStream(m_logfile) << "[" << QDateTime::currentDateTime().toString(Qt::ISODate) << "] " << "[Sandbox] " << "Transport error: " << error << "\n";
}

void TransportMain::processTransportFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	QTextStream(m_logfile) << "[" << QDateTime::currentDateTime().toString(Qt::ISODate) << "] " << "[Sandbox] " << "Transport finished (exit code: " << exitCode << ")" << "\n";

	QProcess *transport = qobject_cast<QProcess*>(sender());
	delete transport;

	if ( exitStatus != QProcess::NormalExit || exitCode != 0 ) {
		QTextStream(m_logfile) << "[" << QDateTime::currentDateTime().toString(Qt::ISODate) << "] " << "[Sandbox] " << "Transport will be restarted in 30 seconds" << "\n";
		QTimer::singleShot( 30000, this, SLOT( launchTransport() ) );
	}
}

void TransportMain::processTransportStarted()
{
	QTextStream(m_logfile) << "[" << QDateTime::currentDateTime().toString(Qt::ISODate) << "] " << "[Sandbox] " << "Transport started" << "\n";
}