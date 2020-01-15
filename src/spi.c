#include <string.h>
#include "netx_io_areas.h"
#include "spi.h"
#include "powerboard.h"


void spi_delay(unsigned int uiDelay)
{
	unsigned int uiCnt;


	uiCnt = 0;
	while( uiCnt<uiDelay )
	{
		__asm__("nop");
		++uiCnt;
	}
}



void spi_init_IO_pins(void)
{
	unsigned int uiValue;
	HOSTDEF(ptAsicCtrlArea);
	HOSTDEF(ptGpioArea);
	HOSTDEF(ptHifIoCtrlArea);

	uiValue  = ptGpioArea->aulGpio_cfg[1];
	uiValue |= 0x05; // GPIO 1	High	// SPI_SRT_CS_3 high
	ptGpioArea->aulGpio_cfg[1] = uiValue;

	uiValue  = ptHifIoCtrlArea->aulHif_pio_out[0];
	uiValue |= SPI_MSK_CS_0|SPI_MSK_CS_1|SPI_MSK_CS_2|SPI_MSK_CLK;
	ptAsicCtrlArea->ulAsic_ctrl_access_key = ptAsicCtrlArea->ulAsic_ctrl_access_key;	/* @suppress("Assignment to itself") */
	ptHifIoCtrlArea->aulHif_pio_out[0] = uiValue;

	spi_delay(DELAY_CYCLES);

	uiValue  = ptHifIoCtrlArea->aulHif_pio_oe[0];
	uiValue |= SPI_MSK_CS_0|SPI_MSK_CS_1|SPI_MSK_CS_2|SPI_MSK_CLK|SPI_MSK_MOSI|ENABLE_SOURCE|ENABLE_SINK;
	ptAsicCtrlArea->ulAsic_ctrl_access_key = ptAsicCtrlArea->ulAsic_ctrl_access_key;	/* @suppress("Assignment to itself") */
	ptHifIoCtrlArea->aulHif_pio_oe[0] = uiValue;

	uiValue  = ptHifIoCtrlArea->ulHif_pio_cfg;
	uiValue |= 0x01;
	ptAsicCtrlArea->ulAsic_ctrl_access_key = ptAsicCtrlArea->ulAsic_ctrl_access_key;	/* @suppress("Assignment to itself") */
	ptHifIoCtrlArea->ulHif_pio_cfg = uiValue;
}

unsigned int spi_get_value(void)
{
	unsigned int uiValue;
	unsigned int uiAdcValue = 0x00;
	unsigned int uiBitcount;
	HOSTDEF(ptAsicCtrlArea);

	for (uiBitcount = 0;uiBitcount < 16;uiBitcount++)
	{
		HOSTDEF(ptHifIoCtrlArea);
		uiValue  = ptHifIoCtrlArea->aulHif_pio_out[0];
		uiValue &= ~SPI_MSK_CLK;	// CLK low
		ptAsicCtrlArea->ulAsic_ctrl_access_key = ptAsicCtrlArea->ulAsic_ctrl_access_key;	/* @suppress("Assignment to itself") */
		ptHifIoCtrlArea->aulHif_pio_out[0] = uiValue;
		spi_delay(DELAY_CYCLES);

		uiValue  = ptHifIoCtrlArea->aulHif_pio_out[0];
		uiValue |= SPI_MSK_CLK;
		ptAsicCtrlArea->ulAsic_ctrl_access_key = ptAsicCtrlArea->ulAsic_ctrl_access_key;	/* @suppress("Assignment to itself") */
		ptHifIoCtrlArea->aulHif_pio_out[0] = uiValue;
		spi_delay(DELAY_CYCLES);

		uiValue = ptHifIoCtrlArea->aulHif_pio_in[0];
		uiValue >>= SPI_SRT_MISO;
		uiValue &= 0x01;
		uiAdcValue <<= 0x01;
		uiAdcValue |= uiValue;
	}

	uiAdcValue >>= 3;

return uiAdcValue;
}


void spi_set_value(unsigned int uiSpiValue)
{
	unsigned int uiBitcount;
	unsigned int uiValue;
	HOSTDEF(ptAsicCtrlArea);
	HOSTDEF(ptHifIoCtrlArea);

	for (uiBitcount = 0;uiBitcount < 16;uiBitcount++)
	{
		uiValue  = ptHifIoCtrlArea->aulHif_pio_out[0];
		uiValue &= ~SPI_MSK_CLK;	// CLK low
		ptAsicCtrlArea->ulAsic_ctrl_access_key = ptAsicCtrlArea->ulAsic_ctrl_access_key;	/* @suppress("Assignment to itself") */
		ptHifIoCtrlArea->aulHif_pio_out[0] = uiValue;

		spi_delay(DELAY_CYCLES);

		uiValue = ptHifIoCtrlArea->aulHif_pio_out[0];
		if ((uiSpiValue & (1<<16)) == (1<<16))
		{
			uiValue |= SPI_MSK_MOSI;
		}
		else
		{
			uiValue &= ~SPI_MSK_MOSI;
		}

		ptAsicCtrlArea->ulAsic_ctrl_access_key = ptAsicCtrlArea->ulAsic_ctrl_access_key;	/* @suppress("Assignment to itself") */
		ptHifIoCtrlArea->aulHif_pio_out[0] = uiValue;
		uiSpiValue <<= 1;

		uiValue = ptHifIoCtrlArea->aulHif_pio_out[0];
		uiValue |= SPI_MSK_CLK;			// CLK high
		ptAsicCtrlArea->ulAsic_ctrl_access_key = ptAsicCtrlArea->ulAsic_ctrl_access_key;	/* @suppress("Assignment to itself") */
		ptHifIoCtrlArea->aulHif_pio_out[0] = uiValue;

		spi_delay(DELAY_CYCLES);
	}

	uiValue = ptHifIoCtrlArea->aulHif_pio_out[0];
	uiValue &= ~SPI_MSK_CLK;		// CLK low
	ptAsicCtrlArea->ulAsic_ctrl_access_key = ptAsicCtrlArea->ulAsic_ctrl_access_key;	/* @suppress("Assignment to itself") */
	ptHifIoCtrlArea->aulHif_pio_out[0] = uiValue;
}
