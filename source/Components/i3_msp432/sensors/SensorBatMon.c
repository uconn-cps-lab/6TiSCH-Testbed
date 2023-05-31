/*
* Copyright (c) 2015-2016, Texas Instruments Incorporated
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*
* *  Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
* *  Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* *  Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
* THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/** ============================================================================
*  @file       SensorBatMon.c
*
*  @brief      Driver for CC13XX/CCC26XX Internal Battery Monitor and 
*              Tempeprature Sensor
*  ============================================================================
*/

/* -----------------------------------------------------------------------------
*  Includes
* ------------------------------------------------------------------------------
*/
//#include "aon_batmon.h"
#include "SensorBatMon.h"

/* -----------------------------------------------------------------------------
*  Public functions
* ------------------------------------------------------------------------------
*/

/*******************************************************************************
* @fn          SensorBatmon_init
*
* @brief       Enables the battey monitor regsiter
*
* @return      true if success
*/
bool SensorBatmon_init(void)
{
  AONBatMonEnable();
  return true;
}

/*******************************************************************************
* @fn         SensorBatmon_oneShotRead
*
* @brief       Read the sensor voltage and temperature registers in CC26XX
*
* @param       data - buffer for temperature and battery voltage
*
* @return      TRUE if valid data
*/
bool SensorBatmon_oneShotRead(SAL_sensorBatData_t* batData)
{
  int32_t temp;
  uint32_t bat;
  
  temp = AONBatMonTemperatureGetDegC();
  bat = AONBatMonBatteryVoltageGet();
  
  batData->temp = temp*25;     
  batData->bat = bat*125/320; 
  
  return true;
}
