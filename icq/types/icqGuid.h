/*
 * icqGuid.h - 16-byte UUID generator for ICQ capabilities.
 * Copyright (C) 2002-2005 by Kopete developers <kopete-devel@kde.org>
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

#ifndef ICQGUID_H_
#define ICQGUID_H_

#include <QByteArray>
#include <QString>

namespace ICQ {


class Guid
{
    public:
        Guid();

        static Guid fromRawData(const char* raw);
        static Guid fromRawData(const QByteArray& raw);
        static Guid fromString(const QString& guidstr);

        QByteArray data() const;

        bool isEqual(const Guid& rhs, int n = 16) const;
        bool isValid() const;
        bool isZero() const;

        void setData(const QByteArray& data);

        QString toString() const;

        Guid& operator=(const QByteArray& data);
        bool operator==(const Guid& rhs) const;
        operator QByteArray() const;
    private:
        void initData(const QString& data);
        QByteArray m_data;
};

}

// vim:ts=4:sw=4:et:nowrap
#endif /*ICQGUID_H_*/
