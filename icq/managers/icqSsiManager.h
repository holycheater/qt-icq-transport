/*
 * icqSsiManager.h - server-side information manager for an icq connection
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

#ifndef SSIMANAGER_H_
#define SSIMANAGER_H_

#include "icqConnection.h"
#include "types/icqSnacBuffer.h"

#include <QObject>
#include <QString>

namespace ICQ
{

class Contact;

class SSIManager: public QObject
{
	Q_OBJECT

	public:
		SSIManager(Connection *parent);
		~SSIManager();

		QList<Contact> contactList() const;
		QList<Contact> groupList() const;
		QList<Contact> visibleList() const;
		QList<Contact> invisibleList() const;
		QList<Contact> ignoreList() const;

		/* SNAC(13,05) - Check server copy of CL for update */
		void checkContactList();

		/* SNAC(13,02) - Request SSI rights/limitations  */
		void requestParameters();

		Word size() const;
		DWord lastChangeTime() const;
		void setLastChangeTime(DWord time);
	signals:
		void newGroup(Contact* contact);
		void newBuddy(Contact *contact);
		void newIgnore(Contact *contact);
		void newVisible(Contact *contact);
		void newInvisible(Contact *contact);
	private slots:
		void incomingSnac(SnacBuffer& snac);
	private:
		class Private;
		Private *d;
};

}

#endif /*SSIMANAGER_H_*/
