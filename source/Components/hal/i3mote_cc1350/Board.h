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
*  ====================== Board.h =========================================
*  CC2650 I3Mote Board Specific header file.
*/

#ifndef __CC2650_I3MOTE_H__
#define __CC2650_I3MOTE_H__

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
  
  /** ============================================================================
  *  Externs
  *  ==========================================================================*/
  extern const PIN_Config BoardGpioInitTable[];
  
  /** ============================================================================
  *  Defines
  *  ==========================================================================*/
  
  /* Same RF Configuration as 7x7 EM */
#define CC2650EM_7ID
#define TI_DRIVERS_I2C_INCLUDED
  
  /* Mapping of pins to board signals using general board aliases
  *      <board signal alias>        <pin mapping>
  */
#define Board_LED1                 IOID_5
#define Board_LED2                 IOID_6
  
#define Board_LED_ON                1
#define Board_LED_OFF               0
  
  /* Sensor outputs */
#define Board_MPU_INT               IOID_30
#define Board_TMP_RDY               IOID_29
  
  /* I2C */
#define Board_I2C0_SDA0             IOID_24
#define Board_I2C0_SCL0             IOID_25
  
#define Board_I2C0_SDA1             PIN_UNASSIGNED
#define Board_I2C0_SCL1             PIN_UNASSIGNED
    
  /* Power control */
#define Board_SSM_POWER             IOID_26
#define Board_VBAT_OK               IOID_27
#define Board_SSM_5V_EN             IOID_28
  
  /* UART Board */
#define Board_UART_RX               IOID_2  /* RXD  */
#define Board_UART_TX               IOID_3  /* TXD  */
    
  /* SPI Board */
#define Board_SPI0_MISO         PIN_UNASSIGNED
#define Board_SPI0_MOSI         PIN_UNASSIGNED
#define Board_SPI0_CLK          PIN_UNASSIGNED
#define Board_SPI0_CSN          PIN_UNASSIGNED
  
#define Board_SPI1_MISO                 IOID_18
#define Board_SPI1_MOSI                 IOID_19
#define Board_SPI1_CLK                  IOID_20
#define Board_SPI1_CSN                  IOID_21
#define Board_MSP432_IRQ                IOID_22
#define Board_SSM_IRQ                  Board_MSP432_IRQ
  
  /* Interface #0 */
#define     Board_HDC2010_ADDR      (0x40)
#define     Board_OPT3001_ADDR      (0x45)
#define     Board_BMP280_ADDR       (0x77)
#define     Board_LIS2DW12_ADDR     (0x19)

  /** ============================================================================
  *  Instance identifiers
  *  ==========================================================================*/
  /* Generic I2C instance identifiers */
#define Board_I2C                   CC2650_I3MOTE_I2C0
  
  /* Generic UART instance identifiers */
#define Board_UART                  CC2650_I3MOTE_UART0
  
#define BSP_IOID_SDA                Board_I2C0_SDA0
#define BSP_IOID_SCL                Board_I2C0_SCL0
  
  
#define     Board_initI2C()         I2C_init()
  
  /* Generic SPI instance identifiers */
#define Board_SPI0                  CC2650_I3MOTE_SPI0
#define Board_SPI1                  CC2650_I3MOTE_SPI1
  
#define Board_SPI                   Board_SPI1
  
  /** ============================================================================
  *  Number of peripherals and their names
  *  ==========================================================================*/
  /*!
  *  @def    CC2650_I3MOTE_I2CName
  *  @brief  Enum of I2C names on the CC2650 I3 mote
  */
  typedef enum CC2650_I3MOTE_I2CName {
    CC2650_I3MOTE_I2C0= 0,
    
    CC2650_I3MOTE_I2CCOUNT
  } CC2650_I3MOTE_I2CName;
  
  /*!
  *  @def    CC2650_I3MOTE_UARTName
  *  @brief  Enum of UARTs on the CC2650 I3 mote
  */
  typedef enum CC2650_I3MOTE_UARTName {
    CC2650_I3MOTE_UART0 = 0,
    
    CC2650_I3MOTE_UARTCOUNT
  } CC2650_I3MOTE_UARTName;
  
  /*!
  *  @def    CC2650_I3MOTE_SPIName
  *  @brief  Enum of SPI names on the CC2650 I3 mote
  */
  typedef enum CC2650_I3MOTE_SPIName {
    CC2650_I3MOTE_SPI0 = 0,
    CC2650_I3MOTE_SPI1,
    CC2650_I3MOTE_SPICOUNT
  } CC2650_I3MOTE_SPIName;
 
  
 /*!
 *  @def    CC2650_I3MOTE_UdmaName
 *  @brief  Enum of DMA buffers
 */
typedef enum CC2650_I3MOTE_UdmaName {
    CC2650_I3MOTE_UDMA0 = 0,
    CC2650_I3MOTE_UDMACOUNT
} CC2650_I3MOTE_UdmaName;

#endif /* __CC2650_I3MOTE_H__ */
  