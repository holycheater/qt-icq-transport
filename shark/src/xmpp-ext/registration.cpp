/*
 * registration.cpp - In-Band Registration (XEP-0077)
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

/* TODO: Implement Data Forms usage. */

#include <QList>

#include "registration.h"

namespace XMPP {


class Registration::Private
{
    public:
        static QString fieldToString(Field name);
        static int stringToField(const QString& name);

        typedef struct {
            int num;
            const char* str;
        } IntStringPair;

        static IntStringPair fieldStringTable[];
};
Registration::Private::IntStringPair Registration::Private::fieldStringTable[] = {
        { Instructions  , "instructions" },
        { Username      , "username" },
        { Nick          , "nick" },
        { Password      , "password" },
        { Name          , "name" },
        { First         , "first" },
        { Last          , "last" },
        { Email         , "email" },
        { Address       , "address" },
        { City          , "city" },
        { State         , "state" },
        { Zip           , "zip" },
        { Phone         , "phone" },
        { Url           , "url" },
        { Date          , "date" },
        { Misc          , "misc" },
        { Text          , "text" },
        { Registered    , "registered" },
        { Remove        , "remove" },
        { 0, 0 }
};

QString Registration::Private::fieldToString(Field name)
{
    for (int i = 0; fieldStringTable[i].str; ++i) {
        if (fieldStringTable[i].num == name) {
            return fieldStringTable[i].str;
        }
    }
    return QString();
}

int Registration::Private::stringToField(const QString& name)
{
    for (int i = 0; fieldStringTable[i].str; ++i) {
        if (fieldStringTable[i].str == name) {
            return fieldStringTable[i].num;
        }
    }
    return -1;
}

/**
 * @class Registration
 * Represents registration-stanza and implements XEP-0077.
 */

/**
 * Constructs registration iq-stanza.
 */
Registration::Registration()
    : IQ()
{
    setChildElement("query", NS_IQ_REGISTER);
}

/**
 * Constructs registration iq-stanza from IQ-object.
 */
Registration::Registration(const IQ& other)
    : IQ(other)
{
}

/**
 * Constructs a deep copy of other registration-stanza.
 */
Registration::Registration(const Registration& other)
    : IQ(other)
{
}

/**
 * Constructs a registration stanza from stanza dom-element.
 */
Registration::Registration(const QDomElement& element)
    : IQ(element)
{
}

/**
 * Destroys registration-object
 */
Registration::~Registration()
{
}

/**
 * Returns a list of registration fields.
 */
QList<Registration::Field> Registration::fields() const
{
    QList<Field> list;

    QDomNodeList childs = childElement().childNodes();
    for (int i = 0; i < childs.size(); ++i) {
        if ( childs.item(i).namespaceURI() != NS_IQ_REGISTER ) {
            // we don't count non-standard fields from something like jabber:x:data
            continue;
        }
        list << (Field)Private::stringToField( childs.item(i).nodeName() );
    }

    return list;
}

/**
 * Returns @a field field text.
 */
QString Registration::getField(Field name) const
{
    return childElement().firstChildElement( Private::fieldToString(name) ).text().trimmed();
}

/**
 * Returns true is field called @a name is present in the query.
 * @note It doesn't depend if field is empty or not.
 */
bool Registration::hasField(Field name) const
{
    return !childElement().firstChildElement( Private::fieldToString(name) ).isNull();
}

/**
 * Sets @a name field text to @a value
 */
void Registration::setField(Field name, const QString& value)
{
    QString field = Private::fieldToString(name);

    childElement().removeChild( childElement().firstChildElement(field) );
    QDomElement element = doc()->createElement(field);
    childElement().appendChild(element);

    QDomText text = doc()->createTextNode(value);
    element.appendChild(text);
}

/**
 * Sets up an empty @a name field if @a present is true, removes @a name field if @a present is false.
 *
 * @note If @a present is true and @a name field has some text, it becomes an empty element (i.e. internal text is removed).
 */
void Registration::setField(Field name, bool present)
{
    QString field = Private::fieldToString(name);

    childElement().removeChild( childElement().firstChildElement(field) );
    if (present) {
        QDomElement element = doc()->createElement(field);
        childElement().appendChild(element);
    }
}


} /* end of namespace XMPP */

// vim:ts=4:sw=4:nowrap:et
