/***************************************************************************
 *   Copyright (C) 2019 by Christoph Thelen                                *
 *   doc_bacardi@users.sourceforge.net                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <stdarg.h>
#include <stdint.h>

#include "muhkuh_static_assert.h"


#ifndef __KATSCHA_H__
#define __KATSCHA_H__


/* Do not use these structures in Swig. */
#if !defined(SWIG)

#include <libusb.h>
#if (!defined(LIBUSB_API_VERSION)) || (defined(LIBUSB_API_VERSION) && LIBUSB_API_VERSION<0x01000102)
#       error "This plugin needs at least libusb 1.0.16."
#endif

#endif  /* !defined(SWIG) */

/*-----------------------------------*/

#if defined(_WINDOWS)
#       define MUHKUH_EXPORT __declspec(dllexport)
#else
#       define MUHKUH_EXPORT
#endif

/*-----------------------------------*/

#ifndef SWIGRUNTIME
#include <swigluarun.h>

/* swigluarun does not include the lua specific defines. Add them here. */
typedef struct
{
        lua_State* L; /* the state */
        int ref;      /* a ref in the lua global index */
} SWIGLUA_REF;
#endif

/*-----------------------------------*/

/* Do not use these structures in Swig. */
#if !defined(SWIG)

/* Endianness macros translating 16 and 32bit from the host to the netX byte
 * order. Depend on GLIBC for now.
 * NOTE: This can be done with CMAKE too: https://cmake.org/cmake/help/v3.5/module/TestBigEndian.html
 */
#if defined(__GLIBC__)
/* GLIBC provides "htole*" and "le*toh" macros. */
#       include <endian.h>
#       define HTONETX16(a) (htole16(a))
#       define HTONETX32(a) (htole32(a))
#       define NETXTOH16(a) (htole16(a))
#       define NETXTOH32(a) (htole32(a))
#elif defined(_WIN32)
#	include <winsock2.h>
#       include <sys/param.h>
#       if BYTE_ORDER == LITTLE_ENDIAN
#               define HTONETX16(a) (a)
#               define HTONETX32(a) (a)
#               define NETXTOH16(a) (a)
#               define NETXTOH32(a) (a)
#       elif BYTE_ORDER == BIG_ENDIAN
#               define HTONETX16(a) __builtin_bswap16(a)
#               define HTONETX32(a) __builtin_bswap32(a)
#               define NETXTOH16(a) __builtin_bswap16(a)
#               define NETXTOH32(a) __builtin_bswap32(a)
#       else
#               error "Unknown endianness."
#       endif
#else
#       error "Unknown system. Add a way to detect the endianness here or use CMake."
#endif


/* NOTE: Use "pragma pack" instead of "attribute packed" as the latter does not work on MinGW.
 *       See here for details: https://sourceforge.net/p/mingw-w64/bugs/588/
 */
#pragma pack(push, 1)

/* This is a packet header up to the packet type. */
struct MIV3_PACKET_HEADER_STRUCT
{
	uint8_t  ucStreamStart;
	uint16_t usDataSize;
	uint8_t  ucSequenceNumber;
	uint8_t  ucPacketType;
};
MUHKUH_STATIC_ASSERT( sizeof(struct MIV3_PACKET_HEADER_STRUCT)==5, "Packing of MIV3_PACKET_HEADER_STRUCT does not work.");

typedef union MIV3_PACKET_HEADER_UNION
{
	struct MIV3_PACKET_HEADER_STRUCT s;
	uint8_t auc[5];
} MIV3_PACKET_HEADER_T;
MUHKUH_STATIC_ASSERT( sizeof(MIV3_PACKET_HEADER_T)==5, "Packing of MIV3_PACKET_HEADER_T does not work.");



/* This is a complete sync packet. */
struct MIV3_PACKET_SYNC_STRUCT
{
	MIV3_PACKET_HEADER_T tHeader;
	uint8_t  aucMagic[4];
	uint16_t  usVersionMinor;
	uint16_t  usVersionMajor;
	uint8_t   ucChipType;
	uint16_t  usMaximumPacketSize;
	uint8_t   ucCrcHi;
	uint8_t   ucCrcLo;
};
MUHKUH_STATIC_ASSERT( sizeof(struct MIV3_PACKET_SYNC_STRUCT)==18, "Packing of MIV3_PACKET_SYNC_STRUCT does not work.");

typedef union MIV3_PACKET_SYNC_UNION
{
	struct MIV3_PACKET_SYNC_STRUCT s;
	uint8_t auc[18];
} MIV3_PACKET_SYNC_T;
MUHKUH_STATIC_ASSERT( sizeof(MIV3_PACKET_SYNC_T)==18, "Packing of MIV3_PACKET_SYNC_T does not work.");



/* This is a complete acknowledge packet. */
struct MIV3_PACKET_ACK_STRUCT
{
	MIV3_PACKET_HEADER_T tHeader;
	uint8_t  ucCrcHi;
	uint8_t  ucCrcLo;
};
MUHKUH_STATIC_ASSERT( sizeof(struct MIV3_PACKET_ACK_STRUCT)==7, "Packing of MIV3_PACKET_ACK_STRUCT does not work.");

typedef union MIV3_PACKET_ACK_UNION
{
	struct MIV3_PACKET_ACK_STRUCT s;
	uint8_t auc[7];
} MIV3_PACKET_ACK_T;
MUHKUH_STATIC_ASSERT( sizeof(MIV3_PACKET_ACK_T)==7, "Packing of MIV3_PACKET_ACK_T does not work.");



/* This is a complete status packet. */
struct MIV3_PACKET_STATUS_STRUCT
{
	MIV3_PACKET_HEADER_T tHeader;
	uint8_t  ucStatus;
	uint8_t  ucCrcHi;
	uint8_t  ucCrcLo;
};
MUHKUH_STATIC_ASSERT( sizeof(struct MIV3_PACKET_STATUS_STRUCT)==8, "Packing of MIV3_PACKET_STATUS_STRUCT does not work.");

typedef union MIV3_PACKET_STATUS_UNION
{
	struct MIV3_PACKET_STATUS_STRUCT s;
	uint8_t auc[8];
} MIV3_PACKET_STATUS_T;
MUHKUH_STATIC_ASSERT( sizeof(MIV3_PACKET_STATUS_T)==8, "Packing of MIV3_PACKET_STATUS_T does not work.");



/* This is a complete status packet. */
struct MIV3_PACKET_CANCEL_CALL_STRUCT
{
	MIV3_PACKET_HEADER_T tHeader;
	uint8_t  ucData;
	uint8_t  ucCrcHi;
	uint8_t  ucCrcLo;
};
MUHKUH_STATIC_ASSERT( sizeof(struct MIV3_PACKET_CANCEL_CALL_STRUCT)==8, "Packing of MIV3_PACKET_CANCEL_CALL_STRUCT does not work.");

typedef union MIV3_PACKET_CANCEL_CALL_UNION
{
	struct MIV3_PACKET_CANCEL_CALL_STRUCT s;
	uint8_t auc[8];
} MIV3_PACKET_CANCEL_CALL_T;
MUHKUH_STATIC_ASSERT( sizeof(MIV3_PACKET_CANCEL_CALL_T)==8, "Packing of MIV3_PACKET_CANCEL_CALL_T does not work.");



/* This is a complete read packet. */
struct MIV3_PACKET_COMMAND_READ_DATA_STRUCT
{
	MIV3_PACKET_HEADER_T tHeader;
	uint16_t usDataSize;
	uint32_t ulAddress;
	uint8_t  ucCrcHi;
	uint8_t  ucCrcLo;
};
MUHKUH_STATIC_ASSERT( sizeof(struct MIV3_PACKET_COMMAND_READ_DATA_STRUCT)==13, "Packing of MIV3_PACKET_COMMAND_READ_DATA_STRUCT does not work.");

typedef union MIV3_PACKET_COMMAND_READ_DATA_UNION
{
	struct MIV3_PACKET_COMMAND_READ_DATA_STRUCT s;
	uint8_t auc[13];
} MIV3_PACKET_COMMAND_READ_DATA_T;
MUHKUH_STATIC_ASSERT( sizeof(MIV3_PACKET_COMMAND_READ_DATA_T)==13, "Packing of MIV3_PACKET_COMMAND_READ_DATA_T does not work.");



/* This is the start of a write packet. */
struct MIV3_PACKET_COMMAND_WRITE_DATA_HEADER_STRUCT
{
	MIV3_PACKET_HEADER_T tHeader;
	uint16_t usDataSize;
	uint32_t ulAddress;
};
MUHKUH_STATIC_ASSERT( sizeof(struct MIV3_PACKET_COMMAND_WRITE_DATA_HEADER_STRUCT)==11, "Packing of MIV3_PACKET_COMMAND_WRITE_DATA_HEADER_STRUCT does not work.");

typedef union MIV3_PACKET_COMMAND_WRITE_DATA_HEADER_UNION
{
	struct MIV3_PACKET_COMMAND_WRITE_DATA_HEADER_STRUCT s;
	uint8_t auc[11];
} MIV3_PACKET_COMMAND_WRITE_DATA_HEADER_T;
MUHKUH_STATIC_ASSERT( sizeof(MIV3_PACKET_COMMAND_WRITE_DATA_HEADER_T)==11, "Packing of MIV3_PACKET_COMMAND_WRITE_DATA_HEADER_T does not work.");



/* This is a complete call package. */
struct MIV3_PACKET_COMMAND_CALL_STRUCT
{
	MIV3_PACKET_HEADER_T tHeader;
	uint32_t ulAddress;
	uint32_t ulR0;
	uint8_t  ucCrcHi;
	uint8_t  ucCrcLo;
};
MUHKUH_STATIC_ASSERT( sizeof(struct MIV3_PACKET_COMMAND_CALL_STRUCT)==15, "Packing of MIV3_PACKET_COMMAND_CALL_STRUCT does not work.");

typedef union MIV3_PACKET_COMMAND_CALL_UNION
{
	struct MIV3_PACKET_COMMAND_CALL_STRUCT s;
	uint8_t auc[15];
} MIV3_PACKET_COMMAND_CALL_T;
MUHKUH_STATIC_ASSERT( sizeof(MIV3_PACKET_COMMAND_CALL_T)==15, "Packing of MIV3_PACKET_COMMAND_CALL_T does not work.");

#pragma pack(pop)


#endif  /* !defined(SWIG) */


/*-----------------------------------*/

class MUHKUH_EXPORT katscha
{
public:
	katscha(void);
	~katscha(void);

/* *** LUA interface start *** */
	/* Call a routine on the netX. */
	void set_callback(SWIGLUA_REF tLuaFn, long lCallbackUserData);

	unsigned int scan(lua_State *ptLuaStateForTableAccess);

	void open(const char *pcDevice);
	void close(void);

	void test(void);
/* *** LUA interface end *** */

#if !defined(SWIG)
protected:

private:
	int identifyDevice(libusb_device *ptDevice);

	void log(const char *pcFmt, ...);
	void callback_long(SWIGLUA_REF *ptLuaFn, long lData, long lCallbackUserData);
	void callback_string(SWIGLUA_REF *ptLuaFn, const char *pcData, size_t sizData, long lCallbackUserData);
	void callback_common(SWIGLUA_REF *ptLuaFn, long lCallbackUserData, int iOldTopOfStack);

	SWIGLUA_REF m_tCallbackLuaFn;
	long m_lCallbackUserData;

	libusb_context *m_ptLibUsbContext;
	libusb_device_handle *m_ptDeviceHandle;

	static const char *m_pcPluginNamePattern;
	const unsigned short m_usKatschaVendor = 0x1939U;
	const unsigned short m_usKatschaProduct = 0x002fU;
	const unsigned char m_ucEndpointIn = 0x85U;
	const unsigned char m_ucEndpointOut = 0x04U;

#endif  /* !defined(SWIG) */
};

/*-----------------------------------*/

#endif  /* __KATSCHA_H__ */
