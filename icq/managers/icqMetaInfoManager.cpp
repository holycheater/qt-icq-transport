/*
 * icqMetaInfoManager.cpp - handles ICQ specific family 0x15 requests.
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

#include "icqMetaInfoManager.h"
#include "icqSocket.h"

#include "types/icqSnacBuffer.h"
#include "types/icqTlvChain.h"

namespace ICQ
{


class MetaInfoManager::Private
{
    public:
        QString uin;
        Socket *socket;
        Word metaSequence;
};

MetaInfoManager::MetaInfoManager(Socket *socket, QObject *parent)
    : QObject(parent)
{
    d = new Private;
    d->socket = socket;
    d->metaSequence = 0;

    QObject::connect( d->socket, SIGNAL( incomingSnac(SnacBuffer&) ), SLOT( incomingSnac(SnacBuffer&) ) );
}

MetaInfoManager::~MetaInfoManager()
{
    delete d;
}

void MetaInfoManager::setUin(const QString uin)
{
    d->uin = uin;
}

void MetaInfoManager::sendMetaRequest(Word type)
{
    Tlv tlv(0x01); // ENCAPSULATED_METADATA
    tlv.addLEWord(8); // data chunk size (tlv size - 2 )
    tlv.addLEDWord( d->uin.toUInt() ); // own UIN
    tlv.addLEWord(type);
    tlv.addLEWord( ++(d->metaSequence) ); // request sequence number

    SnacBuffer snac(0x15, 0x02);
    snac.addTlv(tlv);
    d->socket->write(snac);
}

void MetaInfoManager::sendMetaRequest(Word type, Buffer& metadata)
{
    Tlv tlv(0x01); // ENCAPSULATED_METADATA
    tlv.addLEWord( metadata.size() + 8 ); // data chunk size (tlv size - 2 )
    tlv.addLEDWord( d->uin.toUInt() ); // own UIN
    tlv.addLEWord(type);
    tlv.addLEWord( ++(d->metaSequence) ); // request sequence number
    tlv.addData(metadata);

    SnacBuffer snac(0x15, 0x02);
    snac.addTlv(tlv);
    d->socket->write(snac);
}

void MetaInfoManager::handle_meta_info(SnacBuffer& snac)
{
    Tlv metaReply = Tlv::fromBuffer(snac);
    metaReply.seekForward( sizeof(Word) ); // data chunk size = tlv_length - 2
    metaReply.seekForward( sizeof(DWord) ); // requester uin

    Word type = metaReply.getLEWord(); // meta request type

    metaReply.seekForward( sizeof(Word) ); // meta request id

    Buffer data = metaReply.readAll();

    emit metaInfoAvailable(type, data);
}

void MetaInfoManager::incomingSnac(SnacBuffer& snac)
{
    if ( snac.family() == 0x15 && snac.subtype() == 0x03 ) {
        handle_meta_info(snac);
    }
}


} /* end of namespace ICQ */

// vim:sw=4:ts=4:et:nowrap
