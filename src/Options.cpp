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
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QTextStream>

/* Default Parameters */
static QString defaultConfigFile("config.xml");

Options::Options()
{
	m_options.insert("config-file", defaultConfigFile);
}

Options::~Options()
{
}

QString Options::getOption(const QString& name) const
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
			exit(0);
		}
		if ( arg == "-config-file" ) {
			m_options.insert( "config-file", i.next() );
			continue;
		}
		if ( arg == "-database" ) {
			QString db = i.next();
			QFileInfo fi(db);
			if ( fi.exists() && !( fi.isReadable() && fi.isWritable() ) ) {
				qCritical("Database file '%s' is not readable/writeable", qPrintable(db) );
				exit(1);
			}
			m_options.insert( "database", db );
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
		if ( arg == "-jabber-domain" ) {
			m_options.insert( "jabber-domain", i.next() );
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
	readXmlFile( m_options.value("config-file") );
}

void Options::readXmlFile(const QString& fileName)
{
	QDomDocument doc("qt-icq-transport");
	QFile file(fileName);
	if ( !file.open(QIODevice::ReadOnly) ) {
		qWarning( "Unable to open config file: %s", qPrintable(file.errorString()) );
		return;
	}
	QString errString;
	int errLine, errColumn;
	if ( !doc.setContent(&file, &errString, &errLine, &errColumn) ) {
		qWarning("Unable to process config file content. Line: %d. Column: %d. Error string: %s", errLine, errColumn, qPrintable(errString));
		file.close();
		return;
	}

	QDomElement root = doc.documentElement();
	if ( root.isNull() || root.tagName() != "qt-icq-transport" ) {
		qWarning("Specified config file is not a valid config file");
		return;
	}
	QDomNodeList nodes = root.childNodes();
	for (uint i = 0; i < nodes.length(); ++i) {
		QDomElement e = nodes.item(i).toElement();
		if ( e.isNull() ) {
			continue;
		}
		if ( !m_options.contains("database") && e.tagName() == "database" ) {
			m_options.insert( "database", e.text() );
			continue;
		}
		if ( !m_options.contains("jabber-server") && e.tagName() == "jabber-server" ) {
			m_options.insert( "jabber-server", e.text() );
			continue;
		}
		if ( !m_options.contains("jabber-port") && e.tagName() == "jabber-port" ) {
			m_options.insert( "jabber-port", e.text() );
			continue;
		}
		if ( !m_options.contains("jabber-domain") && e.tagName() == "jabber-domain" ) {
			m_options.insert( "jabber-domain", e.text() );
			continue;
		}
		if ( !m_options.contains("jabber-secret") && e.tagName() == "jabber-secret" ) {
			m_options.insert( "jabber-secret", e.text() );
			continue;
		}
		if ( !m_options.contains("icq-server") && e.tagName() == "icq-server" ) {
			m_options.insert( "icq-server", e.text() );
			continue;
		}
		if ( !m_options.contains("icq-port") && e.tagName() == "icq-port" ) {
			m_options.insert( "icq-port", e.text() );
			continue;
		}
	}
}

void Options::printUsage()
{
	QTextStream stream(stdout, QIODevice::WriteOnly);
	stream << "You can use the options below to override default settings or config file settings\n"
		<< "Options:\n"
		<< "   -config-file <file>       XML Configuration file (note: command-line options override xml configuration)\n"
		<< "   -database <file>          Service users database file (default: users.db)\n"
		<< "   -jabber-server <host>     Jabber server hostname/ip\n"
		<< "   -jabber-port <port>       Jabber server port\n"
		<< "   -jabber-domain <domain>   Jabber domain name which is server by this component\n"
		<< "   -jabber-secret <secret>   Secret passphrase for authenticating on the server\n"
		<< "   -icq-server <host>        ICQ login server hostname (default: login.icq.com)\n"
		<< "   -icq-port <port>          ICQ login server port(default: 5190)\n";
}
