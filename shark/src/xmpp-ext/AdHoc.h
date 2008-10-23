/*
 * AdHoc.h - Ad-hoc command (XEP-0050)
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

#ifndef XMPP_ADHOC_H_
#define XMPP_ADHOC_H_

#define NS_QUERY_ADHOC "http://jabber.org/protocol/commands"

#include <QSharedDataPointer>

class QString;

namespace XMPP
{

class DataForm;
class IQ;


class AdHoc
{
	public:
		enum Action { ActionNone, Cancel, Complete, Execute, Next, Prev };
		enum Status { StatusNone, Canceled, Completed, Executing };

		AdHoc();
		AdHoc(const AdHoc& other);
		virtual ~AdHoc();
		AdHoc& operator=(const AdHoc& other);

		static AdHoc fromIQ(const IQ& iq);
		void toIQ(IQ& iq);

		Action action() const;
		void setAction(Action action);

		DataForm form() const;
		bool hasForm() const;
		void setForm(const DataForm& form);

		QString node() const;
		void setNode(const QString& node);

		QString sessionID() const;
		void setSessionID(const QString& id);

		Status status() const;
		void setStatus(Status status);
	private:
		class Private;
		QSharedDataPointer<Private> d;
};


}

#endif /* XMPP_ADHOC_H_ */
