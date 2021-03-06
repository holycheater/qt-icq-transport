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
#include <QTextCodec>
#include <QTextStream>
#include <QTimer>

enum { PermOk, PermErrIsDir, PermErrDir, PermErrFile };

static int checkFilePermissions(const QString& fileName)
{
    QFileInfo info(fileName);

    if ( info.exists() ) {
        if ( info.isDir() ) {
            return PermErrIsDir;
        }
        if ( !info.isReadable() || !info.isWritable() ) {
            return PermErrFile;
        }
    } else {
        QFileInfo parent = QFileInfo( info.absolutePath() );
        if ( !parent.isExecutable() || !parent.isReadable() || !parent.isWritable() ) {
            return PermErrDir;
        }
    }
    return PermOk;
}

TransportMain::TransportMain(int& argc, char **argv)
    : QCoreApplication(argc, argv)
{
    m_options = new Options;
    m_options->parseCommandLine();

    m_gateway = 0;
    m_connection = 0;
    m_logfile = 0;
    m_transport = 0;

    m_runmode = Sandbox;
    if ( m_options->getOption("fork") == "yes" ) {
        m_runmode = Transport;
    }
    if ( m_runmode == Transport ) {
        setup_transport();
    } else {
        /* Check if it is just a process for daemonization, then create a sandbox */
        if ( m_options->hasOption("daemonize") ) {
            QString appFile = applicationFilePath();
            QStringList args = arguments();
            args.removeFirst();
            args.removeOne("-daemonize");

            QProcess *sandbox = new QProcess(this);
            if ( sandbox->startDetached(appFile, args) ) {
                ::exit(0);
            } else {
                qCritical("Failed to start child process");
                ::exit(1);
            }
        }
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
    if ( m_runmode == Transport ) {
        Q_ASSERT(m_gateway != 0);
        m_gateway->processShutdown();
    } else if ( m_runmode == Sandbox ) {
        QFile(m_options->getOption("pid-file")).remove();
        if ( m_transport ) {
            qWarning("Sandbox: terminating child process");
            m_transport->terminate();
        }
    }
}

void TransportMain::createPidFile()
{
    QString pidfile = m_options->getOption("pid-file");
    QFile fPid(pidfile);
    if ( fPid.exists() ) {
        qWarning("Warning. Pid-file '%s' already exists. Overwriting", qPrintable(pidfile));
    }
    fPid.open(QIODevice::WriteOnly);
    fPid.write( QByteArray::number(applicationPid(), 10) );
    fPid.write("\n");
    fPid.close();
}

void TransportMain::setup_sandbox()
{
    QString logfile = m_options->getOption("log-file");
    switch ( checkFilePermissions(logfile) ) {
        case PermErrIsDir:
            qCritical("Log-file '%s' is a directory", qPrintable(logfile));
            abort();
            break;
        case PermErrFile:
            qCritical("Process doesn't have permissions to read/write log-file: %s",
                      qPrintable(logfile));
            abort();
            break;
        case PermErrDir:
            qCritical("Process doesn't have permissions to read/write/execute log-directory: %s",
                      qPrintable( QFileInfo(logfile).absolutePath() ));
            abort();
            break;
        case PermOk:
        default:
            break;
    }

    QString pidfile = m_options->getOption("pid-file");
    if ( pidfile.isEmpty() ) {
        qCritical("Pid-file is not specified");
        abort();
    }
    switch ( checkFilePermissions(pidfile) ) {
        case PermErrIsDir:
            qCritical("Pid-file '%s' is a directory", qPrintable(pidfile));
            abort();
            break;
        case PermErrFile:
            qCritical("Process doesn't have permissions to read/write pid-file: %s",
                      qPrintable(pidfile));
            abort();
            break;
        case PermErrDir:
            qCritical("Process doesn't have permissions to read/write/execute pid-directory: %s",
                      qPrintable( QFileInfo(pidfile).absolutePath() ));
            abort();
            break;
        case PermOk:
        default:
            break;
    }

    if ( !QSqlDatabase::drivers().contains("QSQLITE") ) {
        fprintf( stderr, "Your Qt installation doesn't have the sqlite database driver" );
        abort();
    }

    QString dbfile = m_options->getOption("database");
    switch ( checkFilePermissions(dbfile) ) {
        case PermErrIsDir:
            qCritical("DB-file '%s' is a directory", qPrintable(dbfile));
            abort();
            break;
        case PermErrFile:
            qCritical("Process doesn't have permissions to read/write db-file: %s",
                      qPrintable(dbfile));
            abort();
            break;
        case PermErrDir:
            qCritical("Process doesn't have permissions to read/write/execute db-directory: %s",
                      qPrintable( QFileInfo(dbfile).absolutePath() ));
            abort();
            break;
        case PermOk:
        default:
            break;
    }

    m_logfile = new QFile(logfile);
    m_logfile->open(QIODevice::Append);
    qInstallMsgHandler(loghandler);

    /* TODO: config variables check */

    createPidFile();
    startForkedTransport();
}

void TransportMain::startForkedTransport()
{
    QString appFile = applicationFilePath();
    QStringList args = arguments();
    args.removeFirst(); // app name
    args << "-fork";

    Q_ASSERT( m_transport == 0);
    m_transport = new QProcess(this);
    QObject::connect( m_transport, SIGNAL(error(QProcess::ProcessError)),
                      SLOT(processTransportError(QProcess::ProcessError)) );
    QObject::connect( m_transport, SIGNAL(finished(int,QProcess::ExitStatus)),
                      SLOT(processTransportFinished(int,QProcess::ExitStatus)) );
    QObject::connect( m_transport, SIGNAL(started()),
                      SLOT(processTransportStarted()) );

    m_transport->start(appFile, args);
}

void TransportMain::setup_transport()
{
    m_logfile = new QFile(m_options->getOption("log-file"), this);
    m_logfile->open(QIODevice::Append);
    qInstallMsgHandler(loghandler);

    m_gateway = new GatewayTask(this);
    m_connection = new JabberConnection(this);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName( m_options->getOption("database") );
    if ( !db.isOpen() && !db.open() ) {
        qFatal("Failed to open the database");
    }

    m_gateway->setIcqServer( m_options->getOption("icq-server"),
                             m_options->getOption("icq-port").toUInt() );

    m_connection->setUsername( m_options->getOption("jabber-domain") );
    m_connection->setServer( m_options->getOption("jabber-server"),
                             m_options->getOption("jabber-port").toUInt() );
    m_connection->setPassword( m_options->getOption("jabber-secret") );

    connect_signals();
    m_connection->login();
}

void TransportMain::connect_signals()
{
    QObject::connect( m_connection, SIGNAL(connected()),
                      m_gateway, SLOT(processGatewayOnline()) );
    QObject::connect( m_connection, SIGNAL(userRegistered(XMPP::Jid,QString,QString)),
                      m_gateway, SLOT(processRegister(XMPP::Jid,QString,QString)) );
    QObject::connect( m_connection, SIGNAL(userUnregistered(XMPP::Jid)),
                      m_gateway, SLOT(processUnregister(XMPP::Jid)) );
    QObject::connect( m_connection, SIGNAL(userOnline(XMPP::Jid,int)),
                      m_gateway, SLOT(processUserOnline(XMPP::Jid,int)) );
    QObject::connect( m_connection, SIGNAL(userOffline(XMPP::Jid)),
                      m_gateway, SLOT(processUserOffline(XMPP::Jid)) );
    QObject::connect( m_connection, SIGNAL(userOnlineStatusRequest(XMPP::Jid)),
                      m_gateway, SLOT(processUserStatusRequest(XMPP::Jid)) );
    QObject::connect( m_connection, SIGNAL(userAdd(XMPP::Jid,QString)),
                      m_gateway, SLOT(processSubscribeRequest(XMPP::Jid,QString)) );
    QObject::connect( m_connection, SIGNAL(userDel(XMPP::Jid,QString)),
                      m_gateway, SLOT(processUnsubscribeRequest(XMPP::Jid,QString)) );
    QObject::connect( m_connection, SIGNAL(userAuthGrant(XMPP::Jid,QString)),
                      m_gateway, SLOT(processAuthGrant(XMPP::Jid,QString)) );
    QObject::connect( m_connection, SIGNAL(userAuthDeny(XMPP::Jid,QString)),
                      m_gateway, SLOT(processAuthDeny(XMPP::Jid,QString)) );
    QObject::connect( m_connection, SIGNAL(vCardRequest(XMPP::Jid,QString,QString)),
                      m_gateway, SLOT(processVCardRequest(XMPP::Jid,QString,QString)) );
    QObject::connect( m_connection, SIGNAL(outgoingMessage(XMPP::Jid,QString,QString)),
                      m_gateway, SLOT(processSendMessage(XMPP::Jid,QString,QString)) );
    QObject::connect( m_connection, SIGNAL(cmd_RosterRequest(XMPP::Jid)),
                      m_gateway, SLOT(processCmd_RosterRequest(XMPP::Jid)) );

    QObject::connect( m_gateway, SIGNAL(subscriptionReceived(XMPP::Jid,QString,QString)),
                      m_connection, SLOT(sendSubscribed(XMPP::Jid,QString,QString)) );
    QObject::connect( m_gateway, SIGNAL(subscriptionRemoved(XMPP::Jid,QString)),
                      m_connection, SLOT(sendUnsubscribed(XMPP::Jid,QString)) );
    QObject::connect( m_gateway, SIGNAL(subscriptionRequest(XMPP::Jid,QString)),
                      m_connection, SLOT(sendSubscribe(XMPP::Jid,QString)) );
    QObject::connect( m_gateway, SIGNAL(contactOnline(XMPP::Jid,QString,int,QString)),
                      m_connection, SLOT(sendOnlinePresence(XMPP::Jid,QString, int,QString)) );
    QObject::connect( m_gateway, SIGNAL(contactOffline(XMPP::Jid,QString)),
                      m_connection, SLOT(sendOfflinePresence(XMPP::Jid,QString)) );
    QObject::connect( m_gateway, SIGNAL(onlineNotifyFor(XMPP::Jid,int)),
                      m_connection, SLOT(sendOnlinePresence(XMPP::Jid,int)) );
    QObject::connect( m_gateway, SIGNAL(offlineNotifyFor(XMPP::Jid)),
                      m_connection, SLOT(sendOfflinePresence(XMPP::Jid)) );
    QObject::connect( m_gateway, SIGNAL(probeRequest(XMPP::Jid)),
                      m_connection, SLOT(sendPresenceProbe(XMPP::Jid) )  );
    QObject::connect( m_gateway, SIGNAL(incomingVCard(XMPP::Jid,QString,QString,XMPP::vCard)),
                      m_connection, SLOT(sendVCard(XMPP::Jid,QString,QString,XMPP::vCard)) );
    QObject::connect( m_gateway, SIGNAL(incomingMessage(XMPP::Jid,QString,QString,QString)),
                          m_connection, SLOT(sendMessage(XMPP::Jid,QString,QString,QString)) );
    QObject::connect( m_gateway, SIGNAL(incomingMessage(XMPP::Jid,QString,QString,QString,QDateTime)),
                          m_connection, SLOT(sendMessage(XMPP::Jid,QString,QString,QString,QDateTime)) );
    QObject::connect( m_gateway, SIGNAL(gatewayMessage(XMPP::Jid,QString)),
                      m_connection, SLOT(sendMessage(XMPP::Jid,QString)) );
    QObject::connect( m_gateway, SIGNAL(rosterAdd(XMPP::Jid,QList<XMPP::RosterXItem>)),
                      m_connection, SLOT(slotRosterAdd(XMPP::Jid,QList<XMPP::RosterXItem>)) );
}

void TransportMain::sighandler(int param)
{
    Q_UNUSED(param);

    TransportMain *app = qobject_cast<TransportMain*>(instance());
    Q_ASSERT( app != 0 );

    QTimer::singleShot( 0, app, SLOT(shutdown()) );
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

    QTextStream s(logfile);
    s.setCodec("LATIN1");
    s << "[" << QDateTime::currentDateTime().toString(Qt::ISODate) << "] " << msgType << " " << msg << "\n";
    if ( type == QtFatalMsg ) {
        abort();
    }
}

void TransportMain::processTransportError(QProcess::ProcessError error)
{
    Q_UNUSED(error);
    qCritical( "Sandbox: Transport process error: %s",
               qPrintable(m_transport->errorString()) );
}

void TransportMain::processTransportFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qWarning("Sandbox: Transport process finished (exit code: %d)", exitCode);
    m_transport->deleteLater();
    m_transport = 0;

    if ( exitStatus != QProcess::NormalExit || exitCode != 0 ) {
        qDebug("Sandbox: Crashed transport will be restarted in 30 seconds");
        QTimer::singleShot( 30000, this, SLOT(startForkedTransport()) );
    }
}

void TransportMain::processTransportStarted()
{
    qDebug("Sandbox: Transport process started");
}

// vim:et:ts=4:sw=4:nowrap
