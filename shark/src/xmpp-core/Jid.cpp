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

/* libidn */
#include <stringprep.h>


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

				Result(const QString& s) : norm( new QString(s) )
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
			if (!instance) {
				instance = new StringPrepCache;
			}
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
	if ( in.isEmpty() ) {
		if (out) {
			*out = QString();
		}
		return true;
	}

	StringPrepCache *that = get_instance();

	Result *r = that->nodeprep_table[in];
	if (r) {
		if (!r->norm) {
			return false;
		}
		 if (out) {
			*out = *(r->norm);
		 }
		return true;
	}

	QByteArray cs = in.toUtf8();
	cs.resize(maxbytes);
	if (stringprep(cs.data(), maxbytes, (Stringprep_profile_flags)0, stringprep_xmpp_nodeprep) != 0) {
		that->nodeprep_table.insert(in, new Result);
		return false;
	}

	QString norm = QString::fromUtf8(cs);
	that->nodeprep_table.insert( in, new Result(norm) );
	if (out) {
		*out = norm;
	}
	return true;
}

bool StringPrepCache::nameprep(const QString& in, int maxbytes, QString *out)
{
	if ( in.isEmpty() ) {
		if (out) {
			*out = QString();
		}
		return true;
	}

	StringPrepCache *that = get_instance();

	Result *r = that->nameprep_table[in];
	if (r) {
		if (!r->norm) {
			return false;
		}
		if (out) {
			*out = *(r->norm);
		}
		return true;
	}

	QByteArray cs = in.toUtf8();
	cs.resize(maxbytes);
	if (stringprep(cs.data(), maxbytes, (Stringprep_profile_flags)0, stringprep_nameprep) != 0) {
		that->nameprep_table.insert(in, new Result);
		return false;
	}

	QString norm = QString::fromUtf8(cs);
	that->nameprep_table.insert( in, new Result(norm) );
	if (out) {
		*out = norm;
	}
	return true;
}

bool StringPrepCache::resourceprep(const QString& in, int maxbytes, QString *out)
{
	if ( in.isEmpty() ) {
		if (out) {
			*out = QString();
		}
		return true;
	}

	StringPrepCache *that = get_instance();

	Result *r = that->resourceprep_table[in];
	if (r) {
		if (!r->norm) {
			return false;
		}
		if (out) {
			*out = *(r->norm);
		}
		return true;
	}

	QByteArray cs = in.toUtf8();
	cs.resize(maxbytes);
	if (stringprep(cs.data(), maxbytes, (Stringprep_profile_flags)0, stringprep_xmpp_resourceprep) != 0) {
		that->resourceprep_table.insert(in, new Result);
		return false;
	}

	QString norm = QString::fromUtf8(cs);
	that->resourceprep_table.insert( in, new Result(norm) );
	if (out) {
		*out = norm;
	}
	return true;
}

StringPrepCache *StringPrepCache::instance = 0;


class Jid::Private : public QSharedData
{
	public:
		Private();
		Private(const Private& other);

		void reset();

		static bool validDomain(const QString& domain, QString *normalised = 0);
		static bool validNode(const QString& node, QString *normalised = 0);
		static bool validResource(const QString& resource, QString *normalised = 0);

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

void Jid::Private::reset()
{
	domain.clear();
	node.clear();
	resource.clear();

	valid = false;
}

bool Jid::Private::validDomain(const QString& domain, QString *normalised)
{
	return StringPrepCache::nameprep(domain, 1024, normalised);
}

bool Jid::Private::validNode(const QString& node, QString *normalised)
{
	return StringPrepCache::nodeprep(node, 1024, normalised);
}

bool Jid::Private::validResource(const QString& resource, QString *normalised)
{
	return StringPrepCache::resourceprep(resource, 1024, normalised);
}


/* ***************************************************************************
 * XMPP::Jid
 *************************************************************************** */

/**
 * @class Jid
 * Represents jabber-id.
 */

/**
 * Constructs jabber-id object.
 */
Jid::Jid()
{
	d = new Private;
}

/**
 * Constructs a deep copy of @a other.
 */
Jid::Jid(const Jid& other)
	: d(other.d)
{
}

/**
 * Constructs a jabber-id object from string of type [user@]<domain>[/resource]
 */
Jid::Jid(const QString& str)
{
	d = new Private;
	set(str);
}

/**
 * Constructs a jabber-id object from string of type [user@]<domain>[/resource]
 */
Jid::Jid(const char *str)
{
	d = new Private;
	set( QString(str) );
}

/**
 * Destroys jabber-id object.
 */
Jid::~Jid()
{
}

/**
 * Assigns @a other to this object and return the reference to this.
 * @note The data between objects becomes shared.
 */
Jid& Jid::operator=(const Jid& other)
{
	d = other.d;
	return *this;
}

/**
 * Assigns a string of type [user@]<domain>[/resource] to the jabber-id object.
 */
Jid& Jid::operator=(const QString& str)
{
	set(str);
	return *this;
}

/**
 * Assigns a string of type [user@]<domain>[/resource] to the jabber-id object.
 */
Jid& Jid::operator=(const char *str)
{
	set( QString(str) );
	return *this;
}

/**
 * Returns jabber-id domain string.
 */
QString Jid::domain() const
{
	return d->domain;
}

/**
 * Returns jabber-id node (username) string.
 */
QString Jid::node() const
{
	return d->node;
}

/**
 * Returns jabber-id resource string.
 */
QString Jid::resource() const
{
	return d->resource;
}

/**
 * Returns jabber-id without resource part.
 */
QString Jid::bare() const
{
	QString bare = d->domain;
	if ( !d->node.isEmpty() ) {
		bare.prepend(d->node + "@");
	}
	return bare;
}

/**
 * Returns full jabber-id (user@domain/resource).
 */
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

/**
 * Sets jabber-id to @a jid.
 */
void Jid::set(const QString& jid)
{
	QString rest, domain, node, resource;
	QString normalisedDomain, normalisedNode, normalisedResource;

	int x = jid.indexOf('/');
	if (x != -1) {
		rest = jid.mid(0, x);
		resource = jid.mid(x+1);
	} else {
		rest = jid;
		resource = QString();
	}
	if( !Private::validResource(resource, &normalisedResource) ) {
		d->reset();
		return;
	}

	x = rest.indexOf('@');
	if (x != -1) {
		node = rest.mid(0, x);
		domain = rest.mid(x+1);
	} else {
		node = QString();
		domain = rest;
	}
	if ( !Private::validDomain(domain, &normalisedDomain) || !Private::validNode(node, &normalisedNode) ) {
		d->reset();
		return;
	}

	d->valid = true;
	d->domain = normalisedDomain;
	d->node = normalisedNode;
	d->resource = normalisedResource;
}

/**
 * Sets jabber-id to node@domain[/resource].
 */
void Jid::set(const QString& domain, const QString& node, const QString& resource)
{
	QString normalisedDomain, normalisedNode, normalisedResource;
	if ( !Private::validDomain(domain, &normalisedDomain) || !Private::validNode(node, &normalisedNode) || !Private::validResource(resource, &normalisedResource) ) {
		d->reset();
		return;
	}
	d->valid = true;
	d->domain = normalisedDomain;
	d->node = normalisedNode;
	d->resource = normalisedResource;
}

/**
 * Sets jabber-id domain to @a domain.
 */
void Jid::setDomain(const QString& domain)
{
	if (!d->valid) {
		return;
	}
	QString normalised;
	if ( !Private::validDomain(domain, &normalised) ) {
		d->reset();
		return;
	}
	d->domain = normalised;
}

/**
 * Sets jabber-id node to @a node.
 */
void Jid::setNode(const QString& node)
{
	if (!d->valid) {
		return;
	}
	QString normalised;
	if ( !Private::validNode(node, &normalised) ) {
		d->reset();
		return;
	}
	d->node = normalised;
}

/**
 * Sets jabber-id resource to @a resource.
 */
void Jid::setResource(const QString& resource)
{
	if (!d->valid) {
		return;
	}
	QString normalised;
	if ( !Private::validResource(resource, &normalised) ) {
		d->reset();
		return;
	}
	d->resource = normalised;
}

/**
 * Returns jabber-id with node set to @a node.
 * If node is present, it's replaced to @a node, if not - it's prepended to jabber-id.
 */
Jid Jid::withNode(const QString& node) const
{
	Jid jid = *this;
	jid.setNode(node);
	return jid;
}

/**
 * Returns jabber-id with resource set to @a resource.
 * If resource is present, it's replaced to @a resource, if not - it's appended to jabber-id.
 */
Jid Jid::withResource(const QString& resource) const
{
	Jid jid = *this;
	jid.setResource(resource);
	return jid;
}

/**
 * Returns true if this Jid object is a valid jabber-id.
 */
bool Jid::isValid() const
{
	return d->valid;
}

/**
 * Returns true if jabber-id is empty (i.e. node, domain and resource are empty).
 */
bool Jid::isEmpty() const
{
	return d->node.isEmpty() && d->domain.isEmpty() && d->resource.isEmpty();
}

/**
 * Compares bare jabber-ids if @a compareResource is false, otherwise compares full jabber-ids.
 * Returns true if this jabber-id is equal to @a other jabber-id.
 */
bool Jid::compare(const Jid& other, bool compareResource) const
{
	if ( isEmpty() && other.isEmpty() ) {
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

/**
 * Compares full jabber-ids.
 */
bool Jid::operator==(const Jid& other) const
{
	if ( isEmpty() && other.isEmpty() ) {
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

bool Jid::operator!=(const Jid& other) const
{
	return !(*this == other);
}

Jid::operator QString() const
{
	return full();
}
