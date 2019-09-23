#include <stddef.h>
#include <string.h>

#include "interface.h"
#include "netx_io_areas.h"
#include "plode.h"
#include "powerboard.h"
#include "rdy_run.h"
#include "systime.h"


typedef struct USB_CONFIGURATION_STRUCT
{
	unsigned short us_vendor_id;
	unsigned short us_device_id;
	unsigned short us_release_id;
	unsigned char uc_config_characteristics;
	unsigned char uc_maximum_power_consumption;
	unsigned char t_vendor_string[1+16];
	unsigned char t_device_string[1+16];
	unsigned char t_serial_string[1+16];
} USB_CONFIGURATION_T;



typedef enum USB_FIFO_MODE_ENUM
{
	USB_FIFO_MODE_Stream       = 0,
	USB_FIFO_MODE_StreamZLP    = 1,
	USB_FIFO_MODE_Packet       = 2,
	USB_FIFO_MODE_Transaction  = 3
} USB_FIFO_MODE_T;



typedef enum USB_MI_TYP_ENUM
{
	USB_MI_TYP_None = 0,
	USB_MI_TYP_Uart = 1,
	USB_MI_TYP_JTag = 2
} USB_MI_TYP_T;


static const PLODE_ENTRY_T s_atImplodeOptionsToUnit[] =
{
	{ offsetof(USB_CONFIGURATION_T, us_vendor_id),                  0x00,  0x02 },
	{ offsetof(USB_CONFIGURATION_T, us_device_id),                  0x02,  0x02 },
	{ offsetof(USB_CONFIGURATION_T, us_release_id),                 0x04,  0x02 },
	{ offsetof(USB_CONFIGURATION_T, uc_config_characteristics),     0x06,  0x01 },
	{ offsetof(USB_CONFIGURATION_T, uc_maximum_power_consumption),  0x07,  0x01 },
	/* the following entries are hfields, skip the length byte */
	{ offsetof(USB_CONFIGURATION_T, t_vendor_string)+1,             0x0a,  0x10 },
	{ offsetof(USB_CONFIGURATION_T, t_device_string)+1,             0x1c,  0x10 },
	{ offsetof(USB_CONFIGURATION_T, t_serial_string)+1,             0x2e,  0x10 }
};


/* NOTE: The UART functions are not used yet. The only way to control the
 * module is the USB-JTAG interface. Maybe there will be an ASCII interface
 * for a terminal progamm in the distant future.
 */
#if 0
static unsigned char usb_get(void)
{
	HOSTDEF(ptUsbDevFifoArea)
	HOSTDEF(ptUsbDevFifoCtrlArea)
	unsigned long ulFillLevel;


	/* Get the Usb RX input fill level. */
	do
	{
		ulFillLevel   = ptUsbDevFifoCtrlArea->ulUsb_dev_fifo_ctrl_uart_ep_rx_stat;
		ulFillLevel  &= HOSTMSK(usb_dev_fifo_ctrl_uart_ep_rx_stat_fill_level);
		ulFillLevel >>= HOSTSRT(usb_dev_fifo_ctrl_uart_ep_rx_stat_fill_level);
	} while	( ulFillLevel==0 );

	/* Copy one byte from the receive to the transmit fifo. */
	return (unsigned char)ptUsbDevFifoArea->ulUsb_dev_uart_rx_data;
}


static void usb_put(unsigned char ucChar)
{
	HOSTDEF(ptUsbDevFifoArea)
	HOSTDEF(ptUsbDevFifoCtrlArea)
	unsigned long ulFillLevel;


	/* Get the Usb TX input fill level. */
	do
	{
		ulFillLevel   = ptUsbDevFifoCtrlArea->ulUsb_dev_fifo_ctrl_uart_ep_tx_stat;
		ulFillLevel  &= HOSTMSK(usb_dev_fifo_ctrl_uart_ep_tx_stat_fill_level);
		ulFillLevel >>= HOSTSRT(usb_dev_fifo_ctrl_uart_ep_tx_stat_fill_level);
	} while	( ulFillLevel>=64 );

	ptUsbDevFifoArea->ulUsb_dev_uart_tx_data = ucChar;
}


static unsigned int usb_peek(void)
{
	HOSTDEF(ptUsbDevFifoCtrlArea)
	unsigned long ulFillLevel;


	/* Get the USB RX input fill level. */
	ulFillLevel   = ptUsbDevFifoCtrlArea->ulUsb_dev_fifo_ctrl_uart_ep_rx_stat;
	ulFillLevel  &= HOSTMSK(usb_dev_fifo_ctrl_uart_ep_rx_stat_fill_level);
	ulFillLevel >>= HOSTSRT(usb_dev_fifo_ctrl_uart_ep_rx_stat_fill_level);

	return (ulFillLevel==0) ? 0U : 1U;
}


static void usb_flush(void)
{
	HOSTDEF(ptUsbDevFifoCtrlArea)
	unsigned long ulFillLevel;


	/* Get the USB TX input fill level. */
	do
	{
		ulFillLevel   = ptUsbDevFifoCtrlArea->ulUsb_dev_fifo_ctrl_uart_ep_tx_stat;
		ulFillLevel  &= HOSTMSK(usb_dev_fifo_ctrl_uart_ep_tx_stat_fill_level);
		ulFillLevel >>= HOSTSRT(usb_dev_fifo_ctrl_uart_ep_tx_stat_fill_level);
	} while	( ulFillLevel!=0 );
}


static void usb_close(void)
{
	HOSTDEF(ptUsbDevCtrlArea)
	HOSTDEF(ptUsbDevFifoCtrlArea)


	/* Reset all FIFOs. */
	ptUsbDevFifoCtrlArea->ulUsb_dev_fifo_ctrl_conf = HOSTMSK(usb_dev_fifo_ctrl_conf_reset);
	/* Release the reset. */
	ptUsbDevFifoCtrlArea->ulUsb_dev_fifo_ctrl_conf = 0;

	/* Disable core. */
	ptUsbDevCtrlArea->ulUsb_dev_cfg = 0;

	/* Disable all IRQs. */
	ptUsbDevCtrlArea->ulUsb_dev_irq_mask = 0;
}
#endif


static const USB_CONFIGURATION_T tUsbConfiguration =
{
	.us_vendor_id = 0x1939U,
	.us_device_id = 0x002fU,
	.us_release_id = 0x0001U,
	.uc_config_characteristics = 0xc0U,
	.uc_maximum_power_consumption = 0x00U,
	.t_vendor_string = {  8, 'H', 'i', 'l', 's', 'c', 'h', 'e', 'r',   0,   0,   0,   0,   0,   0,   0,   0 },
	.t_device_string = {  7, 'K', 'a', 't', 's', 'c', 'h', 'a',   0,   0,   0,   0,   0,   0,   0,   0,   0 },
	.t_serial_string = {  2, '0', '0',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 }
};



static void usb_init(void)
{
	HOSTDEF(ptUsbDevFifoCtrlArea)
	HOSTDEF(ptUsbDevCtrlArea)
	union
	{
		unsigned long  aul[64/sizeof(unsigned long)];
		unsigned short aus[64/sizeof(unsigned short)];
		unsigned char  auc[64/sizeof(unsigned char)];
	} uBuf;
	const unsigned long *pulCnt;
	const unsigned long *pulEnd;
	volatile unsigned long *pulDst;
	unsigned long ulValue;
	unsigned int uiStrSize;


	/* Pack the configuration array into the registers. */
	plode(uBuf.aul, &tUsbConfiguration, s_atImplodeOptionsToUnit, sizeof(s_atImplodeOptionsToUnit)/sizeof(s_atImplodeOptionsToUnit[0]));

	/* Get the hField length. */
	uiStrSize   = tUsbConfiguration.t_vendor_string[0];
	/* Add one 0 byte. */
	uiStrSize  += 1U;
	/* Convert to UTF16. */
	uiStrSize <<= 1U;
	uBuf.aus[0x08/sizeof(unsigned short)] = (unsigned short)uiStrSize;

	/* Get the hField length. */
	uiStrSize   = tUsbConfiguration.t_device_string[0];
	/* Add one 0 byte. */
	uiStrSize  += 1U;
	/* Convert to UTF16. */
	uiStrSize <<= 1U;
	uBuf.aus[0x1a/sizeof(unsigned short)] = (unsigned short)uiStrSize;

	/* Get the hField length. */
	uiStrSize   = tUsbConfiguration.t_serial_string[0];
	/* Add one 0 byte. */
	uiStrSize  += 1U;
	/* Convert to UTF16. */
	uiStrSize <<= 1U;
	uBuf.aus[0x2c/sizeof(unsigned short)] = (unsigned short)uiStrSize;

	/* Add the fixed fields for string descriptor types. */
	uBuf.auc[0x09] = 0x03;
	uBuf.auc[0x1b] = 0x03;
	uBuf.auc[0x2d] = 0x03;

	/* Get a pointer to the start and end of the enumeration data. */
	pulCnt = uBuf.aul;
	pulEnd = uBuf.aul + (sizeof(uBuf.aul)/sizeof(uBuf.aul[0]));

	/* Get a pointer to the enumeration ram. */
	pulDst = (volatile unsigned long*)HOSTADDR(usb_dev_enum_ram);

	/* Copy the data. */
	while( pulCnt<pulEnd )
	{
		*(pulDst++) = *(pulCnt++);
	}

	/* Reset the complete core. */
	ptUsbDevCtrlArea->ulUsb_dev_cfg = HOSTMSK(usb_dev_cfg_usb_dev_reset);

	/* Reset all FIFOs. */
	ptUsbDevFifoCtrlArea->ulUsb_dev_fifo_ctrl_conf = HOSTMSK(usb_dev_fifo_ctrl_conf_reset);

	/* Set the FIFO modes and release the reset. */
	ulValue  = USB_FIFO_MODE_Transaction << HOSTSRT(usb_dev_fifo_ctrl_conf_mode_interrupt);
	ulValue |= USB_FIFO_MODE_StreamZLP   << HOSTSRT(usb_dev_fifo_ctrl_conf_mode_uart_rx);
	ulValue |= USB_FIFO_MODE_StreamZLP   << HOSTSRT(usb_dev_fifo_ctrl_conf_mode_uart_tx);
	ulValue |= USB_FIFO_MODE_Packet      << HOSTSRT(usb_dev_fifo_ctrl_conf_mode_jtag_rx);
	ulValue |= USB_FIFO_MODE_Packet      << HOSTSRT(usb_dev_fifo_ctrl_conf_mode_jtag_tx);
	ptUsbDevFifoCtrlArea->ulUsb_dev_fifo_ctrl_conf = ulValue;

	/* Enable the core. */
	ulValue  = HOSTMSK(usb_dev_cfg_usb_core_enable);
	/* Set disconnect timeout to the maximum. */
	ulValue |= 3 << HOSTSRT(usb_dev_cfg_disconn_timeout);
	ptUsbDevCtrlArea->ulUsb_dev_cfg = ulValue;

	/* Disable all IRQs. */
	ptUsbDevCtrlArea->ulUsb_dev_irq_mask = 0;
        /* Acknowledge any pending IRQs. */
        ptUsbDevCtrlArea->ulUsb_dev_irq_raw = 0xffffffffU;
}



static void send_response(const unsigned char *pucData, unsigned int sizData)
{
	HOSTDEF(ptUsbDevCtrlArea);
	HOSTDEF(ptUsbDevFifoArea);
	HOSTDEF(ptUsbDevFifoCtrlArea);
	unsigned long ulValue;
	const unsigned char *pucCnt;
	const unsigned char *pucEnd;


	/* Limit the data to 64 bytes. */
	if( sizData>64 )
	{
		sizData = 64;
	}

	/* Wait until any running transfers are finished. */
	do
	{
		ulValue  = ptUsbDevFifoCtrlArea->ulUsb_dev_fifo_ctrl_jtag_ep_tx_stat;
		ulValue &= HOSTMSK(usb_dev_fifo_ctrl_jtag_ep_tx_stat_transaction_active);
	} while( ulValue!=0 );

	/* Wait until the requested bytes fit into the FIFO. */
	do
	{
		ulValue   = ptUsbDevFifoCtrlArea->ulUsb_dev_fifo_ctrl_jtag_ep_tx_stat;
		ulValue  &= HOSTMSK(usb_dev_fifo_ctrl_jtag_ep_tx_stat_fill_level);
		ulValue >>= HOSTSRT(usb_dev_fifo_ctrl_jtag_ep_tx_stat_fill_level);
	} while( ulValue>(64-sizData) );

	/* Copy the data to the FIFO. */
	pucCnt = pucData;
	pucEnd = pucData + sizData;
	while( pucCnt<pucEnd )
	{
		ptUsbDevFifoArea->ulUsb_dev_jtag_tx_data = *(pucCnt++);
	}

	/* Start a new transfer.
	 * Set the packet length. */
	ulValue  = sizData << HOSTSRT(usb_dev_fifo_ctrl_jtag_ep_tx_len_transaction_len);
	/* Do not send ZLPs. This connection is using a custom driver. */
	ulValue |= HOSTMSK(usb_dev_fifo_ctrl_jtag_ep_tx_len_transaction_no_zlp);
	ptUsbDevFifoCtrlArea->ulUsb_dev_fifo_ctrl_jtag_ep_tx_len = ulValue;

	/* Wait until the packet is sent. */
	do
	{
		ulValue  = ptUsbDevCtrlArea->  ulUsb_dev_irq_raw;
		ulValue &= HOSTMSK(usb_dev_irq_raw_jtag_tx_packet_sent);
	} while( ulValue==0 );

	/* Acknowledge the IRQ. */
	ptUsbDevCtrlArea->ulUsb_dev_irq_raw = HOSTMSK(usb_dev_irq_raw_jtag_tx_packet_sent);
}



static KATSCHA_MODE_T tKatschaMode;


static void process_invalid_command(void)
{
	KATSCHA_PACKET_T tResponse;


	/* Send a status response. */
	tResponse.tResponseStatus.tHeader.ucPacketType = KATSCHA_PACKET_TYPE_RESPONSE_Status;
	tResponse.tResponseStatus.ucStatus = KATSCHA_STATUS_InvalidCommand;
	send_response(tResponse.auc, sizeof(KATSCHA_PACKET_RESPONSE_STATUS_T));
}



static void process_command_reset(KATSCHA_PACKET_COMMAND_RESET_T *ptCommand __attribute__((unused)))
{
	KATSCHA_PACKET_T tResponse;


	powerboard_source_disable();
	powerboard_sink_disable();

	tKatschaMode = KATSCHA_MODE_Idle;

	/* Send a status response. */
	tResponse.tResponseStatus.tHeader.ucPacketType = KATSCHA_PACKET_TYPE_RESPONSE_Status;
	tResponse.tResponseStatus.ucStatus = KATSCHA_STATUS_Ok;
	send_response(tResponse.auc, sizeof(KATSCHA_PACKET_RESPONSE_STATUS_T));
}



static void process_command_get_status(KATSCHA_PACKET_COMMAND_GET_STATUS_T *ptCommand __attribute__((unused)))
{
	KATSCHA_PACKET_T tResponse;
	unsigned long ulRdacValue;


	powerboard_source_get_max_current(&ulRdacValue);

	/* Send a status response. */
	tResponse.tResponseGetStatus.tHeader.ucPacketType = KATSCHA_PACKET_TYPE_RESPONSE_GetStatus;
	tResponse.tResponseGetStatus.ucStatus = KATSCHA_STATUS_Ok;
	tResponse.tResponseGetStatus.ucMode = tKatschaMode;
	tResponse.tResponseGetStatus.ulSourceVoltage = powerboard_source_get_voltage();
	tResponse.tResponseGetStatus.ulSourceCurrent = powerboard_source_get_current();
	tResponse.tResponseGetStatus.ulSinkCurrent = powerboard_sink_get_current();
	tResponse.tResponseGetStatus.ulPwmValue = powerboard_source_get_pwm();
	tResponse.tResponseGetStatus.ulRdacValue = ulRdacValue;
	tResponse.tResponseGetStatus.ulDacCurrentSink = powerboard_sink_get_dac();

	send_response(tResponse.auc, sizeof(KATSCHA_PACKET_RESPONSE_GET_STATUS_T));
}



static void process_command_set_mode(KATSCHA_PACKET_COMMAND_SET_MODE_T *ptCommand)
{
	KATSCHA_STATUS_T tStatus;
	KATSCHA_MODE_T tMode;
	KATSCHA_PACKET_T tResponse;


	tMode = (KATSCHA_MODE_T)(ptCommand->ucMode);
	tStatus = KATSCHA_STATUS_InvalidMode;
	switch(tMode)
	{
	case KATSCHA_MODE_Idle:
	case KATSCHA_MODE_Source:
	case KATSCHA_MODE_Sink:
		tStatus = KATSCHA_STATUS_Ok;
		break;
	}
	if( tStatus==KATSCHA_STATUS_Ok )
	{
		switch(tMode)
		{
		case KATSCHA_MODE_Idle:
			powerboard_source_disable();
			powerboard_sink_disable();
			tKatschaMode = KATSCHA_MODE_Idle;
			break;

		case KATSCHA_MODE_Source:
			powerboard_source_enable();
			powerboard_sink_disable();
			tKatschaMode = KATSCHA_MODE_Source;
			break;

		case KATSCHA_MODE_Sink:
			powerboard_source_disable();
			powerboard_sink_enable();
			tKatschaMode = KATSCHA_MODE_Sink;
			break;
		}
	}

	/* Send a status response. */
	tResponse.tResponseStatus.tHeader.ucPacketType = KATSCHA_PACKET_TYPE_RESPONSE_Status;
	tResponse.tResponseStatus.ucStatus = tStatus;
	send_response(tResponse.auc, sizeof(KATSCHA_PACKET_RESPONSE_STATUS_T));
}



static void process_command_get_mode(KATSCHA_PACKET_COMMAND_GET_MODE_T *ptCommand __attribute__((unused)))
{
	KATSCHA_PACKET_T tResponse;


	/* Send a status response. */
	tResponse.tResponseGetMode.tHeader.ucPacketType = KATSCHA_PACKET_TYPE_RESPONSE_GetMode;
	tResponse.tResponseGetMode.ucStatus = KATSCHA_STATUS_Ok;
	tResponse.tResponseGetMode.ucMode = tKatschaMode;
	send_response(tResponse.auc, sizeof(KATSCHA_PACKET_RESPONSE_GET_MODE_T));
}



static void process_command_source_set_voltage(KATSCHA_PACKET_COMMAND_SOURCE_SET_VOLTAGE_T *ptCommand)
{
	KATSCHA_STATUS_T tStatus;
	unsigned long ulPwmValue;
	KATSCHA_PACKET_T tResponse;


	if( tKatschaMode!=KATSCHA_MODE_Source )
	{
		tStatus = KATSCHA_STATUS_CommandNotPossibleInCurrentMode;
	}
	else
	{
		ulPwmValue = ptCommand->ulPwmValue;

		/* TODO: check the value. */

		powerboard_source_set_voltage(ulPwmValue);

		tStatus = KATSCHA_STATUS_Ok;
	}

	/* Send a status response. */
	tResponse.tResponseStatus.tHeader.ucPacketType = KATSCHA_PACKET_TYPE_RESPONSE_Status;
	tResponse.tResponseStatus.ucStatus = tStatus;
	send_response(tResponse.auc, sizeof(KATSCHA_PACKET_RESPONSE_STATUS_T));
}



static void process_command_source_set_max_current(KATSCHA_PACKET_COMMAND_SOURCE_SET_MAX_CURRENT_T *ptCommand)
{
	KATSCHA_STATUS_T tStatus;
	unsigned long ulMaxCurrent;
	KATSCHA_PACKET_T tResponse;


	if( tKatschaMode!=KATSCHA_MODE_Source )
	{
		tStatus = KATSCHA_STATUS_CommandNotPossibleInCurrentMode;
	}
	else
	{
		ulMaxCurrent = ptCommand->ulMaxCurent;

		/* TODO: Check the value. */

		powerboard_source_set_max_current(ulMaxCurrent);

		tStatus = KATSCHA_STATUS_Ok;
	}

	/* Send a status response. */
	tResponse.tResponseStatus.tHeader.ucPacketType = KATSCHA_PACKET_TYPE_RESPONSE_Status;
	tResponse.tResponseStatus.ucStatus = tStatus;
	send_response(tResponse.auc, sizeof(KATSCHA_PACKET_RESPONSE_STATUS_T));
}



static void process_command_source_get_voltage(KATSCHA_PACKET_COMMAND_SOURCE_GET_VOLTAGE_T *ptCommand __attribute__((unused)))
{
	KATSCHA_STATUS_T tStatus;
	unsigned long ulVoltage;
	KATSCHA_PACKET_T tResponse;


	if( tKatschaMode!=KATSCHA_MODE_Source )
	{
		ulVoltage = 0;
		tStatus = KATSCHA_STATUS_CommandNotPossibleInCurrentMode;
	}
	else
	{
		ulVoltage = powerboard_source_get_voltage();
		tStatus = KATSCHA_STATUS_Ok;
	}

	/* Send a response. */
	tResponse.tResponseGetVoltage.tHeader.ucPacketType = KATSCHA_PACKET_TYPE_RESPONSE_SourceGetVoltage;
	tResponse.tResponseGetVoltage.ucStatus = tStatus;
	tResponse.tResponseGetVoltage.ulVoltage = ulVoltage;
	send_response(tResponse.auc, sizeof(KATSCHA_PACKET_RESPONSE_SOURCE_GET_VOLTAGE_T));
}



static void process_command_source_get_current(KATSCHA_PACKET_COMMAND_SOURCE_GET_CURRENT_T *ptCommand __attribute__((unused)))
{
	KATSCHA_STATUS_T tStatus;
	unsigned long ulCurrent;
	KATSCHA_PACKET_T tResponse;


	if( tKatschaMode!=KATSCHA_MODE_Source )
	{
		ulCurrent = 0;
		tStatus = KATSCHA_STATUS_CommandNotPossibleInCurrentMode;
	}
	else
	{
		ulCurrent = powerboard_source_get_current();
		tStatus = KATSCHA_STATUS_Ok;
	}

	/* Send a response. */
	tResponse.tResponseGetCurrent.tHeader.ucPacketType = KATSCHA_PACKET_TYPE_RESPONSE_SourceGetCurrent;
	tResponse.tResponseGetCurrent.ucStatus = tStatus;
	tResponse.tResponseGetCurrent.ulCurrent = ulCurrent;
	send_response(tResponse.auc, sizeof(KATSCHA_PACKET_RESPONSE_SOURCE_GET_CURRENT_T));
}



static void process_command_sink_set_current(KATSCHA_PACKET_COMMAND_SINK_SET_CURRENT_T *ptCommand)
{
	KATSCHA_STATUS_T tStatus;
	unsigned long ulCurrent;
	KATSCHA_PACKET_T tResponse;


	if( tKatschaMode!=KATSCHA_MODE_Sink )
	{
		tStatus = KATSCHA_STATUS_CommandNotPossibleInCurrentMode;
	}
	else
	{
		ulCurrent = ptCommand->ulCurent;

		/* TODO: Check the value. */

		powerboard_sink_set_current(ulCurrent);

		tStatus = KATSCHA_STATUS_Ok;
	}

	/* Send a status response. */
	tResponse.tResponseStatus.tHeader.ucPacketType = KATSCHA_PACKET_TYPE_RESPONSE_Status;
	tResponse.tResponseStatus.ucStatus = tStatus;
	send_response(tResponse.auc, sizeof(KATSCHA_PACKET_RESPONSE_STATUS_T));
}



static void process_packet(void)
{
	HOSTDEF(ptUsbDevCtrlArea)
	HOSTDEF(ptUsbDevFifoArea)
	HOSTDEF(ptUsbDevFifoCtrlArea)
	unsigned long ulValue;
	unsigned long ulFillLevel;
	unsigned char *pucCnt;
	unsigned char *pucEnd;
	KATSCHA_PACKET_TYPE_T tCommand;
	int iResult;
	KATSCHA_PACKET_T tPacketRx;


	/* Wait for a new packet. */
	ulValue  = ptUsbDevCtrlArea->ulUsb_dev_irq_raw;
	ulValue &= HOSTMSK(usb_dev_irq_raw_jtag_rx_packet_received);
	if( ulValue!=0 )
	{
		/* Acknowledge the IRQ. */
		ptUsbDevCtrlArea->ulUsb_dev_irq_raw = HOSTMSK(usb_dev_irq_raw_jtag_rx_packet_received);

		/* Get the size of the received packet. */
		ulFillLevel   = ptUsbDevFifoCtrlArea->ulUsb_dev_fifo_ctrl_jtag_ep_rx_len;
		ulFillLevel  &= HOSTMSK(usb_dev_fifo_ctrl_jtag_ep_rx_len_packet_len);
		ulFillLevel >>= HOSTSRT(usb_dev_fifo_ctrl_jtag_ep_rx_len_packet_len);

		/* Ignore ZLPs. */
		if( ulFillLevel>0 )
		{
			if( ulFillLevel>64 )
			{
				/* Reset the FIFO. */
				ulValue  = ptUsbDevFifoCtrlArea->ulUsb_dev_fifo_ctrl_conf;
				ulValue |= (1U << 5U) << HOSTSRT(usb_dev_fifo_ctrl_conf_reset);
				ptUsbDevFifoCtrlArea->ulUsb_dev_fifo_ctrl_conf = ulValue;

				/* Clear the reset. */
				ulValue &= ~(HOSTMSK(usb_dev_fifo_ctrl_conf_reset));
				ptUsbDevFifoCtrlArea->ulUsb_dev_fifo_ctrl_conf = ulValue;
			}
			else
			{
				/* Copy the complete packet to the buffer. */
				pucCnt = tPacketRx.auc;
				pucEnd = tPacketRx.auc + ulFillLevel;
				do
				{
					*(pucCnt++) = (unsigned char)(ptUsbDevFifoArea->ulUsb_dev_jtag_rx_data);
				} while( pucCnt<pucEnd );

				tCommand = (KATSCHA_PACKET_TYPE_T)(tPacketRx.tHeader.ucPacketType);
				iResult = -1;
				switch(tCommand)
				{
				case KATSCHA_PACKET_TYPE_COMMAND_Reset:
				case KATSCHA_PACKET_TYPE_COMMAND_GetStatus:
				case KATSCHA_PACKET_TYPE_COMMAND_SetMode:
				case KATSCHA_PACKET_TYPE_COMMAND_GetMode:
				case KATSCHA_PACKET_TYPE_COMMAND_SourceSetVoltage:
				case KATSCHA_PACKET_TYPE_COMMAND_SourceSetMaxCurrent:
				case KATSCHA_PACKET_TYPE_COMMAND_SourceGetVoltage:
				case KATSCHA_PACKET_TYPE_COMMAND_SourceGetCurrent:
				case KATSCHA_PACKET_TYPE_COMMAND_SinkSetCurrent:
					iResult = 0;
					break;

				case KATSCHA_PACKET_TYPE_RESPONSE_Status:
				case KATSCHA_PACKET_TYPE_RESPONSE_GetStatus:
				case KATSCHA_PACKET_TYPE_RESPONSE_GetMode:
				case KATSCHA_PACKET_TYPE_RESPONSE_SourceGetVoltage:
				case KATSCHA_PACKET_TYPE_RESPONSE_SourceGetCurrent:
					/* This is no command. */
					break;
				}
				if( iResult!=0 )
				{
					/* Invalid command. */
					process_invalid_command();
				}
				else
				{
					switch(tCommand)
					{
					case KATSCHA_PACKET_TYPE_COMMAND_Reset:
						process_command_reset(&(tPacketRx.tCommandReset));
						break;

					case KATSCHA_PACKET_TYPE_COMMAND_GetStatus:
						process_command_get_status(&(tPacketRx.tCommandGetStatus));
						break;

					case KATSCHA_PACKET_TYPE_COMMAND_SetMode:
						process_command_set_mode(&(tPacketRx.tCommandSetMode));
						break;

					case KATSCHA_PACKET_TYPE_COMMAND_GetMode:
						process_command_get_mode(&(tPacketRx.tCommandGetMode));
						break;

					case KATSCHA_PACKET_TYPE_COMMAND_SourceSetVoltage:
						process_command_source_set_voltage(&(tPacketRx.tCommandSourceSetVoltage));
						break;

					case KATSCHA_PACKET_TYPE_COMMAND_SourceSetMaxCurrent:
						process_command_source_set_max_current(&(tPacketRx.tCommandSourceSetMaxCurrent));
						break;

					case KATSCHA_PACKET_TYPE_COMMAND_SourceGetVoltage:
						process_command_source_get_voltage(&(tPacketRx.tCommandSourceGetVoltage));
						break;

					case KATSCHA_PACKET_TYPE_COMMAND_SourceGetCurrent:
						process_command_source_get_current(&(tPacketRx.tCommandSourceGetCurrent));
						break;

					case KATSCHA_PACKET_TYPE_COMMAND_SinkSetCurrent:
						process_command_sink_set_current(&(tPacketRx.tCommandSinkSetCurrent));
						break;

					case KATSCHA_PACKET_TYPE_RESPONSE_Status:
					case KATSCHA_PACKET_TYPE_RESPONSE_GetStatus:
					case KATSCHA_PACKET_TYPE_RESPONSE_GetMode:
					case KATSCHA_PACKET_TYPE_RESPONSE_SourceGetVoltage:
					case KATSCHA_PACKET_TYPE_RESPONSE_SourceGetCurrent:
						break;
					}
				}
			}
		}
	}
}



void katscha_main(void) __attribute__((noreturn));
void katscha_main(void)
{
	BLINKI_HANDLE_T tBlinkiHandle;
	int iResult;


	systime_init();
	usb_init();

	iResult = powerboard_initialize();
	if( iResult==0 )
	{
		tKatschaMode = KATSCHA_MODE_Idle;

		rdy_run_blinki_init(&tBlinkiHandle, 0x00000055, 0x00000150);
		while(1)
		{
			process_packet();
			rdy_run_blinki(&tBlinkiHandle);
		};
	}
	else
	{
		rdy_run_setLEDs(RDYRUN_YELLOW);
		while(1);
	}
}
