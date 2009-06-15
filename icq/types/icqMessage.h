/*
 * icqMessage.h - ICQ Message data type.
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

#ifndef ICQMESSAGE_H_
#define ICQMESSAGE_H_

#include "icqTypes.h"

#include <QSharedDataPointer>

class QByteArray;
class QDateTime;
class QString;
class QTextCodec;

namespace ICQ
{


class Message
{
    public:
        enum Flag { Normal = 0x01, AutoMessage = 0x03, MultiMessage = 0x80 };

        enum Type { InvalidType = 0xFF, PlainText = 0x01, ChatRequest, FileRequest, URL, AuthRequest = 0x06,
            AuthDeny, AuthGranted, Server, YouWereAdded = 0x0C, WebPager,
            EmailExpress, ContactList = 0x13, PluginMessage = 0x1A,
            AutoAway = 0xE8, AutoBusy, AutoNA, AutoDND, AutoFFC };

        enum Encoding {
            UserDefined,
            Ascii,
            Latin1,
            Utf8,
            Ucs2
        };

        Message();
        Message(const Message& other);
        Message& operator=(const Message& other);
        ~Message();

        bool isEmpty() const;
        bool isValid() const;

        bool isOffline() const;
        void setOffline(bool offline = true);

        /* get/set message channel */
        Byte channel() const;
        void setChannel(Byte channel);

        Encoding encoding() const;
        void setEncoding(Encoding enc);

        /* get/set message flags */
        Byte flags() const;
        void setFlags(Byte flags);

        /* get/set message cookie (8 bytes) */
        QByteArray icbmCookie() const;
        void setIcbmCookie(const QByteArray& cookie);

        /* get/set message receiver uin */
        QString receiver() const;
        void setReceiver(DWord uin);
        void setReceiver(const QString& uin);

        /* get/set message sender uin */
        QString sender() const;
        void setSender(DWord uin);
        void setSender(const QString& uin);

        /* get/set message text */
        QByteArray text() const;
        QString text(QTextCodec *codec) const;
        void setText(const QByteArray& text);

        /* get/set message timestamp */
        QDateTime timestamp() const;
        void setTimestamp(QDateTime timestamp);
        void setTimestamp(DWord timestamp_t);

        /* get/set message type */
        Byte type() const;
        void setType(Byte type);
    private:
        class Private;
        QSharedDataPointer<Private> d;
};

}

// vim:ts=4:sw=4:et:nowrap
#endif /* ICQMESSAGE_H_ */
