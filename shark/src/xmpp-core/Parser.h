/*
 * Parser.h - parse an XMPP "document"
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

#ifndef XMPP_CORE_PARSER_H_
#define XMPP_CORE_PARSER_H_

#include <QString>
#include <QSharedDataPointer>

class QDomElement;
class QXmlAttributes;

namespace XMPP
{


class Parser
{
	public:
		Parser();
		~Parser();

		class Event;

		void reset();
		void appendData(const QByteArray& data);
		Event readNext();
		QByteArray unprocessed() const;
		QString encoding() const;

	private:
		class Private;
		Private *d;
};

class Parser::Event
{
	public:
		enum Type { DocumentOpen, DocumentClose, Element, Error };
		Event();
		Event(const Event& other);
		Event& operator=(const Event& other);
		~Event();

		bool isNull() const;
		int type() const;
		QString typeString() const;

		// for document open
		QString nsprefix(const QString& str = QString() ) const;

		// for document open / close
		QString namespaceURI() const;
		QString localName() const;
		QString qualifiedName() const;
		QXmlAttributes attributes() const;

		// for element
		QDomElement element() const;

		// for any
		QString actualString() const;

		// setup
		void setDocumentOpen(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts, const QStringList &nsnames, const QStringList &nsvalues);
		void setDocumentClose(const QString &namespaceURI, const QString &localName, const QString &qName);
		void setElement(const QDomElement &elem);
		void setError();
		void setActualString(const QString &);

	private:
		class Private;
		QSharedDataPointer<Private> d;
};


}

// vim:ts=4:sw=4:noet:nowrap
#endif /* XMPP_CORE_PARSER_H_ */
