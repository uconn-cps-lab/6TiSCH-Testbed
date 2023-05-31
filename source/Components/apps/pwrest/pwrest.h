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
 *  ====================== pwrest.h =============================================
 *  Defines and prototypes for power estimation module
 */
#ifndef PWREST_H_
#define PWREST_H_

#include <stdint.h>

/*******************************************************************************
 * TYPEDEFS
 */

/*  Types of Power Estimation*/
enum pwr_type {
    PWREST_TYPE_RX,
    PWREST_TYPE_TX,
    PWREST_TYPE_CPU,
    PWREST_TYPE_SENSORS_TMP,
    PWREST_TYPE_SENSORS_HUM,
    PWREST_TYPE_SENSORS_LIGHT,
    PWREST_TYPE_SENSORS_PRES,
    PWREST_TYPE_SENSORS_MOTION,
    PWREST_TYPE_TOTAL,
    PWREST_TYPE_MAX,
};

enum pwr_rf_slot {
    PWREST_RX_SLOT,
    PWREST_RX_NO_PACKET,
    PWREST_RX_UNICAST_PACKET,
    PWREST_RX_BROADCAST_PACKET,
    PWREST_TX_SLOT,
    PWREST_TX_NO_PACKET,
    PWREST_TX_UNICAST_PACKET,
    PWREST_RF_SLOT_MAX,
};

/* 2 decimal place for duty cycle representation*/
#define DUTY_CYCLE_MAX (10000)

/* Active and Sleep Current for different power states , uint: nA*/
/* CC2650*/
#define CC2650_CPU_ACTIVE_CUR (2194000)
#define CC2650_CPU_SLEEP_CUR (2000)

/* Unit: nC based on 3.3 v input voltage */
#define CC2650_RX_OVERHEAD_PWR (8640)
#define CC2650_TX_OVERHEAD_PWR (11700)
#define CC2650_RX_PER_BYTE_PWR  (206)  
#define CC2650_TX_PER_BYTE_PWR  (275)    
#define CC2650_RX_NO_PACKET_PWR  (14520)
#define CC2650_RX_UNICAST_ADD_PWR  (10250) 
#define CC2650_RX_BROADCAST_ADD_PWR  (6600)
#define CC2650_TX_NO_PACKET_PWR  (3800)
#define CC2650_TX_UNICAST_ADD_PWR  (3650) 

#define CC1350_RX_PER_BYTE_PWR  (1104)  
#define CC1350_TX_PER_BYTE_PWR  (4768)   

#if CC13XX_DEVICES
#define RX_PER_BYTE_PWR   CC1350_RX_PER_BYTE_PWR
#define TX_PER_BYTE_PWR   CC1350_TX_PER_BYTE_PWR
#else
#define RX_PER_BYTE_PWR   CC2650_RX_PER_BYTE_PWR
#define TX_PER_BYTE_PWR   CC2650_TX_PER_BYTE_PWR
#endif


/* General Purpose Sensors*/
#define TMP007_ACTIVE_CUR (270000)
#define TMP007_SLEEP_CUR  (2000)
#define HDC1080_ACTIVE_CUR (190000)
#define HDC1080_SLEEP_CUR  (100)
#define OPT3001_ACTIVE_CUR (3700)
#define OPT3001_SLEEP_CUR  (400)
#define BMP280_ACTIVE_CUR (720000)
#define BMP280_SLEEP_CUR  (100)
#define LIS2HH12_ACTIVE_CUR (180000)
#define LIS2HH12_SLEEP_CUR  (500)
#define MPU9250_ACTIVE_CUR (3650000)
#define MPU9250_SLEEP_CUR  (42400)
#define LIS2DW12_ACTIVE_CUR (5000)
#define LIS2DW12_SLEEP_CUR  (500)

/* Time Measurement*/
typedef struct {
    uint32_t lastOnTime[PWREST_TYPE_MAX];
    uint32_t totalOnTime[PWREST_TYPE_MAX];
} PWREST_TimeMeasurement_t;

/* Power Measurement*/
typedef struct {
    uint16_t dutyCycle[PWREST_TYPE_TOTAL];
    uint32_t avgActiveCur[PWREST_TYPE_TOTAL];
    uint32_t avgSleepCur[PWREST_TYPE_TOTAL];
} PWREST_PwrMeasurement_t;

/*******************************************************************************
 * FUNCTIONS
 */
void PWREST_init(void);
void PWREST_on(uint8_t type);
void PWREST_off(uint8_t type);
void PWREST_calAvgCur(PWREST_PwrMeasurement_t *pPwrMeasure);
void PWREST_updateRFSlotCount(uint8_t slotType);
void PWREST_updateTxRxBytes(uint32_t numTxBytes,uint32_t numRxBytes);

#endif /* PWREST_H_ */
