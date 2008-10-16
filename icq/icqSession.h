/*
 * icqSession.h - ICQ user session.
 * Copyright (C) 2008  Alexander Saltykov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef ICQ_SESSION_H_
#define ICQ_SESSION_H_

#include <QObject>

class QHostInfo;
class QString;
class QStringList;
class QTextCodec;

#include "types/icqTypes.h"

namespace ICQ
{

class SnacBuffer;
class Message;

class Session : public QObject
{
	Q_OBJECT

	public:
		enum ConnectionStatus { Disconnected, Connecting, Connected };
		enum OnlineStatus { Online, FreeForChat, Away, NotAvailable, Occupied, DoNotDisturb, Offline };

		Session(QObject *parent = 0);
		virtual ~Session();

		void connect();
		void disconnect();

		void contactAdd(const QString& uin);
		void contactDel(const QString& uin);

		void authGrant(const QString& toUin);
		void authDeny(const QString& fromUin);

		void setCodecForMessages(QTextCodec *codec);
		void sendMessage(const QString& recipient, const QString& message);

		ConnectionStatus connectionStatus() const;
		QStringList contactList() const;
		OnlineStatus onlineStatus() const;
		QString password() const;
		QString serverHost() const;
		quint16 serverPort() const;
		QString uin() const;

		void setUin(const QString& uin);
		void setPassword(const QString& password);
		void setServerHost(const QString& server);
		void setServerPort(quint16 port);
		void setOnlineStatus(OnlineStatus status);
	signals:
		void connected();
		void disconnected();
		void error(const QString& errorString);

		void statusChanged(int onlineStatus);

		void userOnline(const QString& uin, int status);
		void userOffline(const QString& uin);

		void authGranted(const QString& fromUin);
		void authDenied(const QString& fromUin);
		void authRequest(const QString& fromUin);

		void incomingMessage(const QString& uin, const QString& msg);
	private slots:
		void processLookupTimeout();
		void processLookupResult(const QHostInfo& result);
		void processConnectionTimeout();
		void processServerAvailable(const QString& host, quint16 port);
		void processRatesRequest();
		void processSsiRequest();
		void processLoginDone();
		void processSnac(SnacBuffer& snac);
		void processIncomingMessage(const Message& msg);
		void processUserStatus(const QString& uin, int status);
		void processStatusChanged(int status);
		void sendKeepAlive();
	private:
		Q_DISABLE_COPY(Session);
		class Private;
		Private *d;
};


} /* end of namespace ICQ */

#endif /* ICQ_SESSION_H_ */