/*
 * Registration.cpp - In-Band Registration (XEP-0077)
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

#include <QSharedData>

#include "Registration.h"

namespace XMPP {


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
 * Returns 'instructions' field text.
 *
 * @sa username(), password(), email()
 */
QString Registration::instructions() const
{
	return childElement().firstChildElement("instructions").text().trimmed();
}

/**
 * Returns 'username' field text.
 *
 * @sa instructions(), password(), email()
 */
QString Registration::username() const
{
	return childElement().firstChildElement("username").text().trimmed();
}

/**
 * Returns 'password' field text.
 *
 * @sa instructions(), username(), email()
 */
QString Registration::password() const
{
	return childElement().firstChildElement("password").text().trimmed();
}

/**
 * Returns 'email' field text.
 *
 * @sa instructions(), username(), password()
 */
QString Registration::email() const
{
	return childElement().firstChildElement("email").text().trimmed();
}

/**
 * Sets 'instructions' field text to @a instructions
 */
void Registration::setInstructions(const QString& instructions)
{
	childElement().removeChild( childElement().firstChildElement("instructions") );
	QDomElement element = doc()->createElement("instructions");
	childElement().appendChild(element);

	QDomText text = doc()->createTextNode(instructions);
	element.appendChild(text);
}

/**
 * Sets up an empty 'instructions' field if @a present is true, removes 'instructions' field if @a present is false.
 *
 * @note If @a present is true and 'instructions' field has some text, it becomes an empty element (i.e. internal text is removed).
 */
void Registration::setInstructions(bool present)
{
	childElement().removeChild( childElement().firstChildElement("instructions") );
	if (present) {
		QDomElement element = doc()->createElement("instructions");
		childElement().appendChild(element);
	}
}

/**
 * Sets 'username' field text to @a username
 */
void Registration::setUsername(const QString& username)
{
	childElement().removeChild( childElement().firstChildElement("username") );
	QDomElement element = doc()->createElement("username");
	childElement().appendChild(element);

	QDomText text = doc()->createTextNode(username);
	element.appendChild(text);
}

/**
 * Sets up an empty 'username' field if @a present is true, removes 'username' field if @a present is false.
 *
 * @note If @a present is true and 'username' field has some text, it becomes an empty element (i.e. internal text is removed).
 */
void Registration::setUsername(bool present)
{
	childElement().removeChild( childElement().firstChildElement("instructions") );
	if (present) {
		QDomElement element = doc()->createElement("instructions");
		childElement().appendChild(element);
	}
}

/**
 * Sets 'password' field text to @a password
 */
void Registration::setPassword(const QString& password)
{
	childElement().removeChild( childElement().firstChildElement("password") );
	QDomElement element = doc()->createElement("password");
	childElement().appendChild(element);

	QDomText text = doc()->createTextNode(password);
	element.appendChild(text);
}

/**
 * Sets up an empty 'password' field if @a present is true, removes 'password' field if @a present is false.
 *
 * @note If @a present is true and 'password' field has some text, it becomes an empty element (i.e. internal text is removed).
 */
void Registration::setPassword(bool present)
{
	childElement().removeChild( childElement().firstChildElement("instructions") );
	if (present) {
		QDomElement element = doc()->createElement("instructions");
		childElement().appendChild(element);
	}
}

/**
 * Sets 'email' field text to @a email
 */
void Registration::setEmail(const QString& email)
{
	childElement().removeChild( childElement().firstChildElement("email") );
	QDomElement element = doc()->createElement("email");
	childElement().appendChild(element);

	QDomText text = doc()->createTextNode(email);
	element.appendChild(text);
}

/**
 * Sets up an empty 'email' field if @a present is true, removes 'email' field if @a present is false.
 *
 * @note If @a present is true and 'email' field has some text, it becomes an empty element (i.e. internal text is removed).
 */
void Registration::setEmail(bool present)
{
	childElement().removeChild( childElement().firstChildElement("instructions") );
	if (present) {
		QDomElement element = doc()->createElement("instructions");
		childElement().appendChild(element);
	}
}


} /* end of namespace XMPP */
