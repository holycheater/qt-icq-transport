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
#include <QSet>
#include <QStringList>
#include <QTextStream>

#include <stdlib.h>

/* Default Parameters */
static QString defaultConfigFile("config.xml");
static QString defaultLogFile("/tmp/qt-icq-transport.log");
static QSet<QString> supportedOptions;

Options::Options()
{
    m_options.insert("config-file", defaultConfigFile);
    supportedOptions << "log-file" << "pid-file" << "database"
                     << "jabber-server" << "jabber-port" << "jabber-domain" << "jabber-secret"
                     << "icq-server" << "icq-port";
}

Options::~Options()
{
}

QString Options::getOption(const QString& name) const
{
    return m_options.value(name);
}

bool Options::hasOption(const QString& name) const
{
    return m_options.contains(name);
}

void Options::parseCommandLine()
{
    Q_ASSERT( QCoreApplication::instance() );

    QStringList args = QCoreApplication::instance()->arguments();
    QStringListIterator i(args);
    QSet<QString> opts;
    opts << "config-file" << "log-file" << "pid-file";

    i.next(); // skip app executable name.

    while ( i.hasNext() ) {
        QString arg = i.next();
        if ( arg == "-help" ) {
            printUsage();
            exit(0);
        }
        if ( arg == "--" ) {
            continue;
        }
        /* daemonize non-forked (top-level) process */
        if ( arg == "-daemonize" && !m_options.contains("fork") ) {
            m_options.insert("daemonize", "yes");
            continue;
        }
        if ( arg == "-fork" ) {
            m_options.insert("fork", "yes");
            continue;
        }
        QString optName = QString(arg).remove(0,1);
        if ( !arg.startsWith("-") || !opts.contains(optName) ) {
            qCritical( "Unknown argument: %s", qPrintable(arg) );
            exit(1);
        }
        if ( !i.hasNext() ) {
            qCritical("No value for argument '%s'", qPrintable(arg));
            exit(1);
        }
        m_options.insert(optName, i.next());
    }
    readXmlFile( m_options.value("config-file") );
    if ( !m_options.contains("log-file") ) {
        m_options.insert("log-file", defaultLogFile);
    }
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
        setOption( e.tagName(), e.text() );
    }
}

void Options::setOption(const QString& option, const QString& value, bool overwrite)
{
    if ( !overwrite && m_options.contains(option) ) {
        return;
    }
    if ( !supportedOptions.contains(option) ) {
        qWarning( "Unsupported option: %s (value: %s)", qPrintable(option), qPrintable(value) );
        return;
    }
    m_options.insert(option, value);
}

void Options::printUsage()
{
    QTextStream stream(stdout, QIODevice::WriteOnly);
    stream << "You can use the options below to override default settings or config file settings\n"
        << "Options:\n"
        << "   -config-file <file>       XML Configuration file (note: command-line options override xml configuration)\n"
        << "   -log-file <file>          Log file (default is /tmp/qt-icq-transport.log)\n"
        << "   -pid-file <file>          PID file"
        << "   -daemonize                Daemonize qt-icq-transport";
}

// vim:et:ts=4:sw=4:nowrap
