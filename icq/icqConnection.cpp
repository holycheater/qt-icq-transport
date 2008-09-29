/*
 * icqConnection.cpp - ICQ Connection class.
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

#include "icqConnection.h"
#include "icqConnection_p.h"

#include "managers/icqSsiManager.h"

#include "types/icqFlapBuffer.h"
#include "types/icqSnacBuffer.h"
#include "types/icqContact.h"
#include "types/icqMessage.h"
#include "types/icqUserInfo.h"

#include <QTcpSocket>
#include <QTimer>
#include <QStringList>
#include <QtDebug>

namespace ICQ
{


/**
 * @class Connection
 * @brief This class manages connection to the ICQ server.
 * @details Details here.
 */

/**
 * Constructs ICQ connection object.
 *
 * @param parent	when the parent will be deleted, connection object will be destroyed.
 */
Connection::Connection(QObject *parent = 0)
	: QObject(parent)
{
	d = new Private(this);
}

/**
 * Constructs ICQ connection object with given parameters.
 *
 * @param uin			ICQ user uin
 * @param password		ICQ user password
 * @param server		Server to connect to
 * @param port			Port to connect to
 * @param parent		when the parent will be deleted, connection object will be destroyed
 */
Connection::Connection(const QString& uin, const QString& password, const QString& server, quint16 port, QObject *parent)
	: QObject(parent)
{
	d = new Private(this);

	d->uin = uin;
	d->password = password;
	d->server = server;
	d->port = port;
}

/**
 * Destroys connection object.
 */
Connection::~Connection()
{
	delete d;
}

/**
 * Adds a @a uin contact to the roster.
 */
void Connection::contactAdd(const QString& uin)
{
	/* if contact is already on the list, just emit the 'added' signal */
	if ( contactList().contains(uin) ) {
		emit contactAdded(uin);
		return;
	}
	qDebug() << "[ICQ;Connection]" << "adding contact" << uin;
	d->ssiManager->addContact(uin);
}

/**
 * Grant authorization to @a uin
 */
void Connection::grantAuth(const QString& uin)
{
	d->ssiManager->grantAuthorization(uin);
}

/**
 * Deny authorization to @a uin
 */
void Connection::denyAuth(const QString& uin)
{
	d->ssiManager->denyAuthorization(uin);
}

/**
 * Removes @a uin contact from the roster.
 */
void Connection::contactDel(const QString& uin)
{
	/* if contact is not on the list, just emit the 'deleted' signal */
	if ( !contactList().contains(uin) ) {
		qDebug() << "[ICQ:Connection]" << "Contact is not on the list:" << uin;
		emit contactDeleted(uin);
		return;
	}
	d->ssiManager->delContact(uin);
}

/**
 * Returns list of UINs in the roster.
 */
QStringList Connection::contactList() const
{
	QList<Contact> contacts = d->ssiManager->contactList();
	QListIterator<Contact> i(contacts);
	QStringList uinList;
	while ( i.hasNext() ) {
		uinList << i.next().name();
	}
	return uinList;
}

/**
 * Sends message to @a recipient uin containing @a text
 */
void Connection::sendMessage(const QString& recipient, const QString& text)
{
	Message msg;

	if ( d->userInfoManager->getUserStatus(recipient) == UserInfo::Offline ) {
		qDebug() << "[ICQ:Connection]" << "sending offline message via channel 1";
		msg.setChannel(0x01);
	} else {
		qDebug() << "[ICQ:Connection]" << "sending message via channel 2";
		msg.setChannel(0x02);
	}

	msg.setSender( userId() );
	msg.setReceiver(recipient);
	msg.setType(Message::PlainText);
	msg.setText( text.toUtf8() );
	d->msgManager->sendMessage(msg);
}

/**
 * Returns current connection status.
 *
 * @sa ConnectionStatus
 */
int Connection::connectionStatus() const
{
	return d->connectionStatus();
}

/**
 * Returns true if connection managed to sign-on on the ICQ server.
 */
bool Connection::isSignedOn() const
{
	if ( connectionStatus() == Connected ) {
		return true;
	}
	return false;
}

/**
 * Returns user ICQ UIN for current connection.
 */
QString Connection::userId() const
{
	return d->uin;
}

/**
 * Sets connection ICQ user UIN to @a uin.
 *
 * @param uin			ICQ UIN
 * @return				Pointer to this connection
 * @sa setPassword(), setServer(), setServerPort()
 */
Connection* Connection::setUin(const QString& uin)
{
	d->uin = uin;
	return this;
}

/**
 * Sets connection ICQ user password to @a password.
 *
 * @param password		ICQ password
 * @return				Pointer to this connection
 * @sa setUin(), setServer(), setServerPort()
 */
Connection* Connection::setPassword(const QString& password)
{
	d->password = password;
	return this;
}

/**
 * Sets connection ICQ login server host to @a server.
 *
 * @param server		ICQ login server host
 * @return				Pointer to this connection
 * @sa setUin(), setPassword(), setServerPort()
 */
Connection* Connection::setServer(const QString& server)
{
	d->server = server;
	return this;
}

/**
 * Sets connection ICQ login server port to @a port.
 *
 * @param port			ICQ login server port
 * @return				Pointer to this connection
 * @sa setUin(), setPassword(), setServer()
 */
Connection* Connection::setServerPort(quint16 port)
{
	d->port = port;
	return this;
}

/**
 * Sets connected user online status. If user is offline, connection starts
 * to login on the icq server.
 *
 * @param onlineStatus	ICQ user online status
 * @return				Pointer to this connection
 * @sa setVisibility()
 */
Connection* Connection::setOnlineStatus(Word onlineStatus)
{
	d->onlineStatus = onlineStatus;

	if ( onlineStatus == UserInfo::Offline ) {
		if ( connectionStatus() == Connecting ) {
			d->socket->disconnectFromHost();
		} else if ( connectionStatus() == Connected ) {
			signOff();
		}
		return this;
	}

	if ( connectionStatus() == Disconnected ) {
		d->startSignOn();
		return this;
	}

	if ( connectionStatus() != Connected ) {
		return this;
	}

	SnacBuffer reqSetStatus(0x01, 0x1E);
	Word flags = flagDCAuth;
	reqSetStatus.addTlv( (Tlv)Tlv(0x06).addWord(flags).addWord(d->onlineStatus) );

	reqSetStatus.addTlv( (Tlv)Tlv(0x08).addWord(0x0) ); // unknown. Don't know what is that;

	// dc info, some dummy data
	Tlv dcInfo(0x0C);
	dcInfo.addDWord(0x00000000); // internal ip
	dcInfo.addDWord(0x00000000); // internal port
	dcInfo.addByte(0x04); // dc type - DC_NORMAL
	dcInfo.addWord(0x000B); // dc protocol version
	dcInfo.addDWord(0x01020304); // dc auth cookie
	dcInfo.addDWord(0x00000050); // web front port?
	dcInfo.addDWord(0x00000001); // "client futures" ?!
	dcInfo.addDWord(0x0); // last info update time ?!
	dcInfo.addDWord(0x0); // last ext status?
	dcInfo.addWord(0x0000); // unknown
	dcInfo.addDWord(0x00000000); //unknown

	reqSetStatus.addTlv(dcInfo);

	write(reqSetStatus);

	return this;
}

Connection* Connection::setVisibility(int vis)
{
	Q_UNUSED(vis)
	return this;
}

/**
 * Send empty snac request 'on the wire'.
 *
 * @param family		Snac family
 * @param subtype		Snac subtype
 */
void Connection::snacRequest(Word family, Word subtype)
{
	write( SnacBuffer(family, subtype) );
}

/**
 * Write @a flap packet to the socket.
 *
 * @param flap			FLAP data packet
 */
void Connection::write(const FlapBuffer& flap)
{
	writeForced( const_cast<FlapBuffer*>(&flap) );
}

/**
 * Write @a snac packet to the socket.
 *
 * @param snac			SNAC data packet
 */
void Connection::write(const SnacBuffer& snac)
{
	if ( d->rateManager && !d->rateManager->canSend(snac) ) {
		d->rateManager->enqueue(snac);
	} else {
		writeForced( const_cast<SnacBuffer*>(&snac) );
	}
}

/**
 * Write @a flap packet to the socket without checking the rate manager.
 *
 * @param flap			Pointer to FLAP data packet
 */
void Connection::writeForced(FlapBuffer* flap)
{
	flap->setSequence( d->flapSequence() );

	// qDebug() << "[ICQ:Connection] >> flap channel" << flap->channel() << "len" << flap->size() << "sequence" << QByteArray::number(flap->sequence(), 16);
	// qDebug() << "[ICQ:Connection] >> flap data" << flap->data().toHex().toUpper();
	d->socket->write( flap->data() );
}

/**
 * Write @a snac packet to the socket without checking the rate manager.
 *
 * @param snac			Pointer to SNAC data packet
 */
void Connection::writeForced(SnacBuffer* snac)
{
	snac->setRequestId( d->snacRequest() );

	writeForced( dynamic_cast<FlapBuffer*>(snac) );

	qDebug() << "[ICQ:Connection] >>"
		<< "snac head: family"
		<< QByteArray::number(snac->family(),16)
		<< "subtype" << QByteArray::number(snac->subtype(),16)
		<< "flags" << QByteArray::number(snac->flags(), 16)
		<< "requestid" << QByteArray::number(snac->requestId(), 16);
}

/**
 * Sign off from the ICQ server.
 */
void Connection::signOff()
{
	qDebug() << "[ICQ:Connection] Signing off";
	write( FlapBuffer(FlapBuffer::CloseChannel) );

	d->socket->disconnectFromHost();
}

void Connection::sendMetaRequest(Word type)
{
	d->metaManager->sendMetaRequest(type);
}

void Connection::sendMetaRequest(Word type, Buffer& data)
{
	d->metaManager->sendMetaRequest(type, data);
}

/**
 * @fn void Connection::statusChanged(int status)
 * @brief This signal is emitted when the the connected user status changes.
 *
 * @param status		ICQ online status
 */

/**
 * @fn void Connection::incomingFlap(FlapBuffer& flap)
 * @brief This signal is emitted when there is incoming @a flap packet.
 *
 * @param flap			FLAP data packet
 */

/**
 * @fn void Connection::incomingSnac(SnacBuffer& snac)
 * @brief This signal is emitted when there is incoming @a snac packet.
 *
 * @param snac			SNAC data packet
 */

/**
 * @fn void Connection::readyRead()
 * @brief This signal is emitted when not all data was processed in the incomingData() slot to process next packet.
 */

/**
 * @fn void Connection::ssiNewGroup(Contact *contact)
 * @brief This signal is emitted when new group was added to the roster.
 */

/**
 * @fn void Connection::ssiNewBuddy(Contact *contact)
 * @brief This signal is emitted when new buddy was added to the roster.
 */

/**
 * @fn void Connection::ssiNewIgnore(Contact *contact)
 * @brief This signal is emitted when new ignore-list record was added to the roster.
 */

/**
 * @fn void Connection::ssiNewVisible(Contact *contact)
 * @brief This signal is emitted when new visible-list record was added to the roster.
 */

/**
 * @fn void Connection::ssiNewInvisible(Contact *contact)
 * @brief This signal is emitted when new invisible-list record was added to the roster.
 */

/**
 * @fn void Connection::userOnline(QString userId)
 * @brief This signal is emitted when a user from buddy list goes online.
 *
 * @param userId		UIN of user which gone online.
 */

/**
 * @fn void Connection::userOffline(QString userId)
 * @brief This signal is emitted when a user from buddy list goes offline.
 *
 * @param userId		UIN of user which gone offline.
 */

/**
 * @fn void Connection::incomingMessage(const Message& msg)
 * @brief This signal is emitted when someone sends a message to connected user.
 *
 * @param msg		Incoming message
 */

/**
 * @fn void Connection::signedOff()
 * @brief This signal is emitted when user signs off from ICQ server.
 */

/**
 * @enum Connection::ConnectionStatus
 * @brief This enum describes connection state to the ICQ service.
 *
 * @li Disconnected		This connection is not connected to the ICQ server.
 * @li Connecting		This connection is trying to connect to the ICQ server.
 * @li Connected		This connection is connected to the ICQ server.
 */

/**
 * @enum Connection::OnlineStatus
 * @brief This enum desribes user online status.
 *
 * @li Online			User is Online
 * @li Away				User is away
 * @li DoNotDisturb		User is in 'Do Not Disturb' mode
 * @li NotAvailable		User is not available
 * @li FreeForChat		User is free for chat
 * @li Invisible		User is invisible (used in conjunction with other statuses)
 * @li Offline			User is Offline (this is an internal status value)
 */


} /* end of namespace ICQ */
