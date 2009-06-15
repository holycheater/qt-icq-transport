/*
 * icqRateClass.h - rate class object for rate manager
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

#ifndef ICQRATECLASS_H_
#define ICQRATECLASS_H_

#include "icqTypes.h"
#include "icqSnacBuffer.h"

#include <QSharedDataPointer>


namespace ICQ {


class RateClass : public QObject
{
	Q_OBJECT

	static const int RATE_SAFETY_TIME = 50;

	public:
		RateClass(QObject *parent = 0);
		RateClass(Word classId, QObject *parent = 0);
		RateClass(const RateClass& other);
		RateClass& operator=(const RateClass& other);
		~RateClass();

		/* add snac to this rate class */
		void addMember(const SnacBuffer& snac);
		void addMember(Word family, Word subtype);

		/* clear the queue */
		void clear();

		/* add packet to the queue */
		void enqueue(SnacBuffer* packet);

		/* check if snac belongs to this rate class */
		bool isMember(const SnacBuffer& snac) const;
		bool isMember(Word family, Word subtype) const;

		/* calculate time to next packet send */
		int timeToNextSend() const;

		/* recalculate the rate info */
		void updateRateInfo();

		/* some setter functions */
		RateClass* setClassId(Word classId);
		RateClass* setWindowSize(DWord windowSize);
		RateClass* setClearLevel(DWord clearLevel);
		RateClass* setAlertLevel(DWord alertLevel);
		RateClass* setLimitLevel(DWord limitLevel);
		RateClass* setDisconnectLevel(DWord disconnectLevel);
		RateClass* setCurrentLevel(DWord currentLevel);
		RateClass* setMaxLevel(DWord maxLevel);

		/* some getter functions */
		Word classId() const;
		DWord windowSize() const;
		DWord clearLevel() const;
		DWord alertLevel() const;
		DWord limitLevel() const;
		DWord disconnectLevel() const;
		DWord currentLevel() const;
		DWord maxLevel() const;
	signals:
		void dataReady(SnacBuffer* packet);
	private:
		DWord calcNewLevel(int timeDiff) const;
		void setupTimer();
	private slots:
		void slot_send();
	private:
		class Private;
		QSharedDataPointer<Private> d;
};

}

// vim:ts=4:sw=4:noet:nowrap
#endif /*ICQRATECLASS_H_*/
