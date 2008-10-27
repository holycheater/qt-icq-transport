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

class TransportMain : public QCoreApplication
{
	Q_OBJECT

	public:
		TransportMain(int& argc, char **argv);
		~TransportMain();
	public slots:
		void shutdown();
	private:
		void setup_sandbox();
		void start_transport();

		void setup_transport();
		void connect_signals();

		static void sighandler(int param);
		static void loghandler(QtMsgType type, const char *msg);
	private slots:
		void processTransportError(QProcess::ProcessError error);
		void processTransportFinished(int exitCode, QProcess::ExitStatus exitStatus);
		void processTransportStarted();
		void processTransportStateChanged(QProcess::ProcessState newState);
	private:
		GatewayTask *m_gateway;
		JabberConnection *m_connection;
		Options *m_options;
		QFile *m_logfile;
};

#endif /* TRANSPORT_MAIN_H_ */
