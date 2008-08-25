#include "ComponentStream.h"
#include "ComponentProtocol.h"

#include "xmpp.h"
#include "bytestream.h"
#include "securestream.h"

namespace XMPP {


class ComponentStream::Private
{
	public:
		AdvancedConnector *connector;
		ByteStream *bs;
		ComponentProtocol protocol;

		Jid jid;
		QString secret;
};

ComponentStream::ComponentStream(AdvancedConnector *connector, QObject *parent)
	: Stream(parent)
{
	d = new Private;

	d->connector = connector;
	QObject::connect( d->connector, SIGNAL( connected() ), SLOT( cr_connected() ) );
	QObject::connect( d->connector, SIGNAL( error() ), SLOT( cr_error() ) );
};

ComponentStream::~ComponentStream()
{
	delete d;
}

void ComponentStream::connectToServer(const Jid& jid, quint16 port, const QString& secret)
{
	// TODO : use the parameters it receives.
	d->jid = jid;
	d->secret = secret;

	d->protocol.setJid(d->jid);
	d->connector->setOptHostPort(jid.domain(), port);
	d->connector->connectToServer( jid.domain() );
}

QDomDocument& ComponentStream::doc() const
{

}

QString ComponentStream::baseNS() const
{
	return NS_COMPONENT;
}

bool ComponentStream::old() const
{
	return false;
}

void ComponentStream::close()
{
	// TODO
}

bool ComponentStream::stanzaAvailable() const
{
	// TODO
}

Stanza ComponentStream::read()
{
	// TODO
}

void ComponentStream::write(const Stanza& stanza)
{
	// TODO
}

int ComponentStream::errorCondition() const
{
	// TODO
}

QString ComponentStream::errorText() const
{
	// TODO
}

QDomElement ComponentStream::errorAppSpec() const
{
	// TODO
}

void ComponentStream::processNext()
{
	// TODO
}

void ComponentStream::bs_bytesWritten(int size)
{
	qDebug() << "[CS]" << "Bytestream bytesWritten:" << size;
}

void ComponentStream::bs_closed()
{
	qDebug() << "[CS]" << "Bytestream closed";
}

void ComponentStream::bs_error(int errno)
{
	qDebug() << "[CS]" << "Bytestream error. Errno:" << errno;
}

void ComponentStream::bs_readyRead()
{
	QByteArray data = d->bs->read();
	qDebug() << "[CS]" << "-recv-" << data;

	d->protocol.addIncomingData(data);
}

void ComponentStream::cr_connected()
{
	qDebug() << "[CS]" << "Connector connected";

	d->bs = d->connector->stream();

	QByteArray spare = d->bs->read();
	qDebug() << "[CS]" << "'spare' size" << spare.size();

	QObject::connect( d->bs, SIGNAL( connectionClosed() ), SLOT( bs_closed() ) );
	QObject::connect( d->bs, SIGNAL( bytesWritten(int) ), SLOT( bs_bytesWritten(int) ) );
	QObject::connect( d->bs, SIGNAL( error(int) ), SLOT( bs_error(int) ) );
	QObject::connect( d->bs, SIGNAL( readyRead() ), SLOT( bs_readyRead() ) );

	processNext();
}

void ComponentStream::cr_error()
{
	qDebug() << "[CS]" << "Connector error";
}


} // end of namespace XMPP
