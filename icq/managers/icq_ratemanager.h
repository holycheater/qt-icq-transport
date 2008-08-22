/**
 * icq_ratemanager.h - packet rate manager for an icq connection
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

#ifndef ICQRATEMANAGER_H_
#define ICQRATEMANAGER_H_

#include "icq_types.h"
#include "icq_connection.h"
#include "icq_rateclass.h"
#include "icq_snacbuffer.h"

#include <QList>
#include <QByteArray>
#include <QObject>

namespace ICQ
{


class RateManager: public QObject
{
	Q_OBJECT

	public:
		RateManager(Connection* parent);
		~RateManager();

		/* reset the rate manager */
		void reset();

		/* add new rate class */
		void addClass(RateClass* rc);

		/* check if we can send the packet right now */
		bool canSend(const SnacBuffer& snac) const;

		/* enqueue the packet */
		void enqueue(const SnacBuffer& snac);

		void requestRates();
	public slots:
		/* this slot sends data to socket */
		void dataAvailable(ICQ::SnacBuffer* snac);
	private:
		/* find rate class for the packet */
		RateClass* findRateClass(const SnacBuffer* snac) const;

		/* get rate class by ID */
		RateClass* findRateClass(Word rateClassId) const;

		/* snac (01,07) handler */
		void recv_server_rates(SnacBuffer& reply);

		/* snac (01,0a) handler */
		void recv_rates_update(SnacBuffer& reply);
	private slots:
		void incomingSnac(ICQ::SnacBuffer& snac);
	private:
		class Private;
		Private* d;
};

}

#endif /*ICQRATEMANAGER_H_*/
