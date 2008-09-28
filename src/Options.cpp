/*
 * Options.cpp - command-line and file options reader/parser
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

#include "Options.h"

#include <QCoreApplication>
#include <QStringList>
#include <QTextStream>

/* TODO: Load parameters from config file */

static QString defaultDatabase("users.db");
static QString defaultJabberServer("localhost");
static QString defaultJabberPort("5555");
static QString defaultIcqServer("login.icq.com");
static QString defaultIcqPort("5190");

Options::Options()
{
	m_options.insert("database", defaultDatabase);
	m_options.insert("jabber-server", defaultJabberServer);
	m_options.insert("jabber-port", defaultJabberPort);
	m_options.insert("icq-server", defaultIcqServer);
	m_options.insert("icq-port", defaultIcqPort);
}

Options::~Options()
{
}

QString Options::getOption(const QString& name)
{
	return m_options.value(name);
}

void Options::parseCommandLine()
{
	Q_ASSERT( QCoreApplication::instance() );

	QStringList args = QCoreApplication::instance()->arguments();
	QStringListIterator i(args);

	i.next(); // skip app executable name.

	while ( i.hasNext() ) {
		QString arg = i.next();
		if ( arg == "-help" ) {
			printUsage();
			QCoreApplication::exit();
		}
		if ( arg == "-database" ) {
			m_options.insert( "database", i.next() );
			continue;
		}
		if ( arg == "-jabber-server" ) {
			m_options.insert( "jabber-server", i.next() );
			continue;
		}
		if ( arg == "-jabber-port" ) {
			m_options.insert( "jabber-port", i.next() );
			continue;
		}
		if ( arg == "-jabber-user" ) {
			m_options.insert( "jabber-user", i.next() );
			continue;
		}
		if ( arg == "-jabber-secret" ) {
			m_options.insert( "jabber-secret", i.next() );
			continue;
		}
		if ( arg == "-icq-server" ) {
			m_options.insert( "icq-server", i.next() );
			continue;
		}
		if ( arg == "-icq-port" ) {
			m_options.insert( "icq-port", i.next() );
			continue;
		}
	}
}

void Options::printUsage()
{
	QTextStream stream(stdout, QIODevice::WriteOnly);
	stream << "You can use the options below to override default settings or config file settings\n"
		<< "Options:\n"
		<< "   -database <file>          Path to database file \n"
		<< "   -jabber-server <host>     Jabber server hostname/ip \n"
		<< "   -jabber-port <port>       Jabber server port\n"
		<< "   -jabber-user <username>   Jabber username (domain name which is server by this component)\n"
		<< "   -jabber-secret <secret>   Secret passphrase for authenticating on the server\n"
		<< "   -icq-server <host>        ICQ login server hostname\n"
		<< "   -icq-port <port>          ICQ login server port\n";
}
