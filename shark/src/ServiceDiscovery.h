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

#ifndef XMPP_SERVICE_DISCOVERY_H_
#define XMPP_SERVICE_DISCOVERY_H_

class QDomElement;
class QString;

#define NS_QUERY_DISCO_INFO  "http://jabber.org/protocol/disco#info"
#define NS_QUERY_DISCO_ITEMS "http://jabber.org/protocol/disco#items"

namespace XMPP
{

class Jid;


class DiscoInfo
{
	public:

		class Identity;

		DiscoInfo();
		virtual ~DiscoInfo();

		void addIdentity(const QString& category, const QString& type, const QString& name = "");
		void addFeature(const QString& feature);

		void pushToDomElement(QDomElement& element) const;

		DiscoInfo& operator<<(const QString& feature);
		DiscoInfo& operator<<(const Identity& identity);
	private:
		class Private;
		QSharedDataPointer<Private> d;
};

class DiscoInfo::Identity
{
	public:
		Identity();
		Identity(const QString& category, const QString& type, const QString& name = "");
		virtual ~Identity();

		QString category() const;
		QString type() const;
		QString name() const;

		void setCategory(const QString& category);
		void setType(const QString& type);
		void setName(const QString& name);
	private:
		QString m_category;
		QString m_type;
		QString m_name;
};

class DiscoItem
{
	public:
		DiscoItem();
		DiscoItem(const Jid& jid, const QString& node = "", const QString& name = "");
		virtual ~DiscoItem();

		Jid jid() const;
		QString name() const;
		QString node() const;

		void setJid(const Jid& jid);
		void setName(const QString& name);
		void setNode(const QString& node);
	private:
		Jid m_jid;
		QString m_name;
		QString m_node;
};

class DiscoItems
{
	public:
		DiscoItems();
		virtual ~DiscoItems();

		void addItem(const DiscoItem& item);

		void clear();

		QList<DiscoItem> items() const;

		void pushToDomElement(QDomElement& element) const;
	private:
		QList<DiscoItem> m_items;
};

} /* end of namespace XMPP */

#endif /* XMPP_SERVICE_DISCOVERY_H_ */
