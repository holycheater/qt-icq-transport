/*
 * icqRateManager.cpp - packet rate manager for an icq connection
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

#include "icqRateManager.h"
#include "icqSocket.h"

#include <QList>
#include <QtAlgorithms>

#include <QtDebug>

namespace ICQ
{


class RateManager::Private
{
	public:
		RateClass* findRateClass(const SnacBuffer* snac) const; /* find rate class for the packet */
		RateClass* findRateClass(Word rateClassId) const; /* get rate class by ID */

		void recv_server_rates(SnacBuffer& reply); /* snac (01,07) handler */
		void recv_rates_update(SnacBuffer& reply); /* snac (01,0a) handler */

		RateManager *q;

		QList<RateClass*> classList;

		Socket *socket;
};

RateManager::RateManager(Socket* socket, QObject *parent)
	: QObject(parent)
{
	d = new Private;
	d->q = this;

	d->socket = socket;
	d->socket->setRateManager(this);
	QObject::connect( d->socket, SIGNAL( incomingSnac(SnacBuffer&) ), SLOT( incomingSnac(SnacBuffer&) ) );
}

RateManager::~RateManager()
{
	qDeleteAll(d->classList);
	delete d;
}

void RateManager::addClass(RateClass* rc)
{
	QObject::connect( rc, SIGNAL( dataReady(SnacBuffer*) ), SLOT( dataAvailable(SnacBuffer*) ) );
	d->classList.append(rc);
}

bool RateManager::canSend(const SnacBuffer& snac) const
{
	RateClass *rc = d->findRateClass(&snac);
	if ( rc ) {
		if ( rc->timeToNextSend() == 0 ) {
			return true;
		} else {
			return false;
		}
	} else {
		// qDebug() << "[ICQ:RM] no rate class for SNAC" << QByteArray::number(snac.family(), 16) << QByteArray::number(snac.subtype(), 16);
		return true;
	}
}

void RateManager::enqueue(const SnacBuffer& packet)
{
	SnacBuffer *p = new SnacBuffer(packet);
	RateClass *rc = d->findRateClass(p);

	if ( rc ) {
		qDebug() << "[ICQ:RM] Enqueuing a packet" << p->channel() << "snac family" << p->family() << "subtype" << p->subtype();
		rc->enqueue(p);
	} else {
		dataAvailable(p);
	}
}

void RateManager::requestRates()
{
	d->socket->snacRequest(0x01, 0x06);
}

void RateManager::dataAvailable(SnacBuffer* packet)
{
	d->socket->writeForced(packet);
	delete packet; // delete packet after writing it to the buffer
}

RateClass* RateManager::Private::findRateClass(const SnacBuffer* packet) const
{
	RateClass *rc = 0L;
	if ( classList.size() == 0 ) {
		return rc;
	}

	QListIterator<RateClass*> i(classList);
	while ( i.hasNext() ) {
		RateClass *item = i.next();
		if ( item->isMember( packet->family(), packet->subtype() ) ) {
			rc = item;
			break;
		}
	}

	return rc;
}

RateClass* RateManager::Private::findRateClass(Word rateClassId) const
{
	RateClass *rc = 0L;
	if ( classList.size() == 0 ) {
		return rc;
	}

	QListIterator<RateClass*> i(classList);
	while ( i.hasNext() ) {
		RateClass *item = i.next();
		if ( item->classId() == rateClassId ) {
			rc = item;
			break;
		}
	}

	return rc;
}


/* << SNAC (01,07) - SRV_RATE_LIMIT_INFO
 * >> SNAC (01,08) - CLI_RATES_ACK */
void RateManager::Private::recv_server_rates(SnacBuffer& reply)
{
	Word rateCount = reply.getWord();
	if ( rateCount == 0 ) {
		return;
	}

	SnacBuffer ratesAck(0x01, 0x08);

	for ( int i = 0; i < rateCount; i++ ) {
		Word classId = reply.getWord();
		ratesAck.addWord(classId);

		RateClass *rc = new RateClass;
		rc
			->setClassId(classId)
			->setWindowSize( reply.getDWord() )
			->setClearLevel( reply.getDWord() )
			->setAlertLevel( reply.getDWord() )
			->setLimitLevel( reply.getDWord() )
			->setDisconnectLevel( reply.getDWord() )
			->setCurrentLevel( reply.getDWord() )
			->setMaxLevel( reply.getDWord() );
		q->addClass(rc);

		reply.getDWord(); // last time (not used)
		reply.getByte(); // current state (not used)
	}

	for ( int i = 0; i < rateCount; i++ ) {
		Word rateClassId = reply.getWord(); // rate class id
		Word pairCount = reply.getWord(); // rate pairs count

		RateClass *rc = findRateClass(rateClassId);
		if ( !rc ) {
			qCritical() << "[ICQ:RM]" << "[Critical Error]" << "rate class not found" << rateClassId;
			continue;
		}

		for (int j=0; j < pairCount; j++ ) {
			Word family = reply.getWord();
			Word subtype = reply.getWord();
			rc->addMember(family, subtype);
		}
	}

	/* send out CLI_RATES_ACK */
	socket->write(ratesAck);
}

/* << SNAC (01,0A) - SRV_RATE_LIMIT_WARN */
void RateManager::Private::recv_rates_update(SnacBuffer& snac)
{
	Word msgCode = snac.getWord();
	Word classId = snac.getWord();

	QString codeDesc;
	switch ( msgCode ) {
		case 0x0001:
			// qDebug() << "[ICQ:RM]" << "Rate limits parameters changed";
			break;
		case 0x0002:
			qDebug() << "[ICQ:RM]" << "Rate limits warning (current-lvl < alert-lvl)";
			break;
		case 0x0003:
			qDebug() << "[ICQ:RM]" << "Rate limit hit (current-lvl < limit-lvl)";
			break;
		case 0x0004:
			qDebug() << "[ICQ:RM]" << "Rate limit clear (current-lvl > clear-lvl)";
			break;
		default:
			break;
	}
	RateClass *rc = findRateClass(classId);
	if ( rc ) {
		rc
			->setWindowSize( snac.getDWord() )
			->setClearLevel( snac.getDWord() )
			->setAlertLevel( snac.getDWord() )
			->setLimitLevel( snac.getDWord() )
			->setDisconnectLevel( snac.getDWord() )
			->setCurrentLevel( snac.getDWord() )
			->setMaxLevel( snac.getDWord() );

		snac.getDWord(); // last time (not used)
		snac.getByte(); // current state (not used)

		rc->updateRateInfo();
	}
	snac.seekEnd(); // mark as read.
}

void RateManager::incomingSnac(SnacBuffer& snac)
{
	if ( snac.family() != 0x01 ) {
		return;
	}
	if ( snac.subtype() == 0x07 ) {
		d->recv_server_rates(snac);
	}
	if ( snac.subtype() == 0x0A ) {
		d->recv_rates_update(snac);
	}
}


} /* end of namespace ICQ */

// vim:sw=4:ts=4:noet:nowrap
