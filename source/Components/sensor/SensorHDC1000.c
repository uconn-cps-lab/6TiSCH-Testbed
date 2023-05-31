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
 *  @file       SensorHdc1000.c
 *
 *  @brief      Driver for Texas Instruments HCD1000 humidity sensor.
 *  ============================================================================
 */

/* -----------------------------------------------------------------------------
*  Includes
* ------------------------------------------------------------------------------
*/
#include "Board.h"
#include "SensorHdc1000.h"
#include "SensorUtil.h"
#include "SensorI2C.h"
#include "pwrest/pwrest.h"

/* -----------------------------------------------------------------------------
*  Constants and macros
* ------------------------------------------------------------------------------
*/
// Registers
#define HDC1000_REG_TEMP           0x00 // Temperature
#define HDC1000_REG_HUM            0x01 // Humidity
#define HDC1000_REG_CONFIG         0x02 // Configuration
#define HDC1000_REG_SERID_H        0xFB // Serial ID high
#define HDC1000_REG_SERID_M        0xFC // Serial ID middle
#define HDC1000_REG_SERID_L        0xFD // Serial ID low
#define HDC1000_REG_MANF_ID        0xFE // Manufacturer ID
#define HDC1000_REG_DEV_ID         0xFF // Device ID

// Fixed values
#define HDC1000_VAL_MANF_ID        0x5449
#define HDC1000_VAL_DEV_ID         0x1000

//#define HDC1000_VAL_CONFIG         0x1000 // 14 bit, acquired in sequence

/* 11 bit resoultion temp, 8 bit resolution humidity*/
#define HDC1000_VAL_CONFIG       0x1600
/* 2.50 ms for hum, 3.65 ms for temp*/
#define HDC1000_CONVERSION_TIME  13

// Sensor selection/de-selection
#define SENSOR_SELECT()     SensorI2C_select(SENSOR_I2C_0,Board_HDC1000_ADDR)
#define SENSOR_DESELECT()   SensorI2C_deselect()

uint16_t hum_dbg;

/* -----------------------------------------------------------------------------
*  Type Definitions
* ------------------------------------------------------------------------------
*/
typedef struct
{
    uint16_t temp;
    uint16_t hum;
} SensorData_t;

/* -----------------------------------------------------------------------------
*  Local Variables
* ------------------------------------------------------------------------------
*/
static bool success;
static SensorData_t data;

/* -----------------------------------------------------------------------------
*  Public functions
* ------------------------------------------------------------------------------
*/

/*******************************************************************************
* @fn          SensorHdc1000_init
*
* @brief       Initialize the humidity sensor driver
*
* @return      true if I2C operation successful
*******************************************************************************/
bool SensorHdc1000_init(void)
{
    uint16_t val;

    if (!SENSOR_SELECT())
    {
        return false;
    }

    // Enable reading data in one operation
    val = SWAP(HDC1000_VAL_CONFIG);
    success = SensorI2C_writeReg(HDC1000_REG_CONFIG, (uint8_t*)&val, 2);

    SENSOR_DESELECT();

    return success;
}

/*******************************************************************************
* @fn          SensorHdc1000_start
*
* @brief       Start measurement
*
* @return      none
*/
void SensorHdc1000_start(void)
{
    if (success)
    {
        uint8_t val;

        if (!SENSOR_SELECT())
        {
            return;
        }

        val = HDC1000_REG_TEMP;
        success = SensorI2C_write(&val, sizeof(val));

        SENSOR_DESELECT();
    }
}


/*******************************************************************************
* @fn          SensorHdc1000_read
*
* @brief       Get humidity sensor data
*
* @param       rawTemp - raw temperature value
*
* @param       rawHum - raw humidity value
*
* @return      true if I2C operations successful
*/
bool SensorHdc1000_read(uint16_t *rawTemp, uint16_t *rawHum)
{
    bool valid;

    if (success)
    {
        if (!SENSOR_SELECT())
        {
            return false;
        }

        success = SensorI2C_read((uint8_t*)&data, sizeof(data));
        SENSOR_DESELECT();

        // Store temperature
        *rawTemp = SWAP(data.temp);

        // Store humidity
        *rawHum = SWAP(data.hum);
    }

    valid = success;
    success = true; // Ready for next cycle

    return valid;
}

/*******************************************************************************
* @fn          SensorHdc1000_convert
*
* @brief       Convert raw data to temperature and humidity
*
* @param       rawTemp - raw temperature value
*
* @param       rawHum - raw humidity value
*
* @param       temp - converted temperature
*
* @param       hum - converted humidity
*
* @return      none
*******************************************************************************/
void SensorHdc1000_convert(uint16_t rawTemp, uint16_t rawHum,
                          float *temp, float *hum)
{
    //-- calculate temperature [°C]
    *temp = ((double)(int16_t)rawTemp / 65536)*165 - 40;

    //-- calculate relative humidity [%RH]
    *hum = ((double)rawHum / 65536)*100;
}


/*******************************************************************************
* @fn          SensorHdc1000_test
*
* @brief       Humidity sensor self test
*
* @return      true if test passes
*******************************************************************************/
bool SensorHdc1000_test(void)
{
    uint16_t val;

    SENSOR_SELECT();

    // Verify manufacturer ID
    ST_ASSERT(SensorI2C_readReg(HDC1000_REG_MANF_ID,(uint8_t*)&val,2));
    val = SWAP(val);
    ST_ASSERT(val == HDC1000_VAL_MANF_ID);

    // Verify device ID
    ST_ASSERT(SensorI2C_readReg(HDC1000_REG_DEV_ID,(uint8_t*)&val,2));
    val = SWAP(val);
    ST_ASSERT(val == HDC1000_VAL_DEV_ID);

    SENSOR_DESELECT();

    return true;
}

/*******************************************************************************
* @fn          SensorHdc1000_oneShotRead
*
* @brief       Do one shot read of HDC1000 humidity sensor and return to sleep
*
* @param       hdc1000Data - humidity and temperature sensor values
*
* @return      true if valid data
*/
bool SensorHdc1000_oneShotRead(SENSOR_HumData_t* hdc1000Data)
{

  bool success;
  uint8_t val;
  SENSOR_HumData_t rawData;
  
  if (!SENSOR_SELECT())
  {
    return false;
  }
  
  SensorI2C_writeReg(HDC1000_REG_CONFIG, (uint8_t*)&val, 2);
  DELAY_MS(HDC1000_CONVERSION_TIME);
  
  
  PWREST_on(PWREST_TYPE_SENSORS_HUM);
  val = HDC1000_REG_HUM;
  success = SensorI2C_write(&val, sizeof(val));
  
  if(success)
  {
    DELAY_MS(HDC1000_CONVERSION_TIME);
    success = SensorI2C_read((uint8_t*)&rawData,sizeof(rawData));
    rawData.temp = SWAP(rawData.temp);
    rawData.hum= SWAP(rawData.hum);
  }
  
  PWREST_off(PWREST_TYPE_SENSORS_HUM);
  SENSOR_DESELECT();
 
  if(success)
  {
    //-- calculate temperature [°C]
    hdc1000Data->temp = (int16_t)(((double)(int16_t)rawData.temp/ 65536)*165 - 40);
    //-- calculate relative humidity [%RH]
    hdc1000Data->hum = (uint8_t)((double)rawData.hum/65536*100);
    hum_dbg =  hdc1000Data->hum ;
  }
//  else
//  {
//    hdc1000Data->temp = 0;
//    hdc1000Data->hum = 0;
//  }

  return success;
}
