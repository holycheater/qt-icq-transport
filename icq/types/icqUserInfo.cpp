/*
 * icqUserInfo.cpp - ICQ User Info block.
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

#include "icqUserInfo.h"
#include "icqTlv.h"
#include "icqTlvChain.h"

#include <QHostAddress>
#include <QSet>

namespace ICQ
{


class UserInfo::Private : public QSharedData
{
	public:
		Private();
		Private(const Private& other);

		/* user id - UIN number for ICQ */
		QString userId;
		/* contained in TLV 0x01. User class (nick flags) */
		DWord classFlags;
		/* contained in TLV 0x03. User sign-on unix timestamp */
		DWord signOnTime;
		/* contained in TLV 0x05. User registration unix timestamp */
		DWord registerTime;

		Word onlineStatus;
		Word statusFlags;

		/* user external IP address. Contained in TLV 0x0A */
		DWord externalIP;

		/* Various info contained in TLV 0x0C */
		DWord dcInternalIP; // DC internal IP
		DWord dcPort; // DC TCP port
		Byte dcType; // DC Type: disabled/https/socks/normal/enabled
		Word dcVersion; // ICQ Protocol version
		DWord dcAuthCookie;
		DWord clientFeatures;
		DWord lastInfoUpdateTime;

		/* user idle time in seconds (contained in TLV 0x04) */
		Word idleTime;

		/* Capabilities list */
		QList<Guid> capabilities;

		/* client ID GUID */
		Guid clientIdent;

		/* UserInfo block tlv flags. Needed for merging this block into other */
		QSet<Word> tlvSet;
};

UserInfo::Private::Private()
	: QSharedData()
{
	classFlags 			= 0;
	signOnTime 			= 0;
	registerTime 		= 0;
	onlineStatus 		= 0xFFFF;
	statusFlags 		= 0;
	externalIP 			= 0;

	dcInternalIP 		= 0;
	dcPort 				= 0;
	dcAuthCookie		= 0;
	dcType 				= 0;
	dcVersion 			= 0;

	clientFeatures		= 0;
	lastInfoUpdateTime	= 0;

	idleTime			= 0;
}
UserInfo::Private::Private(const Private& other)
	: QSharedData(other)
{
	userId = other.userId;
	classFlags = other.classFlags;
	signOnTime = other.signOnTime;
	registerTime = other.registerTime;

	onlineStatus = other.onlineStatus;
	statusFlags = other.statusFlags;

	externalIP = other.externalIP;

	dcInternalIP = other.dcInternalIP;
	dcPort = other.dcPort;
	dcAuthCookie = other.dcAuthCookie;
	dcType = other.dcType;
	dcVersion = other.dcVersion;

	clientFeatures = other.clientFeatures;
	lastInfoUpdateTime = other.lastInfoUpdateTime;

	idleTime = other.idleTime;

	capabilities = other.capabilities;
	clientIdent = other.clientIdent;

	tlvSet = other.tlvSet;
}

UserInfo::UserInfo()
{
	d = new Private;
}

UserInfo::UserInfo(const UserInfo& other)
	: d(other.d)
{
}

UserInfo& UserInfo::operator=(const UserInfo& other)
{
	d = other.d;
	return *this;
}

UserInfo::~UserInfo()
{
}

UserInfo UserInfo::fromBuffer(Buffer& buffer)
{
	UserInfo info;

	Byte nameLen = buffer.getByte();
	info.d->userId = buffer.read(nameLen);
	buffer.seekForward( sizeof(Word) ); // Warning Level
	Word tlvCount = buffer.getWord();
	TlvChain chain;
	for ( int i = 0; i < tlvCount; i++ ) {
		chain << Tlv::fromBuffer(buffer);
	}
	if ( chain.hasTlv(0x01) ) {
		info.d->classFlags = chain.getTlv(0x01).getDWord();
		info.d->tlvSet.insert(0x01);
	}
	if ( chain.hasTlv(0x03) ) {
		info.d->signOnTime = chain.getTlv(0x03).getDWord();
		info.d->tlvSet.insert(0x03);
	}
	if ( chain.hasTlv(0x04) ) {
		info.d->idleTime = chain.getTlv(0x04).getWord();
		info.d->tlvSet.insert(0x04);
	}
	if ( chain.hasTlv(0x05) ) {
		info.d->registerTime = chain.getTlv(0x05).getDWord();
		info.d->tlvSet.insert(0x05);
	}
	if ( chain.hasTlv(0x06) ) {
		Tlv tlv06 = chain.getTlv(0x06);
		info.d->statusFlags = tlv06.getWord();
		info.d->onlineStatus = tlv06.getWord();
		info.d->tlvSet.insert(0x06);
	}
	if ( chain.hasTlv(0x0A) ) {
		info.d->externalIP = chain.getTlv(0x0A).getDWord();
		info.d->tlvSet.insert(0x0A);
	}
	if ( chain.hasTlv(0x0C) ) {
		Tlv tlv0C = chain.getTlv(0x0C);
		info.d->dcInternalIP = tlv0C.getDWord();
		info.d->dcPort = tlv0C.getDWord();
		info.d->dcType = tlv0C.getByte();
		info.d->dcVersion = tlv0C.getWord();
		info.d->dcAuthCookie = tlv0C.getDWord();
		info.d->clientFeatures = tlv0C.getDWord();
		info.d->lastInfoUpdateTime = tlv0C.getDWord();
		info.d->tlvSet.insert(0x0C);
	}
	if ( chain.hasTlv(0x0D) ) {
		Tlv tlv0D = chain.getTlv(0x0D);
		while ( !tlv0D.atEnd() ) {
			info.d->capabilities << Guid::fromRawData( tlv0D.read(16) );
		}
		info.d->tlvSet.insert(0x0D);
	}

	return info;
}

void UserInfo::mergeFrom(const UserInfo& info)
{
	if ( info.hasTlv(0x01) ) {
		d->classFlags = info.d->classFlags;

		d->tlvSet.insert(0x01);
	}
	if ( info.hasTlv(0x03) ) {
		d->signOnTime = info.d->signOnTime;

		d->tlvSet.insert(0x03);
	}
	if ( info.hasTlv(0x04) ) {
		d->idleTime = info.d->idleTime;

		d->tlvSet.insert(0x04);
	}
	if ( info.hasTlv(0x05) ) {
		d->registerTime = info.d->registerTime;

		d->tlvSet.insert(0x05);
	}
	if ( info.hasTlv(0x06) ) {
		d->statusFlags = info.d->statusFlags;
		d->onlineStatus = info.d->onlineStatus;

		d->tlvSet.insert(0x06);
	}
	if ( info.hasTlv(0x0A) ) {
		d->externalIP = info.d->externalIP;

		d->tlvSet.insert(0x0A);
	}
	if ( info.hasTlv(0x0C) ) {
		d->dcInternalIP = info.d->dcInternalIP;
		d->dcPort = info.d->dcPort;
		d->dcType = info.d->dcType;
		d->dcVersion = info.d->dcVersion;
		d->dcAuthCookie = info.d->dcAuthCookie;
		d->clientFeatures = info.d->clientFeatures;
		d->lastInfoUpdateTime = info.d->lastInfoUpdateTime;

		d->tlvSet.insert(0x0C);
	}
	if ( info.hasTlv(0x0D) ) {
		d->capabilities = info.d->capabilities;

		d->tlvSet.insert(0x0D);
	}
}

void UserInfo::updateFromTlv(Tlv& tlv)
{
	switch ( tlv.type() ) {
		case 0x01:
			d->classFlags = tlv.getDWord();
			d->tlvSet.insert(0x01);
			break;
		case 0x03:
			d->signOnTime = tlv.getDWord();
			d->tlvSet.insert(0x03);
			break;
		case 0x06:
			d->statusFlags = tlv.getWord();
			d->onlineStatus = tlv.getWord();
			d->tlvSet.insert(0x06);
			break;
		default:
			break;
	}
}

QString UserInfo::userId() const
{
	return d->userId;
}

DWord UserInfo::classFlags() const
{
	return d->classFlags;
}

QDateTime UserInfo::signOnTime() const
{
	return QDateTime::fromTime_t(d->signOnTime);
}

QDateTime UserInfo::registerTime() const
{
	return QDateTime::fromTime_t(d->registerTime);
}

Word UserInfo::onlineStatus() const
{
	return d->onlineStatus;
}

Word UserInfo::statusFlags() const
{
	return d->statusFlags;
}

QString UserInfo::externalIP() const
{
	return QHostAddress(d->externalIP).toString();
}

QString UserInfo::internalIP() const
{
	return QHostAddress(d->dcInternalIP).toString();
}

DWord UserInfo::dcPort() const
{
	return d->dcPort;
}

Byte UserInfo::dcType() const
{
	return d->dcType;
}

Word UserInfo::dcVersion() const
{
	return d->dcVersion;
}

DWord UserInfo::dcAuthCookie() const
{
	return d->dcAuthCookie;
}

DWord UserInfo::clientFeatures() const
{
	return d->clientFeatures;
}

QDateTime UserInfo::lastInfoUpdateTime() const
{
	return QDateTime::fromTime_t(d->lastInfoUpdateTime);
}

Word UserInfo::idleTime() const
{
	return d->idleTime;
}

const QList<Guid> UserInfo::capabilities() const
{
	return d->capabilities;
}

bool UserInfo::hasCapability(Guid capability) const
{
	return d->capabilities.contains(capability);
}

bool UserInfo::hasCapability(int capId) const
{
	return d->capabilities.contains( Capabilities[capId] );
}

bool UserInfo::hasTlv(Word tlvType) const
{
	return d->tlvSet.contains(tlvType);
}


} /* end of namespace ICQ */

// vim:sw=4:ts=4:noet:nowrap
