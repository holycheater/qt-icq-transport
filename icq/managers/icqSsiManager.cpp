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

namespace ICQ
{


class SSIManager::Private
{
	public:
		void recv_ssi_parameters(SnacBuffer& snac); // 13,03
		void recv_ssi_contact(SnacBuffer& snac); // 13,06
		void recv_ssi_uptodate(SnacBuffer& snac); // 13,0F

		QList<Contact> listOfType(Word type) const;

		SSIManager *q;

		Connection *link;
		QList<Contact> ssiList;

		DWord lastUpdate;

		Word maxContacts;
		Word maxGroups;
		Word maxVisible;
		Word maxInvisible;
		Word maxIgnored;
};

SSIManager::SSIManager(Connection* parent)
	: QObject(parent)
{
	d = new Private;
	d->q = this;
	d->link = parent;

	QObject::connect(d->link, SIGNAL( incomingSnac(SnacBuffer&) ), SLOT( incomingSnac(SnacBuffer&) ) );

	QObject::connect(this, SIGNAL( newGroup(Contact*) ), d->link, SIGNAL( ssiNewGroup(Contact*) ) );
	QObject::connect(this, SIGNAL( newBuddy(Contact*) ), d->link, SIGNAL( ssiNewBuddy(Contact*) ) );
	QObject::connect(this, SIGNAL( newVisible(Contact*) ), d->link, SIGNAL( ssiNewVisible(Contact*) ) );
	QObject::connect(this, SIGNAL( newInvisible(Contact*) ), d->link, SIGNAL( ssiNewInvisible(Contact*) ) );
	QObject::connect(this, SIGNAL( newIgnore(Contact*) ), d->link, SIGNAL( ssiNewIgnore(Contact*) ) );
}

SSIManager::~SSIManager()
{
	delete d;
}

QList<Contact> SSIManager::contactList() const
{
	return d->listOfType(Contact::Buddy);
}

QList<Contact> SSIManager::groupList() const
{
	return d->listOfType(Contact::Group);
}

QList<Contact> SSIManager::visibleList() const
{
	return d->listOfType(Contact::Visible);
}

QList<Contact> SSIManager::invisibleList() const
{
	return d->listOfType(Contact::Invisible);
}

QList<Contact> SSIManager::ignoreList() const
{
	return d->listOfType(Contact::Ignore);
}

QList<Contact> SSIManager::Private::listOfType(Word type) const
{
	QList<Contact> list;

	QListIterator<Contact> i(ssiList);
	while ( i.hasNext() ) {
		Contact contact = i.next();
		if ( contact.type() == type ) {
			list << contact;
		}
	}
	return list;
}

/* >> SNAC(13,05) - CLI_SSI_CHECKOUT */
void SSIManager::checkContactList()
{
	/* send out SNAC(13,05) - check ssi-items */

	SnacBuffer snac(0x13, 0x05);
	snac.addDWord( d->lastUpdate );
	snac.addWord( d->ssiList.size() );

	d->link->write(snac);
}

/* >> SNAC(13,02) - CLI_SSI_RIGHTS_REQUEST */
void SSIManager::requestParameters()
{
	d->link->snacRequest(0x13, 0x02);
}

Word SSIManager::size() const
{
	return d->ssiList.size();
}

DWord SSIManager::lastChangeTime() const
{
	return d->lastUpdate;
}

void SSIManager::setLastChangeTime(DWord time)
{
	d->lastUpdate = time;
}

/* << SNAC(13,03) - SRV_SSI_RIGHTS_REPLY */
void SSIManager::Private::recv_ssi_parameters(SnacBuffer& reply)
{
	TlvChain chain(reply);
	reply.seekEnd();
	Tlv limits = chain.getTlv(0x04);
	maxContacts = limits.getWord();
	maxGroups = limits.getWord();
	maxVisible = limits.getWord();
	maxInvisible = limits.getWord();
	limits.seekForward(sizeof(Word)*10);
	maxIgnored = limits.getWord();
}

/* << SNAC(13,06) - SRV_SSIxREPLY */
void SSIManager::Private::recv_ssi_contact(SnacBuffer& reply)
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

		ssiList << Contact(name, groupId, itemId, itemType, chain);
		switch ( itemType ) {
			case Contact::Group:
				emit q->newGroup( &ssiList.last() );
				break;
			case Contact::Buddy:
				emit q->newBuddy( &ssiList.last() );
				break;
			case Contact::Visible:
				emit q->newVisible( &ssiList.last() );
				break;
			case Contact::Invisible:
				emit q->newInvisible( &ssiList.last() );
				break;
			case Contact::Ignore:
				emit q->newIgnore( &ssiList.last() );
				break;
			default: break;
		}
	}
	DWord lastChangeTime = reply.getDWord();
	lastUpdate = lastChangeTime;
	// qDebug() << lastChangeTime << QDateTime::fromTime_t( lastChangeTime ) << QDateTime::currentDateTime();

	/* SNAC(13,07) */
	link->snacRequest(0x13, 0x07);
}

/* << SNAC(13,0F) - SRV_SSI_UPxTOxDATE
 * >> SNAC(13,07) - CLI_SSI_ACTIVATE */
void SSIManager::Private::recv_ssi_uptodate(SnacBuffer& reply)
{
	DWord modTime = reply.getDWord();
	Word listSize = reply.getWord();
	Q_UNUSED(modTime)
	Q_UNUSED(listSize)
	qDebug() << "[SSI Manager] CL is up-to-date";
}

void SSIManager::incomingSnac(SnacBuffer& snac)
{
	if ( snac.family() != 0x13 ) {
		return;
	}

	switch ( snac.subtype() ) {
		case 0x03:
			d->recv_ssi_parameters(snac);
			break;
		case 0x06:
			d->recv_ssi_contact(snac);
			break;
		case 0x0F:
			d->recv_ssi_uptodate(snac);
			break;
		default:
			break;
	}
}

} /* end of namespace ICQ */
