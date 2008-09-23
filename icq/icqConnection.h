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

#include "types/icqTypes.h"

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
class Buffer;
class FlapBuffer;
class SnacBuffer;

class Connection: public QObject
{
	Q_OBJECT

	static const int KEEP_ALIVE_INTERVAL = 60000;
	static const int CONNECTION_TIMEOUT = 30000;
	public:
		enum ConnectionStatus { Disconnected, Connecting, Connected };
		enum OnlineStatus {
			Online			= 0x0000,
			Away			= 0x0001,
			DoNotDisturb	= 0x0002,
			NotAvailable	= 0x0004,
			FreeForChat		= 0x0020,
			Invisible		= 0x0100,
			Offline			= 0xFFFF
			};

		Connection(QObject *parent);
		Connection(const QString& uin,
				const QString& password,
				const QString& server,
				quint16 port = DEFAULT_PORT,
				QObject *parent = 0);
		virtual ~Connection();

		void contactAdd(const QString& uin);
		void contactDel(const QString& uin);

		QStringList contactList() const;

		/* current connection status (disconnected/connecting/connected) */
		int connectionStatus() const;

		/* returns true if status is 'Connected' */
		bool isSignedOn() const;

		QString userId() const;

		Connection* setUin(const QString& uin);
		Connection* setPassword(const QString& password);
		Connection* setServer(const QString& server);
		Connection* setServerPort(quint16 port);

		Connection* setOnlineStatus(Word onlineStatus);

		Connection* setVisibility(int vis);

		void signOn(QString& uin, QString& password, QString& server);

		void sendMessage(const QString& recipient, const QString& text);

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
		void incomingFlap(FlapBuffer& flap);
		void incomingSnac(SnacBuffer& snac);
		void readyRead();

		void ssiNewGroup(Contact *contact);
		void ssiNewBuddy(Contact *contact);
		void ssiNewIgnore(Contact *contact);
		void ssiNewVisible(Contact *contact);
		void ssiNewInvisible(Contact *contact);

		void userOnline(const QString& userId);
		void userOffline(const QString& userId);

		void contactAdded(const QString& uin);
		void contactDeleted(const QString& uin);

		void incomingMessage(const Message& msg);
		void incomingMessage(const QString& senderUin, const QString& text);

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
		class Private;
		Private *d;
};

}

#endif /*ICQPROTOCOL_H_*/
