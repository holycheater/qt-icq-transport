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

#include <QList>
#include <QtAlgorithms>
#include <QtEndian>

#include <QtDebug>

class ICQ::RateManager::Private
{
	public:
		QList<RateClass*> classList;
		Connection* link;
};

ICQ::RateManager::RateManager(Connection* parent)
	: QObject(parent)
{
	d = new Private;
	d->link = parent;
	QObject::connect(d->link, SIGNAL( incomingSnac(ICQ::SnacBuffer&) ), this, SLOT( incomingSnac(ICQ::SnacBuffer&) ) );
}

ICQ::RateManager::~RateManager()
{
	qDeleteAll(d->classList);
	delete d;
}

void ICQ::RateManager::addClass(RateClass* rc)
{
	QObject::connect( rc, SIGNAL( dataReady(ICQ::SnacBuffer*) ), this, SLOT( dataAvailable(ICQ::SnacBuffer*) ) );
	d->classList.append(rc);
}

bool ICQ::RateManager::canSend(const SnacBuffer& snac) const
{
	RateClass *rc = findRateClass(&snac);
	if ( rc ) {
		if ( rc->timeToNextSend() == 0 ) {
			return true;
		} else {
			return false;
		}
	} else {
		//qDebug() << "[ICQ::RateManager] no rate class for SNAC" << QByteArray::number(snac.family(), 16) << QByteArray::number(snac.subtype(), 16);
		return true;
	}
}

void ICQ::RateManager::enqueue(const SnacBuffer& packet)
{
	SnacBuffer *p = new SnacBuffer(packet);
	RateClass *rc = findRateClass(p);

	if ( rc ) {
		qDebug() << "[ICQ::RateManager] Enqueuing a packet" << p->channel() << "snac family" << p->family() << "subtype" << p->subtype();
		rc->enqueue(p);
	} else {
		dataAvailable(p);
	}
}

void ICQ::RateManager::requestRates()
{
	d->link->snacRequest(ICQ::sfGeneric, 0x06);
}

void ICQ::RateManager::dataAvailable(ICQ::SnacBuffer* packet)
{
	d->link->write(*packet);
	delete packet; // delete packet after writing it to the buffer
}

ICQ::RateClass* ICQ::RateManager::findRateClass(const SnacBuffer* packet) const
{
	RateClass *rc = 0L;
	if ( d->classList.size() == 0 ) {
		return rc;
	}

	QList<RateClass*>::const_iterator it;
	QList<RateClass*>::const_iterator itEnd = d->classList.constEnd();

	for ( it = d->classList.constBegin(); it != itEnd; it++ ) {
		if ( (*it)->isMember( packet->family(), packet->subtype() ) ) {
			rc = *it;
			break;
		}
	}

	return rc;
}

ICQ::RateClass* ICQ::RateManager::findRateClass(Word rateClassId) const
{
	RateClass *rc = 0L;
	if ( d->classList.size() == 0 ) {
		return rc;
	}

	QList<RateClass*>::const_iterator it;
	QList<RateClass*>::const_iterator itEnd = d->classList.constEnd();

	for ( it=d->classList.constBegin(); it!=itEnd; it++ ) {
		if ( (*it)->classId() == rateClassId ) {
			rc = *it;
			break;
		}
	}

	return rc;
}


/* << SNAC (01,07) - SRV_RATE_LIMIT_INFO
 * >> SNAC (01,08) - CLI_RATES_ACK */
void ICQ::RateManager::recv_server_rates(SnacBuffer& reply)
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
		addClass(rc);

		reply.getDWord(); // last time (not used)
		reply.getByte(); // current state (not used)
	}

	for ( int i = 0; i < rateCount; i++ ) {
		Word rateClassId = reply.getWord(); // rate class id
		Word pairCount = reply.getWord(); // rate pairs count

		RateClass *rc = findRateClass(rateClassId);
		if ( !rc ) {
			qCritical() << "[ICQ::RateManager]" << "[Critical Error]" << "rate class not found" << rateClassId;
			continue;
		}

		for (int j=0; j < pairCount; j++ ) {
			Word family = reply.getWord();
			Word subtype = reply.getWord();
			rc->addMember(family, subtype);
		}
	}

	/* send out CLI_RATES_ACK */
	d->link->write(ratesAck);
}

/* << SNAC (01,0A) - SRV_RATE_LIMIT_WARN */
void ICQ::RateManager::recv_rates_update(SnacBuffer& reply)
{
	Word msgCode = reply.getWord();
	Word classId = reply.getWord();

	qWarning() << "[ICQ::RateManager] Received SRV_RATE_LIMIT_WARN. Msg code" << msgCode << "Rate class id" << classId;
	RateClass *rc = findRateClass(classId);
	if ( rc ) {
		rc
			->setWindowSize( reply.getDWord() )
			->setClearLevel( reply.getDWord() )
			->setAlertLevel( reply.getDWord() )
			->setLimitLevel( reply.getDWord() )
			->setDisconnectLevel( reply.getDWord() )
			->setCurrentLevel( reply.getDWord() )
			->setMaxLevel( reply.getDWord() );

		reply.getDWord(); // last time (not used)
		reply.getByte(); // current state (not used)

		rc->updateRateInfo();
	}
}

void ICQ::RateManager::incomingSnac(ICQ::SnacBuffer& snac)
{
	if ( snac.family() == ICQ::sfGeneric && snac.subtype() == 0x07 ) {
		recv_server_rates(snac);
	}
	if ( snac.family() == ICQ::sfGeneric && snac.subtype() == 0x0A ) {
		recv_rates_update(snac);
	}
}
