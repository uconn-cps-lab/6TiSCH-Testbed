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
 *  @file       SensorLis2DW12.c
 *
 *  @brief      Driver for STMicroelectronics Lis2DW12 saccelerometer .
 *  ============================================================================
 */
#include "Board.h"
#include "SensorUtil.h"
#include "SensorI2C.h"
#include "pwrest/pwrest.h"

/* Control registers*/
#define CTRL1                         0x20
#define CTRL2                         0x21
#define CTRL3                         0x22
#define CTRL4                         0x23
#define CTRL5                         0x24

/* Status register*/
#define STATUS                        0x27
/* X,Y,Z axis output registers*/
#define OUT_X_L                       0x28
#define OUT_Y_L                       0x2A
#define OUT_Z_L                       0x2C

/* Values CTRL1 */
#define MODE_INIT                     0x68   
/* Values CTRL3 */
#define TRIG_MEA                      0x03 

#define LIS2DW12_MEAS_SIZE             6
#define LIS2DW12_CONVERSION_TIME       10

#define SENSOR_SELECT()               SensorI2C_select(SENSOR_I2C_0,Board_LIS2DW12_ADDR)
#define SENSOR_DESELECT()             SensorI2C_deselect()



/*******************************************************************************
 * @fn          SensorLIS2DW12_init
 *
 * @brief       Initialize the sensor
 *
 * @return      true if success
 */
bool SensorLIS2DW12_init(void)
{
    bool res;
    uint8_t val;

    if (!SENSOR_SELECT())
    {
        return false;
    }
    
    val = MODE_INIT;
    res = SensorI2C_writeReg(CTRL1, &val,1);

    return res;
}

/*******************************************************************************
 * @fn          SensorLis2DW12_oneShotRead
 *
 * @brief       Do one shot read of acceleration sensor and return to sleep
 *
 * @param       lis2hh12Data - acceleration sensor values
 *
 * @return      true if valid data
 ******************************************************************************/
bool SensorLis2DW12_oneShotRead(SENSOR_MotionData_t* lis2dw12Data)
{
  bool success;
  uint8_t val;
  uint8_t data[LIS2DW12_MEAS_SIZE];
  
  SENSOR_SELECT();
  PWREST_on(PWREST_TYPE_SENSORS_MOTION);
  
  val = TRIG_MEA;
  SensorI2C_writeReg(CTRL3, &val, 1);
  
  DELAY_MS(LIS2DW12_CONVERSION_TIME);
  
  success = SensorI2C_readReg(STATUS, &val, 1);
  
  if ((val&0x1) == 0x1)
  {
    success = SensorI2C_readReg(OUT_X_L ,(uint8_t*)data,LIS2DW12_MEAS_SIZE);
    
    /* default full scale selection 2g
    61 ug/digit = 0.061 mg/digit = 0.000061 g/digit */
    
    if (success)
    {
      lis2dw12Data->accelx = ((int16_t)(data[0] | data[1]<<8))*61/10000;
      lis2dw12Data->accely = ((int16_t)(data[2] | data[3]<<8))*61/10000;
      lis2dw12Data->accelz = ((int16_t)(data[4] | data[5]<<8))*61/10000;
    }
    else
    {
      lis2dw12Data->accelx = 0;
      lis2dw12Data->accely = 0;
      lis2dw12Data->accelz = 0;
    }
  
    lis2dw12Data->gyrox = 0;
    lis2dw12Data->gyroy = 0;
    lis2dw12Data->gyroz = 0;
    lis2dw12Data->temp = 0;
    
  }
    
  PWREST_off(PWREST_TYPE_SENSORS_MOTION);
  SENSOR_DESELECT();
  
  return success;
}