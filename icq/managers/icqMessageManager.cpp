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
#include "icqSocket.h"

#include "types/icqMessage.h"
#include "types/icqSnacBuffer.h"
#include "types/icqTlvChain.h"
#include "types/icqTypes.h"

#include <QDateTime>
#include <QTextCodec>

namespace ICQ
{


class MessageManager::Private {
    public:
        void send_channel_1_message(const Message& msg);
        void send_channel_2_message(const Message& msg);
        void send_channel_4_message(const Message& msg);

        void processServerAck(SnacBuffer& snac); /* SNAC(04,0B) */
        void processMessageAck(SnacBuffer& snac); /* SNAC(04,0C) */

        QString uin;
        Socket *socket;
        QTextCodec *codec;
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

    Tlv msgData(0x02);

    msgData.addByte(0x05); // fragment id
    msgData.addByte(0x01); // fragment version
    msgData.addWord(1); // next data len
    msgData.addByte(1); // required caps, 1 - text.

    Buffer msgChunk;
    msgChunk.addWord(0x0000); // charset
    msgChunk.addWord(0x0000); // lang num
    msgChunk.addData( codec->fromUnicode( QString::fromUtf8(msg.text()) ) );

    msgData.addByte(0x01); // fragment id: message
    msgData.addByte(0x01); // fragment version
    msgData.addWord( msgChunk.size() ); // next data len
    msgData.addData(msgChunk);

    snac.addTlv(msgData);
    snac.addTlv( Tlv(0x06) ); // store if recipient offline

    socket->write(snac);
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
    msgTlv.addData( Capabilities[ccICQServerRelay] );

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

    QString guidStr = "{" + Capabilities[ccUTF8Messages].toString() + "}"; // UTF-8
    msgChunk.addLEDWord( guidStr.length() );
    msgChunk.addData(guidStr);

    Tlv extData(0x2711); // TLV 0x2711

    extData.addLEWord( chunk1.size() );
    extData.addData(chunk1);
    extData.addLEWord( chunk2.size() );
    extData.addData(chunk2);
    extData.addData(msgChunk);

    msgTlv.addData(extData);

    snac.addData( msgTlv.data() );

    socket->write(snac);
}

void MessageManager::Private::send_channel_4_message(const Message& msg)
{
    Q_UNUSED(msg)
    /* TODO: Send channel 4 messages */
}

void MessageManager::Private::processServerAck(SnacBuffer& snac)
{
    snac.seekForward(8); // msg-id cookie

    Word channel = snac.getWord();
    QString uin = snac.read( snac.getByte() );
    Word reason = snac.getWord();
    snac.seekEnd();

    Q_UNUSED(channel)
    Q_UNUSED(reason)

    // qDebug() << "[ICQ:MM] Server ACK" << "channel" << channel << "uin" << uin << "reason" << reason;
}

void MessageManager::Private::processMessageAck(SnacBuffer& snac)
{
    QByteArray cookie = snac.read(8);
    Word channel = snac.getWord();
    Byte uinLen = snac.getByte();
    QString uin = snac.read(uinLen);

    Q_UNUSED(channel)

    // qDebug() << "[ICQ:MM] Msg ACK." << "Msg delivered to" << uin << "via channel" << channel;
}

MessageManager::MessageManager(Socket *socket, QObject *parent)
    : QObject(parent)
{
    d = new Private;
    d->socket = socket;

    QObject::connect(d->socket, SIGNAL( incomingSnac(SnacBuffer&) ), SLOT( incomingSnac(SnacBuffer&) ) );
}

MessageManager::~MessageManager()
{
    delete d;
}

void MessageManager::setTextCodec(QTextCodec *codec)
{
    d->codec = codec;
}

void MessageManager::setUin(const QString& uin)
{
    d->uin = uin;
}

void MessageManager::requestOfflineMessages()
{
    d->socket->sendMetaRequest(0x3C);
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
            qCritical("[ICQ:MM] unknown msg channel: %d", msg.channel());
            break;
    }
}

Message MessageManager::handle_channel_1_msg(TlvChain& chain)
{
    Message msg;

    if ( chain.hasTlv(0x04) ) {
        msg.setFlags(Message::AutoMessage);
    }

    bool offlineMsg = chain.hasTlv(0x06);

    Tlv tlv02 = chain.getTlv(0x02);
    tlv02.seekForward( sizeof(Byte) ); // fragment ident = 05 (capabilities array)
    tlv02.seekForward( sizeof(Byte) ); // fragment version = 01
    Word capsSize = tlv02.getWord();
    tlv02.seekForward(capsSize);

    tlv02.seekForward( sizeof(Byte) ); // fragment ident = 01 (messageLen)
    tlv02.seekForward( sizeof(Byte) ); // fragment version = 01
    Word msgSize = tlv02.getWord() - sizeof(Word)*2;
    Word msgCharset = tlv02.getWord();
    Word msgSubset = tlv02.getWord();
    Q_UNUSED(msgSubset)
    switch ( msgCharset ) {
        case 0x0002:
            msg.setEncoding(Message::Ucs2);
            break;
        case 0x0003:
        {
            if ( offlineMsg ) {
                msg.setEncoding(Message::UserDefined);
            } else {
                msg.setEncoding(Message::Latin1);
            }
            break;
        }
        default:
            msg.setEncoding(Message::UserDefined);
            break;
    }
    // qDebug() << "msg charset" << QString::number(msgCharset, 16) << "subset" << QString::number(msgSubset, 16);

    if ( offlineMsg ) {
        Tlv tlv16 = chain.getTlv(0x16);
        msg.setTimestamp( tlv16.getDWord() );
        msg.setOffline();
    }

    QByteArray message = tlv02.read(msgSize);
    msg.setText(message);
    msg.setType(Message::PlainText);

    return msg;
}

Message MessageManager::handle_channel_2_msg(TlvChain& chain)
{
    Message msg;

    Tlv block = chain.getTlv(0x05);

    block.seekForward( sizeof(Word) ); // message type: 0 - request, 1 - cancel, 2 - accept
    block.seekForward(8); // message cookie (same as in the snac data) Why do they need to repeat everything twice? I'm not stupid!
    Guid cap = Guid::fromRawData( block.read(16) ); // capability, needed for this msg

    if ( cap != Capabilities[ccICQServerRelay] ) {
        qWarning( "[ICQ:MM] [User: %s] Unknown channel 2 message capability: %s. Ignoring.", qPrintable(d->uin), qPrintable(cap.toString()) );
        return Message();
    }

    TlvChain msgChain( block.readAll() );

    if ( !msgChain.hasTlv(0x2711) ) {
        qWarning( "[ICQ:MM] [User: %s] Channel-2 message doesn't contain TLV 0x2711. Ignoring.", qPrintable(d->uin) );
        return Message();
    }

    Tlv msgBlock = msgChain.getTlv(0x2711);

    {
        msgBlock.getLEWord(); // data length
        Word protocolVer = msgBlock.getLEWord(); // protocol version

        Guid cap2 = Guid::fromRawData( msgBlock.read(16) );

        if ( !cap2.isZero() ) {
            qWarning( "[ICQ:MM] [User: %s] Message contains unknown data (Capability: %s)", qPrintable(d->uin), qPrintable(cap2.toString()) );
            return Message();
        }

        msgBlock.seekForward( sizeof(Word) ); //unknown
        DWord capFlags = msgBlock.getLEDWord();
        msgBlock.seekForward( sizeof(Byte) ); //unknown
        msgBlock.seekForward( sizeof(Word) ); //downcounter

        Q_UNUSED(protocolVer)
        Q_UNUSED(capFlags)
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

    if ( msg.type() == Message::PlainText ) {
        msgBlock.seekForward( sizeof(DWord) ); // text color
        msgBlock.seekForward( sizeof(DWord) ); // bg color
        DWord guidStrLen = msgBlock.getLEDWord();
        QByteArray guidStr = msgBlock.read(guidStrLen);
        if ( qstrncmp(guidStr.constData(), "{0946134E-4C7F-11D1-8222-444553540000}", guidStrLen) == 0 ) {
            msg.setEncoding(Message::Utf8);
        }
    }

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
    QByteArray icbmCookie = snac.read(8); // msg-id cookie
    Word msgChannel = snac.getWord();
    Byte uinLen = snac.getByte();
    QString uin = snac.read(uinLen);
    snac.seekForward( sizeof(Word) ); // warning level

    // qDebug() << "[ICQ:MM] Incoming msg. channel" << msgChannel << "from" << uin;

    Word tlvCount = snac.getWord(); // number of tlvs in fixed part
    for ( int i = 0; i < tlvCount; i++ ) {
        Tlv block = Tlv::fromBuffer(snac);
        /* TODO: we should update user info with this */
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
            qWarning("[ICQ:MM] [User: %s] Unknown channel for the incoming message: %d", qPrintable(d->uin), msgChannel);
            break;
    }

    if ( !msg.isValid() ) {
        qWarning( "[ICQ:MM] [User: %s] Incoming message processing failed. Message is not valid.", qPrintable(d->uin) );
        qWarning( "[ICQ:MM] [User: %s] Dumping message SNAC: %s", qPrintable(d->uin), snac.data().toHex().constData() );
        return;
    }

    msg.setChannel(msgChannel);
    msg.setIcbmCookie(icbmCookie);
    msg.setReceiver(d->uin);
    msg.setSender(uin);
    if ( msg.timestamp().isNull() ) {
        msg.setTimestamp( QDateTime::currentDateTime() );
    }

    // qDebug() << "[ICQ:MM]" << "type" << msg.type() << "flags" << msg.flags() << "message" << msg.text();

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
    // qDebug() << timestamp << timestamp.toString() << timestamp.toLocalTime() << timestamp.toLocalTime().toString();

    Byte msgType = data.getByte();
    Byte msgFlags = data.getByte();

    Word msgLen = data.getLEWord();
    QByteArray message = data.read(msgLen - 1);

    Message msg;

    msg.setFlags(msgFlags);
    msg.setType(msgType);
    msg.setText(message);
    msg.setSender(senderUin);
    msg.setReceiver( d->uin );
    msg.setTimestamp(timestamp);
    msg.setOffline(true);

    emit incomingMessage(msg);
}

void MessageManager::incomingMetaInfo(Word type, Buffer& data)
{
    if ( type == 0x41 ) { // offline message block
        handle_offline_message(data);
    } else if ( type == 0x42 ) {
        /* delete offline messages */
        d->socket->sendMetaRequest(0x3E);
    }
}

void MessageManager::incomingSnac(SnacBuffer& snac)
{
    if ( snac.family() != 0x04 ) {
        return;
    }
    switch ( snac.subtype() ) {
        case 0x07:
            handle_incoming_message(snac);
            break;
        case 0x0b:
            d->processServerAck(snac);
            break;
        case 0x0c:
            d->processMessageAck(snac);
            break;
        default:
            break;
    }
}


} /* end of namespace ICQ */

// vim:sw=4:ts=4:et:nowrap
