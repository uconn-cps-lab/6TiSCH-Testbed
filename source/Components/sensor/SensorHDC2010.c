/*
 * Copyright (c) 2017, Texas Instruments Incorporated
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
 *  @file       SensorHdc2010.c
 *
 *  @brief      Driver for Texas Instruments HCD2010 humidity sensor. 
 *  ============================================================================
 */

/* -----------------------------------------------------------------------------
*  Includes
* ------------------------------------------------------------------------------
*/
#include "Board.h"
#include "SensorHdc2010.h"
#include "SensorUtil.h"
#include "SensorI2C.h"
#include "pwrest/pwrest.h"

/* -----------------------------------------------------------------------------
*  Constants and macros
* ------------------------------------------------------------------------------
*/
// Registers
#define HDC2010_REG_TEMP           0x00 // Temperature
#define HDC2010_REG_HUM            0x02 // Humidity
#define HDC2010_REG_DRDY           0x04 // data Ready
#define HDC2010_REG_INT_CONFIG     0x0E // Interrupt Configuration
#define HDC2010_REG_MEA_CONFIG     0x0F // Measurment Configuration

/* 8 bit resoultion temp, 8 bit resolution humidity*/
#define HDC2010_START_MEA          0xA1   // Start measurment
/* 2.50 ms for hum, 3.65 ms for temp*/
#define HDC2010_CONVERSION_TIME    13

// Sensor selection/de-selection
#define SENSOR_SELECT()     SensorI2C_select(SENSOR_I2C_0,Board_HDC2010_ADDR)
#define SENSOR_DESELECT()   SensorI2C_deselect()

/* -----------------------------------------------------------------------------
*  Public functions
* ------------------------------------------------------------------------------
*/

/*******************************************************************************
* @fn          SensorHdc2010_oneShotRead
*
* @brief       Do one shot read of HDC2010 humidity sensor and return to sleep
*
* @param       hdc2010Data - humidity and temperature sensor values
*
* @return      true if valid data
*/
bool SensorHdc2010_oneShotRead(SENSOR_HumData_t* hdc2010Data)
{
  bool success=false;
  uint8_t val;
  uint16_t rawHum;
  uint16_t rawTemp;
  
  if (!SENSOR_SELECT())
  {
    return false;
  }

  PWREST_on(PWREST_TYPE_SENSORS_HUM);
  val = HDC2010_START_MEA;
   
  if (SensorI2C_writeReg(HDC2010_REG_MEA_CONFIG,&val,1))
  {
     DELAY_MS(HDC2010_CONVERSION_TIME);
     success = SensorI2C_readReg(HDC2010_REG_TEMP,(uint8_t*)&rawTemp,2);
     success = SensorI2C_readReg(HDC2010_REG_HUM,(uint8_t*)&rawHum,2);
  }
  
  PWREST_off(PWREST_TYPE_SENSORS_HUM);
  SENSOR_DESELECT();
  
  if(success)
  {
    //-- calculate temperature [°C]
    hdc2010Data->temp = (int16_t)((((double)rawTemp/ 65536)*165 - 40)*10);
    //-- calculate relative humidity [%RH]
    hdc2010Data->hum = (uint8_t)((double)rawHum/65536*100);
  }
  else
  {
    hdc2010Data->temp = 0;
    hdc2010Data->hum = 0;
  }

  return success;
}