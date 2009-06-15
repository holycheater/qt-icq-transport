/*
 * UserManager.h - User Database Manager
 * Copyright (C) 2008  Alexander Saltykov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef USER_MANAGER_H_
#define USER_MANAGER_H_

#include <QHash>

class QString;
class QStringList;
class QVariant;

#define qUsrMgr UserManager::instance()

class UserManager
{
    public:     
        static UserManager* instance();

        void add(const QString& user, const QString& uin, const QString& passwd);
        void del(const QString& user);
        bool isRegistered(const QString& user) const;

        QString getUin(const QString& user) const;
        QString getPassword(const QString& user) const;

        QStringList getUserList() const;
        QStringList getUserListByOptVal(const QString& option, const QVariant& value) const;

        QVariant getOption(const QString& user, const QString& option) const;
        void setOption(const QString& user, const QString& option, const QVariant& value);
        bool hasOption(const QString& user, const QString& option) const;

        QHash<QString,QVariant> options(const QString& user) const;
        void setOptions(const QString& user, const QHash<QString,QVariant>& list);
        void clearOptions(const QString& user);
    private:
        UserManager();
        ~UserManager();
        Q_DISABLE_COPY(UserManager);

        static UserManager* m_instance;
};

// vim:et:ts=4:sw=4:nowrap
#endif /* USER_MANAGER_H_ */

