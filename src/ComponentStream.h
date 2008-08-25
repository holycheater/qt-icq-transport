#ifndef COMPONENTSTREAM_H_
#define COMPONENTSTREAM_H_

#include "xmpp_stream.h"

#include <QDomDocument>
#include <QObject>
#include <QString>

namespace XMPP {

class AdvancedConnector;


class ComponentStream : public Stream
{
	Q_OBJECT

	public:
		ComponentStream(AdvancedConnector *connector, QObject *parent = 0);
		~ComponentStream();

		void connectToServer(const Jid& jid, quint16 port, const QString& secret);

		QDomDocument& doc() const;
		QString baseNS() const;
		bool old() const;

		void close();
		bool stanzaAvailable() const;
		Stanza read();
		void write(const Stanza& stanza);

		int errorCondition() const;
		QString errorText() const;
		QDomElement errorAppSpec() const;
	private:
		void processNext();
	private slots:
		void bs_bytesWritten(int size);
		void bs_error(int errno);
		void bs_readyRead();
		void bs_closed();
		void cr_connected();
		void cr_error();
	private:
		class Private;
		Private *d;
};


} // end of namespace XMPP

#endif /* COMPONENTSTREAM_H_ */
