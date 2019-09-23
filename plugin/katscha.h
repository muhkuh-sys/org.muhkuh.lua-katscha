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

#include "../src/interface.h"

#endif  /* !defined(SWIG) */


/*-----------------------------------*/

typedef int RESULT_INT_TRUE_OR_NIL_WITH_ERR;
typedef int RESULT_INT_NOTHING_OR_NIL_WITH_ERR;

class MUHKUH_EXPORT katscha
{
public:
	katscha(void);
	~katscha(void);

/* *** LUA interface start *** */
	/* Call a routine on the netX. */
	void set_callback(SWIGLUA_REF tLuaFn, long lCallbackUserData);

	unsigned int scan(lua_State *ptLuaStateForTableAccess);

	RESULT_INT_TRUE_OR_NIL_WITH_ERR open(const char *pcDevice);
	RESULT_INT_TRUE_OR_NIL_WITH_ERR close(void);

	RESULT_INT_TRUE_OR_NIL_WITH_ERR reset(void);
	RESULT_INT_NOTHING_OR_NIL_WITH_ERR get_status(lua_State *MUHKUH_SWIG_OUTPUT_CUSTOM_OBJECT);
	RESULT_INT_TRUE_OR_NIL_WITH_ERR set_mode(const char *pcMode);
	RESULT_INT_NOTHING_OR_NIL_WITH_ERR get_mode(const char **ppcBUFFER_OUT, size_t *psizBUFFER_OUT);
	RESULT_INT_TRUE_OR_NIL_WITH_ERR source_set_voltage(unsigned long ulPwmValue);
	RESULT_INT_TRUE_OR_NIL_WITH_ERR source_set_max_current(unsigned long ulMaxCurrent);
	RESULT_INT_NOTHING_OR_NIL_WITH_ERR source_get_voltage(unsigned long *pulARGUMENT_OUT);
	RESULT_INT_NOTHING_OR_NIL_WITH_ERR source_get_current(unsigned long *pulARGUMENT_OUT);
	RESULT_INT_TRUE_OR_NIL_WITH_ERR sink_set_current(unsigned long ulCurrent);
/* *** LUA interface end *** */

#if !defined(SWIG)
	const char *get_error_message(void);

	typedef struct MODE2NAME_STRUCT
	{
		KATSCHA_MODE_T tMode;
		const char *pcName;
	} MODE2NAME_T;
	static const MODE2NAME_T atMode2Name[3];

protected:

private:
	int identifyDevice(libusb_device *ptDevice);
	const MODE2NAME_T *find_mode_string(const char *pcMode);
	const MODE2NAME_T *find_mode_value(KATSCHA_MODE_T tMode);
	int exchange(KATSCHA_PACKET_T *ptPacketSend, size_t sizPacketSend, KATSCHA_PACKET_T *ptPacketReceive, size_t *psizPacketReceive);

	void set_error_message(const char *pcFmt, ...);
	const char *get_katscha_error_message(KATSCHA_STATUS_T tStatus);

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

	char *m_pcErrorString;
#endif  /* !defined(SWIG) */
};

/*-----------------------------------*/

#endif  /* __KATSCHA_H__ */
