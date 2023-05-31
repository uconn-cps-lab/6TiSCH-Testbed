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

#include "sensors.h"

/* Sensor Header files */
#include "SensorTmp007.h"
#include "SensorOpt3001.h"
#include "SensorHdc1000.h"
#include "SensorBmp280.h"
#include "SensorLIS2HH12.h"
#include "SensorUtil.h"


/*
 * Read data from available sensors and fill data structure
 */
bool readSensorsData(SensorCommand_t sensorCommand, SensorDataPack_t* sensorData)
{
    SENSOR_InfraredTempData_t tmp007;
    SENSOR_LightData_t opt3001;
    SENSOR_PressData_t bmp280;
    SENSOR_HumData_t hdc1000;
    SENSOR_MotionData_t lis2hh12;

    bool valid = true;

    /* Read thermopile */
    if (sensorCommand.tmp007)
    {
        valid = SensorTmp007_oneShotRead(&tmp007);
        /* Update sensorData */
        if (valid)
        {
            sensorData->objectTemperature = tmp007.objTemp;
            sensorData->ambientTemperature1 = tmp007.temp;
        }
    }

    /* Read light */
    if (sensorCommand.opt3001)
    {
        valid = SensorOpt3001_oneShotRead(&opt3001);
        /* Update sensorData */
        if (valid)
        {
            sensorData->light = opt3001.lux;
        }
    }

    /* Read pressure */
    if (sensorCommand.bmp280)
    {
        valid = SensorBmp280_oneShotRead(&bmp280);
        /* Update sensorData */
        if (valid)
        {
            /* Update sensorData */
            sensorData->pressure = bmp280.press;
            sensorData->ambientTemperature2 = bmp280.temp;
        }
    }

    /* Read humidity */
    if (sensorCommand.hdc1000)
    {
        valid = SensorHdc1000_oneShotRead(&hdc1000);
        /* Update sensorData */
        if (valid)
        {
            sensorData->humidity = hdc1000.hum;
            sensorData->ambientTemperature3 = hdc1000.temp;
        }
    }

    /* Read acceleration */
    if (sensorCommand.lis2hh12)
    {
        valid = SensorLis2hh12_oneShotRead(&lis2hh12);
        /* Update sensorData */
        if (valid)
        {
            sensorData->accelerationXYZ[0] = lis2hh12.accelx;
            sensorData->accelerationXYZ[1] = lis2hh12.accely;
            sensorData->accelerationXYZ[2] = lis2hh12.accelz;
        }
    }

    return valid;
}

