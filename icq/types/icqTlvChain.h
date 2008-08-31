/*
 * icqTlvChain.h - ICQ TLV (type-length-value) Chain.
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

#ifndef ICQTLVCHAIN_H_
#define ICQTLVCHAIN_H_

#include "icqBuffer.h"
#include "icqTlv.h"

#include <QByteArray>
#include <QHash>

namespace ICQ
{


class TlvChain
{
	public:
		TlvChain();
		TlvChain(const Buffer& data);
		TlvChain(const QByteArray& data);

		/* add a tlv to the chain. if it already exists, it will be overwritten */
		TlvChain& addTlv(const Tlv& tlv);
		TlvChain& addTlv(Word type, const QByteArray& data);
		Tlv& addTlv(Word type);

		QByteArray data() const;

		/* get Tlv from chain */
		Tlv getTlv(Word type) const;

		/* get tlv data from chain */
		QByteArray getTlvData(Word type) const;

		/* check if chain contains specified tlv type */
		bool hasTlv(Word type) const;

		const QHash<Word, Tlv>& list() const;

		/* remove specified tlv from the chain */
		void removeTlv(Word type);

		TlvChain& operator=(const Buffer& buffer);
		TlvChain& operator=(const QByteArray& data);
		TlvChain& operator<<(const Tlv& tlv);
		TlvChain& operator<<(const QByteArray& data);
		TlvChain& operator<<(const TlvChain& other);
	private:
		QHash<Word, Tlv> m_tlvList;
};

}

#endif /* ICQTLVCHAIN_H_ */
