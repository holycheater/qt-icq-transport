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

#include "JabberConnection.h"
#include "GatewayTask.h"

#include <QCoreApplication>
#include <QTimer>
#include <QtSql>
#include <QtDebug>

#include <csignal>

static GatewayTask *gw_ptr = 0;

#define PROCESS_EVENTS_MAX_TIME 60000

void sighandler(int param)
{
	if ( gw_ptr ) {
		QTimer::singleShot( 0, gw_ptr, SLOT( processShutdown() ) );
	}
	QCoreApplication::processEvents(QEventLoop::AllEvents, PROCESS_EVENTS_MAX_TIME);
	QCoreApplication::exit();
}

int main(int argc, char **argv)
{

	QCoreApplication app(argc, argv);

	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
	db.setDatabaseName(":memory:");

	db.open();
	QSqlQuery query;
	query.exec("CREATE TABLE IF NOT EXISTS users ("
				"jid TEXT,"
				"uin TEXT,"
				"password TEXT,"
				"PRIMARY KEY(jid)"
				")");
	// query.exec("INSERT INTO users VALUES ('holy.cheater@dragonfly', '*', '*')");

	GatewayTask gw;
	gw.setDatabaseLink(db);
	gw_ptr = &gw;

	// TODO: Read username/secret/server from config-file
	JabberConnection conn;
	conn.setUsername("icq.dragonfly");
	conn.setServer("192.168.10.10", 5555);
	conn.setPassword("jaba");

	QObject::connect( &conn, SIGNAL( userRegistered(QString,QString,QString) ), &gw, SLOT( processRegister(QString,QString,QString) ) );
	QObject::connect( &conn, SIGNAL( userUnregistered(QString) ), &gw, SLOT( processUnregister(QString) ) );
	QObject::connect( &conn, SIGNAL( userOnline(Jid) ), &gw, SLOT( processLogin(Jid) ) );
	QObject::connect( &conn, SIGNAL( userOffline(Jid) ), &gw, SLOT( processLogout(Jid) ) );
	QObject::connect( &conn, SIGNAL( userAdd(Jid,QString) ), &gw, SLOT( processContactAdd(Jid,QString) ) );
	QObject::connect( &conn, SIGNAL( userDel(Jid,QString) ), &gw, SLOT( processContactDel(Jid,QString) ) );
	QObject::connect( &conn, SIGNAL( connected() ), &gw, SLOT( processGatewayOnline() ) );

	QObject::connect( &gw, SIGNAL( contactAdded(Jid,QString) ), &conn, SLOT( sendSubscribed(Jid,QString) ) );
	QObject::connect( &gw, SIGNAL( contactDeleted(Jid,QString) ), &conn, SLOT( sendUnsubscribed(Jid,QString) ) );
	QObject::connect( &gw, SIGNAL( onlineNotifyFor(Jid) ), &conn, SLOT( sendOnlinePresence(Jid) ) );
	QObject::connect( &gw, SIGNAL( offlineNotifyFor(Jid) ), &conn, SLOT( sendOfflinePresence(Jid) ) );

	conn.login();

	/* try to catch terminate signals to send offline presence to users before quit */
	if ( signal(SIGTERM, sighandler) == SIG_IGN ) {
		signal(SIGTERM, SIG_IGN);
	}
	if ( signal(SIGKILL, sighandler) == SIG_IGN ) {
		signal(SIGKILL, SIG_IGN);
	}
	if ( signal(SIGINT, sighandler) == SIG_IGN ) {
		signal(SIGINT, SIG_IGN);
	}

	return app.exec();
}
