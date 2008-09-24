/*
 * icqMessageManager.cpp - ICQ Messaging service manager.
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

#include "icqMessageManager.h"

#include "types/icqMessage.h"
#include "types/icqSnacBuffer.h"
#include "types/icqTlvChain.h"

#include <QDateTime>

namespace ICQ
{


class MessageManager::Private {
	public:
		void send_channel_1_message(const Message& msg);
		void send_channel_2_message(const Message& msg);
		void send_channel_4_message(const Message& msg);
		Connection *link;
};

void MessageManager::Private::send_channel_1_message(const Message& msg)
{
	SnacBuffer snac(0x04,0x06);

	DWord r1 = qrand(), r2 = qrand();

	snac.addDWord(r1);
	snac.addDWord(r2);
	snac.addWord( msg.channel() );
	snac.addByte( msg.receiver().length() );
	snac.addData( msg.receiver() );

	Tlv msgData;

	msgData.addByte(0x05); // fragment id
	msgData.addByte(0x01); // fragment version
	msgData.addByte(1); // next data len
	msgData.addByte(1); // required caps, 1 - text.

	Buffer msgChunk;
	msgChunk.addWord(0x0000); // charset
	msgChunk.addWord(0x0000); // lang num
	msgChunk.addData( msg.text() );

	msgData.addByte(0x01); // fragment id: message
	msgData.addByte(0x01); // fragment version
	msgData.addWord( msgChunk.size() ); // next data len
	msgData.addData(msgChunk);

	snac.addTlv(msgData);
	snac.addTlv( Tlv(0x06) ); // store if recipient offline

	link->write(snac);

	/*
	 * 2A 02
	 * 6B 4D
	 * 00 37
	 *
	 * 00 04
	 * 00 06
	 * 00 00
	 * 00 00 00 49
	 *
	 * 49 00 00 00
	 * 00 00 00 00
	 *
	 * 00 01 [channel]
	 * 09 [uin len]
	 * 31 37 31 33 36 36 33 36 38
	 *
	 * 00 02 [tlv: msg data]
	 * 00 11 [len]
	 * 05 [frag id]
	 * 01 [frag ver]
	 * 00 01 [rest data len]
	 * 01 [req caps. 1 == text]
	 * 01 [frag id: msg]
	 * 01 [frag ver]
	 * 00 08 [data len]
	 * 00 00 [msg charset]
	 * 00 00 [lang num]
	 * 73 65 6E 64 [msg str]
	 *
	 * 00 06 [store if recip offline]
	 * 00 00
	 */
}

void MessageManager::Private::send_channel_2_message(const Message& msg)
{
	SnacBuffer snac(0x04,0x06);

	DWord r1 = qrand(), r2 = qrand();

	snac.addDWord(r1);
	snac.addDWord(r2);
	snac.addWord( msg.channel() );
	snac.addByte( msg.receiver().length() );
	snac.addData( msg.receiver() );

	Tlv msgTlv(0x05);
	msgTlv.addWord(0x00); // msg type - request
	msgTlv.addDWord( r1 ); // cookie part1
	msgTlv.addDWord( r2 ); // cookie part2
	msgTlv.addData( Guid("09461349-4C7F-11D1-8222-444553540000") );

	Tlv tlv0A(0x0A);
	tlv0A.addWord(0x01);

	Tlv tlv0F(0x0F);

	msgTlv.addData(tlv0A);
	msgTlv.addData(tlv0F);

	Buffer chunk1;
	chunk1.addLEWord(9); // protocol version
	chunk1.addData( Guid() ); // zero-bytes guid.
	chunk1.addWord(0);
	chunk1.addLEDWord(3); // cap flags?!
	chunk1.addByte(4); // unknown
	chunk1.addWord(0xffff); // downcounter

	Buffer chunk2;
	chunk2.addWord(0xffff);
	chunk2.addDWord(0);
	chunk2.addDWord(0);
	chunk2.addDWord(0);

	Buffer msgChunk;
	msgChunk.addByte( msg.type() );
	msgChunk.addByte( msg.flags() );
	msgChunk.addLEWord(5); // status code
	msgChunk.addLEWord(2); // priority code
	QByteArray text = msg.text();
	msgChunk.addLEWord(text.length() + 1); // msg len
	msgChunk.addData(text);
	msgChunk.addByte(0); // null-terminated string.
	msgChunk.addDWord(0x0); // text color
	msgChunk.addDWord(0xffffff00); // bg color

	QString guidStr = "{0946134E-4C7F-11D1-8222-444553540000}";
	msgChunk.addLEWord( guidStr.length() );
	msgChunk.addData(guidStr);

	Tlv extData(0x2711); // TLV 0x2711

	extData.addLEWord( chunk1.size() );
	extData.addData(chunk1);
	extData.addLEWord( chunk2.size() );
	extData.addData(chunk2);
	extData.addData(msgChunk);

	msgTlv.addData(extData);

	snac.addData( msgTlv.data() );

	link->write(snac);
}

void MessageManager::Private::send_channel_4_message(const Message& msg)
{
	/* TODO */
}

MessageManager::MessageManager(Connection *parent)
	: QObject(parent)
{
	d = new Private;
	d->link = parent;

	QObject::connect(d->link, SIGNAL( incomingSnac(SnacBuffer&) ), SLOT( incomingSnac(SnacBuffer&) ) );
	QObject::connect(this, SIGNAL( incomingMessage(Message) ), d->link, SIGNAL( incomingMessage(Message) ) );
}

MessageManager::~MessageManager()
{
	delete d;
}

void MessageManager::requestOfflineMessages()
{
	d->link->sendMetaRequest(0x3C);
}

void MessageManager::sendMessage(const Message& msg)
{
	switch ( msg.channel() ) {
		case 1:
			d->send_channel_1_message(msg);
			break;
		case 2:
			d->send_channel_2_message(msg);
			break;
		case 4:
			d->send_channel_4_message(msg);
			break;
		default:
			qCritical() << "unknown msg channel" << msg.channel();
			break;
	}
}

Message MessageManager::handle_channel_1_msg(TlvChain& chain)
{
	Message msg;

	if ( chain.hasTlv(0x04) ) {
		msg.setFlags(Message::AutoMessage);
	}

	Tlv tlv02 = chain.getTlv(0x02);
	tlv02.seekForward( sizeof(Byte) ); // fragment ident = 05 (capabilities array)
	tlv02.seekForward( sizeof(Byte) ); // fragment version = 01
	Word capsSize = tlv02.getWord();
	QList<Guid> caps;
	while ( capsSize > 0 ) {
		Guid cap = tlv02.read(16);
		qDebug() << cap.toString();
		caps << cap;
		capsSize -= 16;
	}
	tlv02.seekForward( sizeof(Byte) ); // fragment ident = 01 (messageLen)
	tlv02.seekForward( sizeof(Byte) ); // fragment version = 01
	Word msgSize = tlv02.getWord();
	tlv02.seekForward( sizeof(Word) ); // message charset number
	tlv02.seekForward( sizeof(Word) ); // message charset subset
	QByteArray message = tlv02.read( msgSize - sizeof(Word)*2 );
	msg.setText(message);

	return msg;
}

Message MessageManager::handle_channel_2_msg(TlvChain& chain)
{
	Message msg;

	Tlv block = chain.getTlv(0x05);

	block.seekForward( sizeof(Word) ); // message type: 0 - request, 1 - cancel, 2 - accept
	block.seekForward(8); // message cookie (same as in the snac data) Why do they need to repeat everything twice? I'm not stupid!
	Guid cap = block.read(16); // capability, needed for this msg

	qDebug() << "capability" << cap.toString();

	TlvChain msgChain( block.readAll() );

	Tlv msgBlock = msgChain.getTlv(0x2711);

	{
		msgBlock.getLEWord(); // data length
		Word protocolVer = msgBlock.getLEWord(); // protocol version
		qDebug() << "proto version" << protocolVer;
		Guid cap2 = msgBlock.read(16);
		qDebug() << "cap in 0x2711" << cap2.toString();
		msgBlock.seekForward( sizeof(Word) ); //unknown
		DWord capFlags = msgBlock.getLEDWord();
		qDebug() << "cap flags" << QByteArray::number(capFlags, 16);
		msgBlock.seekForward( sizeof(Byte) ); //unknown
		msgBlock.seekForward( sizeof(Word) ); //downcounter
	}
	{
		Word dataLen = msgBlock.getLEWord();
		msgBlock.seekForward(dataLen);
	}
	msg.setType( msgBlock.getByte() );
	msg.setFlags( msgBlock.getByte() );
	msgBlock.seekForward( sizeof(Word) ); // status code
	msgBlock.seekForward( sizeof(Word) ); // priority code
	Word msgLen = msgBlock.getLEWord();
	QByteArray message = msgBlock.read( msgLen - 1);
	msgBlock.seekForward(1); // null-terminating char
	msg.setText(message);

	msgBlock.seekForward( sizeof(DWord) ); // text color
	msgBlock.seekForward( sizeof(DWord) ); // bg color
	DWord guidStrLen = msgBlock.getLEDWord();
	QByteArray guidStr = msgBlock.read(guidStrLen);
	qDebug() << "guid str" << guidStr;

	qDebug() << "!! to read" << msgBlock.bytesAvailable();

	return msg;
}

Message MessageManager::handle_channel_4_msg(TlvChain& chain)
{
	Message msg;

	Tlv msgData = chain.getTlv(0x05);

	DWord sender = msgData.getLEDWord();
	Byte type = msgData.getByte();
	Byte flags = msgData.getByte();
	Word msgLen = msgData.getLEWord();
	QByteArray msgText = msgData.read(msgLen - 1);

	msg.setFlags(flags);
	msg.setType(type);
	msg.setSender(sender);
	msg.setText(msgText);

	return msg;
}

void MessageManager::handle_incoming_message(SnacBuffer& snac)
{
	qDebug() << snac.data().toHex();
	QByteArray icbmCookie = snac.read(8); // msg-id cookie
	Word msgChannel = snac.getWord();
	Byte uinLen = snac.getByte();
	QString uin = snac.read(uinLen);
	Word warningLevel = snac.getWord();
	Q_UNUSED(warningLevel)

	qDebug() << "[MessageManager] channel" << msgChannel << "from" << uin;

	Word tlvCount = snac.getWord(); // number of tlvs in fixed part
	for ( int i = 0; i < tlvCount; i++ ) {
		Tlv block = Tlv::fromBuffer(snac);
		// TODO: we should update user info with this
	}
	TlvChain chain;
	while ( !snac.atEnd() ) {
		chain << Tlv::fromBuffer(snac);
	}

	Message msg;
	switch ( msgChannel ) {
		case 1:
			msg = handle_channel_1_msg(chain);
			break;
		case 2:
			msg = handle_channel_2_msg(chain);
			break;
		case 4:
			msg = handle_channel_4_msg(chain);
			break;
		default:
			qDebug() << "unknown channel for the incoming message" << msgChannel;
			break;
	}

	msg.setChannel(msgChannel);
	msg.setIcbmCookie(icbmCookie);
	msg.setReceiver( d->link->userId() );
	msg.setSender(uin);
	msg.setTimestamp( QDateTime::currentDateTime() );

	qDebug() << "[MessageManager]" << "type" << msg.type() << "flags" << msg.flags() << "message" << msg.text();

	emit incomingMessage(msg);
}

void MessageManager::handle_offline_message(Buffer& data)
{
	DWord senderUin = data.getLEDWord();

	Word dateYear = data.getLEWord();
	Byte dateMonth = data.getByte();
	Byte dateDay = data.getByte();
	Byte dateHour = data.getByte();
	Byte dateMinute = data.getByte();
	QDateTime timestamp;
	timestamp.setTimeSpec(Qt::UTC);
	timestamp.setDate( QDate(dateYear, dateMonth, dateDay) );
	timestamp.setTime( QTime(dateHour, dateMinute) );
	qDebug() << timestamp << timestamp.toString() << timestamp.toLocalTime() << timestamp.toLocalTime().toString();

	Byte msgType = data.getByte();
	Byte msgFlags = data.getByte();

	Word msgLen = data.getWord();
	QByteArray message = data.read(msgLen - 1);

	Message msg;

	msg.setFlags(msgFlags);
	msg.setType(msgType);
	msg.setText(message);
	msg.setSender(senderUin);
	msg.setReceiver( d->link->userId() );
	msg.setTimestamp(timestamp);

	emit incomingMessage(msg);
}

void MessageManager::incomingMetaInfo(Word type, Buffer& data)
{
	if ( type == 0x41 ) { // offline message block
		handle_offline_message(data);
	} else if ( type == 0x42 ) { // end of offline messages
		d->link->sendMetaRequest(0x3E); // delete offline messages
	}
}

void MessageManager::incomingSnac(SnacBuffer& snac)
{
	if ( snac.family() == 0x04 && snac.subtype() == 0x07 ) {
		handle_incoming_message(snac);
	}
}


} /* end of namespace ICQ */
