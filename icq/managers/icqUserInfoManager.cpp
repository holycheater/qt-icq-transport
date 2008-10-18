/*
 * icqUserInfoManager.cpp - ICQ User Info Manager.
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

#include "icqUserInfoManager.h"
#include "icqSocket.h"

#include "types/icqSnacBuffer.h"
#include "types/icqUserInfo.h"
#include "types/icqShortUserDetails.h"
#include "types/icqUserDetails.h"

#include <QHash>
#include <QQueue>
#include <QtDebug>

/*
 * TODO: Set own user details.
 * TODO: Process interests and affiliations into user details.
 */

namespace ICQ
{


class UserInfoManager::Private {
	public:
		void processOwnUserInfo(SnacBuffer& snac); // SNAC(01,0F)
		void processUserOnlineNotification(SnacBuffer& snac); // SNAC(03,0B)
		void processUserOfflineNotification(SnacBuffer& snac); // SNAC(03,0C)

		void processShortUserInfo(Buffer& buf);

		void processBasicUserInfo(Buffer& buf);
		void processMoreUserInfo(Buffer& buf);
		void processEmailUserInfo(Buffer& buf);
		void processHomepageUserInfo(Buffer& buf);
		void processWorkUserInfo(Buffer& buf);
		void processNotesUserInfo(Buffer& buf);
		void processInterestsUserInfo(Buffer& buf);
		void processAffiliationsUserInfo(Buffer& buf);

		QHash<QString, UserInfo> userInfoList;
		QHash<QString, Word> statusList;
		UserInfo ownInfo;

		/* key is UIN */
		QHash<QString,ShortUserDetails> shortDetails;
		QHash<QString,UserDetails> fullDetails;

		/* for multi-step user-details retrieval. */
		UserDetails lastUserDetails;
		/* queue of uins for which details were requested */
		QQueue<QString> uinRequests;

		Socket *socket;

		UserInfoManager *q;
};

void UserInfoManager::Private::processOwnUserInfo(SnacBuffer& snac)
{
	UserInfo info = UserInfo::fromBuffer(snac);
	if ( info.hasTlv(0x06) && info.onlineStatus() != ownInfo.onlineStatus() ) {
		emit q->statusChanged( info.onlineStatus() );
	}
	ownInfo.mergeFrom(info);

	snac.seekEnd(); //mark snac as handled
}

void UserInfoManager::Private::processUserOnlineNotification(SnacBuffer& snac)
{
	while ( ! snac.atEnd() ) {
		UserInfo info = UserInfo::fromBuffer(snac);
		if ( userInfoList.contains( info.userId() ) ) {
			UserInfo existing = userInfoList.take( info.userId() );
			existing.mergeFrom(info);
			userInfoList.insert(existing.userId(), existing);
			statusList.insert( existing.userId(), existing.onlineStatus() );
		} else {
			userInfoList.insert(info.userId(), info);
			statusList.insert( info.userId(), info.onlineStatus() );
		}

		emit q->userOnline( info.userId(), info.onlineStatus() );
	}
}

void UserInfoManager::Private::processUserOfflineNotification(SnacBuffer& snac)
{
	while ( ! snac.atEnd() ) {
		UserInfo info = UserInfo::fromBuffer(snac);
		emit q->userOffline( info.userId() );
	}
}

void UserInfoManager::Private::processShortUserInfo(Buffer& buf)
{
	if ( uinRequests.isEmpty() ) {
		qDebug() << "[ICQ:UIM]" << "error processing short user details. Request queue is empty";
		return;
	}

	Word nickLen = buf.getLEWord() - 1;
	QString nick = buf.read(nickLen);
	buf.seekForward( sizeof(Byte) );

	Word fnLen = buf.getLEWord() - 1;
	QString fn = buf.read(fnLen);
	buf.seekForward( sizeof(Byte) );

	Word lnLen = buf.getLEWord() - 1;
	QString ln = buf.read(lnLen);
	buf.seekForward( sizeof(Byte) );

	Word emailLen = buf.getLEWord() - 1;
	QString email = buf.read(emailLen);
	buf.seekForward( sizeof(Byte) );

	ShortUserDetails details;
	details.setNick(nick);
	details.setFirstName(fn);
	details.setLastName(ln);
	details.setEmail(email);

	QString uin = uinRequests.dequeue();
	details.setUin(uin);
	shortDetails.insert(uin, details);

	emit q->shortUserDetailsAvailable(uin);

	// qDebug() << "short user info!" << "nick" << nick << "first name" << fn << "last name" << ln << "email" << email;
}

void UserInfoManager::Private::processBasicUserInfo(Buffer& buf)
{
	QString nick = buf.read(buf.getLEWord() - 1);
	buf.seekForward( sizeof(Byte) );
	lastUserDetails.setNick(nick);

	QString firstname = buf.read(buf.getLEWord() - 1);
	buf.seekForward( sizeof(Byte) );
	lastUserDetails.setFirstName(firstname);

	QString lastname = buf.read(buf.getLEWord() - 1);
	buf.seekForward( sizeof(Byte) );
	lastUserDetails.setLastName(lastname);

	QString email = buf.read(buf.getLEWord() - 1);
	buf.seekForward( sizeof(Byte) );
	lastUserDetails.setEmail(email);

	QString homecity = buf.read(buf.getLEWord() - 1);
	buf.seekForward( sizeof(Byte) );
	lastUserDetails.setHomeCity(homecity);

	QString homestate = buf.read(buf.getLEWord() - 1);
	buf.seekForward( sizeof(Byte) );
	lastUserDetails.setHomeState(homestate);

	QString homephone = buf.read(buf.getLEWord() - 1);
	buf.seekForward( sizeof(Byte) );
	lastUserDetails.setHomePhone(homephone);

	QString homefax = buf.read(buf.getLEWord() - 1);
	buf.seekForward( sizeof(Byte) );
	lastUserDetails.setHomeFax(homefax);

	QString homeaddress = buf.read(buf.getLEWord() - 1);
	buf.seekForward( sizeof(Byte) );
	lastUserDetails.setHomeAddress(homeaddress);

	QString cellphone = buf.read(buf.getLEWord() - 1);
	buf.seekForward( sizeof(Byte) );
	lastUserDetails.setCellPhone(cellphone);

	QString homezip = buf.read(buf.getLEWord() - 1);
	buf.seekForward( sizeof(Byte) );
	lastUserDetails.setHomeZipCode(homezip);

	// Byte gmt_offset = buf.getByte();

	// qDebug() << "basic info!" << "nick" << nick << "first name" << firstname << "last name" << lastname << "gmt offset" << gmt_offset;
}

void UserInfoManager::Private::processMoreUserInfo(Buffer& buf)
{
	Word age = buf.getWord();
	lastUserDetails.setAge(age);
	Byte gender = buf.getByte();
	qDebug() << "GENDER!!!" << gender;
	/* TODO: process gender */

	QString homepage = buf.read(buf.getLEWord() - 1);
	buf.seekForward( sizeof(Byte) );
	lastUserDetails.setHomepage(homepage);

	Word birth_year = buf.getLEWord();
	Byte birth_month = buf.getByte();
	Byte birth_day = buf.getByte();
	lastUserDetails.setBirthDate( QDate(birth_year, birth_month, birth_day) );

	Byte lang1 = buf.getByte();
	Byte lang2 = buf.getByte();
	Byte lang3 = buf.getByte();
	Q_UNUSED(lang1)
	Q_UNUSED(lang2)
	Q_UNUSED(lang3)
	/* TODO: Process user languages */

	buf.seekForward( sizeof(Word) ); // unknown data

	QString origCity = buf.read(buf.getLEWord() - 1);
	buf.seekForward( sizeof(Byte) );
	lastUserDetails.setOriginalCity(origCity);

	QString origState = buf.read(buf.getLEWord() - 1);
	buf.seekForward( sizeof(Byte) );
	lastUserDetails.setOriginalState(origState);

	/* Word countryCode = buf.getLEWord();
	Byte timezone = buf.getByte(); */
}

void UserInfoManager::Private::processEmailUserInfo(Buffer& buf)
{
	Byte emailCount = buf.getByte();
	for (int i = 0; i < emailCount; ++i) {
		Byte isPrivate = buf.getByte();
		Q_UNUSED(isPrivate)

		QString email = buf.read(buf.getLEWord() - 1);
		buf.seekForward( sizeof(Byte) );

		lastUserDetails.addEmail(email);
	}
}

void UserInfoManager::Private::processHomepageUserInfo(Buffer& buf)
{
	Q_UNUSED(buf)
	/* what the hell is this?
	 *
	buf.seekForward( sizeof(Byte) ); // is enabled

	Word cat = buf.getLEWord(); // category code

	QString keywords = buf.read(buf.getLEWord() - 1);
	buf.seekForward( sizeof(Byte) );
	*/
}

void UserInfoManager::Private::processWorkUserInfo(Buffer& buf)
{
	QString city = buf.read(buf.getLEWord() - 1);
	buf.seekForward( sizeof(Byte) );
	lastUserDetails.setWorkCity(city);

	QString state = buf.read(buf.getLEWord() - 1);
	buf.seekForward( sizeof(Byte) );
	lastUserDetails.setWorkState(state);

	QString phone = buf.read(buf.getLEWord() - 1);
	buf.seekForward( sizeof(Byte) );
	lastUserDetails.setWorkPhone(phone);

	QString fax = buf.read(buf.getLEWord() - 1);
	buf.seekForward( sizeof(Byte) );
	lastUserDetails.setWorkFax(fax);

	QString address = buf.read(buf.getLEWord() - 1);
	buf.seekForward( sizeof(Byte) );
	lastUserDetails.setWorkAddress(address);

	QString zipcode = buf.read(buf.getLEWord() - 1);
	buf.seekForward( sizeof(Byte) );
	lastUserDetails.setWorkZipCode(zipcode);

	Word countryCode = buf.getLEWord();
	Q_UNUSED(countryCode)
	/* TODO: process country-code */

	QString company = buf.read(buf.getLEWord() - 1);
	buf.seekForward( sizeof(Byte) );
	lastUserDetails.setWorkCompany(company);

	QString department = buf.read(buf.getLEWord() - 1);
	buf.seekForward( sizeof(Byte) );
	lastUserDetails.setWorkDepartment(department);

	QString position = buf.read(buf.getLEWord() - 1);
	buf.seekForward( sizeof(Byte) );
	lastUserDetails.setWorkPosition(position);

	buf.seekForward( sizeof(Word) ); // occupation code

	QString webpage = buf.read(buf.getLEWord() - 1);
	buf.seekForward( sizeof(Byte) );
	lastUserDetails.setWorkWebpage(webpage);

	qDebug() << "work info!" << company << "page" << webpage;
}

void UserInfoManager::Private::processNotesUserInfo(Buffer& buf)
{
	QString notes = buf.read(buf.getLEWord() - 1);
	buf.seekForward( sizeof(Byte) );

	qDebug() << "notes!" << notes;
	lastUserDetails.setNotes(notes);
}

void UserInfoManager::Private::processInterestsUserInfo(Buffer& buf)
{
	Q_UNUSED(buf)
	/* TODO: process user interests */
/*
	Byte interestCount = buf.getByte();

	for (int i = 0; i < interestCount; ++i) {
		Word category = buf.getLEWord();

		QString interest = buf.read(buf.getLEWord() - 1);
		buf.seekForward( sizeof(Byte) );

		qDebug() << "interest!" << "cat" << category << "interest" << interest;
	}
*/
}

void UserInfoManager::Private::processAffiliationsUserInfo(Buffer& buf)
{
	Q_UNUSED(buf)

	if ( uinRequests.isEmpty() ) {
		qDebug() << "[ICQ:UIM]" << "Error processing User Details. Request queue is empty";
		lastUserDetails.clear();
		return;
	}

	QString uin = uinRequests.dequeue();
	lastUserDetails.setUin(uin);
	fullDetails.insert(uin, lastUserDetails);
	lastUserDetails.clear();

	emit q->userDetailsAvailable(uin);

/*	Byte pastCount = buf.getByte();

	for (int i = 0; i < pastCount; ++i) {
		Word category = buf.getLEWord();

		QString past = buf.read(buf.getLEWord() - 1);
		buf.seekForward( sizeof(Byte) );

		qDebug() << "past affiliation!" << "cat" << category << "interest" << past;
	}

	Byte affiliationCount = buf.getByte();

	for (int i = 0; i < affiliationCount; ++i) {
		Word category = buf.getLEWord();

		QString affiliation = buf.read(buf.getLEWord() - 1);
		buf.seekForward( sizeof(Byte) );

		qDebug() << "affiliation!" << "cat" << category << "interest" << affiliation;
	}
*/
}

UserInfoManager::UserInfoManager(Socket *socket, QObject *parent)
	: QObject(parent)
{
	d = new Private;
	d->q = this;
	d->socket = socket;

	QObject::connect( d->socket, SIGNAL( incomingSnac(SnacBuffer&) ), SLOT( incomingSnac(SnacBuffer&) ) );
}

UserInfoManager::~UserInfoManager()
{
}

UserInfo UserInfoManager::getUserInfo(const QString& uin)
{
	return d->userInfoList.value(uin);
}

quint16 UserInfoManager::getUserStatus(const QString& uin) const
{
	if ( d->statusList.contains(uin) ) {
		return d->statusList.value(uin);
	}
	return UserInfo::Offline;
}

/**
 * Sends request for own user-info details.
 */
void UserInfoManager::requestOwnUserDetails(const QString& uin)
{
	Buffer buf;
	buf.addLEWord(0x04B2); // data subtype
	buf.addLEDWord( uin.toUInt() );

	d->socket->sendMetaRequest(0x07D0, buf);
	d->uinRequests.enqueue(uin);
}

/**
 * Sends request for user-details for selected @a uin.
 */
void UserInfoManager::requestUserDetails(const QString& uin)
{
	if ( d->fullDetails.contains(uin) ) {
		emit userDetailsAvailable(uin);
		return;
	}

	Buffer buf;
	buf.addLEWord(0x04D0); // data subtype
	buf.addLEDWord( uin.toUInt() );

	d->socket->sendMetaRequest(0x07D0, buf);
	d->uinRequests.enqueue(uin);
}

/**
 * Sends request for short user-details for selected @a uin.
 */
void UserInfoManager::requestShortDetails(const QString& uin)
{
	if ( d->shortDetails.contains(uin) ) {
		emit shortUserDetailsAvailable(uin);
		return;
	}

	Buffer buf;
	buf.addLEWord(0x04BA); // data subtype
	buf.addLEDWord( uin.toUInt() );

	d->socket->sendMetaRequest(0x07D0, buf);
	d->uinRequests.enqueue(uin);
}

ShortUserDetails UserInfoManager::shorUserDetails(const QString& uin) const
{
	return d->shortDetails.value(uin);
}

UserDetails UserInfoManager::userDetails(const QString& uin) const
{
	return d->fullDetails.value(uin);
}

/**
 * Clears short user details for selected @a uin. This function should be used if session wants to re-request new short user details.
 */
void UserInfoManager::clearShortUserDetails(const QString& uin)
{
	d->fullDetails.remove(uin);
}

/**
 * Clears user details for selected @a uin. This function should be used if session wants to re-request new user details.
 */
void UserInfoManager::clearUserDetails(const QString& uin)
{
	d->shortDetails.remove(uin);
}

void UserInfoManager::incomingMetaInfo(Word type, Buffer& data)
{
	if ( type != 0x07DA ) {
		return;
	}

	Word subtype = data.getLEWord();
	Byte success = data.getByte();
	qDebug() << "[ICQ:UIM]" << "metadata subtype" << QString::number(subtype, 16) << "success byte" << QString::number( success, 16);
	if ( success != 0x0A ) {
		return;
	}

	switch ( subtype ) {
		case 0x0104:
			d->processShortUserInfo(data);
			break;
		case 0x00C8:
			d->processBasicUserInfo(data);
			break;
		case 0x00DC:
			d->processMoreUserInfo(data);
			break;
		case 0x00EB:
			d->processEmailUserInfo(data);
			break;
		case 0x010E:
			d->processHomepageUserInfo(data);
			break;
		case 0x00D2:
			d->processWorkUserInfo(data);
			break;
		case 0x00E6:
			d->processNotesUserInfo(data);
			break;
		case 0x00F0:
			d->processInterestsUserInfo(data);
			break;
		case 0x00FA:
			d->processAffiliationsUserInfo(data);
			break;
		default:
			qDebug() << "[ICQ:UIM]" << "unknown subtype" << QString::number(subtype, 16);
			break;
	}
}

void UserInfoManager::incomingSnac(SnacBuffer& snac)
{
	if ( snac.family() == 0x01 && snac.subtype() == 0x0F ) {
		d->processOwnUserInfo(snac);
	} else if ( snac.family() == 0x03 ) {
		if ( snac.subtype() == 0x0B ) {
			d->processUserOnlineNotification(snac);
		} else if ( snac.subtype() == 0x0C ) {
			d->processUserOfflineNotification(snac);
		}
	}
}


} /* end of namespace ICQ */
