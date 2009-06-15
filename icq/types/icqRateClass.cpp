/*
 * icqRateClass.cpp - rate class object for rate manager
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

#include "icqRateClass.h"

#include <QPair>
#include <QQueue>
#include <QTime>
#include <QTimer>

#include <QtDebug>

namespace ICQ
{


class RateClass::Private : public QSharedData
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

RateClass::Private::Private()
    : QSharedData()
{
    bWaitingToSend = false;
    rateTimer.start();
}

RateClass::Private::Private(const Private& other)
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

RateClass::Private::~Private()
{
    qDeleteAll(packetQueue);
}

RateClass::RateClass(QObject *parent)
    : QObject(parent)
{
    d = new Private;
}

RateClass::RateClass(Word classId, QObject *parent)
    : QObject(parent)
{
    d = new Private;
    d->classId = classId;
}

RateClass::RateClass(const RateClass& other)
    : QObject( other.parent() ), d(other.d)
{

}

RateClass& RateClass::operator=(const RateClass& other)
{
    d = other.d;
    return *this;
}

RateClass::~RateClass()
{
}

void RateClass::addMember(const SnacBuffer& snac)
{
    addMember( snac.family(), snac.subtype() );
}

void RateClass::addMember(Word family, Word subtype)
{
    d->memberSnacs.append( qMakePair(family, subtype) );
}

void RateClass::enqueue(SnacBuffer* packet)
{
    d->packetQueue.enqueue(packet);
    setupTimer();
}

bool RateClass::isMember(const SnacBuffer& snac) const
{
    return isMember( snac.family(), snac.subtype() );
}

bool RateClass::isMember(Word family, Word subtype) const
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

int RateClass::timeToNextSend() const
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

void RateClass::updateRateInfo()
{
    d->currentLevel = calcNewLevel( d->rateTimer.elapsed() );
    d->rateTimer.restart();
}

DWord RateClass::calcNewLevel(int timeDiff) const
{
    /* NewLevel = (Window - 1)/Window * OldLevel + 1/Window * CurrentTimeDiff
     * is the same as the code below. It was made so because of problems with precision
     */
    DWord newLevel = ( (d->windowSize - 1) * d->currentLevel + timeDiff ) / d->windowSize;

    return newLevel;
}

void RateClass::setupTimer()
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

void RateClass::slot_send()
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

RateClass* RateClass::setClassId(Word classId)
{
    d->classId = classId;
    return this;
}

RateClass* RateClass::setWindowSize(DWord windowSize)
{
    d->windowSize = windowSize;
    return this;
}

RateClass* RateClass::setClearLevel(DWord clearLevel)
{
    d->clearLevel = clearLevel;
    return this;
}

RateClass* RateClass::setAlertLevel(DWord alertLevel)
{
    d->alertLevel = alertLevel;
    return this;
}

RateClass* RateClass::setLimitLevel(DWord limitLevel)
{
    d->limitLevel = limitLevel;
    return this;
}

RateClass* RateClass::setDisconnectLevel(DWord disconnectLevel)
{
    d->disconnectLevel = disconnectLevel;
    return this;
}

RateClass* RateClass::setCurrentLevel(DWord currentLevel)
{
    d->currentLevel = currentLevel;
    return this;
}

RateClass* RateClass::setMaxLevel(DWord maxLevel)
{
    d->maxLevel = maxLevel;
    return this;
}

Word RateClass::classId() const
{
    return d->classId;
}

DWord RateClass::windowSize() const
{
    return d->windowSize;
}

DWord RateClass::clearLevel() const
{
    return d->clearLevel;
}

DWord RateClass::alertLevel() const
{
    return d->alertLevel;
}

DWord RateClass::limitLevel() const
{
    return d->limitLevel;
}

DWord RateClass::disconnectLevel() const
{
    return d->disconnectLevel;
}

DWord RateClass::currentLevel() const
{
    return d->currentLevel;
}

DWord RateClass::maxLevel() const
{
    return d->maxLevel;
}


} /* end of namespace ICQ */

// vim:sw=4:ts=4:et:nowrap
