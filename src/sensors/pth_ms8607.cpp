/*--------------------------------------------------------------------------
File   : pth_ms8607.cpp

Author : Hoang Nguyen Hoan          			Feb. 12, 2017

Desc   : MS8607 environment sensor implementation
			- Temperature, Humidity, Barometric pressure

Copyright (c) 2017, I-SYST inc., all rights reserved

Permission to use, copy, modify, and distribute this software for any purpose
with or without fee is hereby granted, provided that the above copyright
notice and this permission notice appear in all copies, and none of the
names : I-SYST or its contributors may be used to endorse or
promote products derived from this software without specific prior written
permission.

For info or contributing contact : hnhoan at i-syst dot com

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------------
Modified by          Date              Description

----------------------------------------------------------------------------*/

#include <stdint.h>
#include <string.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include "idelay.h"
#include "iopincfg.h"
#include "sensors/pth_ms8607.h"

uint8_t crc4_PT(uint16_t *pData)
{
	int cnt;
	unsigned int n_rem=0;
	unsigned char n_bit;

	pData[0]=((pData[0]) & 0x0FFF);
	pData[7]=0;

	for (cnt = 0; cnt < 16; cnt++)
	{
		if (cnt % 2 == 1)
			n_rem ^= (unsigned short) ((pData[cnt>>1]) & 0x00FF);
		else
			n_rem ^= (unsigned short) (pData[cnt>>1]>>8);

		for (n_bit = 8; n_bit > 0; n_bit--)
		{
			if (n_rem & (0x8000))
				n_rem = (n_rem << 1) ^ 0x3000;
			else
				n_rem = (n_rem << 1);
		}
	}
	n_rem= ((n_rem >> 12) & 0x000F);

	return (n_rem ^ 0x00);
}

bool PthMS8607::Init(const PTHSENSOR_CFG &CfgData, DeviceIntrf *pIntrf)
{
	SetInterface(pIntrf);
	SetDeviceAddess(CfgData.DevAddr);

	Reset();

	ReadPtProm();

	return true;
}

void PthMS8607::ReadPtProm()
{
	uint8_t cmd;

	cmd = MS8607_PROM_START_ADDR;

	for (int i = 0; i < 7; i++)
	{
		uint8_t d[2];
		vpIntrf->Tx(MS8607_PTDEV_ADDR, &cmd, 1);
		vpIntrf->Rx(MS8607_PTDEV_ADDR, d, 2);
		vPTProm[i] = ((uint16_t)d[0] << 8) | d[1];
		cmd += 2;
	}


	int crc = crc4_PT(vPTProm);
}

bool PthMS8607::StartSampling()
{
	return true;
}

/**
 * @brief Set operating mode
 *
 * @param OpMode : Operating mode
 * 					- PTHSENSOR_OPMODE_SLEEP
 * 					- PTHSENSOR_OPMODE_SINGLE
 * 					- PTHSENSOR_OPMODE_CONTINUOUS
 * @param Freq : Sampling frequency in Hz for continuous mode
 *
 * @return true- if success
 */
bool PthMS8607::SetMode(PTHSENSOR_OPMODE OpMode, uint32_t Freq)
{
	vOpMode = OpMode;
	vSampFreq = Freq;

	switch (OpMode)
	{
		case PTHSENSOR_OPMODE_SLEEP:
			break;
		case PTHSENSOR_OPMODE_SINGLE:
			break;
		case PTHSENSOR_OPMODE_CONTINUOUS:
			break;
	}

	StartSampling();

	return true;
}

bool PthMS8607::Enable()
{
	SetMode(PTHSENSOR_OPMODE_CONTINUOUS, vSampFreq);

	return true;
}

void PthMS8607::Disable()
{
	SetMode(PTHSENSOR_OPMODE_SLEEP, 0);
}

void PthMS8607::Reset()
{
	uint8_t cmd;

	cmd = MS8607_CMD_PT_RESET;
	vpIntrf->Tx(MS8607_PTDEV_ADDR, &cmd, 1);

	cmd = MS8607_CMD_RH_RESET;
	vpIntrf->Tx(MS8607_RHDEV_ADDR, &cmd, 1);
}

bool PthMS8607::ReadPTH(PTHSENSOR_DATA &PthData)
{
	bool retval = false;

	ReadTemperature();
	ReadPressure();
	ReadHumidity();

	PthData.Humidity = (int16_t)vCurRelHum;
	PthData.Pressure = (uint32_t)vCurBarPres;
	PthData.Temperature = vCurTemp;

	return retval;
}

float PthMS8607::ReadTemperature()
{
	uint8_t cmd = MS8607_CMD_T_CONVERT_D2_256;
	uint32_t raw = 0;
	uint8_t d[4];

	vpIntrf->Tx(MS8607_PTDEV_ADDR, &cmd, 1);

	// 560 usec conversion time for res 256
	usDelay(600);

	cmd = MS8607_CMD_ADC_READ;
	vpIntrf->Tx(MS8607_PTDEV_ADDR, &cmd, 1);
	int c = vpIntrf->Rx(MS8607_PTDEV_ADDR, d, 3);
	if ( c > 0)
	{
		uint64_t t2;

		raw = ((uint32_t)d[0] << 16) + ((uint32_t)d[1] << 8) + d[3];
		vCurDT = raw - ((int32_t)vPTProm[5] << 8L);
		vCurTemp = 2000L + (((uint64_t)vCurDT * (int64_t)vPTProm[6]) >> 23LL);

		// Second order conversion
		if (vCurTemp < 2000)
		{
			t2 = (3LL * vCurDT * vCurDT) >> 33LL;
		}
		else
		{
			t2 = (5LL * vCurDT * vCurDT) >> 38LL;
		}
		vCurTemp -= t2;
	}

	return (float)vCurTemp / 100.0;
}

float PthMS8607::ReadPressure()
{
	uint8_t cmd = MS8607_CMD_P_CONVERT_D1_256;
	uint32_t raw = 0;
	uint8_t d[4];

	vpIntrf->Tx(MS8607_PTDEV_ADDR, &cmd, 1);

	// 560 usec coversion time for res 256
	usDelay(600);

	cmd = MS8607_CMD_ADC_READ;
	vpIntrf->Tx(MS8607_PTDEV_ADDR, &cmd, 1);
	int c = vpIntrf->Rx(MS8607_PTDEV_ADDR, d, 3);
	if (c > 0)
	{
		raw = ((uint32_t)d[0] << 16) + ((uint32_t)d[1] << 8) + d[3];

		int64_t off  = ((int64_t)vPTProm[2] << 17LL) + (((int64_t)vPTProm[4] * vCurDT) >> 6LL);
		int64_t sens = ((int64_t)vPTProm[1] << 16LL) + (((int64_t)vPTProm[3] * vCurDT) >> 7LL);
		int64_t t2;
		int64_t off2, sens2;

		// Second order compensation
		if (vCurTemp < 2000)
		{
			int64_t tx2 = (vCurTemp - 2000) * (vCurTemp - 2000);
			off2 = (61LL * tx2) >> 4LL;
			sens2 = (29LL * tx2) >> 4LL;

			if (vCurTemp < 1500)
			{
				int64_t tx1 = (vCurTemp + 1500) * (vCurTemp + 1500);
				off2 += 16LL * tx1;
				sens2 += 8LL * tx1;
			}
		}
		else
		{
			off2 = 0;
			sens2 = 0;
		}

		off -= off2;
		sens -= sens2;

		// pressure in mBar (1 mBar = 100 Pascal)
		int64_t p = (((int64_t)raw * (sens >> 21LL) - off) >> 15LL);

		vCurBarPres = p * 100;
	}

	// pressur in Pascal
	return (float)vCurBarPres / 100.0;
}

float PthMS8607::ReadHumidity()
{
	uint8_t cmd = MS8607_CMD_RH_HOLD_MASTER;
	uint32_t raw = 0;
	uint8_t d[4];

	vpIntrf->Tx(MS8607_RHDEV_ADDR, &cmd, 1);
	int count = vpIntrf->Rx(MS8607_RHDEV_ADDR, (uint8_t*)d, 3);

	raw = ((uint32_t)d[0] << 8) + (d[1] & 0xFC);

	if ((d[1] & 0x3) == 2)
	{
		int32_t rh = ((12500L * raw) >> 16L) - 600L;

		rh = rh + ((2000 - vCurTemp) * (-18)) / 100;

		vCurRelHum = rh;
	}

	 return (float)vCurRelHum / 100.0;
}

