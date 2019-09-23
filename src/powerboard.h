#include "p2i2c.h"

#ifndef __POWERBOARD_H__
#define __POWERBOARD_H__

#define DELAY_CYCLES 		10

#define SPI_MSK_CS_0 		0x00000200U
#define SPI_SRT_CS_0 		9
#define SPI_MSK_CS_1 		0x00000400U
#define SPI_SRT_CS_1 		10
#define SPI_MSK_CS_2 		0x00000800U
#define SPI_SRT_CS_2 		11

#define SPI_MSK_CLK 		0x00000100U
#define SPI_SRT_CLK 		8
#define SPI_MSK_MISO 		0x00001000U
#define SPI_SRT_MISO 		12
#define SPI_MSK_MOSI 		0x00002000U
#define SPI_SRT_MOSI 		13

#define ENABLE_SOURCE 	0x00000010U
#define ENABLE_SINK 		0x00000020U

#define AD5175_I2C_DELAY_CYCLES 1
#define AD5175_ADDRESS 0x2FU


typedef enum TEST_RESULT_ENUM
{
	TEST_RESULT_OK = 0,
	TEST_RESULT_ERROR = 1
} TEST_RESULT_T;


int powerboard_initialize(void);

void powerboard_source_enable(void);
void powerboard_source_disable(void);
int powerboard_source_set_voltage(unsigned int uiPWMvalue);
int powerboard_source_set_max_current(unsigned int uiMaxCurrent);
int powerboard_source_get_max_current(unsigned long *pulValue);
unsigned int powerboard_source_get_voltage(void);
unsigned int powerboard_source_get_current(void);
unsigned long powerboard_source_get_pwm(void);

void powerboard_sink_enable(void);
void powerboard_sink_disable(void);
void powerboard_sink_set_current(unsigned int uiSinkCurrent);
unsigned int powerboard_sink_get_current(void);
unsigned long powerboard_sink_get_dac(void);

#endif	// __POWERBOARD_H__
