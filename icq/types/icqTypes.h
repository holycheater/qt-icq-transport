/*
 * icqTypes.h - ICQ protocol structures and constants
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

#ifndef ICQ_H_
#define ICQ_H_

#include "icqGuid.h"

#include <QByteArray>
#include <QString>

namespace ICQ {

    typedef quint8 Byte;
    typedef quint16 Word;
    typedef quint32 DWord;

    const char AIM_MD5_STRING[]="AOL Instant Messenger (SM)";

    const char DEFAULT_SERVER[] = "login.icq.com";
    const quint16 DEFAULT_PORT = 5190;

    enum Capability { // cc - client capability
        ccAvatar = 0,
        ccICQDirectConnect,
        ccICQServerRelay,
        ccAIMFileTransfer,
        ccAIMInteroperate,
        ccUTF8Messages,
        ccTypingNotifications,
        ccRTFMessages,
        ccTZers,
        ccRnQProtectMsg,

        ccKopete,
        ccLicq,
        ccAndrq,
        ccRnQ,
        ccQIP,
        ccQIPPDA,
        ccQIPInfium
    };

    const Guid Capabilities[] = {
        Guid::fromString("09460000-4C7F-11D1-8222-444553540000"), // 00 Avatar support
        Guid::fromString("09461344-4C7F-11D1-8222-444553540000"), // 01 ICQ Direct Communication
        Guid::fromString("09461349-4C7F-11D1-8222-444553540000"), // 02 ICQ Server Relay
        Guid::fromString("0946134C-4C7F-11D1-8222-444553540000"), // 03 AIM File transfer
        Guid::fromString("0946134D-4C7F-11D1-8222-444553540000"), // 04 AIM Interoperate (AIM-ICQ user support)
        Guid::fromString("0946134E-4C7F-11D1-8222-444553540000"), // 05 UTF-8 messages. The only good UUID.
        Guid::fromString("563FC809-0B6F-41BD-9F79-422609DFA2F3"), // 06 Typing notifications.
        Guid::fromString("97B12751-243C-4334-AD22-D6ABF73F1492"), // 07 RTF messages. Umm?
        Guid::fromString("B2EC8F16-7C6F-451B-BD79-DC58497888B9"), // 08 tZerz. SAY WHAT?
        Guid::fromString("D6687F4F-3DC3-4BDB-8A8C-4C1A572763CD"), // 09 RnQ protect msg (sounds like encryption support)

        // client list
        Guid::fromRawData("Kopete ICQ      "), // Kopete ICQ
        Guid::fromString("4C696371-2063-6C69-656E-742000000000"), //LICQ
        Guid::fromString("26525169-6E73-6964-6500-000000000000"), // &RQ
        Guid::fromString("52265169-6E73-6964-6500-000000000000"), // RnQ
        Guid::fromString("563FC809-0B6F-4151-4950-203230303561"), // QIP
        Guid::fromString("51ADD190-7204-473D-A1A1-49F4A397A41F"), // QIP PDA
        Guid::fromString("7C737502-C3BE-4F3E-A69F-015313431E1A")  // QIP Infium
    };

    /* user flags */
    const Word flagWebaware     = 0x0001; // Web-aware
    const Word flagShowIP       = 0x0002; // Show IP address
    const Word flagBirthday     = 0x0008; // Show birthday flag
    const Word flagWebfront     = 0x0020; // Active webfront flag (?)
    const Word flagDCDisabled   = 0x0100; // Direct connection not supported
    const Word flagDCAuth       = 0x1000; // Direct connection upon authorization
    const Word flagDCContacts   = 0x2000; // Direct connection only with contact-list

    /* extra status list */
    enum XStatus { xsNone, xsAngry, xsDuck, xsTired, xsParty, xsBeer, xsThinking, xsEating,
        xsTV, xsFriends, xsCoffee, xsMusic, xsBusiness, xsCamera, xsFunny, xsPhone,
        xsGames, xsCollege, xsShopping, xsSick, xsSleep, xsSurfing, xsInternet, xsEngineering,
        xsLove, xsSmoke, xsSearch, xsDiary };

    enum VisibiltyStatus { visAll, visNormal, visContact, visPrivacy, visInvisible };

    const quint8 FLAP_HEADER_SIZE = 6;
    const quint8 SNAC_HEADER_SIZE = 10;
    const quint8 TLV_HEADER_SIZE = 4;

}  // namespace ICQ

// vim:ts=4:sw=4:et:nowrap
#endif /*ICQ_H_*/
