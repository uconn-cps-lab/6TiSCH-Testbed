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
 *  ====================== rf_cc13xx.c =============================================
 *  Implementation file cc13xx specfic RF command setup and usage.
 */

/*******************************************************************************
 * INCLUDES
 */
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <driverlib/setup.h>

#include "hal_api.h"
#include "rf_cc13xx.h"

/*******************************************************************************
* GLOBAL VARIABLES
*/
RF_Handle RF_driverHandle;
RF_Object RF_object;
RF_Params RF_params;
RF_CmdHandle RF_rxCmdHndl = NULL;
uint8_t RF_rxDataEntryBuffer[1+sizeof(rfc_dataEntryGeneral_t)
                                +RF_MAX_PACKET_LENGTH + RF_MAX_APPEND_BYTES];
static dataQueue_t RF_dataQueue;
static rfc_propRxOutput_t RF_rxStatistics;

/* RF Mode Object */
RF_Mode RF_prop =
{
    .rfMode = RF_MODE_PROPRIETARY_SUB_1,
    .cpePatchFxn = &rf_patch_cpe_genfsk,
    .mcePatchFxn = 0,
    .rfePatchFxn = &rf_patch_rfe_genfsk,
};

/* Register Overrides */
uint32_t pOverrides[] =
{
    // override_use_patch_prop_genfsk.xml
    // PHY: Use MCE ROM bank 4, RFE RAM patch
   // MCE_RFE_OVERRIDE(0,4,0,1,0,0),
    // override_synth_prop_863_930_div5.xml
    // Synth: Set recommended RTRIM to 7
    HW_REG_OVERRIDE(0x4038,0x0037),
    // Synth: Set Fref to 4 MHz
    (uint32_t)0x000684A3,
    // Synth: Configure fine calibration setting
    HW_REG_OVERRIDE(0x4020,0x7F00),
    // Synth: Configure fine calibration setting
    HW_REG_OVERRIDE(0x4064,0x0040),
    // Synth: Configure fine calibration setting
    (uint32_t)0xB1070503,
    // Synth: Configure fine calibration setting
    (uint32_t)0x05330523,
    // Synth: Set loop bandwidth after lock to 20 kHz
    (uint32_t)0x0A480583,
    // Synth: Set loop bandwidth after lock to 20 kHz
    (uint32_t)0x7AB80603,
    // Synth: Configure VCO LDO (in ADI1, set VCOLDOCFG=0x9F to use voltage input reference)
    ADI_REG_OVERRIDE(1,4,0x9F),
    // Synth: Configure synth LDO (in ADI1, set SLDOCTL0.COMP_CAP=1)
    ADI_HALFREG_OVERRIDE(1,7,0x4,0x4),
    // Synth: Use 24 MHz XOSC as synth clock, enable extra PLL filtering
    (uint32_t)0x02010403,
    // Synth: Configure extra PLL filtering
    (uint32_t)0x00108463,
    // Synth: Increase synth programming timeout (0x04B0 RAT ticks = 300 us)
    (uint32_t)0x04B00243,
     // override_phy_rx_aaf_bw_0xd.xml
    // Rx: Set anti-aliasing filter bandwidth to 0xD (in ADI0, set IFAMPCTL3[7:4]=0xD)
    ADI_HALFREG_OVERRIDE(0,61,0xF,0xD),
    // override_phy_gfsk_rx.xml
    // Rx: Set LNA bias current trim offset to 3
    (uint32_t)0x00038883,
    // Rx: Freeze RSSI on sync found event
    HW_REG_OVERRIDE(0x6084,0x35F1),
    // override_phy_gfsk_pa_ramp_agc_reflevel_0x1a.xml
    // Tx: Configure PA ramping setting (0x41). Rx: Set AGC reference level to 0x1A.
    HW_REG_OVERRIDE(0x6088,0x411A),
    // Tx: Configure PA ramping setting
    HW_REG_OVERRIDE(0x608C,0x8213),
    // override_phy_rx_rssi_offset_5db.xml
    // Rx: Set RSSI offset to adjust reported RSSI by +5 dB
    (uint32_t)0x00FB88A3,
    // TX power override
    // Tx: Set PA trim to max (in ADI0, set PACTL0=0xF8)
    ADI_REG_OVERRIDE(0,12,0xF8),
    //allow GPRAM
    (uint32_t)0x00018063,
    (uint32_t)0xFFFFFFFF,
};

/* Propritary mode radio Setup command */
rfc_CMD_PROP_RADIO_DIV_SETUP_t RF_cmdPropRadioDivSetup =
{
    .commandNo                  = 0x3807,
    .status                     = 0x0000,
    .pNextOp                    = 0, /*INSERT APPLICABLE POINTER: (uint8_t*)&xxx */
    .startTime                  = 0x00000000,
    .startTrigger.triggerType   = 0x0,
    .startTrigger.bEnaCmd       = 0x0,
    .startTrigger.triggerNo     = 0x0,
    .startTrigger.pastTrig      = 0x0,
    .condition.rule             = 0x1,
    .condition.nSkip            = 0x0,
    .modulation.modType         = 0x1,
    .modulation.deviation       = 0x64,
    .symbolRate.preScale        = 0xf,
    .symbolRate.rateWord        = 0x8000,
    .rxBw                       = 0x24,
    .preamConf.nPreamBytes      = 0x4,
    .preamConf.preamMode        = 0x0,
    .formatConf.nSwBits         = 0x20,
    .formatConf.bBitReversal    = 0x0,
    .formatConf.bMsbFirst       = 0x1,
    .formatConf.fecMode         = 0x0,
    .formatConf.whitenMode      = 0x0,
    .config.bNoFsPowerUp        = 0x0,
    .config.frontEndMode        = 0x00, /* Differential */
    .config.biasMode            = 0x1,  /* External bias */
    .centerFreq                 = 0x0364,
    .intFreq                    = 0x8000,
    .loDivider                  = 0x5,
    .txPower                    = 0xbc28,
    .pRegOverride               = pOverrides, /* pOverrides, */
};

/* Frequency Synthesizer Command */
rfc_CMD_FS_t RF_CmdFs =
{
    .commandNo = 0x0803,
    .status = 0x0000,
    .pNextOp = 0, // INSERT APPLICABLE POINTER: (uint8_t*)&xxx
    .startTime = 0x00000000,
    .startTrigger.triggerType = 0x0,
    .startTrigger.bEnaCmd = 0x0,
    .startTrigger.triggerNo = 0x0,
    .startTrigger.pastTrig = 0x0,
    .condition.rule = 0x1,
    .condition.nSkip = 0x0,
    .frequency = 0x0364,
    .fractFreq = 0x0000,
    .synthConf.bTxMode = 0x0,
    .synthConf.refFreq = 0x0,
    .__dummy0 = 0x00,
    .__dummy1 = 0x00,
    .__dummy2 = 0x00,
    .__dummy3 = 0x0000,
};

/* Proprietary mode TX command */
rfc_CMD_PROP_TX_t RF_cmdPropTx =
{
    .commandNo                  = 0x3801,
    .status                     = 0x0000,
    .pNextOp                    = 0, /*INSERT APPLICABLE POINTER: (uint8_t*)&xxx */
    .startTime                  = 0x00000000,
    .startTrigger.triggerType   = 0x0,
    .startTrigger.bEnaCmd       = 0x0,
    .startTrigger.triggerNo     = 0x0,
    .startTrigger.pastTrig      = 0x0,
    .condition.rule             = 0x1,
    .condition.nSkip            = 0x0,
    .pktConf.bFsOff             = 0x0,
    .pktConf.bUseCrc            = 0x1,
    .pktConf.bVarLen            = 0x1,
    .pktLen                     = 0x0A,
    .syncWord                   = 0x930B51DE,
    .pPkt                       = 0, /*INSERT APPLICABLE POINTER: (uint8_t*)&xxx */
};

/* Proprietary mode RX command */
rfc_CMD_PROP_RX_t RF_cmdPropRx =
{
    .commandNo                  = 0x3802,
    .status                     = 0x0000,
    .pNextOp                    = 0, /*INSERT APPLICABLE POINTER: (uint8_t*)&xxx */
    .startTime                  = 0x00000000,
    .startTrigger.triggerType   = 0x0,
    .startTrigger.bEnaCmd       = 0x0,
    .startTrigger.triggerNo     = 0x0,
    .startTrigger.pastTrig      = 0x0,
    .condition.rule             = 0x1,
    .condition.nSkip            = 0x0,
    .pktConf.bFsOff             = 0x0,
    .pktConf.bRepeatOk          = 0x1,
    .pktConf.bRepeatNok         = 0x1,
    .pktConf.endType            = 0x0,
    .pktConf.bUseCrc            = 0x1,
    .pktConf.bVarLen            = 0x1,
    .pktConf.bChkAddress        = 0x0,
    .pktConf.filterOp           = 0x1,
    .rxConf.bAutoFlushIgnored   = 0x0,
    .rxConf.bAutoFlushCrcErr    = 0x0,
    .rxConf.bIncludeHdr         = 0x1,
    .rxConf.bIncludeCrc         = 0x0,
    .rxConf.bAppendRssi         = 0x1,
    .rxConf.bAppendTimestamp    = 0x1,
    .rxConf.bAppendStatus       = 0x0,
    .syncWord                   = 0x930B51DE,
    .maxPktLen                  = 0x80,
    .address0                   = 0x0,
    .address1                   = 0x0,
    .endTrigger.triggerType     = 0x1,
    .endTrigger.bEnaCmd         = 0x0,
    .endTrigger.triggerNo       = 0x0,
    .endTrigger.pastTrig        = 0x0,
    .endTime                    = 0x00000000,
    .pQueue                     = 0, /*INSERT APPLICABLE POINTER: (dataQueue_t*)&xxx */
    .pOutput                    = 0, /*INSERT APPLICABLE POINTER: (uint8_t*)&xxx */
};

/***************************************************************************//**
* @brief      This function initializes and opens the RF driver for cc13xx
*
* @param[in]   phyMode  - Physical modulation type
*
* @return      Initialization status
******************************************************************************/
HAL_Status_t rf_cc13xx_init(HAL_PhyMode_t phyMode)
{
    HAL_Status_t status = HAL_ERR;
    rfc_dataEntryGeneral_t *pDataEntry;

    RF_Params_init(&RF_params);
    RF_params.nInactivityTimeout = 200;

    RF_driverHandle = RF_open(&RF_object, &RF_prop,
                       (RF_RadioSetup*)&RF_cmdPropRadioDivSetup, &RF_params);

    if (RF_driverHandle != NULL)
    {
        /* Set up the data entery queue*/
        pDataEntry = (rfc_dataEntryGeneral_t*)RF_rxDataEntryBuffer;
        pDataEntry->status = DATA_ENTRY_PENDING;
        pDataEntry->config.type = DATA_ENTRY_TYPE_GEN;
        pDataEntry->config.lenSz = 0;
        pDataEntry->length = RF_MAX_PACKET_LENGTH + RF_MAX_APPEND_BYTES;
        pDataEntry->pNextEntry = (uint8_t *)RF_rxDataEntryBuffer;

        /* Set the Data Entity queue for received data */
        RF_dataQueue.pCurrEntry = (uint8_t*)pDataEntry;
        RF_dataQueue.pLastEntry = NULL;

        /* Set the Data Entity queue for received data */
        RF_cmdPropRx.pQueue   = &RF_dataQueue;
        /* Set rx statistics output struct */
        RF_cmdPropRx.pOutput  = (uint8_t*)&RF_rxStatistics;

        /* Set to active power state initially*/
        Power_setConstraint(PowerCC26XX_SD_DISALLOW);
        Power_setConstraint(PowerCC26XX_IDLE_PD_DISALLOW);
       // Power_setConstraint(PowerCC26XX_SB_DISALLOW);

        status = HAL_SUCCESS;
    }
    return status;
}

/***************************************************************************//**
* @brief      This function transmits cc13xx radio packets.
*
* @param[in]   pCb - Pointer to transmit callback function
* @param[in]   *pBuf - Pointer to transmit packet buffer
* @param[in]   bufLen - Buffer length
* @param[in]   startTime - Relative time to begin packet transmission in unit
*                            of microseconds (0 for immediate).
*
* @return      Transmit status.
******************************************************************************/
HAL_Status_t rf_cc13xx_transmit(RF_Callback pCb, uint8_t* pBuf, uint16_t bufLen,
                                uint32_t startTime)
{
    HAL_Status_t status = HAL_ERR;
    RF_CmdHandle cmdHndl;

    if ((pBuf == NULL) || (bufLen > RF_MAX_PACKET_LENGTH))
        return status;

    /* Clear the TX status */
    RF_cmdPropTx.status = 0;
    RF_cmdPropTx.pktLen = bufLen;
    RF_cmdPropTx.pPkt = pBuf;

     /* Set the TX buffer*/
    if(startTime != 0)
    {
         /* Start in the future*/
        RF_cmdPropTx.startTrigger.triggerType =  TRIG_ABSTIME;
        RF_cmdPropRx.startTrigger.pastTrig = 1;
        RF_cmdPropTx.startTime = startTime;
    }
    else
    {
        /* Immediate start*/
        RF_cmdPropTx.startTrigger.triggerType = TRIG_NOW;
        RF_cmdPropTx.startTime = 0;
    }

    /* Post Proprietary TX command*/
    cmdHndl = RF_postCmd(RF_driverHandle, (RF_Op*)&RF_cmdPropTx, RF_PriorityNormal,
                        pCb,RF_EventLastCmdDone);

    /* Error when the returned command handle is less than 0*/
    if(cmdHndl >= 0)
    {
        status = HAL_SUCCESS;
    }

    return status;
}

/***************************************************************************//**
* @brief      This function turns cc13xx radio receive on.
*
* @param[in]   pCb - Pointer to receuve callback function.
* @param[in]   startTime - Relative time to turn radio on in unit
*                            of microseconds (0 for immediate).
*
* @return      Radio on status
******************************************************************************/
HAL_Status_t rf_cc13xx_receiveOn(RF_Callback pCb, uint32_t startTime)
{
    HAL_Status_t status = HAL_ERR;

    rfc_dataEntryGeneral_t *pDataEntry;

    /* Set data entry to pending status */
    pDataEntry = (rfc_dataEntryGeneral_t*)RF_rxDataEntryBuffer;
    pDataEntry->status = DATA_ENTRY_PENDING;

    /* Clear the Rx status */
    RF_cmdPropRx.status = 0;

    if(startTime != 0)
    {
        /* Start in the future*/
        RF_cmdPropRx.startTrigger.triggerType = TRIG_ABSTIME;;
        RF_cmdPropRx.startTime = startTime;
    }
    else
    {
        /* Immediate start*/
        RF_cmdPropRx.startTrigger.triggerType = TRIG_NOW;
        RF_cmdPropRx.startTime = 0;
    }

    /* RX will not end until command cancel */
    RF_cmdPropRx.startTrigger.pastTrig = 1;
    RF_cmdPropRx.endTrigger.triggerType = TRIG_NEVER;
    RF_cmdPropRx.endTime = 0;

    /* Clear the Rx statistics structure */
    memset(&RF_rxStatistics, 0, sizeof(rfc_propRxOutput_t));

     /* Post Proprietary RX command*/
    RF_rxCmdHndl = RF_postCmd(RF_driverHandle, (RF_Op*)&RF_cmdPropRx,
                              RF_PriorityNormal, pCb,
                              RF_EventMdmSoft|RF_EventRxOk|RF_EventRxNOk);

     /* Error when the returned command handle is less than 0*/
    if(RF_rxCmdHndl >= 0)
    {
        status = HAL_SUCCESS;
    }

    return status;
}

/***************************************************************************//**
* @brief      This function sets cc13xx Sub-GHz radio channel.
*
* @param[in]   channel - radio channel
*
* @return      Radio channel set status.
******************************************************************************/
HAL_Status_t rf_cc13xx_setChannel(uint16_t channel)
{
    uint16_t freqInt, fractFreq, fract;
    RF_EventMask mask;

    /*
    868 MHz frequency band supports channels from 0 - 34
    915 MHz frequency band supports channels from 0 - 129
    */
    if(channel > HAL_RADIO_MAX_CHANNEL)
         return HAL_INVALID_PARAMETER;

    /* Channel to frequency conversion*/
    if (HAL_radioFreqBand == HAL_FreqBand_868MHZ)
    {
        fract = 125 + (channel - 1)*200;
        freqInt = 863 + fract/1000;
        fractFreq = fract%1000*65536/1000;
    }
    else if (HAL_radioFreqBand == HAL_FreqBand_915MHZ)
    {
        fract = 200 + (channel - 1)*200;
        freqInt = 902 + fract/1000;
        fractFreq = fract%1000*65536/1000;
    }

    RF_CmdFs.status = 0;
    RF_CmdFs.frequency = freqInt;
    RF_CmdFs.fractFreq = fractFreq;

     /* Post FS command*/
    mask = RF_runCmd(RF_driverHandle, (RF_Op*)&RF_CmdFs, RF_PriorityNormal,
                        NULL, RF_EventLastCmdDone | RF_EventCmdError);
    
    if (mask == RF_EventCmdError)
      return HAL_ERR;
    else
      return HAL_SUCCESS;
}

/***************************************************************************//**
* @brief     This function disables cache during startup so GPRAM is available.
*
* @param[in]   none
*
* @return      nones
******************************************************************************/
void customResetISR(void)
{
    /* disable iCache prefetching */
    VIMSConfigure(VIMS_BASE, FALSE, FALSE);
    VIMSModeSet(VIMS_BASE, VIMS_MODE_DISABLED);
    while ( VIMSModeGet( VIMS_BASE ) != VIMS_MODE_DISABLED );
}

/***************************************************************************//**
* @brief     This function sets GPRAM as standatd RAM.
*
* @param[in]   none
*
* @return      nones
******************************************************************************/
void manualTrim()
{
   SetupTrimDevice();
   /* Set VIMS back to SRAM mode */
   HWREG( VIMS_BASE + VIMS_O_CTL ) &= ~VIMS_CTL_MODE_M;
}