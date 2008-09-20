/*
 * icqSsiManager.cpp - server-side information manager for an icq connection
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

#include "icqSsiManager.h"
#include "types/icqTlvChain.h"
#include "types/icqContact.h"

#include <QByteArray>
#include <QDateTime>

#include <QList>

#include <QtDebug>

class ICQ::SSIManager::Private
{
	public:
		Connection *link;
		QList<Contact> ssiList;

		DWord lastUpdate;

		Word maxContacts;
		Word maxGroups;
		Word maxVisible;
		Word maxInvisible;
		Word maxIgnored;
};

ICQ::SSIManager::SSIManager(Connection* parent)
	: QObject(parent)
{
	d = new Private;
	d->link = parent;

	QObject::connect(d->link, SIGNAL( incomingSnac(ICQ::SnacBuffer&) ), this, SLOT( incomingSnac(ICQ::SnacBuffer&) ) );

	QObject::connect(this, SIGNAL( newGroup(ICQ::Contact*) ), d->link, SIGNAL( ssiNewGroup(ICQ::Contact*) ) );
	QObject::connect(this, SIGNAL( newBuddy(ICQ::Contact*) ), d->link, SIGNAL( ssiNewBuddy(ICQ::Contact*) ) );
	QObject::connect(this, SIGNAL( newVisible(ICQ::Contact*) ), d->link, SIGNAL( ssiNewVisible(ICQ::Contact*) ) );
	QObject::connect(this, SIGNAL( newInvisible(ICQ::Contact*) ), d->link, SIGNAL( ssiNewInvisible(ICQ::Contact*) ) );
	QObject::connect(this, SIGNAL( newIgnore(ICQ::Contact*) ), d->link, SIGNAL( ssiNewIgnore(ICQ::Contact*) ) );
}

ICQ::SSIManager::~SSIManager()
{
	delete d;
}

inline QList<ICQ::Contact> ICQ::SSIManager::contactList() const
{
	return listOfType(Contact::Buddy);
}

inline QList<ICQ::Contact> ICQ::SSIManager::groupList() const
{
	return listOfType(Contact::Group);
}

inline QList<ICQ::Contact> ICQ::SSIManager::visibleList() const
{
	return listOfType(Contact::Visible);
}

inline QList<ICQ::Contact> ICQ::SSIManager::invisibleList() const
{
	return listOfType(Contact::Invisible);
}

inline QList<ICQ::Contact> ICQ::SSIManager::ignoreList() const
{
	return listOfType(Contact::Ignore);
}

QList<ICQ::Contact> ICQ::SSIManager::listOfType(Word type) const
{
	QList<Contact>::const_iterator it, itEnd = d->ssiList.constEnd();
	QList<Contact> list;
	for ( it = d->ssiList.constBegin(); it != itEnd; ++it ) {
		if ( it->type() == type ) {
			list << *it;
		}
	}
	return list;
}

/* >> SNAC(13,05) - CLI_SSI_CHECKOUT */
void ICQ::SSIManager::checkContactList()
{
	/* send out SNAC(13,05) - check ssi-items */

	SnacBuffer snac(ICQ::sfSSI, 0x05);
	snac.addDWord( d->lastUpdate );
	snac.addWord( d->ssiList.size() );

	d->link->write(snac);
}

/* >> SNAC(13,02) - CLI_SSI_RIGHTS_REQUEST */
void ICQ::SSIManager::requestParameters()
{
	d->link->snacRequest(ICQ::sfSSI, 0x02);
}

ICQ::Word ICQ::SSIManager::size() const
{
	return d->ssiList.size();
}

ICQ::DWord ICQ::SSIManager::lastChangeTime() const
{
	return d->lastUpdate;
}

void ICQ::SSIManager::setLastChangeTime(DWord time)
{
	d->lastUpdate = time;
}

/* << SNAC(13,03) - SRV_SSI_RIGHTS_REPLY */
void ICQ::SSIManager::recv_ssi_parameters(SnacBuffer& reply)
{
	TlvChain chain(reply);
	reply.seekEnd();
	Tlv limits = chain.getTlv(0x04);
	d->maxContacts = limits.getWord();
	d->maxGroups = limits.getWord();
	d->maxVisible = limits.getWord();
	d->maxInvisible = limits.getWord();
	limits.seekForward(sizeof(Word)*10);
	d->maxIgnored = limits.getWord();
}

/* << SNAC(13,06) - SRV_SSIxREPLY */
void ICQ::SSIManager::recv_ssi_contact(SnacBuffer& reply)
{
	reply.getByte(); // ssi version - 0x00

	Word listSize = reply.getWord();

	for ( uint i = 0; i < listSize; i++ ) {
		Word nameLen = reply.getWord();
		QString name = reply.read(nameLen);
		Word groupId = reply.getWord();
		Word itemId = reply.getWord();
		Word itemType = reply.getWord();

		Word dataLen = reply.getWord();
		TlvChain chain = reply.read(dataLen);

		d->ssiList << Contact(name, groupId, itemId, itemType, chain);
		switch ( itemType ) {
			case Contact::Group:
				emit newGroup( &d->ssiList.last() );
				break;
			case Contact::Buddy:
				emit newBuddy( &d->ssiList.last() );
				break;
			case Contact::Visible:
				emit newVisible( &d->ssiList.last() );
				break;
			case Contact::Invisible:
				emit newInvisible( &d->ssiList.last() );
				break;
			case Contact::Ignore:
				emit newIgnore( &d->ssiList.last() );
				break;
			default: break;
		}
	}
	DWord lastChangeTime = reply.getDWord();
	d->lastUpdate = lastChangeTime;
	qDebug() << lastChangeTime << QDateTime::fromTime_t( lastChangeTime ) << QDateTime::currentDateTime();
	contactList();

	/* SNAC(13,07) */
	d->link->snacRequest(ICQ::sfSSI, 0x07);
}

/* << SNAC(13,0F) - SRV_SSI_UPxTOxDATE
 * >> SNAC(13,07) - CLI_SSI_ACTIVATE */
void ICQ::SSIManager::recv_ssi_uptodate(SnacBuffer& reply)
{
	DWord modTime = reply.getDWord();
	Word listSize = reply.getWord();
	Q_UNUSED(modTime)
	Q_UNUSED(listSize)
	qDebug() << "[SSI Manager] CL is up-to-date";
}

void ICQ::SSIManager::incomingSnac(SnacBuffer& snac)
{
	if ( snac.family() != ICQ::sfSSI ) {
		return;
	}

	switch ( snac.subtype() ) {
		case 0x03:
			recv_ssi_parameters(snac);
			break;
		case 0x06:
			recv_ssi_contact(snac);
			break;
		case 0x0F:
			recv_ssi_uptodate(snac);
			break;
		default:
			break;
	}
}
