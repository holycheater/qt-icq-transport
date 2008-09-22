/*
 * icqMetaInfomanager.h - handles ICQ specific family 0x15 requests.
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

#ifndef ICQMETAINFOMANAGER_H_
#define ICQMETAINFOMANAGER_H_

#include "icqConnection.h"

#include <QObject>

namespace ICQ
{


class MetaInfoManager : public QObject
{
	Q_OBJECT

	public:
		MetaInfoManager(Connection *parent);
		~MetaInfoManager();

		void sendMetaRequest(Word type);
		void sendMetaRequest(Word type, Buffer& metadata);
	signals:
		void metaInfoAvailable(Word type, Buffer& data);
	private:
		void handle_meta_info(SnacBuffer& snac);
	private slots:
		void incomingSnac(SnacBuffer& snac);
	private:
		class Private;
		Private *d;
};

}

#endif /* ICQMETAINFOMANAGER_H_ */
