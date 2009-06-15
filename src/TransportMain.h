/*
 * TransportMain.h - Application class
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

#ifndef TRANSPORT_MAIN_H_
#define TRANSPORT_MAIN_H_

#include <QCoreApplication>
#include <QProcess>

class GatewayTask;
class JabberConnection;
class Options;
class QFile;
class QProcess;

class TransportMain : public QCoreApplication
{
	Q_OBJECT

	enum RunMode { Sandbox, Transport };

	public:
		TransportMain(int& argc, char **argv);
		~TransportMain();
	public slots:
		void shutdown();
		void startForkedTransport();
	private:
		void createPidFile();
		void setup_sandbox();

		void setup_transport();
		void connect_signals();

		static void sighandler(int param);
		static void loghandler(QtMsgType type, const char *msg);
	private slots:
		void processTransportError(QProcess::ProcessError error);
		void processTransportFinished(int exitCode, QProcess::ExitStatus exitStatus);
		void processTransportStarted();
	private:
		/* "transport" mode */
		GatewayTask *m_gateway;
		JabberConnection *m_connection;

		/* "sandbox" mode */
		QProcess *m_transport;
		QFile *m_logfile;

		/* common for all */
		Options *m_options;
		RunMode m_runmode;
};

// vim:noet:ts=4:sw=4:nowrap
#endif /* TRANSPORT_MAIN_H_ */
