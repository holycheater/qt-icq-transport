/*
 * IQ.cpp - XMPP Info/Query stanza
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

#include "IQ.h"
#include "jid/jid.h"

#include <QSharedData>

namespace XMPP {


class IQ::Private : public QSharedData
{
	public:
		Private();
		Private(const Private& other);

		Jid from, to;
		QString id;
		int type;

		static quint32 nextId;
};

quint32 IQ::Private::nextId = 0;

IQ::Private::Private()
	: QSharedData()
{
	type = -1;
	id = QString::number(++nextId, 16);
}

IQ::Private::Private(const Private& other)
	: QSharedData(other)
{
	from = other.from;
	to = other.to;
	id = QString::number(++nextId, 16);
	type = other.type;
}

/**
 * Default constructor for info/query stanza
 */
IQ::IQ()
{
	d = new Private;
}

/**
 * Copy constructor for info/query stanza
 *
 * @param other		IQ stanza to clone
 */
IQ::IQ(const IQ& other)
	: d(other.d)
{
}

/**
 * Destroys IQ stanza object
 */
IQ::~IQ()
{
}

IQ& IQ::operator=(const IQ& other)
{
	d = other.d;
	return *this;
}

bool IQ::isValid() const
{
	/* TODO: probably IQ stanza also needs to have 'from' or 'to' field set*/
	return d->type != 1;
}

/**
 * IQ stanza 'to' field
 *
 * @return			recepient jabber-id
 */
Jid IQ::to() const
{
	return d->to;
}

/**
 * IQ stanza 'from' field
 *
 * @return			sender jabber-id
 */
Jid IQ::from() const
{
	return d->from;
}

/**
 * IQ stanza 'type' field
 *
 * @return			type string
 *
 * @sa Type
 */
QString IQ::type() const
{
	return typeToString(d->type);
}

/**
 * IQ stanza 'id' field.
 *
 * @return			unique stanza id string
 */
QString IQ::id() const
{
	return d->id;
}

/**
 * Sets IQ stanza 'to' field to @a toJid jabber-id.
 */
void IQ::setTo(const Jid& toJid)
{
	d->to = toJid;
}

/**
 * Sets IQ stanza 'from' field to @a fromJid jabber-id.
 */
void IQ::setFrom(const Jid& fromJid)
{
	d->from = fromJid;
}

/**
 * Sets IQ stanza 'type' feld to @a type string.
 */
void IQ::setType(const QString& type)
{
	d->type = stringToType(type);
}

/**
 * Sets IQ stanza 'type' field to @a type string.
 */
void IQ::setType(Type type)
{
	d->type = (int)type;
}

/**
 * Sets IQ stanza 'id' field to @a id number converted to hex.
 */
void IQ::setId(int id)
{
	d->id = QString::number(id, 16);
}

QString IQ::typeToString(int type)
{
	switch (type) {
		case Get:
			return "get";
			break;
		case Set:
			return "set";
		case Result:
			return "result";
		case Error:
			return "error";
		default:
			return QString();
	}
}

int IQ::stringToType(const QString& type)
{
	if (type == "get") {
		return Get;
	}
	if (type == "set") {
		return Set;
	}
	if (type == "result") {
		return Result;
	}
	if (type == "error") {
		return Error;
	}
	return -1;
}

/**
 * @enum Type
 * @brief This enum describes possible Info/Query stanza types (get,set,result, error).
 *
 * @var get
 * @var set
 * @var result
 * @var error
 */


} /* end of namespace XMPP */
