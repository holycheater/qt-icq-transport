/*
 * Parser.cpp - parse an XMPP "document"
 * Copyright (C) 2003  Justin Karneges
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

/*
  TODO:

  For XMPP::Parser to be "perfect", some things must be solved/changed in the
  Qt library:

  - Fix weird QDomElement::haveAttributeNS() bug (patch submitted to
    Trolltech on Aug 31st, 2003).
  - Fix weird behavior in QXmlSimpleReader of reporting endElement() when
    the '/' character of a self-closing tag is reached, instead of when
    the final '>' is reached.
  - Fix incremental parsing bugs in QXmlSimpleReader.  At the moment, the
    only bug I've found is related to attribute parsing, but there might
    be more (search for '###' in $QTDIR/src/xml/qxml.cpp).

  We have workarounds for all of the above problems in the code below.

  - Deal with the <?xml?> processing instruction as an event type, so that we
    can feed it back to the application properly.  Right now it is completely
    untrackable and is simply tacked into the first event's actualString.  We
    can't easily do this because QXmlSimpleReader eats an extra byte beyond
    the processing instruction before reporting it.

  - Make QXmlInputSource capable of accepting data incrementally, to ensure
    proper text encoding detection and processing over a network.  This is
    technically not a bug, as we have our own subclass below to do it, but
    it would be nice if Qt had this already.
*/

#include <QDomElement>
#include <QTextCodec>
#include <QString>
#include <QSharedData>
#include <QXmlAttributes>

#include "Parser.h"

using namespace XMPP;

//----------------------------------------------------------------------------
// StreamInput
//----------------------------------------------------------------------------
class StreamInput : public QXmlInputSource
{
	public:
		StreamInput();
		~StreamInput();

		void appendData(const QByteArray& data);

		QString encoding() const;

		bool isPaused();

		QChar lastRead();
		QString lastString() const;

		QChar next();

		void pause(bool paused);

		// NOTE: setting 'peek' to true allows the same char to be read again,
		//       however this still advances the internal byte processing.
		QChar readNext(bool peek = false);

		void reset();
		void resetLastData();

		QByteArray unprocessed() const;

	private:
		bool checkForBadChars(const QString &s);
		void processBuf();
		QString processXmlHeader(const QString &h);
		bool tryExtractPart(QString *s);

		bool m_bCheckBad;
		bool m_bMightChangeEncoding;
		bool m_bPaused;

		int m_pos;

		QTextDecoder* m_decoder;

		QByteArray m_input;
		QString m_output;

		QChar m_lastChar;
		QString m_lastString;

		QString m_encoding;

};

StreamInput::StreamInput()
{
	m_decoder = 0;
	reset();
}

StreamInput::~StreamInput()
{
	delete m_decoder;
}

void StreamInput::reset()
{
	delete m_decoder;
	m_decoder = 0;

	m_bCheckBad = true;
	m_bMightChangeEncoding = true;
	m_bPaused = false;

	m_pos = 0;

	m_input.resize(0);
	m_output = "";

	m_lastChar = QChar();
	m_encoding.clear();

	resetLastData();
}

void StreamInput::resetLastData()
{
	m_lastString.clear();
}

QString StreamInput::lastString() const
{
	return m_lastString;
}

void StreamInput::appendData(const QByteArray& data)
{
	int oldsize = m_input.size();
	m_input.resize( oldsize + data.size() );
	qMemCopy( m_input.data() + oldsize, data.data(), data.size() );
	processBuf();
}

QChar StreamInput::lastRead()
{
	return m_lastChar;
}

QChar StreamInput::next()
{
	if (m_bPaused) {
		return EndOfData;
	} else {
		return readNext();
	}
}

QChar StreamInput::readNext(bool peek)
{
	QChar c;
	if (m_bMightChangeEncoding) {
		c = EndOfData;
	} else {
		if ( m_output.isEmpty() ) {
			QString s;
			if ( !tryExtractPart(&s) ) {
				c = EndOfData;
			} else {
				m_output = s;
				c = m_output[0];
			}
		} else {
			c = m_output[0];
		}
		if (!peek) {
			m_output.remove(0, 1);
		}
	}
	if (c == EndOfData) {
		// qDebug() << "readNext() = EndOfData";
	}
	else {
		// qDebug() << "readNext():" << c;
		m_lastChar = c;
	}

	return c;
}

QByteArray StreamInput::unprocessed() const
{
	QByteArray data;
	data.resize(m_input.size() - m_pos);
	qMemCopy( data.data(), m_input.data() + m_pos, data.size() );
	return data;
}

void StreamInput::pause(bool paused)
{
	m_bPaused = paused;
}

bool StreamInput::isPaused()
{
	return m_bPaused;
}

QString StreamInput::encoding() const
{
	return m_encoding;
}

void StreamInput::processBuf()
{
	// qDebug() << "processing buffer" << "size" << in.size() << "pos" << at;
	if ( !m_decoder ) {
		QTextCodec *codec = 0;
		uchar *p = (uchar *)m_input.data() + m_pos;
		int size = m_input.size() - m_pos;

		// do we have enough information to determine the encoding?
		if (size == 0) {
			return;
		}
		bool utf16 = false;
		if (p[0] == 0xfe || p[0] == 0xff) {
			// probably going to be a UTF-16 byte order mark
			if (size < 2) {
				return;
			}
			if ( (p[0] == 0xfe && p[1] == 0xff) || (p[0] == 0xff && p[1] == 0xfe) ) {
				// ok it is UTF-16
				utf16 = true;
			}
		}
		if (utf16) {
			codec = QTextCodec::codecForMib(1000); // UTF-16
		} else {
			codec = QTextCodec::codecForMib(106); // UTF-8
		}

		m_encoding = codec->name();
		m_decoder = codec->makeDecoder();

		// for utf16, put in the byte order mark
		if (utf16) {
			m_output += m_decoder->toUnicode((const char *)p, 2);
			m_pos += 2;
		}
	}

	if (m_bMightChangeEncoding) {
		forever {
			int n = m_output.indexOf('<');
			if (n != -1) {
				// we need a closing bracket
				int n2 = m_output.indexOf('>', n);
				if (n2 != -1) {
					++n2;
					QString h = m_output.mid(n, n2-n);
					QString enc = processXmlHeader(h);
					QTextCodec *codec = 0;
					if ( !enc.isEmpty() ) {
						codec = QTextCodec::codecForName( enc.toLatin1() );
					}

					// changing codecs
					if (codec) {
						m_encoding = codec->name();
						delete m_decoder;
						m_decoder = codec->makeDecoder();
					}
					m_bMightChangeEncoding = false;
					m_output.truncate(0);
					m_pos = 0;
					resetLastData();
					break;
				}
			}
			QString s;
			if ( !tryExtractPart(&s) ) {
				break;
			}
			if ( m_bCheckBad && checkForBadChars(s) ) {
				// go to the parser
				m_bMightChangeEncoding = false;
				m_output.truncate(0);
				m_pos = 0;
				resetLastData();
				break;
			}
			m_output += s;
		}
	}
}

QString StreamInput::processXmlHeader(const QString& header)
{
	if (header.left(5) != "<?xml") {
		return "";
	}

	int endPos = header.indexOf(">");
	int startPos = header.indexOf("encoding");
	if (startPos < endPos && startPos != -1) {
		QString encoding;
		do {
			startPos++;
			if (startPos > endPos) {
				return "";
			}
		} while (header[startPos] != '"' && header[startPos] != '\'');
		startPos++;
		while (header[startPos] != '"' && header[startPos] != '\'') {
			encoding += header[startPos];
			startPos++;
			if (startPos > endPos) {
				return "";
			}
		}
		return encoding;
	} else {
		return "";
	}
}

bool StreamInput::tryExtractPart(QString *s)
{
	int size = m_input.size() - m_pos;
	if (size == 0) {
		return false;
	}
	uchar *p = (uchar *)m_input.data() + m_pos;
	QString nextChars;
	while(1) {
		nextChars = m_decoder->toUnicode((const char *)p, 1);
		++p;
		++m_pos;
		if ( !nextChars.isEmpty() ) {
			break;
		}
		if ( m_pos == m_input.size() ) {
			return false;
		}
	}
	m_lastString += nextChars;
	*s = nextChars;

	// free processed data?
	if (m_pos >= 1024) {
		char *p = m_input.data();
		int size = m_input.size() - m_pos;
		memmove(p, p + m_pos, size);
		m_input.resize(size);
		m_pos = 0;
	}

	return true;
}

bool StreamInput::checkForBadChars(const QString &s)
{
	int len = s.indexOf('<');
	if (len == -1) {
		len = s.length();
	} else {
		m_bCheckBad = false;
	}
	for(int n = 0; n < len; ++n) {
		if ( !s.at(n).isSpace() ) {
			return true;
		}
	}
	return false;
}

//----------------------------------------------------------------------------
// ParserHandler
//----------------------------------------------------------------------------
namespace XMPP
{


class ParserHandler : public QXmlDefaultHandler
{
	public:
		ParserHandler(StreamInput* input, QDomDocument* document);
		~ParserHandler();

		bool needMore() const;

		bool startDocument();

		bool endDocument();

		bool startPrefixMapping(const QString& prefix, const QString& uri);

		bool startElement(const QString& namespaceURI, const QString& localName, const QString& qualifiedName, const QXmlAttributes& attributes);

		bool endElement(const QString& namespaceURI, const QString& localName, const QString& qualifiedName);

		bool characters(const QString& str);

		void checkNeedMore();

		Parser::Event* takeEvent();
	private:
		bool m_bNeedMore;
		int m_depth;

		StreamInput* m_input;
		QDomDocument* m_document;

		QDomElement m_element, m_current;
		QList<Parser::Event*> m_eventList;

		QStringList m_nsnames, m_nsvalues;
};

ParserHandler::ParserHandler(StreamInput* input, QDomDocument* document)
{
	m_bNeedMore = false;

	m_input = input;
	m_document = document;
}

ParserHandler::~ParserHandler()
{
	while ( !m_eventList.isEmpty() ) {
		delete m_eventList.takeFirst();
	}
}

bool ParserHandler::needMore() const
{
	return m_bNeedMore;
}

bool ParserHandler::startDocument()
{
	m_depth = 0;
	return true;
}

bool ParserHandler::endDocument()
{
	return true;
}

bool ParserHandler::startPrefixMapping(const QString &prefix, const QString &uri)
{
	if (m_depth == 0) {
		m_nsnames += prefix;
		m_nsvalues += uri;
	}
	return true;
}

bool ParserHandler::startElement(const QString& namespaceURI, const QString& localName, const QString& qualifiedName, const QXmlAttributes& attributes)
{
	if (m_depth == 0) {
		Parser::Event *e = new Parser::Event;
		QXmlAttributes a;
		for(int n = 0; n < attributes.length(); ++n) {
			QString uri = attributes.uri(n);
			QString ln = attributes.localName(n);
			if (a.index(uri, ln) == -1) {
				a.append( attributes.qName(n), uri, ln, attributes.value(n) );
			}
		}
		e->setDocumentOpen(namespaceURI, localName, qualifiedName, a, m_nsnames, m_nsvalues);
		m_nsnames.clear();
		m_nsvalues.clear();
		e->setActualString( m_input->lastString() );

		m_input->resetLastData();
		m_eventList.append(e);
		m_input->pause(true);
	}
	else {
		QDomElement e = m_document->createElementNS(namespaceURI, qualifiedName);
		for(int n = 0; n < attributes.length(); ++n) {
			QString uri = attributes.uri(n);
			QString ln = attributes.localName(n);
			bool have;
			if ( !uri.isEmpty() ) {
				have = e.hasAttributeNS(uri, ln);
			} else {
				have = e.hasAttribute(ln);
			}
			if (!have) {
				e.setAttributeNS( uri, attributes.qName(n), attributes.value(n) );
			}
		}

		if (m_depth == 1) {
			m_element = e;
			m_current = e;
		}
		else {
			m_current.appendChild(e);
			m_current = e;
		}
	}
	++m_depth;
	return true;
}

bool ParserHandler::endElement(const QString& namespaceURI, const QString& localName, const QString& qName)
{
	--m_depth;
	if (m_depth == 0) {
		Parser::Event *e = new Parser::Event;
		e->setDocumentClose(namespaceURI, localName, qName);
		e->setActualString( m_input->lastString() );
		m_input->resetLastData();
		m_eventList.append(e);
		m_input->pause(true);
	} else {
		// done with a depth 1 element?
		if (m_depth == 1) {
			Parser::Event *e = new Parser::Event;
			e->setElement(m_element);
			e->setActualString( m_input->lastString() );
			m_input->resetLastData();
			m_eventList.append(e);
			m_input->pause(true);

			m_element = QDomElement();
			m_current = QDomElement();
		} else {
			m_current = m_current.parentNode().toElement();
		}
	}

	if (m_input->lastRead() == '/') {
		checkNeedMore();
	}

	return true;
}

bool ParserHandler::characters(const QString& str)
{
	if (m_depth >= 1) {
		QString content = str;
		if ( content.isEmpty() ) {
			return true;
		}

		if ( !m_current.isNull() ) {
			QDomText text = m_document->createTextNode(content);
			m_current.appendChild(text);
		}
	}
	return true;
}

void ParserHandler::checkNeedMore()
{
	/*
	 * Here we will work around QXmlSimpleReader strangeness and self-closing tags.
	 * The problem is that endElement() is called when the '/' is read, not when
	 * the final '>' is read.  This is a potential problem when obtaining unprocessed
	 * bytes from StreamInput after this event, as the '>' character will end up
	 * in the unprocessed chunk.  To work around this, we need to advance StreamInput's
	 * internal byte processing, but not the xml character data.  This way, the '>'
	 * will get processed and will no longer be in the unprocessed return, but
	 * QXmlSimpleReader can still read it.  To do this, we call StreamInput::readNext
	 * with 'peek' mode.
	 */
	QChar c = m_input->readNext(true); // peek
	if (c == QXmlInputSource::EndOfData) {
		m_bNeedMore = true;
	} else {
		// We'll assume the next char is a '>'.  If it isn't, then
		// QXmlSimpleReader will deal with that problem on the next
		// parse.  We don't need to take any action here.
		m_bNeedMore = false;

		// there should have been a pending event
		if ( !m_eventList.isEmpty() ) {
			Parser::Event *e = m_eventList.first();
			e->setActualString(e->actualString() + '>');
			m_input->resetLastData();
		}
	}
}

Parser::Event* ParserHandler::takeEvent()
{
	if ( m_bNeedMore || m_eventList.isEmpty() ) {
		return 0;
	}

	Parser::Event *e = m_eventList.takeFirst();
	m_input->pause(false);
	return e;
}



} /* end of namespace XMPP */


//----------------------------------------------------------------------------
// Event
//----------------------------------------------------------------------------
class Parser::Event::Private : public QSharedData
{
	public:
		Private();
		Private(const Private& other);
		~Private();

		int type;
		QString ns, localName, qualifiedName;
		QXmlAttributes attributes;
		QDomElement element;
		QString str;
		QStringList nsnames, nsvalues;
};

Parser::Event::Private::Private()
	: QSharedData()
{
	type = -1;
}

Parser::Event::Private::Private(const Private& other)
	: QSharedData(other)
{
	type = other.type;

	ns = other.ns;
	localName = other.localName;
	qualifiedName = other.qualifiedName;
	attributes = other.attributes;
	element = other.element;
	str = other.str;
	nsnames = other.nsnames;
	nsvalues = other.nsvalues;
}

Parser::Event::Private::~Private()
{
}

Parser::Event::Event()
{
	d = new Private;
}

Parser::Event::Event(const Event& other)
	: d(other.d)
{
}

Parser::Event & Parser::Event::operator=(const Event& other)
{
	d = other.d;
	return *this;
}

Parser::Event::~Event()
{
}

bool Parser::Event::isNull() const
{
	return (d->type == -1);
}

int Parser::Event::type() const
{
	return d->type;
}

/**
 * Get string representation of event type. Useful for debugging
 *
 * @return			event type string
 */
QString Parser::Event::typeString() const
{
	switch (d->type) {
		case -1:
			return "Invalid";
			break;
		case DocumentOpen:
			return "DocumentOpen";
			break;
		case DocumentClose:
			return "DocumentClose";
			break;
		case Element:
			return "Element";
			break;
		case Error:
			return "Error";
			break;
		default:
			return "Unknown";
			break;
	}
}

QString Parser::Event::nsprefix(const QString &s) const
{
	QStringList::ConstIterator it = d->nsnames.begin();
	QStringList::ConstIterator it2 = d->nsvalues.begin();
	for(; it != d->nsnames.end(); ++it) {
		if ( (*it) == s ) {
			return (*it2);
		}
		++it2;
	}
	return QString::null;
}

QString Parser::Event::namespaceURI() const
{
	return d->ns;
}

QString Parser::Event::localName() const
{
	if ( d->type == Element ) {
		return d->element.localName();
	}
	return d->localName;
}

QString Parser::Event::qualifiedName() const
{
	if ( d->type == Element ) {
		return d->element.nodeName();
	}
	return d->qualifiedName;
}

QXmlAttributes Parser::Event::attributes() const
{
	return d->attributes;
}

QString Parser::Event::actualString() const
{
	return d->str;
}

QDomElement Parser::Event::element() const
{
	return d->element;
}

void Parser::Event::setDocumentOpen(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts, const QStringList &nsnames, const QStringList &nsvalues)
{
	d->type = DocumentOpen;
	d->ns = namespaceURI;
	d->localName = localName;
	d->qualifiedName = qName;
	d->attributes = atts;
	d->nsnames = nsnames;
	d->nsvalues = nsvalues;
}

void Parser::Event::setDocumentClose(const QString &namespaceURI, const QString &localName, const QString &qName)
{
	d->type = DocumentClose;
	d->ns = namespaceURI;
	d->localName = localName;
	d->qualifiedName = qName;
}

void Parser::Event::setElement(const QDomElement &elem)
{
	d->type = Element;
	d->element = elem;
}

void Parser::Event::setError()
{
	d->type = Error;
}

void Parser::Event::setActualString(const QString &str)
{
	d->str = str;
}

//----------------------------------------------------------------------------
// Parser
//----------------------------------------------------------------------------
class Parser::Private
{
	public:
		Private();
		~Private();

		void reset(bool create = true);

		QDomDocument *document;
		StreamInput *input;
		ParserHandler *handler;
		QXmlSimpleReader *reader;
};

Parser::Private::Private()
{
	document = 0;
	input = 0;
	handler = 0;
	reader = 0;
	reset();
}

Parser::Private::~Private()
{
	reset(false);
}

void Parser::Private::reset(bool create)
{
	delete reader;
	delete handler;
	delete input;
	delete document;

	if (create) {
		document = new QDomDocument;
		input = new StreamInput;
		handler = new ParserHandler(input, document);
		reader = new QXmlSimpleReader;
		reader->setContentHandler(handler);

		// initialize the reader
		input->pause(true);
		reader->parse(input, true);
		input->pause(false);
	}
}

Parser::Parser()
{
	d = new Private;
}

Parser::~Parser()
{
	delete d;
}

void Parser::reset()
{
	d->reset();
}

void Parser::appendData(const QByteArray& data)
{
	d->input->appendData(data);

	// if handler was waiting for more, give it a kick
	if ( d->handler->needMore() ) {
		d->handler->checkNeedMore();
	}
}

Parser::Event Parser::readNext()
{
	Event e;
	if ( d->handler->needMore() ) {
		return e;
	}
	Event *ep = d->handler->takeEvent();
	if (!ep) {
		if ( !d->reader->parseContinue() ) {
			e.setError();
			return e;
		}
		ep = d->handler->takeEvent();
		if (!ep) {
			return e;
		}
	}
	e = *ep;
	delete ep;
	return e;
}

QByteArray Parser::unprocessed() const
{
	return d->input->unprocessed();
}

QString Parser::encoding() const
{
	return d->input->encoding();
}

// vim:ts=4:sw=4:noet:nowrap
