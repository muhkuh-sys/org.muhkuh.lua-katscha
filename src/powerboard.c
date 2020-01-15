#include "ad5175.h"
#include "netx_io_areas.h"
#include "powerboard.h"
#include "p2i2c.h"
#include "spi.h"


/*-------------------------------------------------------------------------*/


P2I2C_CFG_T s_tI2CCfg;
unsigned long ulLastSinkCurrentValue;

#define P2I2C_MSK_SDC 0x00004000U
#define P2I2C_SRT_SDC 14
#define P2I2C_MSK_SDA 0x00008000U
#define P2I2C_SRT_SDA 15

static void powerboard_i2c_set_pins(P2I2C_PIN_STATE_T tSclState, P2I2C_PIN_STATE_T tSdaState)
{
	unsigned int uiVal;
	unsigned int uiOe;
	HOSTDEF(ptAsicCtrlArea);
	HOSTDEF(ptHifIoCtrlArea);

	uiVal = ptHifIoCtrlArea->aulHif_pio_out[0];
	uiOe  = ptHifIoCtrlArea->aulHif_pio_oe[0];

	switch(tSclState)
	{
	case P2I2C_PIN_STATE_out0:
		uiOe  |=  P2I2C_MSK_SDC;
		uiVal &= ~P2I2C_MSK_SDC;
		break;

	case P2I2C_PIN_STATE_out1:
		uiOe  |=  P2I2C_MSK_SDC;
		uiVal |=  P2I2C_MSK_SDC;
		break;

	case P2I2C_PIN_STATE_input:
		uiOe  &= ~P2I2C_MSK_SDC;
		uiVal &= ~P2I2C_MSK_SDC;
		break;
	}

	switch(tSdaState)
	{
	case P2I2C_PIN_STATE_out0:
		uiOe  |=  P2I2C_MSK_SDA;
		uiVal &= ~P2I2C_MSK_SDA;
		break;

	case P2I2C_PIN_STATE_out1:
		uiOe  |=  P2I2C_MSK_SDA;
		uiVal |=  P2I2C_MSK_SDA;
		break;

	case P2I2C_PIN_STATE_input:
		uiOe  &= ~P2I2C_MSK_SDA;
		uiVal &= ~P2I2C_MSK_SDA;
		break;
	}

	ptAsicCtrlArea->ulAsic_ctrl_access_key = ptAsicCtrlArea->ulAsic_ctrl_access_key;	/* @suppress("Assignment to itself") */
	ptHifIoCtrlArea->aulHif_pio_out[0] = uiVal;
	ptAsicCtrlArea->ulAsic_ctrl_access_key = ptAsicCtrlArea->ulAsic_ctrl_access_key;	/* @suppress("Assignment to itself") */
	ptHifIoCtrlArea->aulHif_pio_oe[0] = uiOe;
}



static unsigned int powerboard_i2c_get_sda(void)
{
	unsigned long ulValue;
	HOSTDEF(ptHifIoCtrlArea);

	ulValue = ptHifIoCtrlArea->aulHif_pio_in[0];
	ulValue >>= P2I2C_SRT_SDA;
	ulValue  &= 1;

	return ulValue;
}

/*-------------------------------------------------------------------------*/


static unsigned int read_adc_values(unsigned int uiCsAdc)
{
	unsigned int uiADC_value;
	unsigned int uiValue;
	HOSTDEF(ptHifIoCtrlArea);

	uiValue  = ptHifIoCtrlArea->aulHif_pio_out[0];

	switch ( uiCsAdc )
	{
	case 1:
		uiValue &= ~SPI_MSK_CS_0;				// CS 0 low, Source voltage
		break;
	case 2:
		uiValue &= ~SPI_MSK_CS_1;				// CS 1 low, Source current
		break;
	case 3:
		uiValue &= ~SPI_MSK_CS_2;				// CS 2 low, Sink current
		break;
	}

	ptHifIoCtrlArea->aulHif_pio_out[0] = uiValue;
	spi_delay(DELAY_CYCLES);
	uiADC_value = spi_get_value();
	uiValue  = ptHifIoCtrlArea->aulHif_pio_out[0];

	switch ( uiCsAdc )
	{
	case 1:
		uiValue |= SPI_MSK_CS_0;				// CS 0 high
		break;
	case 2:
		uiValue |= SPI_MSK_CS_1;				// CS 1 high
		break;
	case 3:
		uiValue |= SPI_MSK_CS_2;				// CS 2 high
		break;
	}

	ptHifIoCtrlArea->aulHif_pio_out[0] = uiValue;
	spi_delay(DELAY_CYCLES);

	return uiADC_value;
}



static unsigned long powerboard_read_adc_arit_average(unsigned int uiChipSelect)
{
	unsigned long ulSum;
	unsigned int uiCnt;
	const unsigned int uiRuns = 16;


	/* Make one dummy read. */
	read_adc_values(uiChipSelect);

	/* Collect a few values. */
	ulSum = 0;
	for(uiCnt=0; uiCnt<uiRuns; ++uiCnt)
	{
		ulSum += read_adc_values(uiChipSelect);
	}
	return ulSum / uiRuns;
}



static void write_dac_value(unsigned int uiSpiValue)
{
	HOSTDEF(ptGpioArea);


	/* Set the GPIO pin to low. */
	ptGpioArea->aulGpio_cfg[1] = 4;

	spi_delay(4*DELAY_CYCLES);

	spi_set_value(uiSpiValue);

	spi_delay(4*DELAY_CYCLES);

	/* Set the GPIO pin to high. */
	ptGpioArea->aulGpio_cfg[1] = 5;

	spi_delay(DELAY_CYCLES);
}



static const NX56_MMIO_CFG_T atMmioPins[50] =
{
	NX56_MMIO_CFG_PIO,          /* MMIO0 */
	NX56_MMIO_CFG_PIO,          /* MMIO1 */
	NX56_MMIO_CFG_PIO,          /* MMIO2 */
	NX56_MMIO_CFG_PIO,          /* MMIO3 */
	NX56_MMIO_CFG_PIO,          /* MMIO4 */
	NX56_MMIO_CFG_PIO,          /* MMIO5 */
	NX56_MMIO_CFG_PIO,          /* MMIO6 */
	NX56_MMIO_CFG_PIO,          /* MMIO7 */
	NX56_MMIO_CFG_PIO,          /* MMIO8 */
	NX56_MMIO_CFG_PIO,          /* MMIO9 */
	NX56_MMIO_CFG_PIO,          /* MMIO10 */
	NX56_MMIO_CFG_PIO,          /* MMIO11 */
	NX56_MMIO_CFG_PIO,          /* MMIO12 */
	NX56_MMIO_CFG_PIO,          /* MMIO13 */
	NX56_MMIO_CFG_PIO,          /* MMIO14 */
	NX56_MMIO_CFG_PIO,          /* MMIO15 */
	NX56_MMIO_CFG_PIO,          /* MMIO16 */
	NX56_MMIO_CFG_PIO,          /* MMIO17 */
	NX56_MMIO_CFG_PIO,          /* MMIO18 */
	NX56_MMIO_CFG_PIO,          /* MMIO19 */
	NX56_MMIO_CFG_PIO,          /* MMIO20 */
	NX56_MMIO_CFG_gpio0,        /* MMIO21 */
	NX56_MMIO_CFG_PIO,          /* MMIO22 */
	NX56_MMIO_CFG_gpio1,        /* MMIO23 (SPI_CS3) */
	NX56_MMIO_CFG_PIO,          /* MMIO24 */
	NX56_MMIO_CFG_PIO,          /* MMIO25 */
	NX56_MMIO_CFG_PIO,          /* MMIO26 */
	NX56_MMIO_CFG_PIO,          /* MMIO27 */
	NX56_MMIO_CFG_PIO,          /* MMIO28 */
	NX56_MMIO_CFG_PIO,          /* MMIO29 */
	NX56_MMIO_CFG_PIO,          /* MMIO30 */
	NX56_MMIO_CFG_PIO,          /* MMIO31 */
	NX56_MMIO_CFG_uart0_ctsn,   /* MMIO32 */
	NX56_MMIO_CFG_uart0_rtsn,   /* MMIO33 */
	NX56_MMIO_CFG_uart0_rxd,    /* MMIO34 */
	NX56_MMIO_CFG_uart0_txd,    /* MMIO35 */
	NX56_MMIO_CFG_PIO,          /* MMIO36 */
	NX56_MMIO_CFG_PIO,          /* MMIO37 */
	NX56_MMIO_CFG_PIO,          /* MMIO38 */
	NX56_MMIO_CFG_PIO,          /* MMIO39 */
	NX56_MMIO_CFG_PIO,          /* MMIO40 */
	NX56_MMIO_CFG_PIO,          /* MMIO41 */
	NX56_MMIO_CFG_PIO,          /* MMIO42 */
	NX56_MMIO_CFG_PIO,          /* MMIO43 */
	NX56_MMIO_CFG_PIO,          /* MMIO44 */
	NX56_MMIO_CFG_PIO,          /* MMIO45 */
	NX56_MMIO_CFG_PIO,          /* MMIO46 */
	NX56_MMIO_CFG_PIO,          /* MMIO47 */
	NX56_MMIO_CFG_PIO,          /* MMIO48 */
	NX56_MMIO_CFG_PIO           /* MMIO49 */
};



int powerboard_initialize(void)
{
	HOSTDEF(ptAsicCtrlArea);
	HOSTDEF(ptMmioCtrlArea);
	HOSTDEF(ptGpioArea);
	HOSTDEF(ptHifIoCtrlArea);
	unsigned int uiCnt;
	unsigned long ulValue;
	int iResult;


	/* Configure all MMIOs. */
	for(uiCnt=0; uiCnt<(sizeof(atMmioPins)/sizeof(atMmioPins[0])); ++uiCnt)
	{
		ulValue  = atMmioPins[uiCnt];
		ptAsicCtrlArea->ulAsic_ctrl_access_key = ptAsicCtrlArea->ulAsic_ctrl_access_key;  /* @suppress("Assignment to itself") */
		ptMmioCtrlArea->aulMmio_cfg[uiCnt] = ulValue;
	}

	/* Set GPIO1 to high. This is SPI_CS3. */
	ptGpioArea->aulGpio_cfg[1] = 5U << HOSTSRT(gpio_cfg1_mode);
	/* Set all other SPI chip selects and the clock to high.
	 * Disable the source and the sink.
	 */
	ulValue = SPI_MSK_CS_0|SPI_MSK_CS_1|SPI_MSK_CS_2|SPI_MSK_CLK|ENABLE_SINK;
	ptHifIoCtrlArea->aulHif_pio_out[0] = ulValue;
	ulValue = SPI_MSK_CS_0|SPI_MSK_CS_1|SPI_MSK_CS_2|SPI_MSK_CLK|SPI_MSK_MOSI|ENABLE_SOURCE|ENABLE_SINK;
	ptHifIoCtrlArea->aulHif_pio_oe[0] = ulValue;

	/* Sample the HIF PIOs continuously. */
	ulValue = 1U << HOSTSRT(hif_pio_cfg_in_ctrl);
	ptHifIoCtrlArea->ulHif_pio_cfg = ulValue;

	/* Stop the counter. */
	ptGpioArea->aulGpio_counter_ctrl[0] = 0x00000000;

	/* Set GPIO0 to PWM mode. */
	ulValue = 7 << HOSTSRT(gpio_cfg0_mode);
	ptGpioArea->aulGpio_cfg[0]          = ulValue;

	/* Initialize the PWM. */
	ptGpioArea->aulGpio_tc[0]           = 0x00000000;   // GPIO 0 Threshold
	ptGpioArea->aulGpio_counter_max[0]  = 0x00010000;   // GPIO 0 Counter 0 Max Value

	/* Start the counter. */
	ulValue = HOSTMSK(gpio_counter0_ctrl_run);
	ptGpioArea->aulGpio_counter_ctrl[0] = ulValue;


	ulLastSinkCurrentValue = 0;

	s_tI2CCfg.uiChipAddress = AD5175_ADDRESS;
	s_tI2CCfg.uiDelayCycles = AD5175_I2C_DELAY_CYCLES;
	s_tI2CCfg.pfnSetPins = powerboard_i2c_set_pins;
	s_tI2CCfg.pfnGetSda = powerboard_i2c_get_sda;
	p2i2c_init_pins(&s_tI2CCfg);

	/* Delay a while. */
	p2i2c_delay1Cycle(&s_tI2CCfg);
	p2i2c_delay1Cycle(&s_tI2CCfg);
	p2i2c_delay1Cycle(&s_tI2CCfg);
	p2i2c_delay1Cycle(&s_tI2CCfg);

	iResult = AD5175_initialize(&s_tI2CCfg);
	if( iResult==0 )
	{
		iResult = AD5175_set_position(&s_tI2CCfg, 0);
	}

	return iResult;
}



void powerboard_source_enable(void)
{
	unsigned int uiValue;
	HOSTDEF(ptHifIoCtrlArea);


	powerboard_source_set_voltage(0);
	powerboard_source_set_max_current(0);

	uiValue  = ptHifIoCtrlArea->aulHif_pio_out[0];
	uiValue |= ENABLE_SOURCE;		// Enable Source
	ptHifIoCtrlArea->aulHif_pio_out[0] = uiValue;
}



void powerboard_source_disable(void)
{
	unsigned int uiValue;
	HOSTDEF(ptHifIoCtrlArea);


	uiValue  = ptHifIoCtrlArea->aulHif_pio_out[0];
	uiValue &= ~ENABLE_SOURCE;
	ptHifIoCtrlArea->aulHif_pio_out[0] = uiValue;

	powerboard_source_set_voltage(0);
	powerboard_source_set_max_current(0);
}



int powerboard_source_set_voltage(unsigned int uiPWMvalue)
{
	HOSTDEF(ptGpioArea);
	int iResult;


	if( uiPWMvalue>0x00010000U )
	{
		iResult = -1;
	}
	else
	{
		ptGpioArea->aulGpio_tc[0] = uiPWMvalue;	// GPIO 0	Threshold
		iResult = 0;
	}

	return iResult;
}



int powerboard_source_set_max_current(unsigned int uiMaxCurrent)
{
	int iResult;


	if( uiMaxCurrent>1023 )
	{
		iResult = -1;
	}
	else
	{
		iResult = AD5175_set_position(&s_tI2CCfg, uiMaxCurrent);
	}

	return iResult;
}



int powerboard_source_get_max_current(unsigned long *pulValue)
{
	int iResult;


	iResult = AD5175_read(&s_tI2CCfg, pulValue);

	return iResult;
}



unsigned int powerboard_source_get_voltage(void)
{
	/* CS 0 is the source voltage. */
	return powerboard_read_adc_arit_average(1);
}



unsigned int powerboard_source_get_current(void)
{
	/* CS 1 is the source current. */
	return powerboard_read_adc_arit_average(2);
}



unsigned long powerboard_source_get_pwm(void)
{
	HOSTDEF(ptGpioArea);


	return ptGpioArea->aulGpio_tc[0];
}



unsigned int powerboard_sink_get_current(void)
{
	/* CS 2 is the source current. */
	return powerboard_read_adc_arit_average(4);
}



void powerboard_sink_enable(void)
{
	unsigned int uiValue;
	HOSTDEF(ptHifIoCtrlArea);


	powerboard_sink_set_current(0);

	uiValue  = ptHifIoCtrlArea->aulHif_pio_out[0];
	uiValue &= ~ENABLE_SINK;	// Enable Sink
	ptHifIoCtrlArea->aulHif_pio_out[0] = uiValue;
}



void powerboard_sink_disable(void)
{
	unsigned int uiValue;
	HOSTDEF(ptHifIoCtrlArea);


	uiValue  = ptHifIoCtrlArea->aulHif_pio_out[0];
	uiValue |= ENABLE_SINK;	// Disable Sink
	ptHifIoCtrlArea->aulHif_pio_out[0] = uiValue;

	powerboard_sink_set_current(0);
}



void powerboard_sink_set_current(unsigned int uiSinkCurrent)
{
	write_dac_value(uiSinkCurrent);
	ulLastSinkCurrentValue = uiSinkCurrent;
}



unsigned long powerboard_sink_get_dac(void)
{
	return ulLastSinkCurrentValue;
}
