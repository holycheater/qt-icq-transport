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
#include <QtCrypto>
#include <QtSql>
#include <QtDebug>

int main(int argc, char **argv)
{
	QCA::Initializer init;

	QCoreApplication app(argc, argv);

	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
	db.setDatabaseName(":memory:");

	GatewayTask gw;
	gw.setDatabaseLink(db);

	// TODO: Read username/secret/server from config-file
	JabberConnection conn;
	conn.setUsername("icq.dragonfly");
	conn.setServer("192.168.10.10", 5555);
	conn.setPassword("jaba");

	QObject::connect(&conn, SIGNAL( userRegistered(QString,QString,QString) ), &gw, SLOT( processRegister(QString,QString,QString) ) );
	QObject::connect(&conn, SIGNAL( userUnregistered(QString) ), &gw, SLOT( processUnregister(QString) ) );
	QObject::connect(&conn, SIGNAL( userOnline(Jid) ), &gw, SLOT( processLogin(Jid) ) );
	QObject::connect(&conn, SIGNAL( userOffline(Jid) ), &gw, SLOT( processLogout(Jid) ) );

	conn.login();

	return app.exec();
}
