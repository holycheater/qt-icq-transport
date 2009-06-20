/*
 * stream.h - Abstract XMPP stream class.
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

#ifndef XMPP_STREAM_H_
#define XMPP_STREAM_H_

#define NS_ETHERX "http://etherx.jabber.org/streams"
#define NS_STREAMS "urn:ietf:params:xml:ns:xmpp-streams"

#include <QObject>
#include "streamerror.h"
#include "xmpp-core/Parser.h"

class QIODevice;

namespace XMPP {

class Jid;
class Stanza;
class IQ;
class Message;
class Presence;


class Stream : public QObject
{
    Q_OBJECT

    public:
        enum State { Open, Closed };

        Stream(QObject *parent = 0);
        virtual ~Stream();

        StreamError lastStreamError() const;

        virtual QString baseNS() const = 0;

        void sendStanza(const Stanza& stanza);

    public slots:
        void sendStreamOpen();
        void sendStreamClose();
    signals:
        void streamOpened();
        void streamClosed();
        void streamError();
        void streamReady();

        void stanzaIQ(const XMPP::IQ&);
        void stanzaMessage(const XMPP::Message&);
        void stanzaPresence(const XMPP::Presence&);
    protected:
        void setByteStream(QIODevice *bs);
        void setRemoteEntity(const Jid& entity);

        virtual void handleStreamOpen(const Parser::Event& e) = 0;
        virtual bool handleUnknownElement(const Parser::Event& e) = 0;

        void write(const QByteArray& data);
    private:
        void handleStreamError(const Parser::Event& event);
        void processEvent(const Parser::Event& event);
        void processStanza(const Parser::Event& event);
    private slots:
        void bsReadyRead();
        void bsClosing();
    private:
        class Private;
        Private *d;
};


} /* end of namespace XMPP */

// vim:ts=4:sw=4:et:nowrap
#endif /* XMPP_STREAM_H_ */
