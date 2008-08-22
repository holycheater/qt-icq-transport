/**
 * icq_metainfomanager.cpp - handles ICQ specific family 0x15 requests.
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

#include "icq_metainfomanager.h"

class ICQ::MetaInfoManager::Private
{
	public:
		Connection *link;
		Word metaSequence;
};

ICQ::MetaInfoManager::MetaInfoManager(Connection *parent)
	: QObject(parent)
{
	d = new Private;
	d->link = parent;
	d->metaSequence = 0;

	QObject::connect(d->link, SIGNAL( incomingSnac(ICQ::SnacBuffer&) ), this, SLOT( incomingSnac(ICQ::SnacBuffer&) ) );
}

ICQ::MetaInfoManager::~MetaInfoManager()
{
	delete d;
}

void ICQ::MetaInfoManager::sendMetaRequest(Word type)
{
	Tlv tlv(0x01); // ENCAPSULATED_METADATA
	tlv.addLEWord(8); // data chunk size (tlv size - 2 )
	tlv.addLEDWord( d->link->userId().toUInt() ); // own UIN
	tlv.addLEWord(type);
	tlv.addLEWord( ++(d->metaSequence) ); // request sequence number

	SnacBuffer snac(0x15, 0x02);
	snac.addTlv(tlv);
	d->link->write(snac);
}

void ICQ::MetaInfoManager::sendMetaRequest(Word type, Buffer& metadata)
{
	Tlv tlv(0x01); // ENCAPSULATED_METADATA
	tlv.addLEWord( metadata.size() + 8 ); // data chunk size (tlv size - 2 )
	tlv.addLEDWord( d->link->userId().toUInt() ); // own UIN
	tlv.addLEWord(type);
	tlv.addLEWord( ++(d->metaSequence) ); // request sequence number
	tlv.addData(metadata);

	SnacBuffer snac(0x15, 0x02);
	snac.addTlv(tlv);
	d->link->write(snac);
}

void ICQ::MetaInfoManager::handle_meta_info(SnacBuffer& snac)
{
	Tlv metaReply = Tlv::fromBuffer(snac);
	metaReply.seekForward( sizeof(Word) ); // data chunk size = tlv_length - 2
	metaReply.seekForward( sizeof(DWord) ); // requester uin

	Word type = metaReply.getLEWord(); // meta request type

	metaReply.seekForward( sizeof(Word) ); // meta request id

	Buffer data = metaReply.readAll();

	emit metaInfoAvailable(type, data);
}

void ICQ::MetaInfoManager::incomingSnac(ICQ::SnacBuffer& snac)
{
	if ( snac.family() == 0x15 && snac.subtype() == 0x03 ) {
		handle_meta_info(snac);
	}
}
