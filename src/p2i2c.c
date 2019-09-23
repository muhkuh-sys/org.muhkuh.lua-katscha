#include <string.h>
#include "p2i2c.h"
#include "netx_io_areas.h"


void p2i2c_sendByte(P2I2C_CFG_T *ptCfg, unsigned int uiData)
{
	unsigned int uiCnt;


	uiCnt = 8;
	do
	{
		p2i2c_sendData(ptCfg, uiData>>7);
		uiData <<= 1;
		--uiCnt;
	} while( uiCnt!=0 );
}


unsigned int p2i2c_receiveByte(P2I2C_CFG_T *ptCfg)
{
	unsigned int uiCnt;
	unsigned int uiData;


	uiCnt = 8;
	uiData = 0;
	do
	{
		uiData <<= 1;
		uiData |= p2i2c_receiveData(ptCfg);
		--uiCnt;
	} while( uiCnt!=0 );

	return uiData;
}


void p2i2c_sendStart(P2I2C_CFG_T *ptCfg)
{
	// send a start condition
	//      _____ 
	// clk        
	//      __    
	// data   |__ 
	//            


	// set clk to hi, data to hi
	ptCfg->pfnSetPins(P2I2C_PIN_STATE_out1, P2I2C_PIN_STATE_out1);
	// delay for one cycle
	p2i2c_delay1Cycle(ptCfg);

	// clk to hi, data to lo
	ptCfg->pfnSetPins(P2I2C_PIN_STATE_out1, P2I2C_PIN_STATE_out0);
	// delay for one cycle
	p2i2c_delay1Cycle(ptCfg);
}


void p2i2c_sendStop(P2I2C_CFG_T *ptCfg)
{
	// send a stop condition
	//      _____ 
	// clk        
	//         __ 
	// data __|   
	//            


	// set clk to hi, data to lo
	ptCfg->pfnSetPins(P2I2C_PIN_STATE_out1, P2I2C_PIN_STATE_out0);
	// delay for one cycle
	p2i2c_delay1Cycle(ptCfg);

	// clk to hi, data to hi
	ptCfg->pfnSetPins(P2I2C_PIN_STATE_out1, P2I2C_PIN_STATE_out1);
	// delay for one cycle
	p2i2c_delay1Cycle(ptCfg);
}


void p2i2c_sendData(P2I2C_CFG_T *ptCfg, unsigned int uiDataBit)
{
	P2I2C_PIN_STATE_T tDataBit;


	// send a data bit
	//         __ 
	// clk  __|   
	//            
	// data DDDDD 
	//            


	// get data bit
	tDataBit = ((uiDataBit&1)!=0)?P2I2C_PIN_STATE_out1:P2I2C_PIN_STATE_out0;

	// set clk to lo, output data
	ptCfg->pfnSetPins(P2I2C_PIN_STATE_out0, tDataBit);
	// delay for one cycle
	p2i2c_delay1Cycle(ptCfg);

	// clk to hi, output data
	ptCfg->pfnSetPins(P2I2C_PIN_STATE_out1, tDataBit);
	// delay for one cycle
	p2i2c_delay1Cycle(ptCfg);

	// clk to lo, keep data
	ptCfg->pfnSetPins(P2I2C_PIN_STATE_out0, tDataBit);
	// delay for one cycle
	p2i2c_delay1Cycle(ptCfg);	
}


void p2i2c_sendAck(P2I2C_CFG_T *ptCfg, unsigned int uiDataBit)
{
	P2I2C_PIN_STATE_T tDataBit;


	// send a data bit
	//         __ 
	// clk  __|   
	//            
	// data DDDDD 
	//            


	// get data bit
	tDataBit = ((uiDataBit&1)!=0)?P2I2C_PIN_STATE_out1:P2I2C_PIN_STATE_out0;

	// clk lo, data bit
	ptCfg->pfnSetPins(P2I2C_PIN_STATE_out0, tDataBit);
	// delay for one cycle
	p2i2c_delay1Cycle(ptCfg);

	// clk hi, data bit
	ptCfg->pfnSetPins(P2I2C_PIN_STATE_out1, tDataBit);
	// delay for one cycle
	p2i2c_delay1Cycle(ptCfg);
}


unsigned int p2i2c_receiveData(P2I2C_CFG_T *ptCfg)
{
	unsigned int uiReceivedBit;


	// receive a data bit
	//         __ 
	// clk  __|   
	//            
	// data IIISI 
	//            


	// clk lo, data input
	ptCfg->pfnSetPins(P2I2C_PIN_STATE_out0, P2I2C_PIN_STATE_input);
	// delay for one cycle
	p2i2c_delay1Cycle(ptCfg);

	// clk hi, data input
	ptCfg->pfnSetPins(P2I2C_PIN_STATE_out1, P2I2C_PIN_STATE_input);
	uiReceivedBit = ptCfg->pfnGetSda();
	// delay for one cycle
	p2i2c_delay1Cycle(ptCfg);

	// clk lo, data input
	ptCfg->pfnSetPins(P2I2C_PIN_STATE_out0, P2I2C_PIN_STATE_input);

	return uiReceivedBit;
}


void p2i2c_delay1Cycle(P2I2C_CFG_T *ptCfg)
{
	unsigned int uiDelay;

	for (uiDelay = 0;uiDelay <ptCfg->uiDelayCycles;uiDelay++)
	{
	}

}


void p2i2c_init_pins(P2I2C_CFG_T *ptCfg)
{
	//set i2c idle state
	ptCfg->pfnSetPins(P2I2C_PIN_STATE_out1, P2I2C_PIN_STATE_out1);
}
