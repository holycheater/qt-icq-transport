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

#include <QDomDocument>

#include "IQ.h"

namespace X {

/**
 * @class IQ
 * @brief Represents XMPP info/query stanza object.
 */


/**
 * Default constructor for info/query stanza
 */
IQ::IQ()
	: Stanza()
{
	setId( QString::number(++id, 16) );

	QDomElement root = doc()->createElement("iq");
	doc()->appendChild(root);

	m_element = doc()->createElement("query");
	doc()->documentElement().appendChild(m_element);
}

/**
 * Constructs a deep copy of @a other iq stanza.
 */
IQ::IQ(const IQ& other)
	: Stanza(other)
{
	setId( QString::number(++id, 16) );
	m_element = other.m_element;
}

/**
 * Constructs info/query stanza from a DOM document. Constructor makes a deep copy of QDomDocument.
 *
 * @param document	DOM document
 */
IQ::IQ(const QDomDocument& document)
	: Stanza(document)
{
	m_element = doc()->documentElement().firstChild().toElement();
}

/**
 * Constructs info/query stanza from a DOM element setting @a element as root-element.
 * Constructor makes a deep copy of QDomElement.
 */
IQ::IQ(const QDomElement& element)
	: Stanza(element)
{
	m_element = doc()->documentElement().firstChild().toElement();
}

/**
 * Destroys IQ stanza object
 */
IQ::~IQ()
{
}

/**
 * Gives access to info/query stanza child element.
 *
 * @return Reference to stanza child element.
 */
QDomElement& IQ::childElement()
{
	return m_element;
}

/**
 * Sets IQ stanza 'type' field to @a type string.
 */
void IQ::setType(Type type)
{
	setType( typeToString(type) );
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

/**
 * Converts string type representation of @a type to enum-type.
 * @param type	string representation of IQ stanza type
 * @return		Type enum value of the given type.
 */
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

int IQ::id = 0;

/**
 * @enum IQ::Type
 * @brief This enum describes possible Info/Query stanza types.
 *
 * @var Get		IQ is an information request.
 * @var Set		IQ provides required data, sets new values or replace existing ones.
 * @var Result	IQ response to a successful get/set request
 * @var Error	An error has occured during result delivery or get/set request.
 */


} /* end of namespace X */
