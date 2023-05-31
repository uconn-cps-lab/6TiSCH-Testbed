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

/*******************************************************************************
 * TYPEDEFS
 */

/*  Types of Power Estimation*/
enum pwr_type {
    PWREST_TYPE_CPU,
    PWREST_TYPE_SENSORS_TMP,
    PWREST_TYPE_SENSORS_HUM,
    PWREST_TYPE_SENSORS_LIGHT,
    PWREST_TYPE_SENSORS_PRES,
    PWREST_TYPE_SENSORS_MOTION,
    PWREST_TYPE_TOTAL,
    PWREST_TYPE_MAX,
};

/* 2 decimal place for duty cycle representation*/
#define DUTY_CYCLE_MAX (10000)

/* Active and Sleep Current for different power states , uint: nA*/
/* MSP432*/
#define MSP432_CPU_ACTIVE_CUR (3580000)
#define MSP432_CPU_SLEEP_CUR (660)

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
#define LIS2HH12_SLEEP_CUR  (5000)

#define SENSOR_ACTIVE_RATIO(i)      (Clock_tickPeriod/(i*1000))

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

#endif /* PWREST_H_ */
