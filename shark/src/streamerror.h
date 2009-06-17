/*
 * streamerror.h - Stream-level errors (RFC 3920)
 * Copyright (C) 2009  Alexander Saltykov
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

#ifndef XMPP_STREAM_ERROR_H_
#define XMPP_STREAM_ERROR_H_

#include <QSharedDataPointer>

class QDomDocument;
class QDomElement;
class QString;

namespace XMPP {

class StreamError
{
    public:
        enum Condition {
            BadFormat, BadNamespacePrefix, Conflict, ConnectionTimeout, HostGone,
            HostUnknown, ImproperAddressing, InternalServerError, InvalidFrom,
            InvalidId, InvalidNamespace, InvalidXml, NotAuthorized, PolicyViolation,
            RemoteConnectionFailed, ResourceConstraint, RestrictedXml, SeeOtherHost,
            SystemShutdown, UndefinedCondition, UnsupportedEncoding, UnsupportedStanzaType,
            UnsupportedVersion, XmlNotWellFormed
        };

        StreamError();
        StreamError(const StreamError& other);
        StreamError(const QDomDocument& document);
        StreamError(const QDomElement& element);
        ~StreamError();
        StreamError& operator=(const StreamError& other);

        QString appSpec() const;
        QString appSpecNS() const;
        Condition condition() const;
        QString conditionString() const;
        QString text() const;

        void setAppSpec(const QString& ns, const QString& spec);
        void setCondition(Condition type);
        void setText(const QString& text);
    private:
        class Private;
        QSharedDataPointer<Private> d;
};


} /* end of namespace XMPP */

// vim:ts=4:sw=4:et:nowrap
#endif /* XMPP_STREAM_ERROR_H_ */
