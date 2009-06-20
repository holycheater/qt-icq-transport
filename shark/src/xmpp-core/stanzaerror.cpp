/*
 * stanzaerror.cpp - XMPP stanza error (RFC-3920).
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

#include <QDomElement>
#include <QSharedData>
#include <QString>

#include "stanza.h"

namespace XMPP {

/**
 * @class Stanza::Error
 * @brief Represents stanza error.
 *
 * A valid error must contain information about error type and error condition.
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

class Stanza::Error::Private : public QSharedData
{
    public:
        Private();
        ~Private();

        static QString conditionToString(int condition);
        static QString typeToString(int type);
        static int typeForCondition(int condition);
        static int stringToCondition(const QString& condition);
        static int stringToType(const QString& type);

        typedef struct {
            int num;
            const char* str;
        } IntStringPair;

        typedef struct {
            int first;
            int second;
        } IntPair;

        static IntStringPair errorTypeTable[];
        static IntStringPair errorCondTable[];
        static IntPair typeCondTable[];

        Condition condition;
        Type type;

        QString appCondition;
        QString appConditionNS;
        QString text;
};

Stanza::Error::Private::Private()
    : QSharedData()
{
}

Stanza::Error::Private::~Private()
{
}

Stanza::Error::Private::IntStringPair Stanza::Error::Private::errorTypeTable[] = {
        { Cancel    , "cancel" },
        { Continue  , "continue" },
        { Modify    , "modify" },
        { Auth      , "auth" },
        { Wait      , "wait" },
        { 0, 0 }
};

Stanza::Error::Private::IntStringPair Stanza::Error::Private::errorCondTable[] = {
        { BadRequest            , "bad-request" },
        { Conflict              , "conflict" },
        { FeatureNotImplemented , "feature-not-implemented" },
        { Forbidden             , "forbidden" },
        { Gone                  , "gone" },
        { InternalServerError   , "internal-server-error" },
        { ItemNotFound          , "item-not-found" },
        { JidMalformed          , "jid-malformed" },
        { NotAcceptable         , "not-acceptable" },
        { NotAllowed            , "not-allowed" },
        { NotAuthorized         , "not-authorized" },
        { PaymentRequired       , "payment-required" },
        { RecipientUnavailable  , "recipient-unavailable" },
        { Redirect              , "redirect" },
        { RegistrationRequired  , "registration-required" },
        { RemoteServerNotFound  , "remote-server-not-found" },
        { RemoteServerTimeout   , "remote-server-timeout" },
        { ResourceConstraint    , "resource-constraint" },
        { ServiceUnavailable    , "service-unavailable" },
        { SubscriptionRequired  , "subscription-required" },
        { UndefinedCondition    , "undefined-condition" },
        { UnexpectedRequest     , "unexpected-request" },
        { 0, 0 }

};

Stanza::Error::Private::IntPair Stanza::Error::Private::typeCondTable[] = {
        { BadRequest            , Modify },
        { Conflict              , Cancel },
        { FeatureNotImplemented , Cancel },
        { Forbidden             , Auth },
        { Gone                  , Modify },
        { InternalServerError   , Wait },
        { ItemNotFound          , Cancel },
        { JidMalformed          , Modify },
        { NotAcceptable         , Modify },
        { NotAllowed            , Cancel },
        { NotAuthorized         , Auth },
        { PaymentRequired       , Auth },
        { RecipientUnavailable  , Wait },
        { Redirect              , Modify },
        { RegistrationRequired  , Auth },
        { RemoteServerNotFound  , Cancel },
        { RemoteServerTimeout   , Wait },
        { ResourceConstraint    , Wait },
        { ServiceUnavailable    , Cancel },
        { SubscriptionRequired  , Auth },
        { UndefinedCondition    , Wait },
        { UnexpectedRequest     , Wait },
        { 0, 0 }
};

QString Stanza::Error::Private::conditionToString(int condition)
{
    for (int i = 0; errorCondTable[i].str; ++i) {
        if (errorCondTable[i].num == condition) {
            return errorCondTable[i].str;
        }
    }
    return "undefined-condition";
}

QString Stanza::Error::Private::typeToString(int type)
{
    for (int i = 0; errorTypeTable[i].str; ++i) {
        if (errorTypeTable[i].num == type) {
            return errorTypeTable[i].str;
        }
    }
    return "cancel";
}

int Stanza::Error::Private::typeForCondition(int condition)
{
    for (int i = 0; typeCondTable[i].first; ++i) {
        if ( typeCondTable[i].first == condition ) {
            return typeCondTable[i].second;
        }
    }
    return -1;
}

int Stanza::Error::Private::stringToCondition(const QString& condition)
{
    for (int i = 0; errorCondTable[i].str; ++i) {
        if (errorCondTable[i].str == condition) {
            return errorCondTable[i].num;
        }
    }
    return UndefinedCondition;
}

int Stanza::Error::Private::stringToType(const QString& type)
{
    for (int i = 0; errorTypeTable[i].str; ++i) {
        if (errorTypeTable[i].str == type) {
            return errorTypeTable[i].num;
        }
    }
    return Cancel;
}

/**
 * Default constructor for stanza-error object. It does not set any data.
 */
Stanza::Error::Error()
    : d(new Private)
{
}

/**
 * Constructs a deep copy of @a other
 */
Stanza::Error::Error(const Error& other)
    : d(other.d)
{
}

/**
 * Constructs stanza-error with given @a condition and (optionally) @a text.
 */
Stanza::Error::Error(Condition condition, const QString& text)
    : d(new Private)
{
    setCondition(condition);
    d->text = text;
}

/**
 * Constructs stanza-error with given @a type, @a condition and (optionally) @a text
 */
Stanza::Error::Error(Type type, Condition condition, const QString& text)
    : d(new Private)
{
    d->type = type;
    d->condition = condition;
    d->text = text;
}

/**
 * Constructs stanza-error with given type, condition, application-specific condition and (optionally) text.
 * @param type              error type
 * @param condition         error condition
 * @param appConditionNS    application namespace
 * @param appCondition      application-specific error condition
 * @param text              optional description text
 */
Stanza::Error::Error(Type type, Condition condition, const QString& appConditionNS, const QString& appCondition, const QString& text)
    : d(new Private)
{
    d->type = type;
    d->condition = condition;
    d->appConditionNS = appConditionNS;
    d->appCondition = appCondition;
    d->text = text;
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
    error.d->type = (Type)Private::stringToType( eError.attribute("type") );

    QDomNodeList childs = eError.childNodes();
    for ( int i = 0; i < childs.count(); ++i ) {
        QDomElement element = childs.item(i).toElement();
        if ( element.namespaceURI() == NS_STANZAS) {
            int condition = Private::stringToCondition(element.tagName() );
            if ( condition != -1 ) {
                error.d->condition = (Condition)condition;
                break;
            }
        }
    }

    QDomElement eText = eError.firstChildElement("text");
    if ( eText.namespaceURI() == NS_STANZAS && !eText.text().isNull() ) {
        error.d->text = eText.text().trimmed();
    }

    for ( int i = 0; i < childs.count(); ++i ) {
        QDomElement element = childs.item(i).toElement();
        /* first element outside NS_STANZAS ns will be an app-specific error condition */
        if ( element.namespaceURI() != NS_STANZAS) {
            error.d->appConditionNS = element.namespaceURI();
            error.d->appCondition = element.tagName();
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
    return d->appCondition;
}

/**
 * Returns application-namespace string for application-specific error condition.
 */
QString Stanza::Error::appConditionNS() const
{
    return d->appConditionNS;
}

/**
 * Returns stanza error condition.
 *
 * @sa Condition
 */
Stanza::Error::Condition Stanza::Error::condition() const
{
    return d->condition;
}

/**
 * Returns stanza optional error text.
 */
QString Stanza::Error::text() const
{
    return d->text;
}

/**
 * returns stanza error type
 *
 * @sa Type
 */
Stanza::Error::Type Stanza::Error::type() const
{
    return d->type;
}

/**
 * Sets application-specific error condition within application namespace.
 *
 * @param appConditionNS    Application namespace.
 * @param appCondition      Application-specific error condition.
 */
void Stanza::Error::setAppCondition(const QString& appConditionNS, const QString& appCondition)
{
    d->appCondition = appCondition;
    d->appConditionNS = appConditionNS;
}

/**
 * Sets error condition to @a condition.
 */
void Stanza::Error::setCondition(Condition condition)
{
    d->condition = condition;
    int type = Private::typeForCondition(condition);
    if (type != -1) {
        d->type = (Type)type;
    }
}

/**
 * Sets optional error description text to @a text.
 */
void Stanza::Error::setText(const QString& text)
{
    d->text = text;
}

/**
 * Sets error type to @a type.
 */
void Stanza::Error::setType(Type type)
{
    d->type = type;
}

/**
 * Adds and fills an \<error/\> element to the specified @a element.
 *
 * Element SHOULD be a stanza element.
 */
void Stanza::Error::pushToDomElement(QDomElement element) const
{
    QDomElement eError = element.ownerDocument().createElement("error");
    eError.setAttribute( "type", Private::typeToString(d->type) );

    QDomElement eCondition = element.ownerDocument().createElementNS( NS_STANZAS, Private::conditionToString(d->condition) );
    eError.appendChild(eCondition);

    if ( !d->text.isEmpty() ) {
        QDomElement eText = element.ownerDocument().createElementNS(NS_STANZAS, "text");
        eError.appendChild(eText);

        QDomText tText = element.ownerDocument().createTextNode(d->text);
        eText.appendChild(tText);
    }

    if ( !d->appConditionNS.isEmpty() && !d->appCondition.isEmpty() ) {
        QDomElement eAppCond = element.ownerDocument().createElementNS(d->appConditionNS, d->appCondition);
        eError.appendChild(eAppCond);
    }

    element.appendChild(eError);
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
 * of the \<gone/\> element);
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
 * which MUST be a valid JID, in the XML character data of the \<redirect/\> element);
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

// vim:ts=4:sw=4:et:nowrap
