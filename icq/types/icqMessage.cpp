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

#include <QTextCodec>

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
};

Message::Private::Private()
	: QSharedData()
{
	channel = 0;

	type = 0;
	flags = 0;

	encoding = UserDefined;
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

Byte Message::channel() const
{
	return d->channel;
}

void Message::setChannel(Byte channel)
{
	d->channel = channel;
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
			return codec->toUnicode(d->text);
			break;
		case Ascii:
			return QString::fromAscii( d->text.data(), d->text.size() );
			break;
		case Latin1:
			return QString::fromLatin1( d->text.data(), d->text.size() );
			break;
		case Utf8:
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
