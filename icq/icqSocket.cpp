/*
 * icqSocket.cpp - ICQ connection socket.
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

#include "icqSocket.h"

#include "types/icqFlapBuffer.h"
#include "types/icqSnacBuffer.h"

#include "managers/icqRateManager.h"
#include "managers/icqMetaInfoManager.h"

#include <QHostAddress>
#include <QPair>
#include <QTcpSocket>

#include <QtDebug>

namespace ICQ
{


class Socket::Private
{
	public:
		Private();
		~Private();

		Word flapID();
		DWord snacID();

		QTcpSocket *socket;

		RateManager     *rateManager;
		MetaInfoManager *metaManager;
	private:
		Word m_flapID;
		DWord m_snacID;
};

Socket::Private::Private()
{
	socket = 0;
	rateManager = 0;
	metaManager = 0;

	m_flapID = 0;
	m_snacID = 0;
}

Socket::Private::~Private()
{
	delete socket;
}

Word Socket::Private::flapID()
{
	return ++m_flapID;
}

DWord Socket::Private::snacID()
{
	return ++m_snacID;
}

Socket::Socket(QObject *parent)
	: QObject(parent)
{
	d = new Private;
}

Socket::~Socket()
{
	delete d;
}

void Socket::connectToHost(const QHostAddress& host, quint16 port)
{
	d->socket = new QTcpSocket(this);
	QObject::connect( d->socket, SIGNAL( readyRead() ), SLOT( processIncomingData() ) );
	d->socket->connectToHost(host, port);
}

void Socket::disconnectFromHost()
{
	if ( !d->socket ) {
		return;
	}
	d->socket->disconnectFromHost();
	d->socket->deleteLater();
	d->socket = 0;
}

void Socket::setRateManager(RateManager *ptr)
{
	d->rateManager = ptr;
}

void Socket::setMetaManager(MetaInfoManager *ptr)
{
	d->metaManager = ptr;
}

void Socket::snacRequest(Word family, Word subtype)
{
	write( SnacBuffer(family, subtype) );
}

void Socket::sendMetaRequest(Word type)
{
	if ( !d->metaManager ) {
		return;
	}
	d->metaManager->sendMetaRequest(type);
}

void Socket::sendMetaRequest(Word type, Buffer& data)
{
	if ( !d->metaManager ) {
		return;
	}
	d->metaManager->sendMetaRequest(type, data);
}

void Socket::write(const FlapBuffer& flap)
{
	writeForced( const_cast<FlapBuffer*>(&flap) );
}

void Socket::write(const SnacBuffer& snac)
{
	if ( d->rateManager && !d->rateManager->canSend(snac) ) {
		d->rateManager->enqueue(snac);
	} else {
		writeForced( const_cast<SnacBuffer*>(&snac) );
	}
}

void Socket::writeForced(FlapBuffer* flap)
{
	flap->setSequence( d->flapID() );

	// qDebug() << "[ICQ:Socket] >> flap channel" << flap->channel() << "len" << flap->size() << "sequence" << QByteArray::number(flap->sequence(), 16);
	// qDebug() << "[ICQ:Socket] >> flap data" << flap->data().toHex().toUpper();
	d->socket->write( flap->data() );
}

void Socket::writeForced(SnacBuffer* snac)
{
	snac->setRequestId( d->snacID() );

	writeForced( dynamic_cast<FlapBuffer*>(snac) );

	qDebug() << "[ICQ:Socket] >>"
		<< "snac head: family"
		<< QByteArray::number(snac->family(),16)
		<< "subtype" << QByteArray::number(snac->subtype(),16)
		<< "flags" << QByteArray::number(snac->flags(), 16)
		<< "requestid" << QByteArray::number(snac->requestId(), 16);
}

void Socket::processIncomingData()
{
	if ( !d->socket ) {
		return;
	}

	if ( d->socket->bytesAvailable() < FLAP_HEADER_SIZE ) {
		/* we don't have a header at this point */
		return;
	}

	FlapBuffer flap = FlapBuffer::fromRawData( d->socket->peek(FLAP_HEADER_SIZE) );

	if (flap.flapDataSize() > (d->socket->bytesAvailable() - FLAP_HEADER_SIZE) ) {
		/* we don't need an incomplete packet */
		return;
	}

	d->socket->seek(FLAP_HEADER_SIZE);

	flap.setData( d->socket->read( flap.flapDataSize() ) );

	if ( flap.channel() == FlapBuffer::CloseChannel ) {
		d->socket->disconnectFromHost();
	}

	/* now we emit an incoming flap signal, which will be catched by various
	 * services (login manager, rate manager, etc) */
	emit incomingFlap(flap);
	if ( !d->socket ) {
		qDebug() << "[ICQ:Socket] Socket was closed after emitting incomingFlap signal";
		return;
	}

	if ( flap.channel() == FlapBuffer::DataChannel && flap.pos() == 0 ) { // pos == 0 means that flap wasn't touched by flap handlers.
		SnacBuffer snac = flap;

		qDebug()
			<< "[ICQ:Socket] << snac head: family" << QByteArray::number(snac.family(), 16)
			<< "subtype" << QByteArray::number(snac.subtype(), 16)
			<< "flags" << QByteArray::number(snac.flags(), 16)
			<< "requestid" << QByteArray::number(snac.requestId(), 16)
			<< "len" << snac.size();

		if ( snac.flags() & 0x8000 ) {
			Word unknownSize = snac.getWord();
			snac.seek(sizeof(Word)+unknownSize);
		}

		// read out motd
		if ( snac.family() == 0x01 && snac.subtype() == 0x13 ) {
			snac.seekEnd();
		}

		/* ignore SNAC(01,21) : Extended status */
		if ( snac.family() == 0x01 && snac.subtype() == 0x21 ) {
			snac.seekEnd();
		}

		emit incomingSnac(snac);
		if ( !d->socket ) {
			qDebug() << "[ICQ:Socket] Socket was closed after emitting incomingSnac signal";
			return;
		}

		if ( (snac.pos() + 1) < snac.dataSize() ) {
			qDebug() << "[ICQ:Socket]" << (snac.pos() + 1) << snac.dataSize() << "unhandled snac" << QByteArray::number(snac.family(), 16) << QByteArray::number(snac.subtype(), 16);
		}
	}

	if ( d->socket->bytesAvailable() > 0 ) {
		processIncomingData();
	}
}


} /* end of namespace ICQ */
