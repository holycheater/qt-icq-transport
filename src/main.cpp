/*
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
 */

#include "GatewayTask.h"
#include "JabberConnection.h"
#include "Options.h"

#include <QCoreApplication>
#include <QTimer>
#include <QtSql>

#include <signal.h>

static GatewayTask *gw_ptr = 0;

#define PROCESS_EVENTS_MAX_TIME 60000

void sighandler(int param)
{
	Q_UNUSED(param)

	if ( gw_ptr ) {
		QTimer::singleShot( 0, gw_ptr, SLOT( processShutdown() ) );
	}
	QCoreApplication::processEvents(QEventLoop::AllEvents, PROCESS_EVENTS_MAX_TIME);
	exit(0);
}

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);

	Options *options = new Options;

	options->parseCommandLine();

	if ( !QSqlDatabase::drivers().contains("QSQLITE") ) {
		qCritical("Your Qt installation doesn't have the sqlite database driver");
		exit(1);
	}

	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
	db.setDatabaseName( options->getOption("database") );

	GatewayTask gw;
	gw.setDatabaseLink(db);
	gw.setIcqServer( options->getOption("icq-server"), options->getOption("icq-port").toUInt() );
	gw_ptr = &gw;

	JabberConnection conn;
	conn.setUsername( options->getOption("jabber-user") );
	conn.setServer( options->getOption("jabber-server"), options->getOption("jabber-port").toUInt() );
	conn.setPassword( options->getOption("jabber-secret") );

	delete options;

	QObject::connect( &conn, SIGNAL( connected() ),                             &gw, SLOT( processGatewayOnline() ) );
	QObject::connect( &conn, SIGNAL( userRegistered(QString,QString,QString) ), &gw, SLOT( processRegister(QString,QString,QString) ) );
	QObject::connect( &conn, SIGNAL( userUnregistered(QString) ),               &gw, SLOT( processUnregister(QString) ) );
	QObject::connect( &conn, SIGNAL( userOnline(Jid,int) ),                     &gw, SLOT( processUserOnline(Jid,int) ) );
	QObject::connect( &conn, SIGNAL( userOffline(Jid) ),                        &gw, SLOT( processUserOffline(Jid) ) );
	QObject::connect( &conn, SIGNAL( userAdd(Jid,QString) ),                    &gw, SLOT( processSubscribeRequest(Jid,QString) ) );
	QObject::connect( &conn, SIGNAL( userDel(Jid,QString) ),                    &gw, SLOT( processUnsubscribeRequest(Jid,QString) ) );
	QObject::connect( &conn, SIGNAL( userAuthGrant(Jid,QString) ),              &gw, SLOT( processAuthGrant(Jid,QString) ) );
	QObject::connect( &conn, SIGNAL( userAuthDeny(Jid,QString) ),               &gw, SLOT( processAuthDeny(Jid,QString) ) );
	QObject::connect( &conn, SIGNAL( vCardRequest(Jid,QString,QString) ),       &gw, SLOT( processVCardRequest(Jid,QString,QString) ) );
	QObject::connect( &conn, SIGNAL( outgoingMessage(Jid,QString,QString) ),    &gw, SLOT( processSendMessage(Jid,QString,QString) ) );
	QObject::connect( &conn, SIGNAL( cmd_RosterRequest(Jid) ),                  &gw, SLOT( processCmd_RosterRequest(Jid) ) );

	QObject::connect( &gw, SIGNAL( subscriptionReceived(Jid,QString) ),        &conn, SLOT( sendSubscribed(Jid,QString) ) );
	QObject::connect( &gw, SIGNAL( subscriptionRemoved(Jid,QString) ),         &conn, SLOT( sendUnsubscribed(Jid,QString) ) );
	QObject::connect( &gw, SIGNAL( subscriptionRequest(Jid,QString) ),         &conn, SLOT( sendSubscribe(Jid,QString) ) );
	QObject::connect( &gw, SIGNAL( contactOnline(Jid,QString,int) ),           &conn, SLOT( sendOnlinePresence(Jid,QString, int) ) );
	QObject::connect( &gw, SIGNAL( contactOffline(Jid,QString) ),              &conn, SLOT( sendOfflinePresence(Jid,QString) ) );
	QObject::connect( &gw, SIGNAL( onlineNotifyFor(Jid,int) ),                 &conn, SLOT( sendOnlinePresence(Jid,int) ) );
	QObject::connect( &gw, SIGNAL( offlineNotifyFor(Jid) ),                    &conn, SLOT( sendOfflinePresence(Jid) ) );
	QObject::connect( &gw, SIGNAL( incomingVCard(Jid,QString,QString,vCard) ), &conn, SLOT( sendVCard(Jid,QString,QString,vCard) ) );
	QObject::connect( &gw, SIGNAL( incomingMessage(Jid,QString,QString) ),     &conn, SLOT( sendMessage(Jid,QString,QString) ) );
	QObject::connect( &gw, SIGNAL( gatewayMessage(Jid,QString) ),              &conn, SLOT( sendMessage(Jid,QString) ) );

	conn.login();

	/* try to catch terminate signals to send offline presence to users before quit */
	if ( signal(SIGKILL, sighandler) == SIG_IGN ) {
		signal(SIGKILL, SIG_IGN);
	}
	if ( signal(SIGINT, sighandler) == SIG_IGN ) {
		signal(SIGINT, SIG_IGN);
	}

	return app.exec();
}
