/**
 * icq_snacbuffer.h - ICQ snac packet.
 * Copyright (C) 2008  Alexander Saltykov
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 **/

#ifndef ICQSNACBUFFER_H_
#define ICQSNACBUFFER_H_

#include "icq_flapbuffer.h"

namespace ICQ
{


class SnacBuffer: public ICQ::FlapBuffer
{
	public:
		SnacBuffer(Word family, Word subtype);
		SnacBuffer(Word family, Word subtype, const QByteArray& data);
		SnacBuffer(const FlapBuffer& flap);

		/* get snac data */
		virtual QByteArray data() const;

		/* packet data size (no header, data only) */
		Word dataSize() const;

		/* get header params */
		Word family() const;
		Word subtype() const;
		Word flags() const;
		DWord requestId() const;

		/* set header params */
		void setFamily(Word family);
		void setSubtype(Word subtype);
		void setFlags(Word flags);
		void setRequestId(DWord requestId);

		/* packet data size with header */
		Word size() const;

		SnacBuffer& operator=(const Buffer& other);
		SnacBuffer& operator=(const FlapBuffer& other);
		SnacBuffer& operator=(const SnacBuffer& other);
		SnacBuffer& operator=(const QByteArray& other);
	private:
		Word m_family;
		Word m_subtype;
		Word m_flags;
		DWord m_requestId;
};

}

#endif /* ICQSNACBUFFER_H_ */
