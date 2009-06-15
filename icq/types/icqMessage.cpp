/*
 * icqMessage.cpp - ICQ Message data type.
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

#include "icqMessage.h"

#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QTextCodec>

//#include <QtDebug>

namespace ICQ
{


class Message::Private : public QSharedData
{
	public:
		Private();
		Private(const Private& other);

		Byte channel;
		Byte type;
		Byte flags;

		QByteArray icbmCookie;
		QByteArray text;

		QDateTime timestamp;

		QString sender;
		QString receiver;

		Encoding encoding;

		bool bOffline;
};

Message::Private::Private()
	: QSharedData()
{
	channel = 0;

	type = InvalidType;
	flags = 0;

	encoding = UserDefined;

	bOffline = false;
}

Message::Private::Private(const Private& other)
	: QSharedData(other)
{
	channel = other.channel;

	sender = other.sender;
	receiver = other.receiver;

	timestamp = other.timestamp;

	type = other.type;
	flags = other.flags;

	text = other.text;

	encoding = other.encoding;

	bOffline = other.bOffline;
}

Message::Message()
{
	d = new Private;
}

Message::Message(const Message& other)
	: d(other.d)
{
}

Message& Message::operator=(const Message& other)
{
	d = other.d;
	return *this;
}

Message::~Message()
{
}

bool Message::isEmpty() const
{
	if ( d->text.isEmpty() ) {
		return true;
	}
	return false;
}

bool Message::isValid() const
{
	if ( d->type == InvalidType ) {
		return false;
	}
	return true;
}

bool Message::isOffline() const
{
	return d->bOffline;
}

void Message::setOffline(bool offline)
{
	d->bOffline = offline;
}

Byte Message::channel() const
{
	return d->channel;
}

void Message::setChannel(Byte channel)
{
	d->channel = channel;
}

Message::Encoding Message::encoding() const
{
	return d->encoding;
}

void Message::setEncoding(Encoding enc)
{
	d->encoding = enc;
}

Byte Message::flags() const
{
	return d->flags;
}

void Message::setFlags(Byte flags)
{
	d->flags = flags;
}

QByteArray Message::icbmCookie() const
{
	return d->icbmCookie;
}

void Message::setIcbmCookie(const QByteArray& cookie)
{
	d->icbmCookie = cookie;
}

QString Message::receiver() const
{
	return d->receiver;
}

void Message::setReceiver(DWord uin)
{
	d->receiver = QString::number(uin);
}

void Message::setReceiver(const QString& uin)
{
	d->receiver = uin;
}

QString Message::sender() const
{
	return d->sender;
}

void Message::setSender(DWord uin)
{
	d->sender = QString::number(uin);
}

void Message::setSender(const QString& uin)
{
	d->sender = uin;
}

QByteArray Message::text() const
{
	return d->text;
}

QString Message::text(QTextCodec *codec) const
{
	switch ( d->encoding )
	{
		case UserDefined:
			//qDebug() << "[ICQ:Message]" << "user-defined encoding";
			return codec->toUnicode(d->text);
			break;
		case Ascii:
			//qDebug() << "[ICQ:Message]" << "ascii encoding";
			return QString::fromAscii( d->text.data(), d->text.size() );
			break;
		case Latin1:
			//qDebug() << "[ICQ:Message]" << "latin1 encoding";
			return QString::fromLatin1( d->text.data(), d->text.size() );
			break;
		case Utf8:
			//qDebug() << "[ICQ:Message]" << "utf-8 encoding";
			return QString::fromUtf8( d->text.data(), d->text.size() );
			break;
		case Ucs2:
		{
			int len = d->text.size() / 2;
			QString result;

			int p = 0;
			for ( int i = 0; i < len; i++ )
			{
				char row = d->text[p++];
				char cell = d->text[p++];
				result += QChar(cell, row);
			}
			if ( result.at(len - 1).isNull() ) {
				result.resize(len - 1);
			}
			return result;
		}
			break;
		default:
			break;
	}
	return QString();
}

void Message::setText(const QByteArray& text)
{
	d->text = text;
}

QDateTime Message::timestamp() const
{
	return d->timestamp;
}

void Message::setTimestamp(QDateTime timestamp)
{
	d->timestamp = timestamp;
}

void Message::setTimestamp(DWord timestamp_t)
{
	d->timestamp = QDateTime::fromTime_t(timestamp_t);
}

Byte Message::type() const
{
	return d->type;
}

void Message::setType(Byte type)
{
	d->type = type;
}


} /* end of namespace ICQ */

// vim:sw=4:ts=4:noet:nowrap
