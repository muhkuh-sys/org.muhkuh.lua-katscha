#ifndef __SPI_H__
#define __SPI_H__

void spi_delay(unsigned int uiDelay);
void spi_init_IO_pins(void);
unsigned int  spi_get_value(void);
void spi_set_value(unsigned int uiSpiValue);


#endif	// __SPI_H__
