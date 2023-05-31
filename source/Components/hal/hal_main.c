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
 *  ====================== hal_main.c =============================================
 *  main file for cc13xx/cc26xx hardware abstraction layer
 */


/*******************************************************************************
 * INCLUDES
 */
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>

#include <aon_rtc.h>
#include <prcm.h>
#include <hw_rfc_pwr.h>
#include <driverlib/sys_ctrl.h>

#include "hal_api.h"
#include "led.h"

#if CC13XX_DEVICES
#include "rf_cc13xx.h"
#elif CC26XX_DEVICES
#include "rf_cc26xx.h"
#endif

#include "tsch-acm.h"

/*******************************************************************************
 * GLOBAL VARIABLES
 */
// TX Power dBm lookup table - values from SmartRF Studio
typedef struct {
  int8_t dbm;
  uint16_t txPower; /* Value for the PROP_DIV_RADIO_SETUP.txPower field */
} rfPowerConfig_t;

HAL_FreqBand_t HAL_radioFreqBand;
rtimer_clock_t HAL_txEndTime;

static HAL_Callback_t HAL_appRxCb;
static uint16_t HAL_radioCurChannel=0;
static int8_t HAL_radioTxPower=0;

extern PowerCC26XX_ModuleState PowerCC26XX_module;

#if CC13XX_DEVICES
const rfPowerConfig_t rfPowerTable[] = {
    {-10, 0x08c0 },
    {  0, 0x0041 },
    {  1, 0x10c3 },
    {  2, 0x1042 },
    {  3, 0x14c4 },
    {  4, 0x18c5 },
    {  5, 0x18c6 },
    {  6, 0x1cc7 },
    {  7, 0x20c9 },
    {  8, 0x24cb },
    {  9, 0x2ccd },
    { 10, 0x38d3 },
    { 11, 0x50da },
    { 12, 0xb818 },
    { 13, 0xa63f }, /* 12.5 */
    { 14, 0xa73f },
};
#elif CC26XX_DEVICES
const rfPowerConfig_t rfPowerTable[] = {
    {-21, 0x0CC7},
    {-18, 0x0CC9},
    {-15, 0x0CCB},
    {-12, 0x144B},
    {-9, 0x194E},
    {-6, 0x1D52},
    {-3, 0x2558},
    {0, 0x3161},
    {1, 0x4214},
    {2, 0x4E18},
    {3, 0x5A1C},
    {4, 0x9324},
    {5, 0x9330},
};
#endif

const uint8_t rfPowerTableSize = (sizeof(rfPowerTable) / sizeof(rfPowerConfig_t));

/***************************************************************************//**
* @brief      This function initializes and opens the RF driver.
*
* @param[in]   freqBand - Frequency band of radio operation
* @param[in]   phyMode  - Physical modulation type
*
* @return      Initialization status.
******************************************************************************/
HAL_Status_t HAL_RADIO_init(HAL_FreqBand_t freqBand, HAL_PhyMode_t phyMode)
{
    HAL_Status_t status = HAL_INVALID_PARAMETER;

    if (freqBand  > HAL_FreqBand_2400MHZ)
    {
        return status;
    }
    else
    {
        /* Record the frequency band for channel to frequency conversion */
        HAL_radioFreqBand = freqBand;
    }

    /* RF driver init*/
#if CC13XX_DEVICES
    if (phyMode == HAL_PhyMode_PRO50kbpsFSK)
    {
        status = rf_cc13xx_init(phyMode);
    }
#elif CC26XX_DEVICES
    if (phyMode == HAL_PhyMode_IEEE250kbpsDSSS)
    {
        status = rf_cc26xx_init(phyMode);
    }
#endif

    return status;
}

/***************************************************************************//**
* @brief      This is the transmit callback function pass to TX command.
*
* @param[in]   rfHandle - RF driver handler
* @param[in]   cmdHandle - TX command handle
* @param[in]   event  - TX Interrupt Event
*
* @return      none
******************************************************************************/
static void HAL_txDoneCallBack(RF_Handle rfHandle, RF_CmdHandle cmdHandle,
                       RF_EventMask event)
{
    /* Record end of packet transmission time */
    HAL_txEndTime = RTIMER_NOW();
    /* Post to TX done event to TSCH*/
    eventPost(ACM_EVENT_TX_DATA_DONE);
}

/***************************************************************************//**
* @brief      This function transmits radio packet.
*
* @param[in]   *pBuf - Pointer to transmit packet buffer
* @param[in]   bufLen - Buffer length
* @param[in]   startTime - Relative time to begin packet transmission in unit
*                            of microseconds (0 for immediate).
*
* @return      Transmit status.
******************************************************************************/
HAL_Status_t HAL_RADIO_transmit(uint8_t* pBuf, uint16_t bufLen,
                                uint32_t startTime)
{
    HAL_Status_t status = HAL_INVALID_PARAMETER ;

    /*The TX command to post depends on the RF mode*/
#if CC13XX_DEVICES
    status = rf_cc13xx_transmit(HAL_txDoneCallBack, pBuf, bufLen, startTime);
#elif CC26XX_DEVICES
    status = rf_cc26xx_transmit(HAL_txDoneCallBack, pBuf, bufLen, startTime);
#endif
    return status;
}

/***************************************************************************//**
* @brief      This function gets packet buffer pointer from RX command
*
* @param[out]  ppBuf - pointer to RX buffer pointer
*
* @return      none
******************************************************************************/
static void HAL_getRxBuffer(uint8_t** ppBuf)
{
    rfc_dataEntryGeneral_t* pDataEntry;
    pDataEntry = (rfc_dataEntryGeneral_t *)RF_rxDataEntryBuffer;
    *ppBuf = (uint8_t*)(&pDataEntry->data);

    pDataEntry->status = DATA_ENTRY_PENDING;
}
/***************************************************************************//**
* @brief      This is the receive callback function pass to Rx command.
*
* @param[in]   rfHandle - RF driver handler
* @param[in]   cmdHandle - RX command handle
* @param[in]   event  - RX Interrupt Event
*
* @return      none
******************************************************************************/
static void HAL_rxDoneCallBack(RF_Handle rfHandle, RF_CmdHandle cmdHandle,
                       RF_EventMask event)
{
    uint8_t* pBuf;

    HAL_getRxBuffer(&pBuf);

    if (HAL_appRxCb != NULL)
    {
        HAL_appRxCb(event, pBuf);
    }
}

/***************************************************************************//**
* @brief      This function turns radio receive on.
*
* @param[in]   pCb - Pointer to packet receive callback function.
* @param[in]   startTime - Relative time to turn radio on in unit of
*                microseconds (0 for immediate).
*
* @return      Radio on status
******************************************************************************/
HAL_Status_t HAL_RADIO_receiveOn(HAL_Callback_t pCb, uint32_t startTime)
{
    HAL_Status_t status = HAL_INVALID_PARAMETER;

    HAL_appRxCb = pCb;

#if CC13XX_DEVICES
    status = rf_cc13xx_receiveOn(HAL_rxDoneCallBack, startTime);
#elif CC26XX_DEVICES
    status = rf_cc26xx_receiveOn(HAL_rxDoneCallBack, startTime);
#endif
    return status;
}

/***************************************************************************//**
* @brief      This function turns radio receive off.
*
* @return      Radio off status.
******************************************************************************/
HAL_Status_t HAL_RADIO_receiveOff()
{
    if(RF_cancelCmd(RF_driverHandle, RF_rxCmdHndl, 1) == RF_StatSuccess)
    {
        return HAL_SUCCESS;
    }
    else
    {
        return HAL_ERR;
    }
}

/***************************************************************************//**
* @brief      This function sets radio channel.
*
* @param[in]   channel - radio channel
*
* @return      Radio channel set status.
******************************************************************************/
HAL_Status_t HAL_RADIO_setChannel(uint16_t channel)
{
  HAL_Status_t status = HAL_INVALID_PARAMETER;
  
#if CC13XX_DEVICES
  status = rf_cc13xx_setChannel(channel);
#elif CC26XX_DEVICES
  status = rf_cc26xx_setChannel(channel);
#endif
  if (status == HAL_SUCCESS)
  {
    HAL_radioCurChannel = channel;
  }
  return status;
}

/***************************************************************************//**
* @brief      This function gets current radio channel.
*
* @return     Current radio channel
******************************************************************************/
uint16_t HAL_RADIO_getChannel()
{
    return HAL_radioCurChannel;
}

/***************************************************************************//**
* @brief      This function sets tx output power
*
* @param[in]   power -  TX output power
*
* @return      TX output power set status.
******************************************************************************/
HAL_Status_t HAL_RADIO_setTxPower(int8_t power)
{
  HAL_Status_t status = HAL_ERR;
  RF_EventMask result;
  rfc_CMD_SCH_IMM_t immOpCmd = {0};
  rfc_CMD_SET_TX_POWER_t cmdSetPower = {0};
  uint8_t txPowerIdx;
  
  immOpCmd.commandNo = CMD_SCH_IMM;
  immOpCmd.startTrigger.triggerType = TRIG_NOW;
  immOpCmd.startTrigger.pastTrig = 1;
  immOpCmd.startTime = 0;
  
  cmdSetPower.commandNo = CMD_SET_TX_POWER;
  
  for (txPowerIdx = 0;txPowerIdx < rfPowerTableSize;txPowerIdx++)
  {
    if (rfPowerTable[txPowerIdx].txPower == power)
    {
      cmdSetPower.txPower = rfPowerTable[txPowerIdx].txPower;
      continue;
    }
  }
  
  if (txPowerIdx == rfPowerTableSize)
    return HAL_INVALID_PARAMETER;
  
  //point the Operational Command to the immediate set power command
  immOpCmd.cmdrVal = (uint32_t) &cmdSetPower;
  
  result = RF_runCmd(RF_driverHandle, (RF_Op*)&immOpCmd, RF_PriorityNormal,
                     NULL, RF_EventLastCmdDone);
  
  if (result & RF_EventLastCmdDone)
  {
    status = HAL_SUCCESS;
    HAL_radioTxPower = power;
  }
  
  return status;
}

/***************************************************************************//**
* @brief      This function gets the tx output power
*
* @return     tx output power
******************************************************************************/
int8_t HAL_RADIO_getTxPower(void)
{
     return HAL_radioTxPower;
}

/***************************************************************************//**
* @brief      This function sets LED on or off. 1 is on and and 0 is off.
*
* @return     none
******************************************************************************/
void HAL_LED_set(uint8_t ledID, uint8_t val)
{
    LED_set(ledID, val);
}

/***************************************************************************//**
* @brief      This function gets the LED on off status.
*
* @return     LED status
******************************************************************************/
uint8_t HAL_LED_get(uint8_t ledID)
{
     return LED_get(ledID);
}

/***************************************************************************//**
* @brief      This function signals radio to go to standby state.
*
* @return      Radio off status.
******************************************************************************/
void HAL_RADIO_allowStandby(void)
{
    RF_yield(RF_driverHandle);
}
