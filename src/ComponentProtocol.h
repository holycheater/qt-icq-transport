/*
 * ComponentProtocol.h
 *
 *  Created on: Aug 26, 2008
 *      Author: holycheater
 */

#ifndef COMPONENTPROTOCOL_H_
#define COMPONENTPROTOCOL_H_

#include "protocol.h"

#define NS_COMPONENT "jabber:component:accept"

namespace XMPP {


class ComponentProtocol : public BasicProtocol
{
	public:
		ComponentProtocol();
		~ComponentProtocol();

		void reset();

		void setJid(const Jid& jid);
	protected:
		QDomElement docElement();
		void handleDocOpen(const Parser::Event& event);
		bool handleError();
		bool handleCloseFinished();
		bool stepAdvancesParser() const;
		bool stepRequiresElement() const;
		bool doStep(const QDomElement& element);
		void itemWritten(int id, int size);

		// 'debug'
		void stringSend(const QString& string);
		void stringRecv(const QString& string);
		void elementSend(const QDomElement& element);
		void elementRecv(const QDomElement& element);
	private:
		QString defaultNamespace();
		QStringList extraNamespaces();
		void handleStreamOpen(const Parser::Event& event);
		bool doStep2(const QDomElement& element);
};


} // end namespace XMPP

#endif /* COMPONENTPROTOCOL_H_ */
