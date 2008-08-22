/**
 * icq_rateclass.cpp - rate class object for rate manager
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

#include "icq_rateclass.h"

#include <QPair>
#include <QQueue>
#include <QTime>
#include <QTimer>

#include <QtDebug>

class ICQ::RateClass::Private : public QSharedData
{
	public:
		Private();
		Private(const Private& other);
		~Private();

		QList< QPair<Word, Word> > memberSnacs;
		QQueue<SnacBuffer*> packetQueue;
		QTime rateTimer;

		bool bWaitingToSend;

		Word classId;
		DWord windowSize;
		DWord clearLevel;
		DWord alertLevel;
		DWord limitLevel;
		DWord disconnectLevel;
		DWord currentLevel;
		DWord maxLevel;
};

ICQ::RateClass::Private::Private()
	: QSharedData()
{
	bWaitingToSend = false;
	rateTimer.start();
}

ICQ::RateClass::Private::Private(const Private& other)
	: QSharedData(other)
{
	memberSnacs = other.memberSnacs;

	QQueue<SnacBuffer*>::const_iterator it, itEnd = other.packetQueue.constEnd();
	for ( it = other.packetQueue.constBegin(); it != itEnd; ++it )
	{
		SnacBuffer *snac = new SnacBuffer(**it);
		packetQueue.append(snac);
	}

	rateTimer = other.rateTimer;

	bWaitingToSend = other.bWaitingToSend;

	classId = other.classId;
	windowSize = other.windowSize;
	clearLevel = other.clearLevel;
	alertLevel = other.alertLevel;
	limitLevel = other.limitLevel;
	disconnectLevel = other.disconnectLevel;
	currentLevel = other.currentLevel;
	maxLevel = other.maxLevel;
}

ICQ::RateClass::Private::~Private()
{
	qDeleteAll(packetQueue);
}

ICQ::RateClass::RateClass(QObject *parent)
	: QObject(parent)
{
	d = new Private;
}

ICQ::RateClass::RateClass(Word classId, QObject *parent)
	: QObject(parent)
{
	d = new Private;
	d->classId = classId;
}

ICQ::RateClass::RateClass(const RateClass& other)
	: QObject( other.parent() ), d(other.d)
{

}

ICQ::RateClass& ICQ::RateClass::operator=(const RateClass& other)
{
	d = other.d;
	return *this;
}

ICQ::RateClass::~RateClass()
{
}

void ICQ::RateClass::addMember(const SnacBuffer& snac)
{
	addMember( snac.family(), snac.subtype() );
}

void ICQ::RateClass::addMember(Word family, Word subtype)
{
	d->memberSnacs.append( qMakePair(family, subtype) );
}

void ICQ::RateClass::enqueue(SnacBuffer* packet)
{
	d->packetQueue.enqueue(packet);
	setupTimer();
}

bool ICQ::RateClass::isMember(const SnacBuffer& snac) const
{
	return isMember( snac.family(), snac.subtype() );
}

bool ICQ::RateClass::isMember(Word family, Word subtype) const
{
	QList< QPair<Word, Word> >::const_iterator it;
	QList< QPair<Word, Word> >::const_iterator itEnd = d->memberSnacs.constEnd();
	for (it = d->memberSnacs.constBegin(); it != itEnd; ++it ) {
		if ( it->first == family && it->second == subtype ) {
			return true;
		}
	}
	return false;
}

int ICQ::RateClass::timeToNextSend() const
{
	int timeDiff = d->rateTimer.elapsed();
	DWord newLevel = calcNewLevel(timeDiff);
	DWord maxLevel = d->alertLevel + RATE_SAFETY_TIME;

	if ( newLevel < maxLevel || newLevel < d->disconnectLevel )
	{
		int waitTime = ( d->windowSize * maxLevel ) - ( ( d->windowSize - 1 ) * d->currentLevel );
		qDebug() << "We're sending too fast. Will wait " << waitTime << "ms before sending";
		exit(1);
		return waitTime;
	}

	return 0;
}

void ICQ::RateClass::updateRateInfo()
{
	d->currentLevel = calcNewLevel( d->rateTimer.elapsed() );
	d->rateTimer.restart();
}

ICQ::DWord ICQ::RateClass::calcNewLevel(int timeDiff) const
{
	/* NewLevel = (Window - 1)/Window * OldLevel + 1/Window * CurrentTimeDiff
	 * is the same as the code below. It was made so because of problems with precision
	 */
	DWord newLevel = ( (d->windowSize - 1) * d->currentLevel + timeDiff ) / d->windowSize;

	return newLevel;
}

void ICQ::RateClass::setupTimer()
{
	if ( !d->bWaitingToSend ) {
		d->bWaitingToSend = true;

		int ttns = timeToNextSend();
		if ( ttns <= 0 ) {
			slot_send();
		} else {
			QTimer::singleShot( ttns, this, SLOT( slot_send() ) );
		}
	}
}

void ICQ::RateClass::slot_send()
{
	// hey, we don't need to send anything
	if ( d->packetQueue.isEmpty() ) {
		return;
	}

	emit dataReady( d->packetQueue.dequeue() );
	d->bWaitingToSend = false;

	// more to go
	if ( !d->packetQueue.isEmpty() ) {
		setupTimer();
	}
}

ICQ::RateClass* ICQ::RateClass::setClassId(Word classId)
{
	d->classId = classId;
	return this;
}

ICQ::RateClass* ICQ::RateClass::setWindowSize(DWord windowSize)
{
	d->windowSize = windowSize;
	return this;
}

ICQ::RateClass* ICQ::RateClass::setClearLevel(DWord clearLevel)
{
	d->clearLevel = clearLevel;
	return this;
}

ICQ::RateClass* ICQ::RateClass::setAlertLevel(DWord alertLevel)
{
	d->alertLevel = alertLevel;
	return this;
}

ICQ::RateClass* ICQ::RateClass::setLimitLevel(DWord limitLevel)
{
	d->limitLevel = limitLevel;
	return this;
}

ICQ::RateClass* ICQ::RateClass::setDisconnectLevel(DWord disconnectLevel)
{
	d->disconnectLevel = disconnectLevel;
	return this;
}

ICQ::RateClass* ICQ::RateClass::setCurrentLevel(DWord currentLevel)
{
	d->currentLevel = currentLevel;
	return this;
}

ICQ::RateClass* ICQ::RateClass::setMaxLevel(DWord maxLevel)
{
	d->maxLevel = maxLevel;
	return this;
}

ICQ::Word ICQ::RateClass::classId() const
{
	return d->classId;
}

ICQ::DWord ICQ::RateClass::windowSize() const
{
	return d->windowSize;
}

ICQ::DWord ICQ::RateClass::clearLevel() const
{
	return d->clearLevel;
}

ICQ::DWord ICQ::RateClass::alertLevel() const
{
	return d->alertLevel;
}

ICQ::DWord ICQ::RateClass::limitLevel() const
{
	return d->limitLevel;
}

ICQ::DWord ICQ::RateClass::disconnectLevel() const
{
	return d->disconnectLevel;
}

ICQ::DWord ICQ::RateClass::currentLevel() const
{
	return d->currentLevel;
}

ICQ::DWord ICQ::RateClass::maxLevel() const
{
	return d->maxLevel;
}
