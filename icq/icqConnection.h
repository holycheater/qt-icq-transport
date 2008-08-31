/*
 * icqConnection.h - ICQ Connection class.
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

#ifndef ICQPROTOCOL_H_
#define ICQPROTOCOL_H_

#include "icqTypes.h"
#include "icqFlapBuffer.h"
#include "icqSnacBuffer.h"

#include <QByteArray>
#include <QHostInfo>
#include <QObject>
#include <QString>

namespace ICQ
{


class RateManager;
class SSIManager;

class Contact;
class Message;

class Connection: public QObject
{
	Q_OBJECT

	static const int KEEP_ALIVE_INTERVAL = 60000;
	static const int CONNECTION_TIMEOUT = 30000;
	public:
		enum ConnectionStatus { Disconnected, Connecting, Connected };
		Connection(QObject *parent);
		Connection(const QString& uin,
				const QString& password,
				const QString& server,
				quint16 port = DEFAULT_PORT,
				QObject *parent = 0);
		virtual ~Connection();

		/* current connection status (disconnected/connecting/connected) */
		int connectionStatus() const;

		/* returns true if status is 'Connected' */
		bool isSignedOn() const;

		QString userId() const;

		Connection* setUin(const QString& uin);
		Connection* setPassword(const QString& password);
		Connection* setServer(const QString& server);
		Connection* setServerPort(quint16 port);

		/* set user online status*/
		Connection* setOnlineStatus(Word onlineStatus);

		/* set user visibility status */
		Connection* setVisibility(int vis);

		/* start login sequence */
		void signOn(QString& uin, QString& password, QString& server);

		/* send empty SNAC request packet (only snac header in the flap) */
		void snacRequest(Word family, Word subtype);

		/* write an ICQ packet to socket */
		void write(const FlapBuffer& flap);
		void write(const SnacBuffer& snac);

		/* write an ICQ packet to the socket without asking rate manager */
		void writeForced(FlapBuffer* flap);
		void writeForced(SnacBuffer* snac);
	public slots:
		/* send flap channel 0x04 packet and disconnect from server */
		void signOff();
	signals:
		void statusChanged(int status);
		void incomingFlap(ICQ::FlapBuffer& flap);
		void incomingSnac(ICQ::SnacBuffer& snac);
		void readyRead();

		void ssiNewGroup(ICQ::Contact *contact);
		void ssiNewBuddy(ICQ::Contact *contact);
		void ssiNewIgnore(ICQ::Contact *contact);
		void ssiNewVisible(ICQ::Contact *contact);
		void ssiNewInvisible(ICQ::Contact *contact);

		void userOnline(QString userId);
		void userOffline(QString userId);

		void incomingMessage(const ICQ::Message& msg);

		void signedOff();
	protected:
		friend class LoginManager;
		friend class MessageManager;
		/* connect to server */
		void connectToHost(const QString& hostname, quint16 port);
		void connectToHost(const QHostAddress& host, quint16 port);
		/* disconnect from server */
		void disconnectFromHost();

		void startConnectionTimer();

		/* get access to the rate manager */
		RateManager* rateManager() const;
		SSIManager* ssiManager() const;

		void sendMetaRequest(Word type);
		void sendMetaRequest(Word type, Buffer& data);
	private:
		/* SNAC(xx,01) */
		void handle_error(SnacBuffer& snac);
	private slots:
		void connectToServer(const QHostInfo& host);
		void incomingData();
		void sendKeepAlive();

		void slot_connected();
		void slot_disconnected();
		void slot_lookupFailed();
		void slot_connectionTimeout();

		void slot_signedOn();
		void slot_signedOff();
	private:
		class Private;
		Private *d;
};

}

#endif /*ICQPROTOCOL_H_*/
