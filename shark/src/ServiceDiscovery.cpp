/*
 * ServiceDiscovery.h - XMPP Service Discovery (XEP-0030)
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

/*
 * TODO: Implement disco#items.
 */

#include <QDomDocument>
#include <QListIterator>
#include <QStringList>
#include "core/Jid.h"

#include "ServiceDiscovery.h"

namespace XMPP
{


/* ***************************************************************************
 * XMPP::DiscoInfo
 *************************************************************************** */

/**
 * @class DiscoInfo
 * @brief Represents DiscoInfo element (list of identities and features).
 * @details More details can be found @link http://www.xmpp.org/extensions/xep-0030.html here @endlink
 */

/**
 * Constructs disco-info object.
 */
DiscoInfo::DiscoInfo()
{
}

/**
 * Destroys disco-info object.
 */
DiscoInfo::~DiscoInfo()
{
}

/**
 * Adds identity to disco-info identities list.
 *
 * @param category
 * @param type
 * @param name
 */
void DiscoInfo::addIdentity(const QString& category, const QString& type, const QString& name)
{
	m_identities << Identity(category, type, name);
}

/**
 * Adds a @a feature to the disco-info features list.
 */
void DiscoInfo::addFeature(const QString& feature)
{
	m_features << feature;
}

/**
 * Inserts identity and feature lists into a DOM @a element.
 */
void DiscoInfo::pushToDomElement(QDomElement& element) const
{
	QDomDocument doc = element.ownerDocument();
	XIdentityListIterator ii(m_identities);
	while ( ii.hasNext() ) {
		QDomElement eIdentity = doc.createElement("identity");
		Identity identity = ii.next();

		eIdentity.setAttribute( "category", identity.category() );
		eIdentity.setAttribute( "type", identity.type() );
		if ( !identity.name().isEmpty() ) {
			eIdentity.setAttribute( "name", identity.name() );
		}

		element.appendChild(eIdentity);
	}
	QStringListIterator fi(m_features);
	while ( fi.hasNext() ) {
		QDomElement eFeature = doc.createElement("feature");
		eFeature.setAttribute( "var", fi.next() );
		element.appendChild(eFeature);
	}
}

/**
 * Adds a feature to disco-info features list.
 */
DiscoInfo& DiscoInfo::operator<<(const QString& feature)
{
	m_features << feature;
	return *this;
}

/**
 * Adds an identity to disco-info identities list.
 */
DiscoInfo& DiscoInfo::operator<<(const Identity& identity)
{
	m_identities << identity;
	return *this;
}

/* ***************************************************************************
 * XMPP::DiscoInfo::Identity
 *************************************************************************** */

/**
 * @class DiscoInfo::Identity
 * @brief Represents DiscoInfo identity element.
 */

DiscoInfo::Identity::Identity()
{
}

DiscoInfo::Identity::Identity(const QString& category, const QString& type, const QString& name)
{
	m_category = category;
	m_type = type;
	m_name = name;
}

DiscoInfo::Identity::~Identity()
{
}

/**
 * Returns identity category string.
 *
 * @sa type(), name()
 */
QString DiscoInfo::Identity::category() const
{
	return m_category;
}

/**
 * Returns identity name string.
 *
 * @sa category(), type()
 */
QString DiscoInfo::Identity::name() const
{
	return m_name;
}

/**
 * Returns identity type string.
 *
 * @sa category(), name()
 */
QString DiscoInfo::Identity::type() const
{
	return m_type;
}

/**
 * Sets identity category to @a category
 *
 * @sa setType(), setName()
 */
void DiscoInfo::Identity::setCategory(const QString& category)
{
	m_category = category;
}

/**
 * Sets identity name to @a name.
 *
 * @sa setCategory(), setType()
 */
void DiscoInfo::Identity::setName(const QString& name)
{
	m_name = name;
}

/**
 * Sets identity type to @a type.
 *
 * @sa setCategory(), setName()
 */
void DiscoInfo::Identity::setType(const QString& type)
{
	m_type = type;
}

/* ***************************************************************************
 * XMPP::DiscoItem
 *************************************************************************** */

/**
 * @class DiscoItem
 * @brief Represents service discovery disco-item.
 */

/**
 * Constructs disco-item element.
 */
DiscoItem::DiscoItem()
{
}

/**
 * Constructs disco-item element with initialising it's fields.
 * @param jid		Item jabber-id.
 * @param node		Item node.
 * @param name		Item name.
 */
DiscoItem::DiscoItem(const Jid& jid, const QString& node, const QString& name)
{
	m_jid = jid;
	m_name = name;
	m_node = node;
}

/**
 * Destroys disco-item element.
 */
DiscoItem::~DiscoItem()
{
}

/**
 * Returns disco-item jabber-id.
 *
 * @sa name(), node()
 */
Jid DiscoItem::jid() const
{
	return m_jid;
}

/**
 * Returns disco-item name.
 *
 * @sa jid(), node()
 */
QString DiscoItem::name() const
{
	return m_name;
}

/**
 * Returns disco-item node.
 *
 * @sa jid(), name()
 */
QString DiscoItem::node() const
{
	return m_node;
}

/**
 * Sets disco-item jabber-id to @a jid.
 */
void DiscoItem::setJid(const Jid& jid)
{
	m_jid = jid;
}

/**
 * Sets disco-item name to @a name.
 */
void DiscoItem::setName(const QString& name)
{
	m_name = name;
}

/**
 * Sets disco-item node to @a node.
 */
void DiscoItem::setNode(const QString& node)
{
	m_node = node;
}

/* ***************************************************************************
 * XMPP::DiscoItems
 *************************************************************************** */

/**
 * @class DiscoItems
 * @brief Represents list of service discovery disco-items.
 */

/**
 * Constructs disco-items list.
 */
DiscoItems::DiscoItems()
{
}

/**
 * Destroys disco-items list.
 */
DiscoItems::~DiscoItems()
{
}

/**
 * Adds an @a item to disco-items list.
 */
void DiscoItems::addItem(const DiscoItem& item)
{
	m_items << item;
}

/**
 * Clears disco-items list.
 */
void DiscoItems::clear()
{
	m_items.clear();
}

/**
 * Returns list of disco-items.
 */
QList<DiscoItem> DiscoItems::items() const
{
	return m_items;
}

/**
 * Inserts list of disco-items into DOM @a element.
 */
void DiscoItems::pushToDomElement(QDomElement& element) const
{
	QDomDocument doc = element.ownerDocument();

	QListIterator<DiscoItem> i(m_items);
	while ( i.hasNext() ) {
		DiscoItem item = i.next();

		QDomElement eItem = doc.createElement("item");
		eItem.setAttribute( "jid", item.jid() );
		if ( !item.name().isEmpty() ) {
			eItem.setAttribute( "name", item.name() );
		}
		if ( !item.node().isEmpty() ) {
			eItem.setAttribute( "node", item.node() );
		}
		element.appendChild(eItem);
	}
}


} /* end of namespace XMPP */
