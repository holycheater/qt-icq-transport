/*
 * Stanza.cpp
 *
 *  Created on: Sep 1, 2008
 *      Author: holycheater
 */

#include <QDomDocument>

#include "Stanza.h"

#include "jid/jid.h"

namespace X
{


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
