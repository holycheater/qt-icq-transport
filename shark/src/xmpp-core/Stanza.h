/*
 * Stanza.h - XMPP Stanza
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

#ifndef XMPP_CORE_STANZA_H_
#define XMPP_CORE_STANZA_H_

#include <QDomDocument>
#include <QSharedDataPointer>

#define NS_STANZAS "urn:ietf:params:xml:ns:xmpp-stanzas"
#define NS_PROTOCOL_NICK "http://jabber.org/protocol/nick"

namespace XMPP {

class Jid;


class Stanza
{
    public:
        Stanza();
        Stanza(const Stanza& other);
        Stanza(const QDomDocument& document);
        Stanza(const QDomElement& element);
        virtual ~Stanza();

        class Error;

        Error error() const;
        bool hasError() const;
        void setError(const Error& error);

        bool isValid() const;

        Jid to() const;
        Jid from() const;
        QString type() const;
        QString id() const;

        void setTo(const Jid& toJid);
        void setFrom(const Jid& fromJid);
        void setType(const QString& type);
        void setId(const QString& id);

        void swapFromTo();
        QString toString() const;

        Stanza& operator=(const Stanza& other);

        /* XEP-0172: User Nickname */
        QString nick() const;
        void setNick(const QString& nick);

        QDomDocument* doc();
        QDomDocument* doc() const;
    protected:
        void setProperty(const QString& name, const QString& value);
    private:
        QDomDocument m_doc;
};

class Stanza::Error {
    public:
        enum Type { Cancel = 1, Continue, Modify, Auth, Wait };
        enum Condition {
            BadRequest = 1, Conflict, FeatureNotImplemented, Forbidden, Gone,
            InternalServerError, ItemNotFound, JidMalformed, NotAcceptable,
            NotAllowed, NotAuthorized, PaymentRequired, RecipientUnavailable,
            Redirect, RegistrationRequired, RemoteServerNotFound, RemoteServerTimeout,
            ResourceConstraint, ServiceUnavailable, SubscriptionRequired, UndefinedCondition,
            UnexpectedRequest
        };

        Error();
        Error(const Error& other);
        Error(Condition condition, const QString& text = "");
        Error(Type type, Condition condition, const QString& text = "");
        Error(Type type, Condition condition, const QString& appConditionNS, const QString& appCondition, const QString& text = "");
        static Error fromStanza(const Stanza& stanza);
        ~Error();

        QString appCondition() const;
        QString appConditionNS() const;
        Condition condition() const;
        QString text() const;
        Type type() const;

        void setAppCondition(const QString& appConditionNS, const QString& appCondition);
        void setCondition(Condition condition);
        void setText(const QString& text);
        void setType(Type type);

        void pushToDomElement(QDomElement element) const;
    private:
        class Private;
        QSharedDataPointer<Private> d;
};


}

// vim:ts=4:sw=4:et:nowrap
#endif /* XMPP_CORE_STANZA_H_ */
