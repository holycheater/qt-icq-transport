/*
 * ComponentProtocol.cpp
 *
 *  Created on: Aug 26, 2008
 *      Author: holycheater
 */

#include "ComponentProtocol.h"

namespace XMPP {


ComponentProtocol::ComponentProtocol()
{
}

ComponentProtocol::~ComponentProtocol()
{
}

void ComponentProtocol::reset()
{

}

void ComponentProtocol::setJid(const Jid& jid)
{

}

QDomElement ComponentProtocol::docElement()
{

}

void ComponentProtocol::handleDocOpen(const Parser::Event& event)
{

}

bool ComponentProtocol::handleError()
{

}

bool ComponentProtocol::handleCloseFinished()
{

}

bool ComponentProtocol::stepAdvancesParser() const
{

}

bool ComponentProtocol::stepRequiresElement() const
{

}

bool ComponentProtocol::doStep(const QDomElement& element)
{

}

void ComponentProtocol::itemWritten(int id, int size)
{

}

void ComponentProtocol::stringSend(const QString& string)
{

}

void ComponentProtocol::stringRecv(const QString& string)
{

}

void ComponentProtocol::elementSend(const QDomElement& element)
{

}

void ComponentProtocol::elementRecv(const QDomElement& element)
{

}

QString ComponentProtocol::defaultNamespace()
{
	return NS_COMPONENT;
}

QStringList ComponentProtocol::extraNamespaces()
{

}

void ComponentProtocol::handleStreamOpen(const Parser::Event& event)
{

}

bool ComponentProtocol::doStep2(const QDomElement& element)
{

}


} // end namespace XMPP
