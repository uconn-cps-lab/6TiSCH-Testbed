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
*  CC2650 TIDA-01439 Board Specific header file.
*/

#ifndef __CC2650_TIDA_01439_H__
#define __CC2650_TIDA_01439_H__

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
#define Board_LED1                 IOID_2
#define Board_LED2                 IOID_3
  
#define Board_LED_ON                1
#define Board_LED_OFF               0
  
 
/* SPI Board */
#define Board_SPI0_MOSI         PIN_UNASSIGNED
#define Board_SPI0_MISO         PIN_UNASSIGNED
#define Board_SPI0_CLK          PIN_UNASSIGNED
#define Board_SPI0_CSN          PIN_UNASSIGNED
#define Board_SPI1_MISO                 IOID_10
#define Board_SPI1_MOSI                 IOID_12
#define Board_SPI1_CLK                  IOID_13
#define Board_SPI1_CSN                  IOID_11
  

 
/** ============================================================================
  *  Instance identifiers
  *  ==========================================================================*/
/* Generic SPI instance identifiers */
#define Board_SPI0                  CC2650_TIDA_SPI0
#define Board_SPI1                  CC2650_TIDA_SPI1
  
#define Board_SPI                   Board_SPI1
  
  /** ============================================================================
  *  Number of peripherals and their names
  *  ==========================================================================*/
  /*!
  *  @def    CC2650_TIDA_SPIName
  *  @brief  Enum of SPI names
  */
  typedef enum CC2650_I3MOTE_SPIName {
    CC2650_TIDA_SPI0 = 0,
    CC2650_TIDA_SPI1,
    CC2650_TIDA_SPICOUNT
  } CC2650_TIDA_SPIName;
 
  
 /*!
 *  @def    CC2650_TIDA_UdmaName
 *  @brief  Enum of DMA buffers
 */
typedef enum CC2650_TIDA_UdmaName {
    CC2650_TIDA_UDMA0 = 0,
    CC2650_TIDA_UDMACOUNT
} CC2650_ITIDA_UdmaName;

#endif /* __CC2650_TIDA_01439_H__ */
  