/*
 * Stanza.h - XMPP Stanza
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

#include "Stanza.h"

#include "jid/jid.h"

namespace X
{

/**
 * @class Stanza
 * @brief Represents XMPP Stanza.
 */


using namespace XMPP;

/**
 * Default constructor for Stanza element.
 */
Stanza::Stanza()
{
}

/**
 * Constructs a deep copy of @a other.
 */
Stanza::Stanza(const Stanza& other)
{
	m_doc = other.m_doc.cloneNode(true).toDocument();
}

/**
 * Constructs stanza from a DOM document. Constructor makes a deep copy of QDomDocument.
 *
 * @param document	DOM document
 */
Stanza::Stanza(const QDomDocument& document)
{
	m_doc = document.cloneNode(true).toDocument();
}

/**
 * Constructs a stanza from a DOM element setting @a element as root-element.
 * Constructor makes a deep copy of QDomElement.
 */
Stanza::Stanza(const QDomElement& element)
{
	QDomNode root = m_doc.importNode(element, true);
	m_doc.appendChild(root);
}

/**
 * Destroys stanza object.
 */
Stanza::~Stanza()
{
}

/**
 * Returns true if this stanza is valid.
 */
bool Stanza::isValid() const
{
	/* TODO: Some stanzas can have no type set.. Some stanzas require other attributes, like IQ requires ID to be set */
	return !m_doc.documentElement().attribute("type").isEmpty();
}

/**
 * Returns recipient's jabber-id.
 */
Jid Stanza::to() const
{
	return m_doc.documentElement().attribute("to");
}

/**
 * Returns sender's jabber-id.
 */
Jid Stanza::from() const
{
	return m_doc.documentElement().attribute("from");
}

/**
 * Returns stanza type attribute.
 */
QString Stanza::type() const
{
	return m_doc.documentElement().attribute("type");
}

/**
 * Returns stanza id attribute.
 */
QString Stanza::id() const
{
	return m_doc.documentElement().attribute("id");
}

/**
 * Set stanza recipient's jabber-id to @a toJid.
 */
void Stanza::setTo(const XMPP::Jid& toJid)
{
	m_doc.documentElement().setAttribute( "to", toJid.full() );
}

/**
 * Set stanza sender's jabber-id to @a fromJid.
 */
void Stanza::setFrom(const XMPP::Jid& fromJid)
{
	m_doc.documentElement().setAttribute( "from",fromJid.full() );
}

/**
 * Set stanza type to @a type.
 */
void Stanza::setType(const QString& type)
{
	m_doc.documentElement().setAttribute("type", type);
}

/**
 * Set stanza id attribute to @a id
 */
void Stanza::setId(const QString& id)
{
	m_doc.documentElement().setAttribute("id", id);
}

/**
 * Serializes internal XML data to string.
 * @return string with xml data
 */
QString Stanza::toString() const
{
	return m_doc.toString();
}

/**
 * Returns pointer to DOM document containing stanza element.
 */
QDomDocument* Stanza::doc()
{
	return &m_doc;
}

/**
 * Returns const pointer to DOM document containing stanza element.
 *
 * @overload
 */
QDomDocument* Stanza::doc() const
{
	return const_cast<QDomDocument*>(&m_doc);
}

/**
 * Sets internal stanza element called @a name to @a value
 *
 * @param name		tag name
 * @param value		tag value
 */
void Stanza::setProperty(const QString& name, const QString& value)
{
	QDomElement element;
	if ( !m_doc.documentElement().elementsByTagName(name).isEmpty() ) {
		element = m_doc.documentElement().elementsByTagName(name).item(0).toElement();
		m_doc.documentElement().removeChild(element);
	}
	element = m_doc.createElement(name);
	m_doc.documentElement().appendChild(element);

	QDomText text = m_doc.createTextNode(value);
	element.appendChild(text);
}


}
