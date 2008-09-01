/*
 * Stanza.cpp
 *
 *  Created on: Sep 1, 2008
 *      Author: holycheater
 */

#include "Stanza.h"

#include "jid/jid.h"
#include <QSharedData>
#include <QDomDocument>

namespace X
{


using namespace XMPP;

class Stanza::Private : public QSharedData
{
	public:
		Private();
		Private(const Private& other);

		Jid from, to;
		QString id, type;

		static quint32 nextId;

		/* dom document containing stanza element */
		QDomDocument doc;
};

quint32 Stanza::Private::nextId = 0;

Stanza::Private::Private()
	: QSharedData()
{
	type = -1;
	id = QString::number(++nextId, 16);
}

Stanza::Private::Private(const Private& other)
	: QSharedData(other)
{
	from = other.from;
	to = other.to;
	id = QString::number(++nextId, 16);
	type = other.type;
	doc = other.doc.cloneNode(true).toDocument();
}

/**
 * Default constructor for Stanza element.
 */
Stanza::Stanza()
{
	d = new Private;
}

/**
 * Constructs a deep copy of @a other.
 */
Stanza::Stanza(const Stanza& other)
	: d(other.d)
{
}

/**
 * Destroys stanza object.
 */
Stanza::~Stanza()
{
}

/**
 * Assigns @a other to this stanza and returns a reference to this stanza.
 */
Stanza& Stanza::operator=(const Stanza& other)
{
	d = other.d;
	return *this;
}

/**
 * Returns true if this stanza is valid.
 */
bool Stanza::isValid() const
{
	return !d->type.isEmpty();
}

/**
 * Returns recepient's jabber-id.
 */
Jid Stanza::to() const
{
	return d->to;
}

/**
 * Returns sender's jabber-id.
 */
Jid Stanza::from() const
{
	return d->from;
}

/**
 * Returns stanza type attribute.
 */
QString Stanza::type() const
{
	return d->type;
}

/**
 * Returns stanza id attribute.
 */
QString Stanza::id() const
{
	return d->id;
}

/**
 * Set stanza recepient's jabber-id to @a toJid.
 */
void Stanza::setTo(const XMPP::Jid& toJid)
{
	d->to = toJid;
}

/**
 * Set stanza sender's jabber-id to @a fromJid.
 */
void Stanza::setFrom(const XMPP::Jid& fromJid)
{
	d->from = fromJid;
}

/**
 * Set stanza type to @a type.
 */
void Stanza::setType(const QString& type)
{
	d->type = type;
}

/**
 * Set stanza id attribute to @a id
 */
void Stanza::setId(const QString& id)
{
	d->id = id;
}


}
