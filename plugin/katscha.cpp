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


#include <stdio.h>

#include "katscha.h"


/*-----------------------------------*/

#define MUHKUH_PLUGIN_PUSH_ERROR(L,...) { lua_pushfstring(L,__VA_ARGS__); }
#define MUHKUH_PLUGIN_EXIT_ERROR(L) { lua_error(L); }
#define MUHKUH_PLUGIN_ERROR(L,...) { lua_pushfstring(L,__VA_ARGS__); lua_error(L); }

#if !defined(LUA_VERSION_NUM) || LUA_VERSION_NUM < 501
#       define WRAPPER_LUA_RAWLEN lua_strlen
#elif LUA_VERSION_NUM == 501
#       define WRAPPER_LUA_RAWLEN lua_objlen
#else
#       define WRAPPER_LUA_RAWLEN lua_rawlen
#endif

/*-----------------------------------*/


katscha::katscha(void)
 : m_tCallbackLuaFn({0, 0})
 , m_lCallbackUserData(0)
 , m_ptLibUsbContext(NULL)
 , m_ptDeviceHandle(NULL)
{
	libusb_init(&m_ptLibUsbContext);
	libusb_set_option(m_ptLibUsbContext, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO);
}



katscha::~katscha(void)
{
	if( m_ptDeviceHandle!=NULL )
	{
		close();
	}

	if( m_ptLibUsbContext!=NULL )
	{
		libusb_exit(m_ptLibUsbContext);
	}
}



const char *katscha::m_pcPluginNamePattern = "katscha_%02x_%02x@%s";



void katscha::set_callback(SWIGLUA_REF tLuaFn, long lCallbackUserData)
{
	m_tCallbackLuaFn.L = tLuaFn.L;
	m_tCallbackLuaFn.ref = tLuaFn.ref;
	m_lCallbackUserData = lCallbackUserData;
}



int katscha::identifyDevice(libusb_device *ptDevice)
{
	int iDeviceIsKatscha;
	int iResult;
	struct libusb_device_descriptor sDevDesc;
	libusb_device_handle *ptDevHandle;


	iDeviceIsKatscha = 0;
	if( ptDevice!=NULL )
	{
		/* Try to open the device. */
		iResult = libusb_open(ptDevice, &ptDevHandle);
		if( iResult==LIBUSB_SUCCESS )
		{
			iResult = libusb_get_descriptor(ptDevHandle, LIBUSB_DT_DEVICE, 0, (unsigned char*)&sDevDesc, sizeof(struct libusb_device_descriptor));
			if( iResult==sizeof(struct libusb_device_descriptor) )
			{
				if(
					sDevDesc.idVendor==m_usKatschaVendor &&
					sDevDesc.idProduct==m_usKatschaProduct
				)
				{
					/* Found a katscha device. */
					log("identifyDevice: Found device %04x:%04x", sDevDesc.idVendor, sDevDesc.idProduct);
					iDeviceIsKatscha = 1;
				}
			}
			libusb_close(ptDevHandle);
		}
	}

	return iDeviceIsKatscha;
}



unsigned int katscha::scan(lua_State *ptLuaStateForTableAccess)
{
	unsigned int uiDetectedDevices;
	ssize_t ssizDevList;
	libusb_device **ptDeviceList;
	libusb_device **ptDevCnt, **ptDevEnd;
	libusb_device *ptDev;
	libusb_error tError;
	int iDeviceIsKatscha;
	int iResult;
	const size_t sizMaxName = 32;
	char acName[sizMaxName];
	unsigned int uiBusNr;
	unsigned int uiDevAdr;
	const int iPathMax = 32;
	unsigned char aucPath[iPathMax];
	/* bus number as a digit plus path elements as single digits */
	/* This is the Location ID format used by USBView, but is this sufficient? */
	char acPathString[iPathMax * 2 + 2] = {0};
	int iPathStringPos;
	int iCnt;
        size_t sizTable;


	log("Starting scan.");

	uiDetectedDevices = 0;

	/* detect devices */
	ptDeviceList = NULL;
	ssizDevList = libusb_get_device_list(m_ptLibUsbContext, &ptDeviceList);
	if( ssizDevList<0 )
	{
		/* failed to detect devices */
		tError = (libusb_error)ssizDevList;
		log("failed to detect usb devices: %d:%s", tError, libusb_strerror(tError));
	}
	else
	{
		/* Loop over all devices. */
		ptDevCnt = ptDeviceList;
		ptDevEnd = ptDevCnt + ssizDevList;
		while( ptDevCnt<ptDevEnd )
		{
			ptDev = *ptDevCnt;
			iDeviceIsKatscha = identifyDevice(ptDev);
			if( iDeviceIsKatscha!=0 )
			{
				/* Get the location. */
				iResult = libusb_get_port_numbers(ptDev, aucPath, iPathMax);
				if( iResult<=0 )
				{
					log("Failed to get the port numbers: %d\n", iResult);
				}
				else
				{
					/* Build the path string. */
					uiBusNr = libusb_get_bus_number(ptDev);
					uiDevAdr = libusb_get_device_address(ptDev);
					sprintf(acPathString, "%02x", uiBusNr);
					iPathStringPos = 2;
					for(iCnt=0; iCnt<iResult; ++iCnt)
					{
						sprintf(acPathString+iPathStringPos, "%02x", aucPath[iCnt]);
						iPathStringPos += 2;
					}
					acPathString[iPathStringPos] = 0;

					/* construct the name */
					snprintf(acName, sizMaxName-1, m_pcPluginNamePattern, uiBusNr, uiDevAdr, acPathString);

				        /* Get the size of the table. */
				        sizTable = WRAPPER_LUA_RAWLEN(ptLuaStateForTableAccess, 2);
				        /* Create a new LUA string. */
				        lua_pushstring(ptLuaStateForTableAccess, acName);
				        /* add the pointer object to the table */
				        lua_rawseti(ptLuaStateForTableAccess, 2, sizTable+1);

					log("Found Katscha at %s", acName);

					++uiDetectedDevices;
				}
			}

			++ptDevCnt;
		}

		/* free the device list */
		libusb_free_device_list(ptDeviceList, 1);
	}

	log("Finished scan. Found %d devices.", uiDetectedDevices);
	return uiDetectedDevices;
}



void katscha::open(const char *pcDevice)
{
	ssize_t ssizDevList;
	libusb_device **ptDeviceList;
	libusb_device **ptDevCnt, **ptDevEnd;
	libusb_device *ptDev;
	libusb_error tError;
	int iDeviceIsKatscha;
	int iResult;
	const size_t sizMaxName = 32;
	char acName[sizMaxName];
	unsigned int uiBusNr;
	unsigned int uiDevAdr;
	const int iPathMax = 32;
	unsigned char aucPath[iPathMax];
	/* bus number as a digit plus path elements as single digits */
	/* This is the Location ID format used by USBView, but is this sufficient? */
	char acPathString[iPathMax * 2 + 2] = {0};
	int iPathStringPos;
	int iCnt;
	libusb_device_handle *ptDevHandle;


	log("Open device '%s'.", pcDevice);
	if( strlen(pcDevice)>sizMaxName )
	{
		log("The device name is too long.");
	}
	else
	{
		ptDeviceList = NULL;
		ssizDevList = libusb_get_device_list(m_ptLibUsbContext, &ptDeviceList);
		if( ssizDevList<0 )
		{
			/* failed to detect devices */
			tError = (libusb_error)ssizDevList;
			log("failed to detect usb devices: %d:%s", tError, libusb_strerror(tError));
		}
		else
		{
			/* Loop over all devices. */
			ptDevCnt = ptDeviceList;
			ptDevEnd = ptDevCnt + ssizDevList;
			while( ptDevCnt<ptDevEnd )
			{
				ptDev = *ptDevCnt;
				iDeviceIsKatscha = identifyDevice(ptDev);
				if( iDeviceIsKatscha!=0 )
				{
					/* Get the location. */
					iResult = libusb_get_port_numbers(ptDev, aucPath, iPathMax);
					if( iResult<=0 )
					{
						log("Failed to get the port numbers: %d\n", iResult);
					}
					else
					{
						/* Build the path string. */
						uiBusNr = libusb_get_bus_number(ptDev);
						uiDevAdr = libusb_get_device_address(ptDev);
						sprintf(acPathString, "%02x", uiBusNr);
						iPathStringPos = 2;
						for(iCnt=0; iCnt<iResult; ++iCnt)
						{
							sprintf(acPathString+iPathStringPos, "%02x", aucPath[iCnt]);
							iPathStringPos += 2;
						}
						acPathString[iPathStringPos] = 0;

						/* construct the name */
						snprintf(acName, sizMaxName-1, m_pcPluginNamePattern, uiBusNr, uiDevAdr, acPathString);

						if( strcmp(acName, pcDevice)==0 )
						{
							log("Open Katscha at bus %d, device address %d, USB location '%s'.", uiBusNr, uiDevAdr, acPathString);

							iResult = libusb_open(ptDev, &ptDevHandle);
							if( iResult==LIBUSB_SUCCESS )
							{
								/* Claim interface 1. */
								iResult = libusb_claim_interface(ptDevHandle, 1);
								if( iResult!=LIBUSB_SUCCESS )
								{
									/* Failed to claim the interface. */
									tError = (libusb_error)iResult;
									log("failed to claim interface 1: %d:%s\n", iResult, libusb_strerror(tError));
									libusb_close(ptDevHandle);
								}
								else
								{
									m_ptDeviceHandle = ptDevHandle;
								}
							}

							break;
						}
					}
				}

				++ptDevCnt;
			}

			/* free the device list */
			libusb_free_device_list(ptDeviceList, 1);
		}
	}
}



void katscha::close(void)
{
	if( m_ptDeviceHandle!=NULL )
	{
		libusb_close(m_ptDeviceHandle);
		m_ptDeviceHandle = NULL;
	}
}



void katscha::test(void)
{
	unsigned int uiCnt;
	int iResult;
	unsigned char aucData[64];
	int iProcessed;
	libusb_error tError;
	unsigned int uiTimeoutMs = 1000U;


	if( m_ptDeviceHandle==NULL )
	{
		log("Not open.");
	}
	else
	{
		/* Send a test packet. */
		for(uiCnt=0; uiCnt<64; ++uiCnt)
		{
			aucData[uiCnt] = (unsigned char)uiCnt;
		}

		iResult = libusb_bulk_transfer(m_ptDeviceHandle, m_ucEndpointOut, aucData, 64, &iProcessed, uiTimeoutMs);
		if( iResult!=LIBUSB_SUCCESS )
		{
			tError = (libusb_error)iResult;
			log("failed to claim interface 1: %d:%s\n", iResult, libusb_strerror(tError));
		}
		else
		{
			log("Sent test packet.");
		}
	}
}



void katscha::log(const char *pcFmt, ...)
{
	va_list ptArguments;
	int iMessageLength;
	char acBuffer[1024];


	/* Print the message to the buffer. */
	va_start(ptArguments, pcFmt);
	iMessageLength = vsnprintf(acBuffer, sizeof(acBuffer), pcFmt, ptArguments);
	va_end(ptArguments);

	callback_string(&m_tCallbackLuaFn, acBuffer, iMessageLength, m_lCallbackUserData);
}



void katscha::callback_long(SWIGLUA_REF *ptLuaFn, long lData, long lCallbackUserData)
{
	int iOldTopOfStack;


	/* Check the LUA state and callback tag. */
	if( ptLuaFn->L!=NULL && ptLuaFn->ref!=LUA_NOREF && ptLuaFn->ref!=LUA_REFNIL )
	{
		/* Get the current stack position. */
		iOldTopOfStack = lua_gettop(ptLuaFn->L);
		lua_rawgeti(ptLuaFn->L, LUA_REGISTRYINDEX, ptLuaFn->ref);
		/* Push the arguments on the stack. */
		lua_pushnumber(ptLuaFn->L, lData);
		callback_common(ptLuaFn, lCallbackUserData, iOldTopOfStack);
	}
}


void katscha::callback_string(SWIGLUA_REF *ptLuaFn, const char *pcData, size_t sizData, long lCallbackUserData)
{
	int iOldTopOfStack;


	/* Check the LUA state and callback tag. */
	if( ptLuaFn->L!=NULL && ptLuaFn->ref!=LUA_NOREF && ptLuaFn->ref!=LUA_REFNIL )
	{
		/* Get the current stack position. */
		iOldTopOfStack = lua_gettop(ptLuaFn->L);
		lua_rawgeti(ptLuaFn->L, LUA_REGISTRYINDEX, ptLuaFn->ref);
		/* Push the arguments on the stack. */
		lua_pushlstring(ptLuaFn->L, pcData, sizData);
		callback_common(ptLuaFn, lCallbackUserData, iOldTopOfStack);
	}
}



void katscha::callback_common(SWIGLUA_REF *ptLuaFn, long lCallbackUserData, int iOldTopOfStack)
{
	int iResult;
	const char *pcErrMsg;
	const char *pcErrDetails;


	/* Check the LUA state and callback tag. */
	if( ptLuaFn->L!=NULL && ptLuaFn->ref!=LUA_NOREF && ptLuaFn->ref!=LUA_REFNIL )
	{
		lua_pushnumber(ptLuaFn->L, lCallbackUserData);
		/* Call the function. */
		iResult = lua_pcall(ptLuaFn->L, 2, 0, 0);
		if( iResult!=0 )
		{
			switch( iResult )
			{
			case LUA_ERRRUN:
				pcErrMsg = "runtime error";
				break;
			case LUA_ERRMEM:
				pcErrMsg = "memory allocation error";
				break;
			default:
				pcErrMsg = "unknown errorcode";
				break;
			}
			pcErrDetails = lua_tostring(ptLuaFn->L, -1);
			MUHKUH_PLUGIN_ERROR(ptLuaFn->L, "callback function failed: %s (%d): %s", pcErrMsg, iResult, pcErrDetails);
		}
		/* Restore the old stack top. */
		lua_settop(ptLuaFn->L, iOldTopOfStack);
	}
}
