/*
 * Jid.cpp - class for verifying and manipulating Jabber IDs
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

#include "Jid.h"

#include <QCoreApplication>
#include <QByteArray>
#include <QHash>
#include <QSharedData>
#include <libidn/stringprep.h>


using namespace XMPP;

//----------------------------------------------------------------------------
// StringPrepCache
//----------------------------------------------------------------------------
class StringPrepCache : public QObject
{
	public:
		static bool nameprep(const QString& in, int maxbytes, QString *out);
		static bool nodeprep(const QString& in, int maxbytes, QString *out);
		static bool resourceprep(const QString& in, int maxbytes, QString *out);

	private:
		class Result
		{
			public:
				QString *norm;

				Result() : norm(0)
				{
				}

				Result(const QString& s) : norm(new QString(s))
				{
				}

				~Result()
				{
					delete norm;
				}
		};

		QHash<QString,Result*> nameprep_table;
		QHash<QString,Result*> nodeprep_table;
		QHash<QString,Result*> resourceprep_table;

		static StringPrepCache *instance;

		static StringPrepCache *get_instance()
		{
			if(!instance)
				instance = new StringPrepCache;
			return instance;
		}

		StringPrepCache();
		~StringPrepCache();
};

StringPrepCache::StringPrepCache()
	: QObject(qApp)
{
}

StringPrepCache::~StringPrepCache()
{
	qDeleteAll(nodeprep_table);
	qDeleteAll(nameprep_table);
	qDeleteAll(resourceprep_table);
}

bool StringPrepCache::nodeprep(const QString& in, int maxbytes, QString *out)
{
	if(in.isEmpty())
	{
		if(out)
			*out = QString();
		return true;
	}

	StringPrepCache *that = get_instance();

	Result *r = that->nodeprep_table[in];
	if(r)
	{
		if(!r->norm)
			return false;
		if(out)
			*out = *(r->norm);
		return true;
	}

	QByteArray cs = in.toUtf8();
	cs.resize(maxbytes);
	if(stringprep(cs.data(), maxbytes, (Stringprep_profile_flags)0, stringprep_xmpp_nodeprep) != 0)
	{
		that->nodeprep_table.insert(in, new Result);
		return false;
	}

	QString norm = QString::fromUtf8(cs);
	that->nodeprep_table.insert(in, new Result(norm));
	if(out)
		*out = norm;
	return true;
}

bool StringPrepCache::nameprep(const QString& in, int maxbytes, QString *out)
{
	if(in.isEmpty())
	{
		if(out)
			*out = QString();
		return true;
	}

	StringPrepCache *that = get_instance();

	Result *r = that->nameprep_table[in];
	if(r)
	{
		if(!r->norm)
			return false;
		if(out)
			*out = *(r->norm);
		return true;
	}

	QByteArray cs = in.toUtf8();
	cs.resize(maxbytes);
	if(stringprep(cs.data(), maxbytes, (Stringprep_profile_flags)0, stringprep_nameprep) != 0)
	{
		that->nameprep_table.insert(in, new Result);
		return false;
	}

	QString norm = QString::fromUtf8(cs);
	that->nameprep_table.insert(in, new Result(norm));
	if(out)
		*out = norm;
	return true;
}

bool StringPrepCache::resourceprep(const QString& in, int maxbytes, QString *out)
{
	if(in.isEmpty())
	{
		if(out)
			*out = QString();
		return true;
	}

	StringPrepCache *that = get_instance();

	Result *r = that->resourceprep_table[in];
	if(r)
	{
		if(!r->norm)
			return false;
		if(out)
			*out = *(r->norm);
		return true;
	}

	QByteArray cs = in.toUtf8();
	cs.resize(maxbytes);
	if(stringprep(cs.data(), maxbytes, (Stringprep_profile_flags)0, stringprep_xmpp_resourceprep) != 0)
	{
		that->resourceprep_table.insert(in, new Result);
		return false;
	}

	QString norm = QString::fromUtf8(cs);
	that->resourceprep_table.insert(in, new Result(norm));
	if(out)
		*out = norm;
	return true;
}

StringPrepCache *StringPrepCache::instance = 0;


class Jid::Private : public QSharedData
{
	public:
		Private();
		Private(const Private& other);

		QString node, domain, resource;
		bool valid;
};

Jid::Private::Private()
	: QSharedData()
{
	valid = false;
}

Jid::Private::Private(const Private& other)
	: QSharedData(other)
{
	valid = other.valid;
	node = other.node;
	domain = other.domain;
	resource = other.resource;
}

//----------------------------------------------------------------------------
// Jid
//----------------------------------------------------------------------------
Jid::Jid()
{
	d = new Private;
}

Jid::~Jid()
{
}

Jid::Jid(const Jid& other)
	: d(other.d)
{
}

Jid::Jid(const QString& str)
{
	d = new Private;
	set(str);
}

Jid::Jid(const char *str)
{
	d = new Private;
	set( QString(str) );
}

Jid& Jid::operator=(const Jid& other)
{
	d = other.d;
	return *this;
}

Jid& Jid::operator=(const QString& str)
{
	set(str);
	return *this;
}

Jid& Jid::operator=(const char *str)
{
	set( QString(str) );
	return *this;
}

QString Jid::domain() const
{
	return d->domain;
}

QString Jid::node() const
{
	return d->node;
}

QString Jid::resource() const
{
	return d->resource;
}

QString Jid::bare() const
{
	QString bare = d->domain;
	if ( !d->node.isEmpty() ) {
		bare.prepend(d->node + "@");
	}
	return bare;
}

QString Jid::full() const
{
	QString full = d->domain;
	if ( !d->node.isEmpty() ) {
		full.prepend(d->node + "@");
	}
	if ( !d->resource.isEmpty() ) {
		full.append("/" + d->resource);
	}
	return full;
}

void Jid::reset()
{
	d->domain.clear();
	d->node.clear();
	d->resource.clear();
	d->valid = false;
}

void Jid::update()
{
	// do nothing
}

void Jid::set(const QString& s)
{
	QString rest, domain, node, resource;
	QString norm_domain, norm_node, norm_resource;
	int x = s.indexOf('/');
	if(x != -1) {
		rest = s.mid(0, x);
		resource = s.mid(x+1);
	}
	else {
		rest = s;
		resource = QString();
	}
	if(!validResource(resource, &norm_resource)) {
		reset();
		return;
	}

	x = rest.indexOf('@');
	if(x != -1) {
		node = rest.mid(0, x);
		domain = rest.mid(x+1);
	}
	else {
		node = QString();
		domain = rest;
	}
	if(!validDomain(domain, &norm_domain) || !validNode(node, &norm_node)) {
		reset();
		return;
	}

	d->valid = true;
	d->domain = norm_domain;
	d->node = norm_node;
	d->resource = norm_resource;
	update();
}

void Jid::set(const QString& domain, const QString& node, const QString& resource)
{
	QString norm_domain, norm_node, norm_resource;
	if(!validDomain(domain, &norm_domain) || !validNode(node, &norm_node) || !validResource(resource, &norm_resource)) {
		reset();
		return;
	}
	d->valid = true;
	d->domain = norm_domain;
	d->node = norm_node;
	d->resource = norm_resource;
	update();
}

void Jid::setDomain(const QString& domain)
{
	if(!d->valid)
		return;
	QString norm;
	if(!validDomain(domain, &norm)) {
		reset();
		return;
	}
	d->domain = norm;
	update();
}

void Jid::setNode(const QString& node)
{
	if(!d->valid)
		return;
	QString norm;
	if(!validNode(node, &norm)) {
		reset();
		return;
	}
	d->node = norm;
	update();
}

void Jid::setResource(const QString& resource)
{
	if(!d->valid)
		return;
	QString norm;
	if(!validResource(resource, &norm)) {
		reset();
		return;
	}
	d->resource = norm;
	update();
}

Jid Jid::withNode(const QString& node) const
{
	Jid jid = *this;
	jid.setNode(node);
	return jid;
}

Jid Jid::withResource(const QString& resource) const
{
	Jid jid = *this;
	jid.setResource(resource);
	return jid;
}

bool Jid::isNull() const
{
	return d->node.isEmpty() && d->domain.isEmpty() && d->resource.isEmpty();
}

bool Jid::isValid() const
{
	return d->valid;
}

bool Jid::isEmpty() const
{
	return d->node.isEmpty() && d->domain.isEmpty() && d->resource.isEmpty();
}

bool Jid::compare(const Jid& other, bool compareResource) const
{
	if ( isNull() && other.isNull() ) {
		return true;
	}

	// only compare valid jids
	if ( !isValid() || !other.isValid() ) {
		return false;
	}

	if ( compareResource ? ( full() == other.full() ) : ( bare() == other.bare() ) ) {
		return true;
	}
	return false;
}

bool Jid::operator==(const Jid& other) const
{
	if ( isNull() && other.isNull() ) {
		return true;
	}
	if ( !isValid() || !other.isValid() ) {
		return false;
	}
	if ( full() == other.full() ) {
		return true;
	}
	return false;
}

bool Jid::validDomain(const QString& domain, QString *norm)
{
	return StringPrepCache::nameprep(domain, 1024, norm);
}

bool Jid::validNode(const QString& node, QString *norm)
{
	return StringPrepCache::nodeprep(node, 1024, norm);
}

bool Jid::validResource(const QString& resource, QString *norm)
{
	return StringPrepCache::resourceprep(resource, 1024, norm);
}

Jid::operator QString()
{
	return full();
}
