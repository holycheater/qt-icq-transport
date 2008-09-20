#ifndef ICQ_CONNECTION_PRIVATE_H_
#define ICQ_CONNECTION_PRIVATE_H_

#include "icqConnection.h"

#include "managers/icqLoginManager.h"
#include "managers/icqRateManager.h"
#include "managers/icqSsiManager.h"
#include "managers/icqUserInfoManager.h"
#include "managers/icqMessageManager.h"
#include "managers/icqMetaInfoManager.h"

#include <QTcpSocket>
#include <QTimer>

namespace ICQ {


class Connection::Private
{
	public:
		Private(Connection* parent);
		~Private();

		Word flapSequence();
		Word snacRequest() { return ++m_snacRequest; };

		int connectionStatus() const { return m_connectionStatus; }
		void setConnectionStatus(int status);

		/* SNAC(xx,01) */
		void handle_error(SnacBuffer& snac);

		LoginManager 	*loginManager;
		RateManager 	*rateManager;
		SSIManager 		*ssiManager;
		UserInfoManager *userInfoManager;
		MessageManager 	*msgManager;
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
	public slots:
		void connectToServer(const QHostInfo& host);
		void incomingData();
		void sendKeepAlive();

		void slot_connected();
		void slot_disconnected();
		void slot_lookupFailed();
		void slot_connectionTimeout();

		void slot_signedOn();
		void slot_signedOff();
	private:
		Connection *q;
		Word m_snacRequest;
		Word m_flapSequence;
		int m_connectionStatus;
};

} /* end of namespace ICQ */

#endif /* ICQ_CONNECTION_PRIVATE_H_ */
