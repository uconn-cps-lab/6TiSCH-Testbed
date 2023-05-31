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
#include "pwrest.h"
#include <string.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include "sys/clock.h"
/* -----------------------------------------------------------------------------
*  Local Variables
* ------------------------------------------------------------------------------
*/
static PWREST_TimeMeasurement_t timeMeasure;
static uint32_t activeCurrent[PWREST_TYPE_TOTAL];
static uint32_t sleepCurrent[PWREST_TYPE_TOTAL];
static uint32_t rfSlotCount[PWREST_RF_SLOT_MAX];
static uint32_t totalTXBytes, totalRXBytes;


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
    memset(rfSlotCount, 0 , sizeof(rfSlotCount));
    totalRXBytes = 0;
    totalTXBytes = 0;

    activeCurrent[PWREST_TYPE_CPU] = CC2650_CPU_ACTIVE_CUR;
    sleepCurrent[PWREST_TYPE_CPU] = CC2650_CPU_SLEEP_CUR;

    activeCurrent[PWREST_TYPE_SENSORS_TMP] = TMP007_ACTIVE_CUR;
    activeCurrent[PWREST_TYPE_SENSORS_HUM] = HDC1080_ACTIVE_CUR;
    activeCurrent[PWREST_TYPE_SENSORS_LIGHT] = OPT3001_ACTIVE_CUR;
    activeCurrent[PWREST_TYPE_SENSORS_PRES] = BMP280_ACTIVE_CUR;
    sleepCurrent[PWREST_TYPE_SENSORS_TMP] = TMP007_SLEEP_CUR;
    sleepCurrent[PWREST_TYPE_SENSORS_HUM] = HDC1080_SLEEP_CUR;
    sleepCurrent[PWREST_TYPE_SENSORS_LIGHT] = OPT3001_SLEEP_CUR;
    sleepCurrent[PWREST_TYPE_SENSORS_PRES] = BMP280_SLEEP_CUR;
    
#if I3_MOTE
#if CC13XX_DEVICES
    activeCurrent[PWREST_TYPE_SENSORS_MOTION] = LIS2DW12_ACTIVE_CUR ;
    sleepCurrent[PWREST_TYPE_SENSORS_MOTION] = LIS2DW12_SLEEP_CUR;
    activeCurrent[PWREST_TYPE_SENSORS_TMP] = 0;
    sleepCurrent[PWREST_TYPE_SENSORS_TMP] = 0; 
#else
    activeCurrent[PWREST_TYPE_SENSORS_MOTION] = LIS2HH12_ACTIVE_CUR ;
    sleepCurrent[PWREST_TYPE_SENSORS_MOTION] = LIS2HH12_SLEEP_CUR;
#endif
#else
    activeCurrent[PWREST_TYPE_SENSORS_MOTION] =MPU9250_ACTIVE_CUR;
    sleepCurrent[PWREST_TYPE_SENSORS_MOTION] =MPU9250_SLEEP_CUR;
#endif
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
        uint32_t totalOnTimeInMs;
         
        totalOnTimeInMs =  timeMeasure.totalOnTime[PWREST_TYPE_TOTAL]*1000/ CLOCK_SECOND;
       
        /* nA*/
        pPwrMeasure->avgActiveCur[PWREST_TYPE_RX] =  
          (uint32_t)((rfSlotCount[PWREST_RX_SLOT]*CC2650_RX_OVERHEAD_PWR + 
           rfSlotCount[PWREST_RX_NO_PACKET]*CC2650_RX_NO_PACKET_PWR + 
           rfSlotCount[PWREST_RX_UNICAST_PACKET]*CC2650_RX_UNICAST_ADD_PWR +
           rfSlotCount[PWREST_RX_BROADCAST_PACKET]*CC2650_RX_BROADCAST_ADD_PWR+
           totalRXBytes * RX_PER_BYTE_PWR)*(1000.0/totalOnTimeInMs));
        
        pPwrMeasure->avgActiveCur[PWREST_TYPE_TX] =  
          (uint32_t)((rfSlotCount[PWREST_TX_SLOT]*CC2650_TX_OVERHEAD_PWR + 
           rfSlotCount[PWREST_TX_NO_PACKET]*CC2650_TX_NO_PACKET_PWR + 
             rfSlotCount[PWREST_TX_UNICAST_PACKET]*CC2650_TX_UNICAST_ADD_PWR +
                 totalTXBytes * TX_PER_BYTE_PWR)*(1000.0/totalOnTimeInMs));
        
        memset(rfSlotCount, 0 , sizeof(rfSlotCount));
        totalRXBytes = 0;
        totalTXBytes = 0;
          
        for(i = PWREST_TYPE_CPU; i < PWREST_TYPE_TOTAL; i++)
        {
            pPwrMeasure->dutyCycle[i] =
                (uint16_t)((uint64_t)DUTY_CYCLE_MAX*timeMeasure.totalOnTime[i]
                           /timeMeasure.totalOnTime[PWREST_TYPE_TOTAL]);
            sleepDutyCycle = DUTY_CYCLE_MAX - pPwrMeasure->dutyCycle[i];
            pPwrMeasure->avgActiveCur[i] = (uint32_t) (activeCurrent[i]
                                     *pPwrMeasure->dutyCycle[i]/DUTY_CYCLE_MAX);

            pPwrMeasure->avgSleepCur[i] = (uint32_t)(sleepCurrent[i]
                                                *sleepDutyCycle/DUTY_CYCLE_MAX);
        }
        
        memset(&timeMeasure, 0 , sizeof(PWREST_TimeMeasurement_t));
    }
}

/***************************************************************************//**
* @brief      This functions updates the total number of TX and RX bytes
*
* @param[in]  numTxBytes - number of TX bytes
* @param[in]  numRxBytes - number of RX bytes
*
* @return      none
******************************************************************************/
void PWREST_updateTxRxBytes(uint32_t numTxBytes,uint32_t numRxBytes)
{
    totalTXBytes += numTxBytes;
    totalRXBytes += numRxBytes;
}

/***************************************************************************//**
* @brief      This functions updates the total number of TX and RX bytes
*
* @param[in]  slotType - RF slot type
*
* @return      none
******************************************************************************/
void PWREST_updateRFSlotCount(uint8_t slotType)
{
  if (slotType < PWREST_RF_SLOT_MAX)
  {
    rfSlotCount[slotType]++;
  }
}
