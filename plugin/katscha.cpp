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
 , m_pcErrorString(NULL)
{
	libusb_init(&m_ptLibUsbContext);
	libusb_set_option(m_ptLibUsbContext, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO);
}



katscha::~katscha(void)
{
	if( m_pcErrorString!=NULL )
	{
		free(m_pcErrorString);
		m_pcErrorString = NULL;
	}

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



const katscha::MODE2NAME_T katscha::atMode2Name[3] =
{
	{ KATSCHA_MODE_Idle,   "idle" },
	{ KATSCHA_MODE_Source, "source" },
	{ KATSCHA_MODE_Sink,   "sink"}
};



const katscha::MODE2NAME_T *katscha::find_mode_string(const char *pcMode)
{
	const katscha::MODE2NAME_T *ptCnt;
	const katscha::MODE2NAME_T *ptEnd;
	const katscha::MODE2NAME_T *ptHit;
	int iCmp;


	ptCnt = atMode2Name;
	ptEnd = atMode2Name + (sizeof(atMode2Name)/sizeof(atMode2Name[0]));
	ptHit = NULL;
	while( ptCnt<ptEnd )
	{
		iCmp = strcmp(ptCnt->pcName, pcMode);
		if( iCmp==0 )
		{
			ptHit = ptCnt;
			break;
		}
		else
		{
			++ptCnt;
		}
	}

	return ptHit;
}



const katscha::MODE2NAME_T *katscha::find_mode_value(KATSCHA_MODE_T tMode)
{
	const katscha::MODE2NAME_T *ptCnt;
	const katscha::MODE2NAME_T *ptEnd;
	const katscha::MODE2NAME_T *ptHit;


	ptCnt = atMode2Name;
	ptEnd = atMode2Name + (sizeof(atMode2Name)/sizeof(atMode2Name[0]));
	ptHit = NULL;
	while( ptCnt<ptEnd )
	{
		if( ptCnt->tMode==tMode )
		{
			ptHit = ptCnt;
			break;
		}
		else
		{
			++ptCnt;
		}
	}

	return ptHit;
}



int katscha::exchange(KATSCHA_PACKET_T *ptPacketSend, size_t sizPacketSend, KATSCHA_PACKET_T *ptPacketReceive, size_t *psizPacketReceive)
{
	int iResult;
	int iProcessed;
	libusb_error tError;
	unsigned int uiTimeoutMs = 1000U;


	/* Send the command packet. */
	iResult = libusb_bulk_transfer(m_ptDeviceHandle, m_ucEndpointOut, ptPacketSend->auc, sizPacketSend, &iProcessed, uiTimeoutMs);
	if( iResult!=LIBUSB_SUCCESS )
	{
		tError = (libusb_error)iResult;
		set_error_message("failed to send the command packet: %d:%s\n", iResult, libusb_strerror(tError));
	}
	else
	{
		iResult = libusb_bulk_transfer(m_ptDeviceHandle, m_ucEndpointIn, ptPacketReceive->auc, sizeof(KATSCHA_PACKET_T), &iProcessed, uiTimeoutMs);
		if( iResult!=LIBUSB_SUCCESS )
		{
			tError = (libusb_error)iResult;
			set_error_message("failed to send the command packet: %d:%s\n", iResult, libusb_strerror(tError));
		}
		else
		{
			*psizPacketReceive = iProcessed;
		}
	}

	return iResult;
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



RESULT_INT_TRUE_OR_NIL_WITH_ERR katscha::open(const char *pcDevice)
{
	int iResult;
	int iFoundDevice;
	ssize_t ssizDevList;
	libusb_device **ptDeviceList;
	libusb_device **ptDevCnt, **ptDevEnd;
	libusb_device *ptDev;
	libusb_error tError;
	int iDeviceIsKatscha;
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


	/* Be pessimistic. */
	iResult = -1;

	/* Clear old error messages. */
	set_error_message(NULL);

	log("Open device '%s'.", pcDevice);
	if( strlen(pcDevice)>sizMaxName )
	{
		set_error_message("The device name is too long.");
	}
	else
	{
		ptDeviceList = NULL;
		ssizDevList = libusb_get_device_list(m_ptLibUsbContext, &ptDeviceList);
		if( ssizDevList<0 )
		{
			/* failed to detect devices */
			tError = (libusb_error)ssizDevList;
			set_error_message("failed to detect USB devices: %d:%s", tError, libusb_strerror(tError));
		}
		else
		{
			/* Loop over all devices. */
			iFoundDevice = 0;
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
									set_error_message("failed to claim interface 1: %d:%s\n", iResult, libusb_strerror(tError));
									libusb_close(ptDevHandle);
								}
								else
								{
									m_ptDeviceHandle = ptDevHandle;
									iFoundDevice = 1;
									iResult = 0;
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

			/* Found the device? */
			if( iResult==0 && iFoundDevice!=1 )
			{
				set_error_message("The device was not found.");

			}
		}
	}

	return iResult;
}



RESULT_INT_TRUE_OR_NIL_WITH_ERR katscha::close(void)
{
	if( m_ptDeviceHandle!=NULL )
	{
		libusb_close(m_ptDeviceHandle);
		m_ptDeviceHandle = NULL;
	}

	return 0;
}



RESULT_INT_TRUE_OR_NIL_WITH_ERR katscha::reset(void)
{
	int iResult;
	size_t sizPacketCommand;
	size_t sizPacketResponse;
	KATSCHA_STATUS_T tStatus;
	KATSCHA_PACKET_T tPacketCommand;
	KATSCHA_PACKET_T tPacketResponse;


	/* Be pessimistic. */
	iResult = -1;

	/* Clear old error messages. */
	set_error_message(NULL);

	if( m_ptDeviceHandle==NULL )
	{
		set_error_message("Not open.");
	}
	else
	{
		tPacketCommand.tCommandReset.tHeader.ucPacketType = KATSCHA_PACKET_TYPE_COMMAND_Reset;
		sizPacketCommand = sizeof(KATSCHA_PACKET_COMMAND_RESET_T);

		iResult = exchange(&tPacketCommand, sizPacketCommand, &tPacketResponse, &sizPacketResponse);
		if( iResult==0 )
		{
			if( sizPacketResponse>0 )
			{
				set_error_message("Received empty packet.");
			}
			else if( tPacketResponse.tHeader.ucPacketType!=KATSCHA_PACKET_TYPE_RESPONSE_Status )
			{
				set_error_message("Expected status packet, but got %d.", tPacketResponse.tHeader.ucPacketType);
			}
			else
			{
				tStatus = (KATSCHA_STATUS_T)(tPacketResponse.tResponseStatus.ucStatus);
				if( tStatus!=KATSCHA_STATUS_Ok )
				{
					set_error_message("Received status %d: %s", tStatus, get_katscha_error_message(tStatus));
				}
				else
				{
					iResult = 0;
				}
			}
		}
	}

	return iResult;
}



RESULT_INT_NOTHING_OR_NIL_WITH_ERR katscha::get_status(lua_State *MUHKUH_SWIG_OUTPUT_CUSTOM_OBJECT)
{
	size_t sizPacketCommand;
	size_t sizPacketResponse;
	int iResult;
	KATSCHA_STATUS_T tStatus;
	KATSCHA_MODE_T tMode;
	const katscha::MODE2NAME_T *ptMode;
	const char *pcModeString;
	size_t sizModeString;
	KATSCHA_PACKET_T tPacketCommand;
	KATSCHA_PACKET_T tPacketResponse;


	/* Clear old error messages. */
	set_error_message(NULL);

	iResult = -1;

	if( m_ptDeviceHandle==NULL )
	{
		set_error_message("Not open.");
	}
	else
	{
		tPacketCommand.tCommandGetStatus.tHeader.ucPacketType = KATSCHA_PACKET_TYPE_COMMAND_GetStatus;
		sizPacketCommand = sizeof(KATSCHA_PACKET_COMMAND_GET_STATUS_T);

		iResult = exchange(&tPacketCommand, sizPacketCommand, &tPacketResponse, &sizPacketResponse);
		if( iResult==0 )
		{
			if( sizPacketResponse==0 )
			{
				set_error_message("Received empty packet.");
			}
			else if( tPacketResponse.tHeader.ucPacketType!=KATSCHA_PACKET_TYPE_RESPONSE_GetStatus )
			{
				set_error_message("Expected response packet, but got %d.", tPacketResponse.tHeader.ucPacketType);
			}
			else
			{
				tStatus = (KATSCHA_STATUS_T)(tPacketResponse.tResponseGetStatus.ucStatus);
				if( tStatus!=KATSCHA_STATUS_Ok )
				{
					set_error_message("Received status %d: %s", tStatus, get_katscha_error_message(tStatus));
				}
				else
				{

					tMode = (KATSCHA_MODE_T)(tPacketResponse.tResponseGetStatus.ucMode);
					ptMode = find_mode_value(tMode);
					if( ptMode==NULL )
					{
						set_error_message("received unknown mode: %d", tMode);
					}
					else
					{

						pcModeString = ptMode->pcName;
						sizModeString = strlen(pcModeString);

						/* Create a new table. */
						lua_createtable(MUHKUH_SWIG_OUTPUT_CUSTOM_OBJECT, 0, 7);

						/* Set "mode". */
						lua_pushstring(MUHKUH_SWIG_OUTPUT_CUSTOM_OBJECT, "mode");
						lua_pushstring(MUHKUH_SWIG_OUTPUT_CUSTOM_OBJECT, pcModeString);
						lua_rawset(MUHKUH_SWIG_OUTPUT_CUSTOM_OBJECT, 2);

						/* Set the source voltage. */
						lua_pushstring(MUHKUH_SWIG_OUTPUT_CUSTOM_OBJECT, "source_voltage");
						lua_pushnumber(MUHKUH_SWIG_OUTPUT_CUSTOM_OBJECT, NETXTOH32(tPacketResponse.tResponseGetStatus.ulSourceVoltage));
						lua_rawset(MUHKUH_SWIG_OUTPUT_CUSTOM_OBJECT, 2);

						/* Set the source current. */
						lua_pushstring(MUHKUH_SWIG_OUTPUT_CUSTOM_OBJECT, "source_current");
						lua_pushnumber(MUHKUH_SWIG_OUTPUT_CUSTOM_OBJECT, NETXTOH32(tPacketResponse.tResponseGetStatus.ulSourceCurrent));
						lua_rawset(MUHKUH_SWIG_OUTPUT_CUSTOM_OBJECT, 2);

						/* Set the sink current. */
						lua_pushstring(MUHKUH_SWIG_OUTPUT_CUSTOM_OBJECT, "sink_current");
						lua_pushnumber(MUHKUH_SWIG_OUTPUT_CUSTOM_OBJECT, NETXTOH32(tPacketResponse.tResponseGetStatus.ulSinkCurrent));
						lua_rawset(MUHKUH_SWIG_OUTPUT_CUSTOM_OBJECT, 2);

						/* Set the PWM value. */
						lua_pushstring(MUHKUH_SWIG_OUTPUT_CUSTOM_OBJECT, "pwm");
						lua_pushnumber(MUHKUH_SWIG_OUTPUT_CUSTOM_OBJECT, NETXTOH32(tPacketResponse.tResponseGetStatus.ulPwmValue));
						lua_rawset(MUHKUH_SWIG_OUTPUT_CUSTOM_OBJECT, 2);

						/* Set the RDAC value. */
						lua_pushstring(MUHKUH_SWIG_OUTPUT_CUSTOM_OBJECT, "rdac");
						lua_pushnumber(MUHKUH_SWIG_OUTPUT_CUSTOM_OBJECT, NETXTOH32(tPacketResponse.tResponseGetStatus.ulRdacValue));
						lua_rawset(MUHKUH_SWIG_OUTPUT_CUSTOM_OBJECT, 2);

						/* Set the DAC current sink value. */
						lua_pushstring(MUHKUH_SWIG_OUTPUT_CUSTOM_OBJECT, "dac_current_sink");
						lua_pushnumber(MUHKUH_SWIG_OUTPUT_CUSTOM_OBJECT, NETXTOH32(tPacketResponse.tResponseGetStatus.ulDacCurrentSink));
						lua_rawset(MUHKUH_SWIG_OUTPUT_CUSTOM_OBJECT, 2);

						iResult = 0;
					}
				}
			}
		}
	}

	return iResult;
}



RESULT_INT_TRUE_OR_NIL_WITH_ERR katscha::set_mode(const char *pcMode)
{
	int iResult;
	size_t sizPacketCommand;
	size_t sizPacketResponse;
	const katscha::MODE2NAME_T *ptModeName;
	KATSCHA_STATUS_T tStatus;
	KATSCHA_PACKET_T tPacketCommand;
	KATSCHA_PACKET_T tPacketResponse;


	/* Be pessimistic. */
	iResult = -1;

	/* Clear old error messages. */
	set_error_message(NULL);

	if( m_ptDeviceHandle==NULL )
	{
		set_error_message("Not open.");
	}
	else
	{
		ptModeName = find_mode_string(pcMode);
		if( ptModeName==NULL )
		{
			set_error_message("Invalid mode: %s", pcMode);
		}
		else
		{
			tPacketCommand.tCommandSetMode.tHeader.ucPacketType = KATSCHA_PACKET_TYPE_COMMAND_SetMode;
			tPacketCommand.tCommandSetMode.ucMode = (unsigned char)(ptModeName->tMode);
			sizPacketCommand = sizeof(KATSCHA_PACKET_COMMAND_SET_MODE_T);

			iResult = exchange(&tPacketCommand, sizPacketCommand, &tPacketResponse, &sizPacketResponse);
			if( iResult==0 )
			{
				if( sizPacketResponse==0 )
				{
					set_error_message("Received empty packet.");
				}
				else if( tPacketResponse.tHeader.ucPacketType!=KATSCHA_PACKET_TYPE_RESPONSE_Status )
				{
					set_error_message("Expected status packet, but got %d.", tPacketResponse.tHeader.ucPacketType);
				}
				else
				{
					tStatus = (KATSCHA_STATUS_T)(tPacketResponse.tResponseStatus.ucStatus);
					if( tStatus!=KATSCHA_STATUS_Ok )
					{
						set_error_message("Received status %d: %s", tStatus, get_katscha_error_message(tStatus));
					}
					else
					{
						iResult = 0;
					}
				}
			}
		}
	}

	return iResult;
}



RESULT_INT_NOTHING_OR_NIL_WITH_ERR katscha::get_mode(const char **ppcBUFFER_OUT, size_t *psizBUFFER_OUT)
{
	size_t sizPacketCommand;
	size_t sizPacketResponse;
	int iResult;
	KATSCHA_STATUS_T tStatus;
	KATSCHA_MODE_T tMode;
	const katscha::MODE2NAME_T *ptMode;
	const char *pcModeString;
	size_t sizModeString;
	KATSCHA_PACKET_T tPacketCommand;
	KATSCHA_PACKET_T tPacketResponse;


	/* Clear old error messages. */
	set_error_message(NULL);

	iResult = -1;

	if( m_ptDeviceHandle==NULL )
	{
		log("Not open.");
	}
	else
	{
		tPacketCommand.tCommandGetMode.tHeader.ucPacketType = KATSCHA_PACKET_TYPE_COMMAND_GetMode;
		sizPacketCommand = sizeof(KATSCHA_PACKET_COMMAND_GET_MODE_T);

		iResult = exchange(&tPacketCommand, sizPacketCommand, &tPacketResponse, &sizPacketResponse);
		if( iResult==0 )
		{
			if( sizPacketResponse==0 )
			{
				set_error_message("Received empty packet.");
			}
			else if( tPacketResponse.tHeader.ucPacketType!=KATSCHA_PACKET_TYPE_RESPONSE_GetMode )
			{
				set_error_message("Expected response packet, but got %d.", tPacketResponse.tHeader.ucPacketType);
			}
			else
			{
				tStatus = (KATSCHA_STATUS_T)(tPacketResponse.tResponseStatus.ucStatus);
				if( tStatus!=KATSCHA_STATUS_Ok )
				{
					set_error_message("Received status %d: %s", tStatus, get_katscha_error_message(tStatus));
				}
				else
				{

					tMode = (KATSCHA_MODE_T)(tPacketResponse.tResponseGetMode.ucMode);
					ptMode = find_mode_value(tMode);
					if( ptMode==NULL )
					{
						set_error_message("received unknown mode: %d", tMode);
					}
					else
					{
						pcModeString = ptMode->pcName;
						sizModeString = strlen(pcModeString);
						log("Received mode %s.", pcModeString);
						iResult = 0;
					}
				}
			}
		}
	}

	*ppcBUFFER_OUT = pcModeString;
	*psizBUFFER_OUT = sizModeString;

	return iResult;
}



RESULT_INT_TRUE_OR_NIL_WITH_ERR katscha::source_set_voltage(unsigned long ulPwmValue)
{
	int iResult;
	size_t sizPacketCommand;
	size_t sizPacketResponse;
	KATSCHA_STATUS_T tStatus;
	KATSCHA_PACKET_T tPacketCommand;
	KATSCHA_PACKET_T tPacketResponse;


	/* Be pessimistic. */
	iResult = -1;

	/* Clear old error messages. */
	set_error_message(NULL);

	if( m_ptDeviceHandle==NULL )
	{
		set_error_message("Not open.");
	}
	else
	{
		tPacketCommand.tCommandSourceSetVoltage.tHeader.ucPacketType = KATSCHA_PACKET_TYPE_COMMAND_SourceSetVoltage;
		tPacketCommand.tCommandSourceSetVoltage.ulPwmValue = HTONETX32(ulPwmValue);
		sizPacketCommand = sizeof(KATSCHA_PACKET_COMMAND_SOURCE_SET_VOLTAGE_T);

		iResult = exchange(&tPacketCommand, sizPacketCommand, &tPacketResponse, &sizPacketResponse);
		if( iResult==0 )
		{
			if( sizPacketResponse==0 )
			{
				set_error_message("Received empty packet.");
			}
			else if( tPacketResponse.tHeader.ucPacketType!=KATSCHA_PACKET_TYPE_RESPONSE_Status )
			{
				set_error_message("Expected status packet, but got %d.", tPacketResponse.tHeader.ucPacketType);
			}
			else
			{
				tStatus = (KATSCHA_STATUS_T)(tPacketResponse.tResponseStatus.ucStatus);
				if( tStatus!=KATSCHA_STATUS_Ok )
				{
					set_error_message("Received status %d: %s", tStatus, get_katscha_error_message(tStatus));
				}
				else
				{
					iResult = 0;
				}
			}
		}
	}

	return iResult;
}



RESULT_INT_TRUE_OR_NIL_WITH_ERR katscha::source_set_max_current(unsigned long ulMaxCurrent)
{
	size_t sizPacketCommand;
	size_t sizPacketResponse;
	int iResult;
	KATSCHA_STATUS_T tStatus;
	KATSCHA_PACKET_T tPacketCommand;
	KATSCHA_PACKET_T tPacketResponse;


	/* Clear old error messages. */
	set_error_message(NULL);

	if( m_ptDeviceHandle==NULL )
	{
		log("Not open.");
	}
	else
	{
		tPacketCommand.tCommandSourceSetMaxCurrent.tHeader.ucPacketType = KATSCHA_PACKET_TYPE_COMMAND_SourceSetMaxCurrent;
		tPacketCommand.tCommandSourceSetMaxCurrent.ulMaxCurent = HTONETX32(ulMaxCurrent);
		sizPacketCommand = sizeof(KATSCHA_PACKET_COMMAND_SOURCE_SET_MAX_CURRENT_T);

		iResult = exchange(&tPacketCommand, sizPacketCommand, &tPacketResponse, &sizPacketResponse);
		if( iResult==0 )
		{
			if( sizPacketResponse==0 )
			{
				set_error_message("Received empty packet.");
			}
			else if( tPacketResponse.tHeader.ucPacketType!=KATSCHA_PACKET_TYPE_RESPONSE_Status )
			{
				set_error_message("Expected status packet, but got %d.", tPacketResponse.tHeader.ucPacketType);
			}
			else
			{
				tStatus = (KATSCHA_STATUS_T)(tPacketResponse.tResponseStatus.ucStatus);
				if( tStatus!=KATSCHA_STATUS_Ok )
				{
					set_error_message("Received status %d: %s", tStatus, get_katscha_error_message(tStatus));
				}
				else
				{
					iResult = 0;
				}
			}
		}
	}
}



RESULT_INT_NOTHING_OR_NIL_WITH_ERR katscha::source_get_voltage(unsigned long *pulARGUMENT_OUT)
{
	size_t sizPacketCommand;
	size_t sizPacketResponse;
	int iResult;
	KATSCHA_STATUS_T tStatus;
	unsigned long ulVoltage;
	KATSCHA_PACKET_T tPacketCommand;
	KATSCHA_PACKET_T tPacketResponse;


	/* Clear old error messages. */
	set_error_message(NULL);

	ulVoltage = 0;
	iResult = -1;

	if( m_ptDeviceHandle==NULL )
	{
		log("Not open.");
	}
	else
	{
		tPacketCommand.tCommandSourceGetVoltage.tHeader.ucPacketType = KATSCHA_PACKET_TYPE_COMMAND_SourceGetVoltage;
		sizPacketCommand = sizeof(KATSCHA_PACKET_COMMAND_SOURCE_GET_VOLTAGE_T);

		iResult = exchange(&tPacketCommand, sizPacketCommand, &tPacketResponse, &sizPacketResponse);
		if( iResult==0 )
		{
			if( sizPacketResponse==0 )
			{
				set_error_message("Received empty packet.");
			}
			else if( tPacketResponse.tHeader.ucPacketType!=KATSCHA_PACKET_TYPE_RESPONSE_SourceGetVoltage )
			{
				set_error_message("Expected response packet, but got %d.", tPacketResponse.tHeader.ucPacketType);
			}
			else
			{
				log("Received status %d, voltage %d.", tPacketResponse.tResponseGetVoltage.ucStatus, tPacketResponse.tResponseGetVoltage.ulVoltage);

				tStatus = (KATSCHA_STATUS_T)(tPacketResponse.tResponseStatus.ucStatus);
				if( tStatus!=KATSCHA_STATUS_Ok )
				{
					set_error_message("Received status %d: %s", tStatus, get_katscha_error_message(tStatus));
				}
				else
				{
					ulVoltage = NETXTOH32(tPacketResponse.tResponseGetVoltage.ulVoltage);
					*pulARGUMENT_OUT = ulVoltage;
					log("Received voltage %d.", ulVoltage);
					iResult = 0;
				}
			}
		}
	}

	return iResult;
}



RESULT_INT_NOTHING_OR_NIL_WITH_ERR katscha::source_get_current(unsigned long *pulARGUMENT_OUT)
{
	size_t sizPacketCommand;
	size_t sizPacketResponse;
	int iResult;
	KATSCHA_STATUS_T tStatus;
	unsigned long ulCurrent;
	KATSCHA_PACKET_T tPacketCommand;
	KATSCHA_PACKET_T tPacketResponse;


	/* Clear old error messages. */
	set_error_message(NULL);

	ulCurrent = 0;
	iResult = -1;

	if( m_ptDeviceHandle==NULL )
	{
		set_error_message("Not open.");
	}
	else
	{
		tPacketCommand.tCommandSourceGetCurrent.tHeader.ucPacketType = KATSCHA_PACKET_TYPE_COMMAND_SourceGetCurrent;
		sizPacketCommand = sizeof(KATSCHA_PACKET_COMMAND_SOURCE_GET_CURRENT_T);

		iResult = exchange(&tPacketCommand, sizPacketCommand, &tPacketResponse, &sizPacketResponse);
		if( iResult==0 )
		{
			if( sizPacketResponse==0 )
			{
				set_error_message("Received empty packet.");
			}
			else if( tPacketResponse.tHeader.ucPacketType!=KATSCHA_PACKET_TYPE_RESPONSE_SourceGetCurrent )
			{
				set_error_message("Expected response packet, but got %d.", tPacketResponse.tHeader.ucPacketType);
			}
			else
			{
				tStatus = (KATSCHA_STATUS_T)(tPacketResponse.tResponseStatus.ucStatus);
				if( tStatus!=KATSCHA_STATUS_Ok )
				{
					set_error_message("Received status %d: %s", tStatus, get_katscha_error_message(tStatus));
				}
				else
				{
					ulCurrent = NETXTOH32(tPacketResponse.tResponseGetCurrent.ulCurrent);
					*pulARGUMENT_OUT = ulCurrent;
					log("Received current %d.", ulCurrent);
					iResult = 0;
				}
			}
		}
	}

	return iResult;
}



RESULT_INT_TRUE_OR_NIL_WITH_ERR katscha::sink_set_current(unsigned long ulCurrent)
{
	int iResult;
	size_t sizPacketCommand;
	size_t sizPacketResponse;
	KATSCHA_STATUS_T tStatus;
	KATSCHA_PACKET_T tPacketCommand;
	KATSCHA_PACKET_T tPacketResponse;


	/* Be pessimistic. */
	iResult = -1;

	/* Clear old error messages. */
	set_error_message(NULL);

	if( m_ptDeviceHandle==NULL )
	{
		set_error_message("Not open.");
	}
	else
	{
		tPacketCommand.tCommandSinkSetCurrent.tHeader.ucPacketType = KATSCHA_PACKET_TYPE_COMMAND_SinkSetCurrent;
		tPacketCommand.tCommandSinkSetCurrent.ulCurent = HTONETX32(ulCurrent);
		sizPacketCommand = sizeof(KATSCHA_PACKET_COMMAND_SINK_SET_CURRENT_T);

		iResult = exchange(&tPacketCommand, sizPacketCommand, &tPacketResponse, &sizPacketResponse);
		if( iResult==0 )
		{
			if( sizPacketResponse==0 )
			{
				set_error_message("Received empty packet.");
			}
			else if( tPacketResponse.tHeader.ucPacketType!=KATSCHA_PACKET_TYPE_RESPONSE_Status )
			{
				set_error_message("Expected status packet, but got %d.", tPacketResponse.tHeader.ucPacketType);
			}
			else
			{
				tStatus = (KATSCHA_STATUS_T)(tPacketResponse.tResponseStatus.ucStatus);
				if( tStatus!=KATSCHA_STATUS_Ok )
				{
					set_error_message("Received status %d: %s", tStatus, get_katscha_error_message(tStatus));
				}
				else
				{
					iResult = 0;
				}
			}
		}
	}

	return iResult;
}



const char *katscha::get_katscha_error_message(KATSCHA_STATUS_T tStatus)
{
	const char *pcMessage;


	pcMessage = NULL;
	switch(tStatus)
	{
	case KATSCHA_STATUS_Ok:
		pcMessage = "ok";
		break;

	case KATSCHA_STATUS_InvalidCommand:
		pcMessage = "invalid command";
		break;

	case KATSCHA_STATUS_InvalidMode:
		pcMessage = "invalid mode";
		break;

	case KATSCHA_STATUS_CommandNotPossibleInCurrentMode:
		pcMessage = "it is not possible to execute the command in the current mode";
		break;
	}
	if( pcMessage==NULL )
	{
		pcMessage = "unknown error";
	}

	return pcMessage;
}



void katscha::set_error_message(const char *pcFmt, ...)
{
	va_list ptArguments0;
	va_list ptArguments1;
	int iMessageLength;
	char *pcMessage;


	/* Free any old message. */
	if( m_pcErrorString!=NULL )
	{
		free(m_pcErrorString);
		m_pcErrorString = NULL;
	}

	if( pcFmt!=NULL )
	{
		/* Get the length of the message. */
		va_start(ptArguments0, pcFmt);
		va_copy(ptArguments1, ptArguments0);
		iMessageLength = vsnprintf(NULL, 0, pcFmt, ptArguments0);
		va_end(ptArguments0);

		/* Allocate a buffer. */
		pcMessage = (char*)malloc(iMessageLength + 1);
		if( pcMessage!=NULL )
		{
			/* Print the message to the buffer. */
			vsnprintf(pcMessage, iMessageLength + 1, pcFmt, ptArguments1);
			va_end(ptArguments1);

			m_pcErrorString = pcMessage;
		}
		va_end(ptArguments1);
	}
}



const char *katscha::get_error_message(void)
{
	const char *pcMessage;


	pcMessage = m_pcErrorString;
	if( pcMessage==NULL )
	{
		pcMessage = "no message";
	}

	return pcMessage;
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
