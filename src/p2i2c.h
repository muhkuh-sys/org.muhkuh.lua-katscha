#ifndef __P2I2C_H__
#define __P2I2C_H__

/*-------------------------------------------------------------------------*/

typedef enum P2I2C_PIN_STATE_ENUM
{
	P2I2C_PIN_STATE_out0	= 0,
	P2I2C_PIN_STATE_out1	= 1,
	P2I2C_PIN_STATE_input	= 2
} P2I2C_PIN_STATE_T;

typedef void (*PFN_P2I2C_SET_PINS) (P2I2C_PIN_STATE_T tSclState, P2I2C_PIN_STATE_T tSdaState);
typedef unsigned int (*PFN_P2I2C_GET_SDA) (void);

typedef struct P2I2C_CFG_STRUCT
{
	unsigned int uiChipAddress;
	unsigned int uiDelayCycles;

	PFN_P2I2C_SET_PINS pfnSetPins;
	PFN_P2I2C_GET_SDA pfnGetSda;
} P2I2C_CFG_T;


//-------------------------------------

// piggy's pio i2c routines
void p2i2c_init_pins(P2I2C_CFG_T *ptCfg);
void p2i2c_sendByte(P2I2C_CFG_T *ptCfg, unsigned int uiData);
unsigned int p2i2c_receiveByte(P2I2C_CFG_T *ptCfg);
void p2i2c_sendStart(P2I2C_CFG_T *ptCfg);
void p2i2c_sendStop(P2I2C_CFG_T *ptCfg);
void p2i2c_sendData(P2I2C_CFG_T *ptCfg, unsigned int uiDataBit);
void p2i2c_sendAck(P2I2C_CFG_T *ptCfg, unsigned int uiDataBit);
unsigned int p2i2c_receiveData(P2I2C_CFG_T *ptCfg);
void p2i2c_delay1Cycle(P2I2C_CFG_T *ptCfg);

//-------------------------------------

#endif	// __P2I2C_H__

