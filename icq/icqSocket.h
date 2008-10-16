#ifndef ICQ_SOCKET_H_
#define ICQ_SOCKET_H_

#include <QObject>

#include "types/icqTypes.h"

class QHostAddress;

namespace ICQ
{

class Buffer;
class FlapBuffer;
class SnacBuffer;
class RateManager;
class MetaInfoManager;

class Socket : public QObject
{
	Q_OBJECT

	public:
		Socket(QObject *parent = 0);
		virtual ~Socket();

		int connectionStatus() const;

		void connectToHost(const QHostAddress& host, quint16 port);
		void disconnectFromHost();

		void setRateManager(RateManager *ptr);
		void setMetaManager(MetaInfoManager *ptr);

		void snacRequest(Word family, Word subtype);

		void sendMetaRequest(Word type);
		void sendMetaRequest(Word type, Buffer& data);

		void write(const FlapBuffer& flap);
		void write(const SnacBuffer& snac);

		void writeForced(FlapBuffer* flap);
		void writeForced(SnacBuffer* snac);
	signals:
		void incomingFlap(FlapBuffer& flap);
		void incomingSnac(SnacBuffer& snac);

		void readyRead();
	private slots:
		void processIncomingData();
	private:
		Q_DISABLE_COPY(Socket)
		class Private;
		Private *d;
};


} /* end of namespace ICQ */

#endif /* ICQ_SOCKET_H_ */
