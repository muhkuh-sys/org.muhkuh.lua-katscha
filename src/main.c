#include <stddef.h>
#include <string.h>

#include "netx_io_areas.h"
#include "plode.h"
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


	/* Pack the config array. */
	plode(uBuf.aul, &tUsbConfiguration, s_atImplodeOptionsToUnit, sizeof(s_atImplodeOptionsToUnit)/sizeof(s_atImplodeOptionsToUnit[0]));

	/* Get the hfield length. */
	uiStrSize   = tUsbConfiguration.t_vendor_string[0];
	/* Add one 0 byte. */
	uiStrSize  += 1U;
	/* Convert to UTF16. */
	uiStrSize <<= 1U;
	uBuf.aus[0x08/sizeof(unsigned short)] = (unsigned short)uiStrSize;

	/* Get the hfield length. */
	uiStrSize   = tUsbConfiguration.t_device_string[0];
	/* Add one 0 byte. */
	uiStrSize  += 1U;
	/* Convert to UTF16. */
	uiStrSize <<= 1U;
	uBuf.aus[0x1a/sizeof(unsigned short)] = (unsigned short)uiStrSize;

	/* Get the hfield length. */
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



void process_packet(void)
{
	HOSTDEF(ptUsbDevCtrlArea)
	HOSTDEF(ptUsbDevFifoArea)
	HOSTDEF(ptUsbDevFifoCtrlArea)
	unsigned long ulValue;
	unsigned long ulFillLevel;
	unsigned char *pucCnt;
	unsigned char *pucEnd;
	unsigned char aucPacketRx[64];


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
				pucCnt = aucPacketRx;
				pucEnd = aucPacketRx + ulFillLevel;
				do
				{
					*(pucCnt++) = (unsigned char)(ptUsbDevFifoArea->ulUsb_dev_jtag_rx_data);
				} while( pucCnt<pucEnd );

			}
		}
	}
}



void katscha_main(void) __attribute__((noreturn));
void katscha_main(void)
{
	BLINKI_HANDLE_T tBlinkiHandle;


	systime_init();
	usb_init();

	rdy_run_blinki_init(&tBlinkiHandle, 0x00000055, 0x00000150);
	while(1)
	{
		process_packet();
		rdy_run_blinki(&tBlinkiHandle);
	};
}
