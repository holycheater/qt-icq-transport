/*
 * icqSsiManager.h - server-side information manager for an icq connection
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

#ifndef SSIMANAGER_H_
#define SSIMANAGER_H_

#include "types/icqSnacBuffer.h"

#include <QObject>
#include <QString>

class QDateTime;

namespace ICQ
{

class Contact;
class Socket;

class SSIManager: public QObject
{
    Q_OBJECT

    public:
        SSIManager(QObject *parent = 0);
        ~SSIManager();
        void setSocket(Socket *socket);

        void addContact(const QString& uin);
        void delContact(const QString& uin);

        Word addGroup(const QString& name);
        void delGroup(const QString& name);

        void grantAuthorization(const QString& uin);
        void denyAuthorization(const QString& uin);
        void requestAuthorization(const QString& uin);

        Contact contactByUin(const QString& uin);

        QList<Contact> contactList() const;
        QList<Contact> groupList() const;
        QList<Contact> visibleList() const;
        QList<Contact> invisibleList() const;
        QList<Contact> ignoreList() const;

        void checkContactList();
        void requestContactList();

        /* SNAC(13,02) - Request SSI rights/limitations  */
        void requestParameters();

        Word size() const;
        QDateTime lastChangeTime() const;
        void setLastChangeTime(const QDateTime& time);
    signals:
        void authGranted(const QString& uin);
        void authDenied(const QString& uin);

        void authRequest(const QString& uin);

        void contactAdded(const QString& uin);
        void contactDeleted(const QString& uin);

        void ssiActivated();
    private slots:
        void incomingSnac(SnacBuffer& snac);
    private:
        Q_DISABLE_COPY(SSIManager)
        class Private;
        Private *d;
};

}

// vim:ts=4:sw=4:et:nowrap
#endif /*SSIMANAGER_H_*/
