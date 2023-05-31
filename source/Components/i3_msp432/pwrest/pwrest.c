/*
 * Copyright (c) 2016, Texas Instruments Incorporated
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
/*
 *  ====================== pwrest.c =============================================
 *  Power estimation module 
 */

/* -----------------------------------------------------------------------------
*  Includes
* ------------------------------------------------------------------------------
*/
#include <stdbool.h>
#include <string.h>
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include "pwrest.h"

#include "SensorBmp280.h"
#include "SensorHdc1000.h"
/* -----------------------------------------------------------------------------
*  Local Variables
* ------------------------------------------------------------------------------
*/
static PWREST_TimeMeasurement_t timeMeasure;
static uint32_t activeCurrent[PWREST_TYPE_TOTAL];
static uint32_t sleepCurrent[PWREST_TYPE_TOTAL];

/* -----------------------------------------------------------------------------
*  Public functions
* ------------------------------------------------------------------------------
*/

/***************************************************************************//**
* @brief      This function initializes active and sleep current.
*
* @param[in]   none

* @return      none
******************************************************************************/
void PWREST_init(void)
{
    memset(&timeMeasure, 0 , sizeof(PWREST_TimeMeasurement_t));

    activeCurrent[PWREST_TYPE_CPU] = MSP432_CPU_ACTIVE_CUR;
    sleepCurrent[PWREST_TYPE_CPU] = MSP432_CPU_SLEEP_CUR;

    activeCurrent[PWREST_TYPE_SENSORS_TMP] = TMP007_ACTIVE_CUR;
    activeCurrent[PWREST_TYPE_SENSORS_HUM] = HDC1080_ACTIVE_CUR;
    activeCurrent[PWREST_TYPE_SENSORS_LIGHT] = OPT3001_ACTIVE_CUR;
    activeCurrent[PWREST_TYPE_SENSORS_PRES] = BMP280_ACTIVE_CUR;
    activeCurrent[PWREST_TYPE_SENSORS_MOTION] = LIS2HH12_ACTIVE_CUR ;
    sleepCurrent[PWREST_TYPE_SENSORS_TMP] = TMP007_SLEEP_CUR;
    sleepCurrent[PWREST_TYPE_SENSORS_HUM] = HDC1080_SLEEP_CUR;
    sleepCurrent[PWREST_TYPE_SENSORS_LIGHT] = OPT3001_SLEEP_CUR;
    sleepCurrent[PWREST_TYPE_SENSORS_PRES] = BMP280_SLEEP_CUR;
    sleepCurrent[PWREST_TYPE_SENSORS_MOTION] = LIS2HH12_SLEEP_CUR;
}

/***************************************************************************//**
* @brief      This functions records the on time of a specific component
*
* @param[in]   type - power estimation type
*
* @return      none
******************************************************************************/
void PWREST_on(uint8_t type)
{
  if (type < PWREST_TYPE_MAX)
  {
    timeMeasure.lastOnTime[type] = Clock_getTicks();  
  }
}
/***************************************************************************//**
* @brief      This functions records the on time of a specific component
*
* @param[in]   type - power estimation type
*
* @return      none
******************************************************************************/
void PWREST_off(uint8_t type)
{
    if (type < PWREST_TYPE_MAX)
    {
        uint32_t offTime = Clock_getTicks();

        if ((timeMeasure.lastOnTime[type] > 0)
            && (offTime > timeMeasure.lastOnTime[type]))
    	{
            timeMeasure.totalOnTime[type] += (offTime -
                                              timeMeasure.lastOnTime[type]);
            timeMeasure.lastOnTime[type] = 0;
        }
    }
}

/***************************************************************************//**
* @brief      This functions calculates the average current consumption
*
* @param[out]  pPwrMeasure - pointer to power measuremnet
*
* @return      none
******************************************************************************/
void PWREST_calAvgCur(PWREST_PwrMeasurement_t *pPwrMeasure)
{
    if (timeMeasure.totalOnTime[PWREST_TYPE_TOTAL] > 0 && pPwrMeasure != NULL)
    {
        int8_t i;
        uint16_t sleepDutyCycle;

        for(i = PWREST_TYPE_CPU; i < PWREST_TYPE_TOTAL; i++)
        {
            pPwrMeasure->dutyCycle[i] =
                (uint16_t)((uint64_t)DUTY_CYCLE_MAX*timeMeasure.totalOnTime[i]
                           /timeMeasure.totalOnTime[PWREST_TYPE_TOTAL]);

            /* MSP432 WDT tick Period is too big for measurement of certain sensor type.
             * Light, humidity, and pressure sensors are able to sleep automatically
             * after conversion complete. Need to include the actual active time.
             * */
            if ((i == PWREST_TYPE_SENSORS_PRES) &&
                    (SENSOR_ACTIVE_RATIO(BMP280_CONVERSION_TIME)> 0))
            {
                pPwrMeasure->dutyCycle[i] /= SENSOR_ACTIVE_RATIO(BMP280_CONVERSION_TIME);
            }

            if ((i == PWREST_TYPE_SENSORS_HUM) &&
                    (SENSOR_ACTIVE_RATIO(HDC1000_CONVERSION_TIME)> 0))
            {
                pPwrMeasure->dutyCycle[i] /= SENSOR_ACTIVE_RATIO(HDC1000_CONVERSION_TIME);
            }

            sleepDutyCycle = DUTY_CYCLE_MAX - pPwrMeasure->dutyCycle[i];

            /*DC-DC efficiency in I3 is 80%*/
            pPwrMeasure->avgActiveCur[i] = (uint32_t) (activeCurrent[i]
                                     *pPwrMeasure->dutyCycle[i]/DUTY_CYCLE_MAX*10/8);

            pPwrMeasure->avgSleepCur[i] = (uint32_t)(sleepCurrent[i]
                                                *sleepDutyCycle/DUTY_CYCLE_MAX*10/8);
        }
        
        memset(&timeMeasure, 0 , sizeof(PWREST_TimeMeasurement_t));
    }
}
