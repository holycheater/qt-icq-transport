/*
 * Jid.h
 * Copyright (C) 2003  Justin Karneges
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

#ifndef XMPP_JID_H
#define XMPP_JID_H

#include <QSharedDataPointer>
#include <QString>

namespace XMPP
{


class Jid
{
	public:
		Jid();
		~Jid();

		Jid(const Jid& other);
		Jid(const QString& str);
		Jid(const char *str);

		Jid& operator=(const Jid& other);
		Jid& operator=(const QString& str);
		Jid& operator=(const char *str);

		void set(const QString& str);
		void set(const QString& domain, const QString& node, const QString& resource = "");

		void setDomain(const QString& domain);
		void setNode(const QString& node);
		void setResource(const QString& resource);

		bool isNull() const;
		QString domain() const;
		QString node() const;
		QString resource() const;
		QString bare() const;
		QString full() const;

		Jid withNode(const QString &s) const;
		Jid withResource(const QString &s) const;

		bool isValid() const;
		bool isEmpty() const;
		bool compare(const Jid& other, bool compareResource = true) const;
		bool operator==(const Jid& other) const;

		static bool validDomain(const QString &s, QString *norm=0);
		static bool validNode(const QString &s, QString *norm=0);
		static bool validResource(const QString &s, QString *norm=0);

		operator QString();
	private:
		void reset();
		void update();

		class Private;
		QSharedDataPointer<Private> d;
};


} // end namespace XMPP

#endif
