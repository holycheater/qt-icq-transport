/*
 * AdHoc.cpp - Ad-hoc command (XEP-0050)
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

#include "AdHoc.h"
#include "DataForm.h"

#include "xmpp-core/IQ.h"

#include <QSharedData>
#include <QString>

/* TODO: Process and represent <actions/> element in <command/> */
/* TODO: Process and represent <note/> element in <command/> */

namespace XMPP
{


/**
 * @class AdHoc
 * Represents an ad-hoc command from XEP-0050.
 */

class AdHoc::Private : public QSharedData
{
	public:
		Private();
		Private(const Private& other);
		~Private();

		static QString actionToString(Action action);
		static QString statusToString(Status status);

		static Action stringToAction(const QString& action);
		static Status stringToStatus(const QString& status);

		Action action;

		DataForm form;

		QString node;
		QString sessionID;

		Status status;
};

AdHoc::Private::Private()
	: QSharedData()
{
	action = Execute;
	status = StatusNone;
}

AdHoc::Private::Private(const Private& other)
	: QSharedData(other)
{
	action = other.action;
	form = other.form;

	node = other.node;
	sessionID = other.sessionID;

	status = other.status;
}

AdHoc::Private::~Private()
{
}

QString AdHoc::Private::actionToString(Action action)
{
	QString actionStr;
	switch ( action ) {
		case Cancel:
			actionStr = "cancel";
			break;
		case Complete:
			actionStr = "complete";
			break;
		case Next:
			actionStr = "next";
			break;
		case Prev:
			actionStr = "prev";
			break;
		default:
			actionStr = "execute";
			break;
	}
	return actionStr;
}

QString AdHoc::Private::statusToString(Status status)
{
	QString statusStr;
	switch ( status ) {
		case Canceled:
			statusStr = "canceled";
			break;
		case Completed:
			statusStr = "completed";
			break;
		case Executing:
			statusStr = "executing";
			break;
		default:
			break;
	}
	return statusStr;
}

AdHoc::Action AdHoc::Private::stringToAction(const QString& action)
{
	if ( action == "cancel" ) {
		return Cancel;
	}
	if ( action == "complete" ) {
		return Complete;
	}
	if ( action == "next" ) {
		return Next;
	}
	if ( action == "prev" ) {
		return Prev;
	}
	return Execute;
}

AdHoc::Status AdHoc::Private::stringToStatus(const QString& status)
{
	if ( status == "canceled" ) {
		return Canceled;
	}
	if ( status == "completed" ) {
		return Completed;
	}
	if ( status == "executing" ) {
		return Executing;
	}
	return StatusNone;
}

AdHoc::AdHoc()
	: d(new Private)
{
}

AdHoc::AdHoc(const AdHoc& other)
	: d(other.d)
{
}

AdHoc::~AdHoc()
{
}

AdHoc& AdHoc::operator=(const AdHoc& other)
{
	d = other.d;
	return *this;
}

/**
 * Constructs an ad-hoc command object from IQ stanza.
 */
AdHoc AdHoc::fromIQ(const IQ& iq)
{
	QDomElement eCommand = iq.childElement();
	if ( eCommand.namespaceURI() != NS_QUERY_ADHOC || eCommand.tagName() != "command" ) {
		return AdHoc();
	}
	AdHoc cmd;

	cmd.setSessionID( eCommand.attribute("sessionid") );
	cmd.setNode( eCommand.attribute("node") );
	cmd.setAction( Private::stringToAction(eCommand.attribute("action")) );
	cmd.setStatus( Private::stringToStatus(eCommand.attribute("status")) );

	QDomElement eForm = eCommand.firstChildElement("x");
	if ( eForm.namespaceURI() == NS_DATA_FORMS ) {
		cmd.setForm( DataForm::fromDomElement(eForm) );
	}

	return cmd;
}

/**
 * Inserts ad-hoc command object into IQ stanza.
 */
void AdHoc::toIQ(IQ& iq)
{
	iq.setChildElement("command", NS_QUERY_ADHOC);
	QDomElement eCommand = iq.childElement();

	if ( d->action != ActionNone ) {
		eCommand.setAttribute("action", Private::actionToString(d->action) );
	}
	if ( d->status != StatusNone ) {
		eCommand.setAttribute("status", Private::statusToString(d->status) );
	}
	eCommand.setAttribute("node", d->node);
	eCommand.setAttribute("sessionid", d->sessionID);

	if ( d->form.isValid() ) {
		d->form.toDomElement(eCommand);
	}
}

/**
 * Returns command action
 */
AdHoc::Action AdHoc::action() const
{
	return d->action;
}

/**
 * Sets command action
 */
void AdHoc::setAction(Action action)
{
	d->action = action;
}

/**
 * Returns ad-hoc command form (if any)
 */
DataForm AdHoc::form() const
{
	return d->form;
}

/**
 * Returns true if ad-hoc has a form and it is valid.
 */
bool AdHoc::hasForm() const
{
	return d->form.isValid();
}

void AdHoc::setForm(const DataForm& form)
{
	d->form = form;
}

/**
 * Returns node string assigned to this ad-hoc command.
 */
QString AdHoc::node() const
{
	return d->node;
}

/**
 * Sets @a node string for ad-hoc command.
 */
void AdHoc::setNode(const QString& node)
{
	d->node = node;
}

/**
 * Returns command's session-id string.
 */
QString AdHoc::sessionID() const
{
	return d->sessionID;
}

/**
 * Sets command's session-id string.
 */
void AdHoc::setSessionID(const QString& id)
{
	d->sessionID = id;
}

/**
 * Returns command's status.
 */
AdHoc::Status AdHoc::status() const
{
	return d->status;
}

/**
 * Sets command's status.
 */
void AdHoc::setStatus(Status status)
{
	d->status = status;
}


}

// vim:ts=4:sw=4:nowrap:noet
