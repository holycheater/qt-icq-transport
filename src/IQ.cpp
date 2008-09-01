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


/**
 * Default constructor for info/query stanza
 */
IQ::IQ()
	: Stanza()
{
}

/**
 * Destroys IQ stanza object
 */
IQ::~IQ()
{
}

/**
 * Sets IQ stanza 'type' field to @a type string.
 */
void IQ::setType(Type type)
{
	Stanza::setType( typeToString(type) );
}

QString IQ::typeToString(Type type)
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
