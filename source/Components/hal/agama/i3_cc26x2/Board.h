/*
 * Copyright (c) 2015-2018, Texas Instruments Incorporated
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

/** ============================================================================
*  Includes
*  ==========================================================================*/
#include <ti/drivers/PIN.h>
#include <driverlib/ioc.h>

#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/UART.h>

#include "CC26X2R1_I3.h"

#define Board_initGeneral()     CC26X2R1_I3_initGeneral()


/* These #defines allow us to reuse TI-RTOS across other device families */
#define Board_GPIO_LED0         CC26X2R1_I3_GPIO_LED_RED
#define Board_GPIO_LED1         CC26X2R1_I3_GPIO_LED_GREEN
#define Board_GPIO_LED2         CC26X2R1_I3_GPIO_LED_RED
#define Board_GPIO_RLED         CC26X2R1_I3_GPIO_LED_RED
#define Board_GPIO_GLED         CC26X2R1_I3_GPIO_LED_GREEN
#define Board_GPIO_LED_ON       CC26X2R1_I3_GPIO_LED_ON
#define Board_GPIO_LED_OFF      CC26X2R1_I3_GPIO_LED_OFF

#define Board_I2C0              CC26X2R1_I3_I2C0
#define Board_I2C_TMP           Board_I2C0
#define Board_I2C               Board_I2C0


#define Board_NVSINTERNAL       CC26X2R1_I3_NVSCC26XX0
#define Board_NVSEXTERNAL       CC26X2R1_I3_NVSSPI25X0

#define Board_PIN_LED0          CC26X2R1_I3_PIN_RLED
#define Board_PIN_LED1          CC26X2R1_I3_PIN_GLED
#define Board_PIN_LED2          CC26X2R1_I3_PIN_RLED
#define Board_PIN_RLED          CC26X2R1_I3_PIN_RLED
#define Board_PIN_GLED          CC26X2R1_I3_PIN_GLED


#define Board_SPI0              CC26X2R1_I3_SPI0
#define Board_SPI1              CC26X2R1_I3_SPI1

#define Board_SSM_IRQ           CC26X2R1_I3_SSM_IRQ
#define Board_SPI               Board_SPI1

#define Board_SPI_MASTER        CC26X2R1_I3_SPI0
#define Board_SPI_SLAVE         CC26X2R1_I3_SPI0
#define Board_SPI_MASTER_READY  CC26X2R1_I3_SPI_MASTER_READY
#define Board_SPI_SLAVE_READY   CC26X2R1_I3_SPI_SLAVE_READY

#define Board_UART0             CC26X2R1_I3_UART0
#define Board_UART1             CC26X2R1_I3_UART1


#define Board_LED1              CC26X2R1_I3_PIN_GLED
#define Board_LED2              CC26X2R1_I3_PIN_RLED

#define     Board_HDC1000_ADDR      (0x40)
#define     Board_TMP007_ADDR       (0x44)
#define     Board_OPT3001_ADDR      (0x45)
#define     Board_BMP280_ADDR       (0x77)
#define     Board_LIS2HH12_ADDR     (0x1E)

#define Board_I2C0_SDA1             PIN_UNASSIGNED
#define Board_I2C0_SCL1             PIN_UNASSIGNED

/* Power control */
#define Board_SSM_POWER             IOID_26
#define Board_VBAT_OK               IOID_27
#define Board_SSM_5V_EN             IOID_28


#ifdef __cplusplus
}
#endif

#endif /* __BOARD_H */
