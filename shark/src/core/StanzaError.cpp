/*
 * StanzaError.cpp - XMPP stanza error (RFC-3920).
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

#include "Stanza.h"

namespace XMPP {

/**
 * @class Stanza::Error
 * @brief Represents stanza error.
 *
 * A valid error must contain information about error type and error condition.
 * It can optionally contain error-code for backward compatibility.
 * It can have optional descriptive text and optional application-specific
 * condition within application-namespace.
 *
 * The basic example for replying with error:
 *
 * @code
 * Stanza stanzaErrorReply(someStanzaWithError);
 * Stanza::Error err(Stanza::Error::Modify, Stanza::Error::BadRequest);
 * stanzaErrorReply.swapFromTo();
 * stanzaErrorReply.setError(err);
 * @endcode
 *
 * Then you can simply send it to the stream.
 */


/**
 * Default constructor for stanza-error object. It does not set any data.
 */
Stanza::Error::Error()
{
}

/**
 * Constructs stanza-error with given @a type, @a condition and (optionally) @a text
 */
Stanza::Error::Error(Type type, Condition condition, const QString& text)
{
	m_type = type;
	m_condition = condition;
	m_text = text;
}

/**
 * Constructs stanza-error with given type, condition, application-specific condition and (optionally) text.
 * @param type				error type
 * @param condition			error condition
 * @param appConditionNS	application namespace
 * @param appCondition		application-specific error condition
 * @param text				optional description text
 */
Stanza::Error::Error(Type type, Condition condition, const QString& appConditionNS, const QString& appCondition, const QString& text)
{
	m_type = type;
	m_condition = condition;
	m_appConditionNS = appConditionNS;
	m_appCondition = appCondition;
	m_text = text;
}

/**
 * Constructs stanza-error from @a stanza
 */
Stanza::Error Stanza::Error::fromStanza(const Stanza& stanza)
{
	QDomElement eError = stanza.doc()->documentElement().firstChildElement("error");
	if ( eError.isNull() ) {
		return Error();
	}

	Error error;
	error.m_type = stringToType( eError.attribute("type") );
	error.m_code = eError.attribute("code", 0).toInt();

	QDomNodeList childs = eError.childNodes();
	for ( int i = 0; i < childs.count(); ++i ) {
		QDomElement element = childs.item(i).toElement();
		if ( element.namespaceURI() == NS_STANZAS) {
			int condition = stringToCondition(element.tagName() );
			if ( condition != -1 ) {
				error.m_condition = (Condition)condition;
				break;
			}
		}
	}

	QDomElement eText = eError.firstChildElement("text");
	if ( eText.namespaceURI() == NS_STANZAS && !eText.text().isNull() ) {
		error.m_text = eText.text().trimmed();
	}

	for ( int i = 0; i < childs.count(); ++i ) {
		QDomElement element = childs.item(i).toElement();
		/* first element outside NS_STANZAS ns will be an app-specific error condition */
		if ( element.namespaceURI() != NS_STANZAS) {
			error.m_appConditionNS = element.namespaceURI();
			error.m_appCondition = element.tagName();
		}
	}
	return error;
}

/**
 * Destroys stanza-error object.
 */
Stanza::Error::~Error()
{
}

/**
 * Returns application-specific condition string.
 */
QString Stanza::Error::appCondition() const
{
	return m_appCondition;
}

/**
 * Returns application-namespace string for application-specific error condition.
 */
QString Stanza::Error::appConditionNS() const
{
	return m_appConditionNS;
}

/**
 * Returns error code.
 */
int Stanza::Error::code() const
{
	return m_code;
}

/**
 * Returns stanza error condition.
 *
 * @sa Condition
 */
Stanza::Error::Condition Stanza::Error::condition() const
{
	return m_condition;
}

/**
 * Returns stanza optional error text.
 */
QString Stanza::Error::text() const
{
	return m_text;
}

/**
 * returns stanza error type
 *
 * @sa Type
 */
Stanza::Error::Type Stanza::Error::type() const
{
	return m_type;
}

/**
 * Sets application-specific error condition within application namespace.
 *
 * @param appConditionNS	Application namespace.
 * @param appCondition		Application-specific error condition.
 */
void Stanza::Error::setAppCondition(const QString& appConditionNS, const QString& appCondition)
{
	m_appCondition = appCondition;
	m_appConditionNS = appConditionNS;
}

/**
 * Sets optional error-code number.
 */
void Stanza::Error::setCode(int code)
{
	m_code = code;
}

/**
 * Sets error condition to @a condition.
 */
void Stanza::Error::setCondition(Condition condition)
{
	m_condition = condition;
}

/**
 * Sets optional error description text to @a text.
 */
void Stanza::Error::setText(const QString& text)
{
	m_text = text;
}

/**
 * Sets error type to @a type.
 */
void Stanza::Error::setType(Type type)
{
	m_type = type;
}

/**
 * Adds and fills an <error/> element to the specified @a element.
 *
 * Element SHOULD be a stanza element.
 */
void Stanza::Error::pushToDomElement(QDomElement element) const
{
	QDomElement eError = element.ownerDocument().createElement("error");
	eError.setAttribute( "type", typeToString(m_type) );
	if ( m_code > 0 ) {
		eError.setAttribute("code", m_code);
	}

	QDomElement eCondition = element.ownerDocument().createElementNS( NS_STANZAS, conditionToString(m_condition) );
	eError.appendChild(eCondition);

	if ( !m_text.isEmpty() ) {
		QDomElement eText = element.ownerDocument().createElementNS(NS_STANZAS, "text");
		eError.appendChild(eText);

		QDomText tText = element.ownerDocument().createTextNode(m_text);
		eText.appendChild(tText);
	}

	if ( !m_appConditionNS.isEmpty() && !m_appCondition.isEmpty() ) {
		QDomElement eAppCond = element.ownerDocument().createElementNS(m_appConditionNS, m_appCondition);
		eError.appendChild(eAppCond);
	}

	element.appendChild(eError);
}

/**
 * Returns string representation of error-condition.
 */
QString Stanza::Error::conditionToString(Condition errCondition)
{
	switch (errCondition) {
		case BadRequest:
			return "bad-request";
		case Conflict:
			return "conflict";
		case FeatureNotImplemented:
			return "feature-not-implemented";
		case Forbidden:
			return "forbidden";
		case Gone:
			return "gone";
		case InternalServerError:
			return "internal-server-error";
		case NotAcceptable:
			return "not-acceptable";
		case NotAllowed:
			return "not-allowed";
		case NotAuthorized:
			return "not-authorized";
		case PaymentRequired:
			return "payment-required";
		case RecipientUnavailable:
			return "recipient-unavailable";
		case Redirect:
			return "redirect";
		case RegistrationRequired:
			return "registration-required";
		case RemoteServerNotFound:
			return "remote-server-not-found";
		case RemoteServerTimeout:
			return "remote-server-timeout";
		case ResourceConstraint:
			return "resource-constraint";
		case ServiceUnavailable:
			return "service-unavailable";
		case SubscriptionRequired:
			return "subscription-required";
		case UndefinedCondition:
			return "undefined-condition";
		case UnexpectedRequest:
			return "unexpected-request";
		default:
			return QString();
	}
}

/**
 * Returns string representation of error-type.
 */
QString Stanza::Error::typeToString(Type errType)
{
	switch (errType) {
		case Cancel:
			return "cancel";
		case Continue:
			return "continue";
		case Modify:
			return "modify";
		case Auth:
			return "auth";
		case Wait:
			return "wait";
		default:
			return QString();
	}
}

/**
 * Returns enum-value representation of error-condition string.
 */
int Stanza::Error::stringToCondition(const QString& condition)
{
	if (condition == "bad-request") {
		return BadRequest;
	}
	if (condition == "conflict") {
		return Conflict;
	}
	if (condition == "feature-not-implemented") {
		return FeatureNotImplemented;
	}
	if (condition == "forbidden") {
		return Forbidden;
	}
	if (condition == "gone") {
		return Gone;
	}
	if (condition == "internal-server-error") {
		return InternalServerError;
	}
	if (condition == "item-not-found") {
		return ItemNotFound;
	}
	if (condition == "jid-malformed") {
		return JidMalformed;
	}
	if (condition == "not-acceptable") {
		return NotAcceptable;
	}
	if (condition == "not-allowed") {
		return NotAllowed;
	}
	if (condition == "not-authorized") {
		return NotAuthorized;
	}
	if (condition == "payment-required") {
		return PaymentRequired;
	}
	if (condition == "recipient-unavailable") {
		return RecipientUnavailable;
	}
	if (condition == "redirect") {
		return Redirect;
	}
	if (condition == "registration-required") {
		return RegistrationRequired;
	}
	if (condition == "remote-server-not-found") {
		return RemoteServerNotFound;
	}
	if (condition == "resource-constraint") {
		return ResourceConstraint;
	}
	if (condition == "service-unavailable") {
		return ServiceUnavailable;
	}
	if (condition == "subscription-required") {
		return SubscriptionRequired;
	}
	if (condition == "undefined-condition") {
		return UndefinedCondition;
	}
	if (condition == "unexpected-request") {
		return UnexpectedRequest;
	}
	return -1;
}

/**
 * Returns enum-value representation of error-type string.
 */
Stanza::Error::Type Stanza::Error::stringToType(const QString& type)
{
	if (type == "cancel") {
		return Cancel;
	}
	if (type == "continue") {
		return Continue;
	}
	if (type == "modify") {
		return Modify;
	}
	if (type == "auth") {
		return Auth;
	}
	if (type == "wait") {
		return Wait;
	}
	return Cancel;
}

/**
 * @enum Stanza::Error::Type
 * This enum describes possible stanza error types as defined in RFC-3920.
 */

/**
 * @var Stanza::Error::Cancel
 * Do not retry (the error is unrecoverable).
 */

/**
 * @var Stanza::Error::Continue
 * Proceed (the condition was only a warning).
 */

/**
 * @var Stanza::Error::Modify
 * Retry after changing the data sent.
 */

/**
 * @var Stanza::Error::Auth
 * Retry after providing credentials.
 */

/**
 * @var Stanza::Error::Wait
 * Retry after waiting (the error is temporary).
 */


/**
 * @enum Stanza::Error::Condition
 * This enum describes possible stanza error conditions as defined in RFC-3920.
 */

/**
 * @var Stanza::Error::BadRequest
 * The sender has sent XML that is malformed or that cannot be processed
 * (e.g., an IQ stanza that includes an unrecognized value of the 'type' attribute);
 * <br>the associated error type SHOULD be "modify".
 */

/**
 * @var Stanza::Error::Conflict
 * Access cannot be granted because an existing resource or session exists with
 * the same name or address;
 * <br>the associated error type SHOULD be "cancel".
 */

/**
 * @var Stanza::Error::FeatureNotImplemented
 * The feature requested is not implemented by the recipient or server and
 * therefore cannot be processed;
 * <br>the associated error type SHOULD be "cancel".
 */

/**
 * @var Stanza::Error::Forbidden
 * The requesting entity does not possess the required permissions to perform
 * <br>the action; the associated error type SHOULD be "auth".
 */

/**
 * @var Stanza::Error::Gone
 * The recipient or server can no longer be contacted at this address
 * (the error stanza MAY contain a new address in the XML character data
 * of the <gone/> element);
 * <br>the associated error type SHOULD be "modify".
 */
/**
 * @var Stanza::Error::InternalServerError
 * The server could not process the stanza because of a misconfiguration or an
 * otherwise-undefined internal server error;
 * <br>the associated error type SHOULD be "wait".
 */
/**
 * @var Stanza::Error::ItemNotFound
 * The addressed JID or item requested cannot be found;
 * <br>the associated error type SHOULD be "cancel".
 */

/**
 * @var Stanza::Error::JidMalformed
 * The sending entity has provided or communicated an XMPP address
 * (e.g., a value of the 'to' attribute) or aspect thereof (e.g., a resource identifier)
 * that does not adhere to the syntax defined in Addressing Scheme (Section 3);
 * <br>the associated error type SHOULD be "modify".
 */

/**
 * @var Stanza::Error::NotAcceptable
 * The recipient or server understands the request but is refusing to process it
 * because it does not meet criteria defined by the recipient or server (e.g.,
 * a local policy regarding acceptable words in messages);
 * <br>the associated error type SHOULD be "modify".
 */

/**
 * @var Stanza::Error::NotAllowed
 * The recipient or server does not allow any entity to perform the action;
 * <br>the associated error type SHOULD be "cancel".
 */

/**
 * @var Stanza::Error::NotAuthorized
 * The sender must provide proper credentials before being allowed to perform the action, or has provided improper credentials;
 * <br>the associated error type SHOULD be "auth".
 */

/**
 * @var Stanza::Error::PaymentRequired
 * The requesting entity is not authorized to access the requested service because payment is required;
 * <br>the associated error type SHOULD be "auth".
 */

/**
 * @var Stanza::Error::RecipientUnavailable
 * The intended recipient is temporarily unavailable;
 * <br>the associated error type SHOULD be "wait"
 * <br>(note: an application MUST NOT return this error if doing so would provide
 * information about the intended recipient's network availability to an entity
 * that is not authorized to know such information).
 */

/**
 * @var Stanza::Error::Redirect
 * The recipient or server is redirecting requests for this information to another
 * entity, usually temporarily (the error stanza SHOULD contain the alternate address,
 * which MUST be a valid JID, in the XML character data of the <redirect/> element);
 * <br>the associated error type SHOULD be "modify".
 */

/**
 * @var Stanza::Error::RegistrationRequired
 * The requesting entity is not authorized to access the requested service because registration is required;
 * <br>the associated error type SHOULD be "auth".
 */

/**
 * @var Stanza::Error::RemoteServerNotFound
 * A remote server or service specified as part or all of the JID of the intended recipient does not exist;
 * <br>the associated error type SHOULD be "cancel".
 */

/**
 * @var Stanza::Error::RemoteServerTimeout
 * A remote server or service specified as part or all of the JID of the intended recipient
 * (or required to fulfill a request) could not be contacted within a reasonable amount of time;
 * <br>the associated error type SHOULD be "wait".
 */

/**
 * @var Stanza::Error::ResourceConstraint
 * The server or recipient lacks the system resources necessary to service the request;
 * <br>the associated error type SHOULD be "wait".
 */

/**
 * @var Stanza::Error::ServiceUnavailable
 * The server or recipient does not currently provide the requested service;
 * <br>the associated error type SHOULD be "cancel".
 */

/**
 * @var Stanza::Error::SubscriptionRequired
 * The requesting entity is not authorized to access the requested service because a subscription is required;
 * <br>the associated error type SHOULD be "auth".
 */

/**
 * @var Stanza::Error::UndefinedCondition
 * The error condition is not one of those defined by the other conditions in this list;
 * <br>any error type may be associated with this condition, and it SHOULD be used only in conjunction with an application-specific condition.
 */

/**
 * @var Stanza::Error::UnexpectedRequest
 * The recipient or server understood the request but was not expecting it at this time (e.g., the request was out of order);
 * <br>the associated error type SHOULD be "wait".
 */


} /* end of namespace XMPP */
