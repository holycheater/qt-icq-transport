/*
 * icqBuffer.h - ICQ data buffer (packet).
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

#ifndef ICQBUFFER_H_
#define ICQBUFFER_H_

#include "icqTypes.h"

#include <QBuffer>
#include <QByteArray>
#include <QString>

namespace ICQ {


class Buffer
{
	public:
		Buffer();
		Buffer(const QByteArray& data);
		Buffer(const Buffer& buffer);
		virtual ~Buffer();

		/* add byte */
		Buffer& addByte(Byte data);

		/* add word */
		Buffer& addWord(Word data);

		/* add double-word */
		Buffer& addDWord(DWord data);

		/* add little-endian word */
		Buffer& addLEWord(Word data);

		/* add little-endian double-word */
		Buffer& addLEDWord(DWord data);

		/* add data to the buffer */
		Buffer& addData(const Buffer& buffer);
		Buffer& addData(const QByteArray& data);
		Buffer& addData(const QString& data);

		bool atEnd() const;

		/* get number of bytes available to read (from current position to the end) */
		Word bytesAvailable() const;

		void close();

		/* get data from the buffer */
		virtual QByteArray data() const;

		/* get byte from the buffer */
		Byte getByte();

		/* get block of specified size. same as read */
		QByteArray getBlock(Word blockSize);

		/* get word from the buffer */
		Word getWord();

		/* get double word from the buffer */
		DWord getDWord();

		/* get little-endian word from the buffer */
		Word getLEWord();

		/* get little-endian double-word from the buffer */
		DWord getLEDWord();

		void open();

		Word pos() const;

		/* read maxSize bytes */
		QByteArray read(Word maxSize);

		/* read everything from current pos to the end */
		QByteArray readAll();

		/* move internal buffer pointer to position pos */
		bool seek(Word pos);
		void seekEnd();
		bool seekForward(Word count);
		bool seekBackward(Word count);

		void setData(const QByteArray& data);

		/* get buffer size */
		virtual Word size() const;

		operator QByteArray() const;
		Buffer& operator=(const Buffer& other);
		Buffer& operator=(const QByteArray& data);
	protected:
		QBuffer m_Buffer;
};


}

#endif /* ICQBUFFER_H_ */
