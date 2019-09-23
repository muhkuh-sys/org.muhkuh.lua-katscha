#include <string.h>

#include "ad5175.h"


int AD5175_write(P2I2C_CFG_T *ptCfg, unsigned long ulData)
{
	int iResult;
	unsigned int uiAck;
	unsigned int uiAckPoll;


	iResult = -1;

	uiAckPoll = 16384;
	do
	{
		/* Send the start condition. */
		p2i2c_sendStart(ptCfg);
		/* Send the address byte with a write condition. */
		p2i2c_sendByte(ptCfg, ptCfg->uiChipAddress<<1U);
		/* Receive the ACK bit. */
		uiAck = p2i2c_receiveData(ptCfg);
		if( uiAck==0 )
		{
			break;
		}

		/* Delay a while. */
		p2i2c_delay1Cycle(ptCfg);
		p2i2c_delay1Cycle(ptCfg);
		p2i2c_delay1Cycle(ptCfg);
		p2i2c_delay1Cycle(ptCfg);

		--uiAckPoll;
	} while( uiAckPoll>0 );
	if( uiAck==0 )
	{
		/* Send the data MSB. */
		p2i2c_sendByte(ptCfg, (ulData & 0xFF00U)>>8U);
		/* Receive the ACK bit. */
		uiAck = p2i2c_receiveData(ptCfg);
		if( uiAck==0 )
		{
			/* Send the data LSB. */
			p2i2c_sendByte(ptCfg, ulData & 0x00FFU);
			/* Receive the ACK bit. */
			uiAck = p2i2c_receiveData(ptCfg);
			if( uiAck==0 )
			{
				iResult = 0;
			}
		}
	}

	/* Always send a stop condition. */
	p2i2c_sendStop(ptCfg);

	/* Delay a while. */
	p2i2c_delay1Cycle(ptCfg);
	p2i2c_delay1Cycle(ptCfg);
	p2i2c_delay1Cycle(ptCfg);
	p2i2c_delay1Cycle(ptCfg);

	return iResult;
}

int AD5175_read(P2I2C_CFG_T *ptCfg, unsigned long *pulData)
{
	int iResult;
	unsigned int uiAck;
	unsigned long ulData;
	unsigned int uiAckPoll;


	iResult = -1;

	uiAckPoll = 16384;
	do
	{
		/* Send a start condition. */
		p2i2c_sendStart(ptCfg);
		/* Send the address byte with a read condition. */
		p2i2c_sendByte(ptCfg, (ptCfg->uiChipAddress<<1U)|1U);
		/* Receive the ACK bit. */
		uiAck = p2i2c_receiveData(ptCfg);
		if( uiAck==0 )
		{
			break;
		}

		/* Delay a while. */
		p2i2c_delay1Cycle(ptCfg);
		p2i2c_delay1Cycle(ptCfg);
		p2i2c_delay1Cycle(ptCfg);
		p2i2c_delay1Cycle(ptCfg);

		--uiAckPoll;
	} while( uiAckPoll>0 );
	if( uiAck==0 )
	{
		/* Receive the MSB of the data. */
		ulData = p2i2c_receiveByte(ptCfg) << 8U;
		/* Send the ACK bit. */
		p2i2c_sendAck(ptCfg, 0);

		/* Receive the LSB of the data. */
		ulData |= p2i2c_receiveByte(ptCfg);
		/* Send the ACK bit. */
		p2i2c_sendAck(ptCfg, 0);

		/* Send the stop condition. */
		p2i2c_sendStop(ptCfg);

		/* Delay a while. */
		p2i2c_delay1Cycle(ptCfg);
		p2i2c_delay1Cycle(ptCfg);
		p2i2c_delay1Cycle(ptCfg);
		p2i2c_delay1Cycle(ptCfg);

		if( pulData!=NULL )
		{
			*pulData = ulData;
		}

		iResult = 0;
	}

	return iResult;
}



int AD5175_initialize(P2I2C_CFG_T *ptCfg)
{
	int iResult;
	unsigned long ulControl;
	unsigned long ulValue;


	/* Allow write access to the RDAC register. */
	ulControl = AD5175_MSK_Control_RDAC_register_writeable;

	/* Write the value to the control register. */
	ulValue  = AD5175_COMMAND_WriteControl << 10U;
	ulValue |= ulControl;
	iResult = AD5175_write(ptCfg, ulValue);
	if( iResult==0 )
	{
		/* Read back the control register. */
		ulValue  = AD5175_COMMAND_ReadControl << 10U;
		iResult = AD5175_write(ptCfg, ulValue);
		if( iResult==0 )
		{
			iResult = AD5175_read(ptCfg, &ulValue);
			if( iResult==0 )
			{
				if( ulValue!=ulControl )
				{
					iResult = -1;
				}
			}
		}
	}

	return iResult;
}



int AD5175_set_position(P2I2C_CFG_T *ptCfg, unsigned long ulPosition)
{
	int iResult;
	unsigned long ulValue;


	/* The position is a value in the range [0 .. 1023]. */
	if( ulPosition>1023 )
	{
		iResult = -1;
	}
	else
	{
		/* Write the position. */
		ulValue  = AD5175_COMMAND_WriteRDAC << 10U;
		ulValue |= ulPosition;
		iResult = AD5175_write(ptCfg, ulValue);
		if( iResult==0 )
		{
			ulValue  = AD5175_COMMAND_ReadRDAC << 10U;
			iResult = AD5175_write(ptCfg, ulValue);
			if( iResult==0 )
			{
				iResult = AD5175_read(ptCfg, &ulValue);
				if( iResult==0 )
				{
					if( ulValue!=ulPosition )
					{
						iResult = -1;
					}
				}
			}
		}
	}

	return iResult;
}
