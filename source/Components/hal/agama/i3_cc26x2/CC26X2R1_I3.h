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
/** ============================================================================
 *  @file       CC26X2R1_I3.h
 *
 *  @brief      CC26X2R1_I3 Board Specific header file.
 *
 *  The CC26X2R1_I3 header file should be included in an application as
 *  follows:
 *  @code
 *  #include "CC26X2R1_I3.h"
 *  @endcode
 *
 *  ============================================================================
 */
#ifndef __CC26X2R1_I3_BOARD_H__
#define __CC26X2R1_I3_BOARD_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes */
#include <ti/drivers/PIN.h>
#include <ti/devices/cc13x2_cc26x2_v1/driverlib/ioc.h>

/* Externs */
extern const PIN_Config BoardGpioInitTable[];

/* Defines */
#define CC26X2R1_I3
/* Mapping of pins to board signals using general board aliases
 *      <board signal alias>                <pin mapping>
 */

/* Mapping of pins to board signals using general board aliases
 *      <board signal alias>                  <pin mapping>
 */
/* GPIO */
#define CC26X2R1_I3_GPIO_LED_ON           1
#define CC26X2R1_I3_GPIO_LED_OFF          0

/* I2C */
#define CC26X2R1_I3_I2C0_SDA0             IOID_23
#define CC26X2R1_I3_I2C0_SCL0             IOID_24

/* LEDs */
#define CC26X2R1_I3_PIN_LED_ON            1
#define CC26X2R1_I3_PIN_LED_OFF           0
#define CC26X2R1_I3_PIN_RLED              IOID_5
#define CC26X2R1_I3_PIN_GLED              IOID_6

/* SPI Board */
#define CC26X2R1_I3_SPI0_MISO             PIN_UNASSIGNED         
#define CC26X2R1_I3_SPI0_MOSI             PIN_UNASSIGNED
#define CC26X2R1_I3_SPI0_CLK              PIN_UNASSIGNED
#define CC26X2R1_I3_SPI0_CSN              PIN_UNASSIGNED
#define CC26X2R1_I3_SPI1_MISO             IOID_18
#define CC26X2R1_I3_SPI1_MOSI             IOID_19
#define CC26X2R1_I3_SPI1_CLK              IOID_20
#define CC26X2R1_I3_SPI1_CSN              IOID_21

#define CC26X2R1_I3_SSM_IRQ               IOID_22

/* UART Board */
#define CC26X2R1_I3_UART0_RX              IOID_2          /* RXD */
#define CC26X2R1_I3_UART0_TX              IOID_3          /* TXD */
#define CC26X2R1_I3_UART0_CTS       PIN_UNASSIGNED
#define CC26X2R1_I3_UART0_RTS       PIN_UNASSIGNED
#define CC26X2R1_I3_UART1_RX        PIN_UNASSIGNED
#define CC26X2R1_I3_UART1_TX        PIN_UNASSIGNED
#define CC26X2R1_I3_UART1_CTS       PIN_UNASSIGNED
#define CC26X2R1_I3_UART1_RTS       PIN_UNASSIGNED
/* For backward compatibility */
#define CC26X2R1_I3_UART_RX         CC26X2R1_I3_UART0_RX
#define CC26X2R1_I3_UART_TX         CC26X2R1_I3_UART0_TX
#define CC26X2R1_I3_UART_CTS        CC26X2R1_I3_UART0_CTS
#define CC26X2R1_I3_UART_RTS        CC26X2R1_I3_UART0_RTS

/*!
 *  @brief  Initialize the general board specific settings
 *
 *  This function initializes the general board specific settings.
 */
void CC26X2R1_I3_initGeneral(void);

/*!
 *  @def    CC26X2R1_I3_ECDHName
 *  @brief  Enum of ECDH names
 */
typedef enum CC26X2R1_I3_ECDHName {
    CC26X2R1_I3_ECDH0 = 0,

    CC26X2R1_I3_ECDHCOUNT
} CC26X2R1_I3_ECDHName;

/*!
 *  @def    CC26X2R1_I3_ECDSAName
 *  @brief  Enum of ECDSA names
 */
typedef enum CC26X2R1_I3_ECDSAName {
    CC26X2R1_I3_ECDSA0 = 0,

    CC26X2R1_I3_ECDSACOUNT
} CC26X2R1_I3_ECDSAName;

/*!
 *  @def    CC26X2R1_I3_ECJPAKEName
 *  @brief  Enum of ECJPAKE names
 */
typedef enum CC26X2R1_I3_ECJPAKEName {
    CC26X2R1_I3_ECJPAKE0 = 0,

    CC26X2R1_I3_ECJPAKECOUNT
} CC26X2R1_I3_ECJPAKEName;

/*!
 *  @def    CC26X2R1_I3_AESCCMName
 *  @brief  Enum of AESCCM names
 */
typedef enum CC26X2R1_I3_AESCCMName {
    CC26X2R1_I3_AESCCM0 = 0,

    CC26X2R1_I3_AESCCMCOUNT
} CC26X2R1_I3_AESCCMName;

/*!
 *  @def    CC26X2R1_I3_AESECBName
 *  @brief  Enum of AESECB names
 */
typedef enum CC26X2R1_I3_AESECBName {
    CC26X2R1_I3_AESECB0 = 0,

    CC26X2R1_I3_AESECBCOUNT
} CC26X2R1_I3_AESECBName;

/*!
 *  @def    CC26X2R1_I3_SHA2Name
 *  @brief  Enum of SHA2 names
 */
typedef enum CC26X2R1_I3_SHA2Name {
    CC26X2R1_I3_SHA20 = 0,

    CC26X2R1_I3_SHA2COUNT
} CC26X2R1_I3_SHA2Name;

/*!
 *  @def    CC26X2R1_I3_GPIOName
 *  @brief  Enum of GPIO names
 */
typedef enum CC26X2R1_I3_GPIOName {
    CC26X2R1_I3_GPIO_S1 = 0,
    CC26X2R1_I3_GPIO_S2,
    CC26X2R1_I3_SPI_MASTER_READY,
    CC26X2R1_I3_SPI_SLAVE_READY,
    CC26X2R1_I3_GPIO_LED_GREEN,
    CC26X2R1_I3_GPIO_LED_RED,
    CC26X2R1_I3_GPIOCOUNT
} CC26X2R1_I3_GPIOName;


/*!
 *  @def    CC26X2R1_I3_I2CName
 *  @brief  Enum of I2C names
 */
typedef enum CC26X2R1_I3_I2CName {
    CC26X2R1_I3_I2C0 = 0,

    CC26X2R1_I3_I2CCOUNT
} CC26X2R1_I3_I2CName;

/*!
 *  @def    CC26X2R1_I3_NVSName
 *  @brief  Enum of NVS names
 */
typedef enum CC26X2R1_I3_NVSName {
    CC26X2R1_I3_NVSCC26XX0 = 0,
    CC26X2R1_I3_NVSSPI25X0,

    CC26X2R1_I3_NVSCOUNT
} CC26X2R1_I3_NVSName;


/*!
 *  @def    CC26X2R1_I3_SPIName
 *  @brief  Enum of SPI names
 */
typedef enum CC26X2R1_I3_SPIName {
    CC26X2R1_I3_SPI0 = 0,
    CC26X2R1_I3_SPI1,

    CC26X2R1_I3_SPICOUNT
} CC26X2R1_I3_SPIName;

/*!
 *  @def    CC26X2R1_I3_UARTName
 *  @brief  Enum of UARTs
 */
typedef enum CC26X2R1_I3_UARTName {
    CC26X2R1_I3_UART0 = 0,
    CC26X2R1_I3_UART1,

    CC26X2R1_I3_UARTCOUNT
} CC26X2R1_I3_UARTName;

/*!
 *  @def    CC26X2R1_I3_UDMAName
 *  @brief  Enum of DMA buffers
 */
typedef enum CC26X2R1_I3_UDMAName {
    CC26X2R1_I3_UDMA0 = 0,

    CC26X2R1_I3_UDMACOUNT
} CC26X2R1_I3_UDMAName;

/*!
 *  @def    CC26X2R1_I3_WatchdogName
 *  @brief  Enum of Watchdogs
 */
typedef enum CC26X2R1_I3_WatchdogName {
    CC26X2R1_I3_WATCHDOG0 = 0,

    CC26X2R1_I3_WATCHDOGCOUNT
} CC26X2R1_I3_WatchdogName;

#ifdef __cplusplus
}
#endif

#endif /* __CC26X2R1_I3_BOARD_H__ */
