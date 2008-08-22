/**
 * icq_messagemanager.h - ICQ Messaging service manager.
 * Copyright (C) 2008  Alexander Saltykov
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 **/

#ifndef ICQMESSAGEMANAGER_H_
#define ICQMESSAGEMANAGER_H_

#include "icq_connection.h"
#include "icq_message.h"

#include <QObject>

namespace ICQ
{


class MessageManager : public QObject
{
	Q_OBJECT

	public:
		MessageManager(Connection *parent);
		~MessageManager();

		void requestOfflineMessages();
		void sendMessage(const Message& msg);
	signals:
		void incomingMessage(const ICQ::Message&);
	private:
		Message handle_channel_1_msg(TlvChain& chain);
		Message handle_channel_2_msg(TlvChain& chain);
		Message handle_channel_4_msg(TlvChain& chain);
		void handle_incoming_message(SnacBuffer& snac);
		void handle_offline_message(Buffer& data);
	private slots:
		void incomingMetaInfo(ICQ::Word type, ICQ::Buffer& data);
		void incomingSnac(ICQ::SnacBuffer& snac);
	private:
		class Private;
		Private *d;
};

}

#endif /* ICQMESSAGEMANAGER_H_ */
