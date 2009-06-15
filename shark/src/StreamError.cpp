/*
 * StreamError.cpp - Stream::Error implementation
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
#include <QPair>
#include <QSharedData>

#include "ComponentStream.h"

namespace XMPP {

/**
 * @class ComponentStream::Error
 * @brief Describes stream-level error.
 *
 * The following rules apply to stream-level errors:
 *
 * @li It is assumed that all stream-level errors are unrecoverable;
 * therefore, if an error occurs at the level of the stream, the
 * entity that detects the error MUST send a stream error to the
 * other entity, send a closing \</stream\> tag, and terminate the
 * underlying TCP connection.
 * @li If the error occurs while the stream is being set up, the
 * receiving entity MUST still send the opening \<stream\> tag, include
 * the \<error/\> element as a child of the stream element, send the
 * closing \</stream\> tag, and terminate the underlying TCP
 * connection.  In this case, if the initiating entity provides an
 * unknown host in the 'to' attribute (or provides no 'to' attribute
 * at all), the server SHOULD provide the server's authoritative
 * hostname in the 'from' attribute of the stream header sent before
 * termination.
 *
 * For more detailed information, see <a href="http://tools.ietf.org/html/rfc3920#section-4.7">Section 4.7</a>
 * of the "RFC-3920 - Extensible Message and Presence Protocol (XMPP): Core" document.
 */

class ComponentStream::Error::Private : public QSharedData
{
	public:
		Private();
		Private(const Private& other);
		~Private();

		static QString conditionToString(Condition condition);
		static int stringToCondition(const QString& condition);

		typedef QPair<int, const char*> ErrorEnumStringPair;
		static ErrorEnumStringPair errorConditionTable[];

		QDomDocument doc;
		QDomElement errorCondition;
		QDomElement text;
		QDomElement appSpec;
};

ComponentStream::Error::Private::Private()
	: QSharedData()
{
}

ComponentStream::Error::Private::Private(const Private& other)
	: QSharedData(other)
{
}

ComponentStream::Error::Private::~Private()
{
}

ComponentStream::Error::Private::ErrorEnumStringPair ComponentStream::Error::Private::errorConditionTable[] = {
		ErrorEnumStringPair(BadFormat				, "bad-format"),
		ErrorEnumStringPair(BadNamespacePrefix		, "bad-namespace-prefix"),
		ErrorEnumStringPair(Conflict				, "conflict"),
		ErrorEnumStringPair(ConnectionTimeout		, "connection-timeout"),
		ErrorEnumStringPair(HostGone				, "host-gone"),
		ErrorEnumStringPair(HostUnknown				, "host-unknown"),
		ErrorEnumStringPair(ImproperAddressing		, "improper-addressing"),
		ErrorEnumStringPair(InternalServerError		, "internal-server-error"),
		ErrorEnumStringPair(InvalidFrom				, "invalid-from"),
		ErrorEnumStringPair(InvalidId				, "invalid-id"),
		ErrorEnumStringPair(InvalidNamespace		, "invalid-namespace"),
		ErrorEnumStringPair(InvalidXml				, "invalid-xml"),
		ErrorEnumStringPair(NotAuthorized			, "not-authorized"),
		ErrorEnumStringPair(PolicyViolation			, "policy-violation"),
		ErrorEnumStringPair(RemoteConnectionFailed	, "remote-connection-failed"),
		ErrorEnumStringPair(ResourceConstraint		, "resource-constraint"),
		ErrorEnumStringPair(RestrictedXml			, "restricted-xml"),
		ErrorEnumStringPair(SeeOtherHost			, "see-other-host"),
		ErrorEnumStringPair(SystemShutdown			, "system-shutdown"),
		ErrorEnumStringPair(UndefinedCondition		, "undefined-condition"),
		ErrorEnumStringPair(UnsupportedEncoding		, "unsupported-encoding"),
		ErrorEnumStringPair(UnsupportedStanzaType	, "unsupported-stanza-type"),
		ErrorEnumStringPair(UnsupportedVersion		, "unsupported-version"),
		ErrorEnumStringPair(XmlNotWellFormed		, "xml-not-well-formed"),
		ErrorEnumStringPair(0, 0)
};

QString ComponentStream::Error::Private::conditionToString(Condition condition)
{
	for (int i = 0; errorConditionTable[i].second; ++i) {
		if (errorConditionTable[i].first == condition) {
			return errorConditionTable[i].second;
		}
	}
	return "undefined-condition";
}

int ComponentStream::Error::Private::stringToCondition(const QString& condition)
{
	for (int i = 0; errorConditionTable[i].second; ++i) {
		if (errorConditionTable[i].second == condition) {
			return errorConditionTable[i].first;
		}
	}
	return -1;
}

/**
 * Constructs stream-error object.
 */
ComponentStream::Error::Error()
	: d(new Private)
{
	QDomElement element = d->doc.createElement("stream:error");
	d->doc.appendChild(element);
}

/**
 * Constructs a deep copy of @a other
 */
ComponentStream::Error::Error(const Error& other)
	: d(other.d)
{
}

/**
 * Constructs stream-error object from a DOM element.
 *
 * @param element	\<stream:error/\> DOM element.
 */
ComponentStream::Error::Error(const QDomElement& element)
	: d(new Private)
{
	QDomNode root = d->doc.importNode(element, true);
	d->doc.appendChild(root);

	QDomNodeList childs = root.childNodes();
	for (int i = 0; i < childs.count(); ++i) {
		QDomElement element = childs.item(i).toElement();
		if (element.namespaceURI() == NS_STREAMS) {
			int condition = Private::stringToCondition(element.tagName() );
			if ( condition != -1 ) {
				d->errorCondition = element;
				break;
			}
		}
	}

	QDomElement eText = root.firstChildElement("text");
	if (eText.namespaceURI() == NS_STREAMS) {
		d->text = eText;
	}

	for (int i = 0; i < childs.count(); ++i) {
		QDomElement element = childs.item(i).toElement();
		/* first element outside NS_STREAMS ns will be an app-specific error condition */
		if (element.namespaceURI() != NS_STREAMS) {
			d->appSpec = element;
		}
	}
}

/**
 * Destroys stream-error object.
 */
ComponentStream::Error::~Error()
{
}

ComponentStream::Error& ComponentStream::Error::operator=(const Error& other)
{
	d = other.d;
	return *this;
}

/**
 * Returns application-specific error condition.
 *
 * @sa appSpecNS(), condition(), text()
 */
QString ComponentStream::Error::appSpec() const
{
	return d->appSpec.tagName();
}

/**
 * Returns application-specific error namespace.
 *
 * @sa appSpec(), condition(), text()
 */
QString ComponentStream::Error::appSpecNS() const
{
	return d->appSpec.namespaceURI();
}

/**
 * Returns error condition string.
 *
 * @sa appSpec(), appSpecNS(), text(), Condition
 */
ComponentStream::Error::Condition ComponentStream::Error::condition() const
{
	int condition = Private::stringToCondition( d->errorCondition.tagName() );
	if ( condition != -1 ) {
		return (Condition)condition;
	}
	return UndefinedCondition;
}

QString ComponentStream::Error::conditionString() const
{
	return d->errorCondition.tagName();
}

/**
 * Returns additional error text description.
 *
 * @sa appSpec(), appSpecNS(), condition()
 */
QString ComponentStream::Error::text() const
{
	return d->text.text();
}

/**
 * Sets application-specific error condition.
 *
 * @param ns	application namespace
 * @param spec	application-specific error condition
 * @sa setText(), setCondition()
 */
void ComponentStream::Error::setAppSpec(const QString& ns, const QString& spec)
{
	d->doc.documentElement().removeChild(d->appSpec);
	d->appSpec = d->doc.createElementNS(ns, spec);
	d->doc.documentElement().appendChild(d->appSpec);
}

/**
 * Sets error condition.
 *
 * @sa setAppSpec(), setText(), Condition
 */
void ComponentStream::Error::setCondition(Condition condition)
{
	d->doc.documentElement().removeChild(d->errorCondition);
	d->errorCondition = d->doc.createElementNS( NS_STREAMS, Private::conditionToString(condition) );
	d->doc.documentElement().appendChild(d->errorCondition);
}

/**
 * Sets additional error text description.
 *
 * @sa setAppSpec(), setCondition()
 */
void ComponentStream::Error::setText(const QString& text)
{
	d->doc.documentElement().removeChild(d->text);
	d->text = d->doc.createElementNS(NS_STREAMS, "text");
	d->doc.documentElement().appendChild(d->text);

	QDomText errdesc = d->doc.createTextNode(text);
	d->text.appendChild(errdesc);
}

/**
 * @enum ComponentStream::Error::Type
 * List of error conditions as defined in RFC-3920.
 */

/**
 * @var ComponentStream::Error::BadFormat
 * The entity has sent XML that cannot be processed.
 */

/**
 * @var ComponentStream::Error::BadNamespacePrefix
 * The entity has sent a namespace prefix that is unsupported, or has sent no namespace prefix on an element that requires such a prefix.
 */

/**
 * @var ComponentStream::Error::Conflict
 * The server is closing the active stream for this entity because a new stream has been initiated that conflicts with the existing stream.
 */

/**
 * @var ComponentStream::Error::ConnectionTimeout
 * The entity has not generated any traffic over the stream for some period of time (configurable according to a local service policy).
 */

/**
 * @var ComponentStream::Error::HostGone
 * The value of the 'to' attribute provided by the initiating entity in the stream header corresponds to a hostname that is no longer hosted by the server.
 */

/**
 * @var ComponentStream::Error::HostUnknown
 * The value of the 'to' attribute provided by the initiating entity in the stream header does not correspond to a hostname that is hosted by the server.
 */

/**
 * @var ComponentStream::Error::ImproperAddressing
 * A stanza sent between two servers lacks a 'to' or 'from' attribute (or the attribute has no value).
 */

/**
 * @var ComponentStream::Error::InternalServerError
 * The server has experienced a misconfiguration or an otherwise-undefined internal error that prevents it from servicing the stream.
 */

/**
 * @var ComponentStream::Error::InvalidFrom
 * The JID or hostname provided in a 'from' address does not match an authorized JID or validated domain negotiated between servers via SASL or dialback, or between a client and a server via authentication and resource binding.
 */

/**
 * @var ComponentStream::Error::InvalidId
 * The stream ID or dialback ID is invalid or does not match an ID previously provided.
 */

/**
 * @var ComponentStream::Error::InvalidNamespace
 * The streams namespace name is something other than "http://etherx.jabber.org/streams" or the dialback namespace name is something other than "jabber:server:dialback".
 */

/**
 * @var ComponentStream::Error::InvalidXml
 * The entity has sent invalid XML over the stream to a server that performs validation.
 */

/**
 * @var ComponentStream::Error::NotAuthorized
 * The entity has attempted to send data before the stream has been authenticated, or otherwise is not authorized to perform an action related to stream negotiation; the receiving entity MUST NOT process the offending stanza before sending the stream error
 */

/**
 * @var ComponentStream::Error::PolicyViolation
 * The entity has violated some local service policy; the server MAY choose to specify the policy in the \<text/\> element or an application-specific condition element
 */

/**
 * @var ComponentStream::Error::RemoteConnectionFailed
 * The server is unable to properly connect to a remote entity that is required for authentication or authorization
 */

/**
 * @var ComponentStream::Error::ResourceConstraint
 * The server lacks the system resources necessary to service the stream
 */

/**
 * @var ComponentStream::Error::RestrictedXml
 * The entity has attempted to send restricted XML features such as a comment, processing instruction, DTD, entity reference, or unescaped character
 */

/**
 * @var ComponentStream::Error::SeeOtherHost
 * The server will not provide service to the initiating entity but is redirecting traffic to another host; the server SHOULD specify the alternate hostname or IP address (which MUST be a valid domain identifier) as the XML character data of the <see-other-host/> element
 */

/**
 * @var ComponentStream::Error::SystemShutdown
 * The server is being shut down and all active streams are being closed
 */

/**
 * @var ComponentStream::Error::UndefinedCondition
 * The error condition is not one of those defined by the other conditions in this list; this error condition SHOULD be used only in conjunction with an application-specific condition
 */

/**
 * @var ComponentStream::Error::UnsupportedEncoding
 * The initiating entity has encoded the stream in an encoding that is not supported by the server
 */

/**
 * @var ComponentStream::Error::UnsupportedStanzaType
 * The initiating entity has sent a first-level child of the stream that is not supported by the server
 */

/**
 * @var ComponentStream::Error::UnsupportedVersion
 * The value of the 'version' attribute provided by the initiating entity in the stream header specifies a version of XMPP that is not supported by the server; the server MAY specify the version(s) it supports in the \<text/\> element
 */

/**
 * @var ComponentStream::Error::XmlNotWellFormed
 * The initiating entity has sent XML that is not well-formed as defined by XML specification.
 */


} /* end of namespace XMPP */

// vim:ts=4:sw=4:noet:nowrap
