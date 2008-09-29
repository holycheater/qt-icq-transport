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
#include <QHash>
#include <QSet>
#include <QQueue>

#include <QtDebug>

namespace ICQ
{


class SSIManager::Private
{
	public:
		enum ModifyType {
			ModSuccess = 0x0000,
			ModNotFound = 0x0002,
			ModAlreadyExists = 0x0003,
			ModError = 0x000A,
			ModLimitExceed = 0x000C,
			ModAuthRequired = 0x000E
		};

		void sendContact(const Contact& contact, Word snacSubtype);

		void processSsiParameters(SnacBuffer& reply); /* SNAC(13,03) */
		void processSsiContact(SnacBuffer& reply); /* SNAC(13,06) */
		void processSsiAdd(SnacBuffer& reply); /* SNAC(13,08) */
		void processSsiUpdate(SnacBuffer& reply); /* SNAC(13,09) */
		void processSsiRemove(SnacBuffer& reply); /* SNAC(13,0A) */
		void processServerEditAck(SnacBuffer& reply); /* SNAC(13,0E) */
		void processSsiUpToDate(SnacBuffer& reply); /* SNAC(13,0F) */
		void processAuthGranted(SnacBuffer& reply); /* SNAC(13,15) */
		void processAuthReply(SnacBuffer& reply); /* SNAC(13,1B) */

		void beginTransaction();
		void finishTransaction();

		void requestAuthorization(const QString& uin); /* SNAC(13,18) */

		Word freeItemId() const;
		Word freeGroupId() const;

		QList<Contact> listOfType(Word type) const;

		Contact groupByName(const QString& name);

		SSIManager *q;

		Connection *link;
		QHash<Word, Contact> ssiList;
		Contact masterGroup;
		QSet<Word> existingGroups;
		QSet<Word> existingItems;

		/* list of modified contacts awaiting ack from server */
		QQueue<Contact> outgoingContacts;

		DWord lastUpdate;

		Word maxContacts;
		Word maxGroups;
		Word maxVisible;
		Word maxInvisible;
		Word maxIgnored;
};

void SSIManager::Private::sendContact(const Contact& contact, Word snacSubtype)
{
	SnacBuffer snac(0x13, snacSubtype);
	snac.addData(contact);

	outgoingContacts.enqueue(contact);
	link->write(snac);
	qDebug() << snac.data().toHex();
}

void SSIManager::Private::processSsiParameters(SnacBuffer& reply)
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
void SSIManager::Private::processSsiContact(SnacBuffer& reply)
{
	reply.getByte(); // ssi version - 0x00

	Word listSize = reply.getWord();

	for ( Word i = 0; i < listSize; i++ ) {
		Word nameLen = reply.getWord();
		QString name = reply.read(nameLen);
		Word groupId = reply.getWord();
		Word itemId = reply.getWord();
		Word itemType = reply.getWord();

		existingItems.insert(itemId);

		if ( itemType == Contact::Group ) {
			existingGroups.insert(groupId);
		}

		Word dataLen = reply.getWord();
		TlvChain chain = reply.read(dataLen);

		Contact contact(name, groupId, itemId, itemType, chain);
		if ( contact.type() == Contact::Group && contact.groupId() == 0 && contact.id() == 0 ) {
			masterGroup = contact;
		}

		qDebug() << "[ICQ:SSI]" << "Contact: " << "name" << name << "gid" << groupId << "iid" << itemId << "type" << QString::number(itemType, 16);

		ssiList.insert(itemId, contact);
	}
	DWord lastChangeTime = reply.getDWord();
	lastUpdate = lastChangeTime;

	/* SNAC(13,07) - SSI Activate */
	link->snacRequest(0x13, 0x07);
}

void SSIManager::Private::processSsiAdd(SnacBuffer& reply)
{
	while ( !reply.atEnd() ) {
		Word nameLen = reply.getWord();
		QString name = reply.read(nameLen);
		Word gid = reply.getWord();
		Word iid = reply.getWord();
		Word itemType = reply.getWord();

		existingItems.insert(iid);

		Word dataLen = reply.getWord();
		TlvChain chain = reply.read(dataLen);

		Contact contact(name, gid, iid, itemType, chain);
		ssiList.insert(iid, contact);
		qDebug() << "[ICQ:SSI]" << "Contact of type" << QString::number(itemType, 16) << "with id" << iid << "named" << name << "added";
		if ( contact.type() == Contact::Buddy ) {
			emit q->contactAdded( contact.name() );
		}
		if ( contact.type() == Contact::Deleted ) {
			emit q->contactDeleted( contact.name() );
			/* remove Deleted contact from SSI list */
			sendContact(contact, 0x0A);
		}
	}
}

void SSIManager::Private::processSsiUpdate(SnacBuffer& reply)
{
	while ( !reply.atEnd() ) {
		Word nameLen = reply.getWord();
		QString name = reply.read(nameLen);
		Word gid = reply.getWord();
		Word iid = reply.getWord();
		Word itemType = reply.getWord();

		existingItems.insert(iid);
		if ( itemType == Contact::Group ) {
			existingGroups.insert(gid);
		}

		Word dataLen = reply.getWord();
		TlvChain chain = reply.read(dataLen);

		Contact contact(name, gid, iid, itemType, chain);

		if ( contact.type() == Contact::Group && contact.groupId() == 0 && contact.id() == 0 ) {
			masterGroup = contact;
		}

		ssiList.insert(iid, contact);
		qDebug() << "[ICQ:SSI]" << "Contact of type" << QString::number(itemType, 16) << "with id" << iid << "named" << name << "updated";
	}
}

void SSIManager::Private::processSsiRemove(SnacBuffer& reply)
{
	while ( !reply.atEnd() ) {
		Word nameLen = reply.getWord();
		QString name = reply.read(nameLen);
		Word gid = reply.getWord();
		Word iid = reply.getWord();
		Word itemType = reply.getWord();

		if ( iid ) {
			existingItems.remove(iid);
			ssiList.remove(iid);
		}
		if ( itemType == Contact::Group && gid ) {
			existingGroups.remove(gid);
		}

		Word dataLen = reply.getWord();
		TlvChain chain = reply.read(dataLen);

		if ( itemType == Contact::Buddy ) {
			emit q->contactDeleted(name);
		}

		qDebug() << "[ICQ:SSI]" << "Contact of type" << QString::number(itemType, 16) << "with id" << iid << "named" << name << "deleted";
	}
}

void SSIManager::Private::processServerEditAck(SnacBuffer& reply)
{
	Word code = reply.getWord();
	qDebug() << "[ICQ:SSI]" << "Modify code:" << QByteArray::number(code, 16);

	if ( code == 0x0 ) {
		Contact contact = outgoingContacts.dequeue();

		/* so this is a new buddy item which is not on the list */
		if ( contact.type() == Contact::Buddy && !ssiList.contains( contact.id() ) ) {
			ssiList.insert(contact.id(), contact);
		}
		return;
	}
	/* contact requires auth */
	if ( code == 0x0E ) {
		Contact contact = outgoingContacts.dequeue();
		contact.setAwaitingAuth(true);

		beginTransaction();
		sendContact(contact, 0x08);
		finishTransaction();

		requestAuthorization( contact.name() );

		return;
	}
	outgoingContacts.dequeue();
}

void SSIManager::Private::processSsiUpToDate(SnacBuffer& reply)
{
	DWord modTime = reply.getDWord();
	Word listSize = reply.getWord();
	Q_UNUSED(modTime)
	Q_UNUSED(listSize)
	qDebug() << "[ICQ:SSI]" << "SSI is up-to-date";

	/* SNAC(13,07) - SSI Activate */
	link->snacRequest(0x13, 0x07);
}

void SSIManager::Private::processAuthGranted(SnacBuffer& snac)
{
	Byte uinLen = snac.getByte();
	QString uin = snac.read(uinLen);
	snac.seekEnd();

	emit q->authGranted(uin);
}

void SSIManager::Private::processAuthReply(SnacBuffer& reply)
{
	int uinLen = reply.getByte();
	QString uin = reply.read(uinLen);

	int accepted = reply.getByte();

	reply.seekEnd();

	if ( accepted == 1 ) {
		emit q->authGranted(uin);
	} else {
		emit q->authDenied(uin);
	}
}

inline void SSIManager::Private::beginTransaction()
{
	link->snacRequest(0x13, 0x11);
}

inline void SSIManager::Private::finishTransaction()
{
	link->snacRequest(0x13, 0x12);
}

void SSIManager::Private::requestAuthorization(const QString& uin)
{
	SnacBuffer snac(0x13, 0x18);

	snac.addByte( uin.length() );
	snac.addData(uin);
	snac.addWord(0); // auth msg len
	snac.addWord(0); // unknown

	link->write(snac);
}

Word SSIManager::Private::freeItemId() const
{
	qsrand( existingItems.size() );

	Word iid = qrand();
	while ( existingItems.contains(iid) ) {
		iid = qrand();
	}
	return iid;
}

Word SSIManager::Private::freeGroupId() const
{
	Word gid = 1;
	while ( existingGroups.contains(gid) ) {
		++gid;
	}
	return gid;
}

Contact SSIManager::Private::groupByName(const QString& name)
{
	QHashIterator<Word, Contact> i(ssiList);
	while ( i.hasNext() ) {
		i.next();
		if ( i.value().type() != Contact::Group ) {
			continue;
		}
		if ( i.value().name() == name ) {
			return i.value();
		}
	}
	return Contact();
}

SSIManager::SSIManager(Connection* parent)
	: QObject(parent)
{
	d = new Private;
	d->q = this;
	d->link = parent;

	QObject::connect(this, SIGNAL( contactAdded(QString) ), d->link, SIGNAL( contactAdded(QString) ) );
	QObject::connect(this, SIGNAL( contactDeleted(QString) ), d->link, SIGNAL( contactDeleted(QString) ) );

	QObject::connect(d->link, SIGNAL( incomingSnac(SnacBuffer&) ), SLOT( incomingSnac(SnacBuffer&) ) );
}

SSIManager::~SSIManager()
{
	delete d;
}

void SSIManager::addContact(const QString& uin)
{
	Contact group = d->groupByName("default");
	Word gid;
	if ( !group.isValid() ) {
		gid = addGroup("default");
	} else {
		gid = group.groupId();
	}

	Contact newContact;

	newContact.setType(Contact::Buddy);
	newContact.setName(uin);
	newContact.setGroupId(gid);
	newContact.setItemId( d->freeItemId() );
	newContact.setDisplayName(uin);

	d->beginTransaction();
	d->sendContact(newContact, 0x08);
	d->finishTransaction();
}

void SSIManager::delContact(const QString& uin)
{
	QHashIterator<Word, Contact> i(d->ssiList);
	Contact contact;
	while ( i.hasNext() ) {
		i.next();
		if ( i.value().name() == uin ) {
			contact = i.value();
			break;
		}
	}
	if ( !contact.isValid() ) {
		qDebug() << "[ICQ:SSI] Contact with uin" << uin << "not found";
		return;
	}

	d->beginTransaction();
	d->sendContact(contact, 0x0A);
	d->finishTransaction();
}

/**
 * Adds group named @a name to roster
 */
Word SSIManager::addGroup(const QString& name)
{
	Contact check = d->groupByName(name);
	if ( check.isValid() ) {
		return check.groupId();
	}

	Contact group;
	group.setType(Contact::Group);
	group.setName(name);
	group.setGroupId( d->freeGroupId() );
	group.setItemId(0);
	group.setChilds( QList<Word>() );

	QList<Word> groups = d->masterGroup.childs();
	groups.append( group.groupId() );

	d->beginTransaction();
	d->sendContact(group, 0x08);
	d->sendContact(d->masterGroup, 0x09);
	d->finishTransaction();

	return group.groupId();
}

void SSIManager::delGroup(const QString& name)
{
	/* we won't delete group that doesn't exist */
	Contact group = d->groupByName(name);
	if ( !group.isValid() ) {
		return;
	}

	d->beginTransaction();
	d->sendContact(group, 0x0A);
	d->finishTransaction();
}

void SSIManager::grantAuthorization(const QString& uin)
{
	SnacBuffer snac(0x13, 0x14);

	snac.addByte( uin.length() );
	snac.addData(uin);
	snac.addWord(0); // auth msg len
	snac.addWord(0); // unknown

	d->link->write(snac);
}

void SSIManager::denyAuthorization(const QString& uin)
{
	SnacBuffer snac(0x13, 0x1A);

	snac.addByte( uin.length() );
	snac.addData(uin);
	snac.addByte(0); // auth denied
	snac.addWord(0); // auth msg len
	snac.addWord(0); // unknown

	d->link->write(snac);
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

	QHashIterator<Word, Contact> i(ssiList);
	while ( i.hasNext() ) {
		i.next();
		Contact contact = i.value();
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

void SSIManager::incomingSnac(SnacBuffer& snac)
{
	if ( snac.family() != 0x13 ) {
		return;
	}

	switch ( snac.subtype() ) {
		case 0x03:
			d->processSsiParameters(snac);
			break;
		case 0x06:
			d->processSsiContact(snac);
			break;
		case 0x08:
			d->processSsiAdd(snac);
			break;
		case 0x09:
			d->processSsiUpdate(snac);
			break;
		case 0x0A:
			d->processSsiRemove(snac);
			break;
		case 0x0E:
			d->processServerEditAck(snac);
			break;
		case 0x0F:
			d->processSsiUpToDate(snac);
			break;
		case 0x11:
			qDebug() << "[ICQ:SSI]" << "Server started the transaction";
			break;
		case 0x12:
			qDebug() << "[ICQ:SSI]" << "Server closed the transaction";
		case 0x15:
			d->processAuthGranted(snac);
			break;
		case 0x1B:
			d->processAuthReply(snac);
			break;
		default:
			break;
	}
}

} /* end of namespace ICQ */
