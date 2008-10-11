#ifndef ICQ_CONNECTION_PRIVATE_H_
#define ICQ_CONNECTION_PRIVATE_H_

#include "icqConnection.h"

#include "managers/icqLoginManager.h"
#include "managers/icqRateManager.h"
#include "managers/icqSsiManager.h"
#include "managers/icqUserInfoManager.h"
#include "managers/icqMessageManager.h"
#include "managers/icqMetaInfoManager.h"

#include "types/icqMessage.h"

#include <QObject>
#include <QTcpSocket>
#include <QTimer>

namespace ICQ {


class Connection::Private : public QObject
{
	Q_OBJECT

	public:
		Private(Connection* parent);
		~Private();

		void startSignOn();

		Word flapSequence();
		Word snacRequest();

		int connectionStatus() const;
		void setConnectionStatus(int status);

		/* SNAC(xx,01) */
		void handle_error(SnacBuffer& snac);
	public slots:
		void incomingData();
		void sendKeepAlive();

		void processConnectionTimeout();
		void processConnected();
		void processDisconnected();
		void processLookupResult(const QHostInfo& host);
		void processLookupTimeout();
		void processIncomingMessage(const Message& msg);
		void processNewServer(QString server, quint16 port);
		void processRatesRequest();
		void processSsiActivated();
		void processSignedOn();
		void processSignedOff();
	public:
		LoginManager *loginManager;
		RateManager *rateManager;
		SSIManager *ssiManager;
		UserInfoManager *userInfoManager;
		MessageManager *msgManager;
		MetaInfoManager *metaManager;

		QString uin;
		QString password;
		QString server;
		quint16 port;

		QTcpSocket *socket;

		Word onlineStatus;

		int lookupId;

		QTimer *connectTimer;
		QTimer *keepAliveTimer;
		QTimer *lookupTimer;

		QTextCodec *codec;

		bool ssiActivated, loginFinished;

		typedef QPair<Word, QString> IntStringPair;
		static IntStringPair subtypeOneErrors[];
		static QString errDescForCode(Word errorCode);
	private:
		Connection *q;
		Word m_snacRequest;
		Word m_flapSequence;
		int m_connectionStatus;
};

} /* end of namespace ICQ */

#endif /* ICQ_CONNECTION_PRIVATE_H_ */
