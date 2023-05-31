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

#ifndef __BOARD_H
#define __BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "i3mote_msp432.h"

#define Board_initADC               MSP_EXP432P401R_initADC
#define Board_initGeneral           MSP_EXP432P401R_initGeneral
#define Board_initGPIO              MSP_EXP432P401R_initGPIO
#define Board_initI2C               MSP_EXP432P401R_initI2C
#define Board_initPWM               MSP_EXP432P401R_initPWM
#define Board_initSDSPI             MSP_EXP432P401R_initSDSPI
#define Board_initSPI               MSP_EXP432P401R_initSPI
#define Board_initUART              MSP_EXP432P401R_initUART
#define Board_initWatchdog          MSP_EXP432P401R_initWatchdog

#define Board_LED_ON                MSP_EXP432P401R_LED_ON
#define Board_LED_OFF               MSP_EXP432P401R_LED_OFF

#define Board_LED0                  MSP_EXP432P401R_LED1
#define Board_LED1                  MSP_EXP432P401R_LED2

#define Board_LED_GREEN             Board_LED1
#define Board_LED_RED               Board_LED0

#define Board_GPIO_DBG1             MSP_EXP432P401R_GPIO_DBG

#define Board_BUTTON0               MSP_EXP432P401R_BUTTON1

#define Board_WAKEUP                MSP_EXP432P401R_WAKEUP

#define Board_I2C0                  MSP_EXP432P401R_I2CB0
#define Board_I2C                   Board_I2C0

#define Board_SPI0                  MSP_EXP432P401R_SPIA1
#define Board_SPI0_CS               MSP_EXP432P401R_SPI0_CS
#define Board_SPI                   Board_SPI0
#define Board_SPI_CS                Board_SPI0_CS

#define Board_UART0                 MSP_EXP432P401R_UARTA0

/* Board specific I2C addresses */
#define Board_HDC1000_ADDR          ( 0x40 )
#define Board_TMP007_ADDR           ( 0x44 )
#define Board_OPT3001_ADDR          ( 0x45 )
#define Board_BMP280_ADDR           ( 0x77 )
#define Board_LIS2HH12_ADDR         ( 0x1E )

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_H */
