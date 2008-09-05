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

#include "ComponentStream.h"

namespace XMPP {

/**
 * @class ComponentStream::Error
 * @brief Describes stream-level error.
 */


/**
 * Constructs stream-error object.
 */
ComponentStream::Error::Error()
{
	QDomElement element = m_doc.createElement("stream:error");
	m_doc.appendChild(element);
}

/**
 * Constructs stream-error object from a DOM document.
 *
 * @param document	DOM document containing stream:error element.
 */
ComponentStream::Error::Error(const QDomDocument& document)
{
	m_doc = document.cloneNode(true).toDocument();
}

/**
 * Constructs stream-error object from a DOM element.
 *
 * @param element	DOM element named stream:error.
 */
ComponentStream::Error::Error(const QDomElement& element)
{
	QDomNode root = m_doc.importNode(element, true);
	m_doc.appendChild(root);
	m_errorCondition = m_doc.documentElement().firstChild().toElement();
}

/**
 * Destroys stream-error object.
 */
ComponentStream::Error::~Error()
{
}

/**
 * Returns application-specific error condition.
 */
QString ComponentStream::Error::appSpec() const
{
	return m_appSpec.nodeName();
}

/**
 * Returns additional error text description.
 *
 * @sa appSpec(), type()
 */
QString ComponentStream::Error::text() const
{
	if ( m_doc.documentElement().elementsByTagName("text").isEmpty() ) {
		return QString();
	} else {
		QDomElement eText = m_doc.documentElement().elementsByTagName("text").item(0).toElement();
		return eText.text();
	}
}

/**
 * Returns error condition string.
 *
 * @sa appSpec(), text(), Type
 */
QString ComponentStream::Error::type() const
{
	return m_errorCondition.nodeName();
}

/**
 * Sets application-specific error condition.
 *
 * @param ns	application namespace
 * @param spec	application-specific error condition
 * @sa setText(), setType()
 */
void ComponentStream::Error::setAppSpec(const QString& ns, const QString& spec)
{
	m_doc.removeChild(m_appSpec);
	m_appSpec = m_doc.createElementNS(ns, spec);
	m_doc.appendChild(m_appSpec);
}

/**
 * Sets additional error text description.
 *
 * @sa setAppSpec(), SetType()
 */
void ComponentStream::Error::setText(const QString& text)
{
	QDomElement eText = m_doc.createElementNS("urn:ietf:params:xml:ns:xmpp-streams", "text");
	m_doc.appendChild(eText);

	QDomText errdesc = m_doc.createTextNode(text);
	eText.appendChild(errdesc);
}

/**
 * Sets error condition.
 *
 * @sa setAppSpec(), setText(), Type
 */
void ComponentStream::Error::setType(Type type)
{
	m_doc.removeChild(m_errorCondition);
	m_errorCondition = m_doc.createElementNS( "urn:ietf:params:xml:ns:xmpp-streams", typeToString(type) );
	m_doc.appendChild(m_errorCondition);
}

/**
 * Converts error @link Type condition to string.
 *
 * @return error condition string
 * @sa Type
 */
QString ComponentStream::Error::typeToString(Type type)
{
	switch (type) {
		case BadFormat:
			return "bad-format";
			break;
		case BadNamespacePrefix:
			return "bad-namespace-prefix";
			break;
		case Conflict:
			return "conflict";
			break;
		case ConnectionTimeout:
			return "connection-timeout";
			break;
		case HostGone:
			return "host-gone";
			break;
		case HostUnknown:
			return "host-unkown";
			break;
		case ImproperAddressing:
			return "improper-addressing";
			break;
		case InternalServerError:
			return "internal-server-error";
			break;
		case InvalidFrom:
			return "invalid-from";
			break;
		case InvalidId:
			return "invalid-id";
			break;
		case InvalidNamespace:
			return "invalid-namespace";
			break;
		case InvalidXml:
			return "invalid-xml";
			break;
		case NotAuthorized:
			return "not-authorized";
			break;
		case PolicyViolation:
			return "policy-violation";
			break;
		case RemoteConnectionFailed:
			return "remote-connection-failed";
			break;
		case ResourceConstraint:
			return "resource-constraint";
			break;
		case RestrictedXml:
			return "restricted-xml";
			break;
		case SeeOtherHost:
			return "see-other-host";
			break;
		case SystemShutdown:
			return "system-shutdown";
			break;
		case UndefinedCondition:
			return "undefined-condition";
			break;
		case UnsupportedEncoding:
			return "unsupported-encoding";
			break;
		case UnsupportedStanzaType:
			return "unsupported-stanza-type";
			break;
		case UnsupportedVersion:
			return "unsupported-verison";
			break;
		case XmlNotWellFormed:
			return "xml-not-well-formed";
			break;
		default:
			return "undefined-condition";
			break;
	}
}

/**
 * @enum ComponentStream::Error::Type
 * @brief List of error conditions as defined in RFC 3920.
 * @details Error condition is defined as an empty element inside <stream:error/> element.
 * The following conditions are defined:
 *
 * @li BadFormat				The entity has sent XML that cannot be processed.
 * @li BadNamespacePrefix		The entity has sent a namespace prefix that is unsupported, or has sent no namespace prefix on an element that requires such a prefix.
 * @li Conflict					The server is closing the active stream for this entity because a new stream has been initiated that conflicts with the existing stream.
 * @li ConnectionTimeout		The entity has not generated any traffic over the stream for some period of time (configurable according to a local service policy).
 * @li HostGone					The value of the 'to' attribute provided by the initiating entity in the stream header corresponds to a hostname that is no longer hosted by the server.
 * @li HostUnknown				The value of the 'to' attribute provided by the initiating entity in the stream header does not correspond to a hostname that is hosted by the server.
 * @li ImproperAddressing		A stanza sent between two servers lacks a 'to' or 'from' attribute (or the attribute has no value).
 * @li InternalServerError		The server has experienced a misconfiguration or an otherwise-undefined internal error that prevents it from servicing the stream.
 * @li InvalidFrom				The JID or hostname provided in a 'from' address does not match an authorized JID or validated domain negotiated between servers via SASL or dialback, or between a client and a server via authentication and resource binding.
 * @li InvalidId				The stream ID or dialback ID is invalid or does not match an ID previously provided.
 * @li InvalidNamespace			The streams namespace name is something other than "http://etherx.jabber.org/streams" or the dialback namespace name is something other than "jabber:server:dialback".
 * @li InvalidXml				The entity has sent invalid XML over the stream to a server that performs validation.
 * @li NotAuthorized			The entity has attempted to send data before the stream has been authenticated, or otherwise is not authorized to perform an action related to stream negotiation; the receiving entity MUST NOT process the offending stanza before sending the stream error
 * @li PolicyViolation			The entity has violated some local service policy; the server MAY choose to specify the policy in the <text/> element or an application-specific condition element
 * @li RemoteConnectionFailed	The server is unable to properly connect to a remote entity that is required for authentication or authorization
 * @li ResourceConstraint		The server lacks the system resources necessary to service the stream
 * @li RestrictedXml			The entity has attempted to send restricted XML features such as a comment, processing instruction, DTD, entity reference, or unescaped character
 * @li SeeOtherHost				The server will not provide service to the initiating entity but is redirecting traffic to another host; the server SHOULD specify the alternate hostname or IP address (which MUST be a valid domain identifier) as the XML character data of the <see-other-host/> element
 * @li SystemShutdown			The server is being shut down and all active streams are being closed
 * @li UndefinedCondition		The error condition is not one of those defined by the other conditions in this list; this error condition SHOULD be used only in conjunction with an application-specific condition
 * @li UnsupportedEncoding		The initiating entity has encoded the stream in an encoding that is not supported by the server
 * @li UnsupportedStanzaType	The initiating entity has sent a first-level child of the stream that is not supported by the server
 * @li UnsupportedVersion		The value of the 'version' attribute provided by the initiating entity in the stream header specifies a version of XMPP that is not supported by the server; the server MAY specify the version(s) it supports in the <text/> element
 * @li XmlNotWellFormed			The initiating entity has sent XML that is not well-formed as defined by XML specification.
 */


} /* end of namespace XMPP */
