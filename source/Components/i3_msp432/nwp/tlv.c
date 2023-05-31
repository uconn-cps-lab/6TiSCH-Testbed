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

#include "tlv.h"

void convertSensorData2TLVs(SensorCommand_t * sensorCommand, SensorDataPack_t * sensorData,
                            HCT_TLV_t * tlvs, uint8_t * numTLVs, uint16_t * lenTLVs)
{
    uint8_t pos = *numTLVs;

    /* Temperature */
    if (sensorCommand->tmp007)
    {
        tlvs[pos].type = HCT_TLV_TYPE_TEMP;
        tlvs[pos].length = sizeof(sensorData->objectTemperature);
        tlvs[pos].value = (uint8_t*) &sensorData->objectTemperature;
        pos++;
        (*numTLVs)++;
        (*lenTLVs) += 2 + sizeof(sensorData->objectTemperature);
    }
    else
    {
        /* Use ambient temperature from pressure sensor*/
        tlvs[pos].type = HCT_TLV_TYPE_TEMP;
        tlvs[pos].length = sizeof(sensorData->ambientTemperature2);
        tlvs[pos].value = (uint8_t*) &sensorData->ambientTemperature2;
        pos++;
        (*numTLVs)++;
        (*lenTLVs) += 2 + sizeof(sensorData->ambientTemperature2);
    }

    /* Light */
    if (sensorCommand->opt3001)
    {
        tlvs[pos].type = HCT_TLV_TYPE_LIGHT;
        tlvs[pos].length = sizeof(sensorData->light);
        tlvs[pos].value = (uint8_t*) &sensorData->light;
        pos++;
        (*numTLVs)++;
        (*lenTLVs) += 2 + sizeof(sensorData->light);
    }

    /* Pressure */
    if (sensorCommand->bmp280)
    {
        tlvs[pos].type = HCT_TLV_TYPE_PRE;
        tlvs[pos].length = sizeof(sensorData->pressure);
        tlvs[pos].value = (uint8_t*) &sensorData->pressure;
        pos++;
        (*numTLVs)++;
        (*lenTLVs) += 2 + sizeof(sensorData->pressure);
    }

    /* Humidity */
    if (sensorCommand->hdc1000)
    {
        tlvs[pos].type = HCT_TLV_TYPE_HUM;
        tlvs[pos].length =  sizeof(sensorData->humidity);
        tlvs[pos].value = (uint8_t*) &sensorData->humidity;
        pos++;
        (*numTLVs)++;
        (*lenTLVs) += 2 + sizeof(sensorData->humidity);
    }

    /* Acceleration */
   if (sensorCommand->lis2hh12)
   {
       tlvs[pos].type = HCT_TLV_TYPE_MOT;
       tlvs[pos].length = sizeof(sensorData->accelerationXYZ);
       // value points to the beginning of the array with size sizeof(sensorData->accelerationXYZ);
       tlvs[pos].value = (uint8_t*) &sensorData->accelerationXYZ;
       pos++;
       (*numTLVs)++;
       (*lenTLVs) += 2 + sizeof(sensorData->accelerationXYZ);
   }
}

void convertPowerMeasurement2TLVs(uint16_t * pwr, HCT_TLV_t * tlvs,
                                  uint8_t * numTLVs, uint16_t * lenTLVs)
{
    uint8_t pos = *numTLVs;

    /* MSP432 Power */
    tlvs[pos].type = HCT_TLV_TYPE_MSP432_POWER;
    tlvs[pos].length = 4;
    tlvs[pos].value = (uint8_t*)pwr;
    pos++;
    (*numTLVs)++;
    (*lenTLVs) += 6;

    /* Sensor Power */
    tlvs[pos].type = HCT_TLV_TYPE_SENSOR_POWER;
    tlvs[pos].length = 4;
    tlvs[pos].value = (uint8_t*)(pwr+2);
    pos++;
    (*numTLVs)++;
    (*lenTLVs) += 6;
}

