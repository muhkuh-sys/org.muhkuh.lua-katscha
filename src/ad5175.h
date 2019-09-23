#include "p2i2c.h"

#ifndef __AD5175_H__
#define __AD5175_H__

typedef enum AD5175_COMMAND_ENUM
{
	AD5175_COMMAND_NOP                 = 0,
	AD5175_COMMAND_WriteRDAC           = 1,
	AD5175_COMMAND_ReadRDAC            = 2,
	AD5175_COMMAND_StoreRDAC           = 3,
	AD5175_COMMAND_SoftReset           = 4,
	AD5175_COMMAND_Read50TP            = 5,
	AD5175_COMMAND_ReadLast50TPAddress = 6,
	AD5175_COMMAND_WriteControl        = 7,
	AD5175_COMMAND_ReadControl         = 8,
	AD5175_COMMAND_SetShutdownMode     = 9
} AD5175_COMMAND_T;


#define AD5175_MSK_Control_50TP_program_enable     0x1U
#define AD5175_SRT_Control_50TP_program_enable     0
#define AD5175_MSK_Control_RDAC_register_writeable 0x2U
#define AD5175_SRT_Control_RDAC_register_writeable 1
#define AD5175_MSK_Control_50TP_program_success    0x4U
#define AD5175_SRT_Control_50TP_program_success    2


int AD5175_write(P2I2C_CFG_T *ptCfg, unsigned long ulData);
int AD5175_read(P2I2C_CFG_T *ptCfg, unsigned long *pulData);

int AD5175_initialize(P2I2C_CFG_T *ptCfg);
int AD5175_set_position(P2I2C_CFG_T *ptCfg, unsigned long ulPosition);


#endif  /* __AD5175_H__ */
