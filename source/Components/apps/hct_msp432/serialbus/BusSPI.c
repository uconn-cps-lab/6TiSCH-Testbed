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

/** ============================================================================
 *  @file       BusSPI.c
 *
 *  @brief      Simple interface to the TI-RTOS driver. Also manages switching
 *              between I2C-buses.
 *
 *  ============================================================================
 */

/* -----------------------------------------------------------------------------
*  Includes
* ------------------------------------------------------------------------------
*/
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/drivers/GPIO.h>

#ifdef CC26XX_DEVICES
#include <ti/drivers/spi/SPICC26XXDMA.h>
#endif

#ifdef TARGET_MSP432
#include <ti/drivers/spi/SPIMSP432DMA.h>
#endif

#include "Board.h"
#include "BusSPI.h"

/* -----------------------------------------------------------------------------
*  Constants
* ------------------------------------------------------------------------------
*/
#define BUS_SPI_TX_BUFFER_LENGTH      128
#define BUS_SPI_RX_BUFFER_LENGTH      128

#define BUS_SPI_TIMEOUT   (5000000L/Clock_tickPeriod)

/* -----------------------------------------------------------------------------
*  Public Variables
* ------------------------------------------------------------------------------
*/
uint16_t spiTimeoutCount=0;
uint16_t spiTransferErrCount=0;
/* -----------------------------------------------------------------------------
*  Private Variables
* ------------------------------------------------------------------------------
*/

/* SPI driver interface */
static SPI_Handle spiHandle;
static SPI_Params spiParams;
static SPI_Transaction spiTransaction;

//semaphore
static Semaphore_Struct semaphore;
static Semaphore_Params semParams;

/* -----------------------------------------------------------------------------
*  Private Functions
* ------------------------------------------------------------------------------
*/
void BusSPI_transferCallback(SPI_Handle handle, SPI_Transaction *transaction);

/* -----------------------------------------------------------------------------
*  Public Functions
* ------------------------------------------------------------------------------
*/

/*******************************************************************************
* @fn          BusSPI_open_master
*
* @brief       Initialize the RTOS SPI driver (must be called only once)
*
* @param       none
*
* @return      true if SPI_open succeeds
*/
bool BusSPI_open_master(unsigned int index)
{
    /* Configure SPI */
    SPI_Params_init(&spiParams);
    spiParams.transferMode        = SPI_MODE_BLOCKING;
    spiParams.transferTimeout     = BUS_SPI_TIMEOUT;
    spiParams.transferCallbackFxn = NULL;
    spiParams.mode                = SPI_MASTER;
    spiParams.bitRate             = 500000;
    spiParams.dataSize            = 8;
    spiParams.frameFormat         = SPI_POL1_PHA1;

    /* Open SPI */
    spiHandle = SPI_open(index, &spiParams);
    if (spiHandle == NULL) {
        return false;
    }

    return true;
}

/*******************************************************************************
* @fn          BusSPI_open_slave
*
* @brief       Initialize the RTOS SPI driver (must be called only once)
*
* @param       none
*
* @return      true if SPI_open succeeds
*/
bool BusSPI_open_slave(unsigned int index)
{
    /* Configure SPI */
    SPI_Params_init(&spiParams);
    spiParams.transferMode        = SPI_MODE_BLOCKING;
    spiParams.transferTimeout     = BUS_SPI_TIMEOUT;
    spiParams.transferCallbackFxn = NULL;
    spiParams.mode                = SPI_SLAVE;
    spiParams.bitRate             = 500000;
    spiParams.dataSize            = 8;
    spiParams.frameFormat         = SPI_POL1_PHA1;

    /* Open SPI */
    spiHandle = SPI_open(index, &spiParams);
    if (spiHandle == NULL) {
        return false;
    }

    /* Initialize semaphore */
    Semaphore_Params_init(&semParams);
    semParams.mode = Semaphore_Mode_BINARY;
    Semaphore_construct(&semaphore, 0, &semParams);


    return true;
}



/*******************************************************************************
* @fn          BusSPI_close
*
* @brief       Close the SPI interface and release the data lines
*
* @param       none
*
* @return      true if SPI_close succeeds
*/
void BusSPI_close(void)
{
    /* Close SPI */
    if (spiHandle != NULL) {
        /* Close SPI */
        SPI_close(spiHandle);

        /* Release SPI handle */
        spiHandle = NULL;
    }
}

/*******************************************************************************
* @fn          BusSPI_writeRead
*
* @brief       Burst write/read from an SPI device
*
* @param       wdata - pointer to write data buffer
* @param       wlen - number of bytes to write
* @param       rdata - pointer to read data buffer
* @param       rlen - number of bytes to read
*
* @return      true if success
*/
bool BusSPI_writeRead(uint8_t *wdata, uint8_t wlen, uint8_t *rdata, uint8_t rlen)
{
    bool status;

    /* Ensure both buffers have the same length */
    if (wlen != rlen) {
        return false;
    }

    /* Configure SPI transaction */
    spiTransaction.count = wlen;
    spiTransaction.txBuf = wdata;
    spiTransaction.rxBuf = rdata;

    /* Execute SPI transaction */
    status = SPI_transfer(spiHandle, &spiTransaction);

    if (!status) {
      /* Error in SPI transfer or transfer is already in progress */
      spiTransferErrCount++;
      return false;
    }
    else
    {
      if (spiParams.transferMode == SPI_MODE_CALLBACK) {
        if (!Semaphore_pend(Semaphore_handle(&semaphore), BUS_SPI_TIMEOUT))
        {
          spiTimeoutCount++;
          return false;
        }
      }
    }

    return true;
}

/*******************************************************************************
* @fn
*
* @brief
*
* @param       none
*
* @return      true if SPI transaction succeeds
*/
bool BusSPI_transaction(uint8_t *wdata, uint8_t wlen, uint8_t *rdata, uint8_t rlen) {
    bool status;

    status = BusSPI_writeRead(wdata, wlen, rdata, rlen);

    return status;
}


// Callback function
 void BusSPI_transferCallback(SPI_Handle handle, SPI_Transaction *transaction)
 {
     //toggle semaphore
     Semaphore_post(Semaphore_handle(&semaphore));

 }

 /*******************************************************************************
 * @fn          RadioSPI_select
 *
 * @brief       Select an SPI interface and slave
 *
 * @param       index - index of the GPIO that acts as CS
 *
 * @return      none
 */
 void BusSPI_select(unsigned int index)
 {
     /* Select SPI */
     GPIO_write(index, 0);
 }

 /*******************************************************************************
 * @fn          RadioSPI_deselect
 *
 * @brief       Allow other tasks to access the SPI driver
 *
 * @param       index - index of the GPIO that acts as CS
 *
 * @return      none
 */
 void BusSPI_deselect(unsigned int index)
 {
     /* Deselect SPI */
     GPIO_write(index, 1);
 }
