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
 *  ====================== rf_cc26xx.c =============================================
 *  Implementation file cc26xx specfic RF command setup and usage.
 */


/*******************************************************************************
 * INCLUDES
 */
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>

#include "hal_api.h"
#include "rf_cc26xx.h"
#if DMM_ENABLE
/* Include DMM module */
#include "dmm_scheduler.h"
//#include "dmm_rfmap.h"
#endif //DMM_ENABLE

#ifndef RF_EventLastFGCmdDone
#define   RF_EventLastFGCmdDone       (1<<3)   ///< Last radio operation command in a chain finished
#endif //RF_EventLastFGCmdDone
/*******************************************************************************
* GLOBAL VARIABLES
*/
RF_Handle RF_driverHandle;
RF_Object RF_object;
RF_Params RF_params;
RF_CmdHandle RF_rxCmdHndl = NULL;
uint8_t RF_rxDataEntryBuffer[1+sizeof(rfc_dataEntryGeneral_t)
                                +RF_MAX_PACKET_LENGTH + RF_MAX_APPEND_BYTES];
dataQueue_t RF_dataQueue;
rfc_ieeeRxOutput_t RF_rxStatistics;

/* RF Mode Object */
RF_Mode RF_mode =
{
    .rfMode                     = RF_MODE_IEEE_15_4,
    .cpePatchFxn                = 0,
    .rfePatchFxn                = 0,
    .mcePatchFxn                = 0,
};

#ifdef DeviceFamily_CC26X2
/* Register Overrides */
uint32_t RF_pOverrides_IEEE250kbpsDSSS[] =
{
 // override_agama_48_mhz_crystal.xml
 // Calculate the frequencies for synth based on 48MHz crystal (default is 24MHz).
 (uint32_t)0x00400403,
 // override_turn_off_dcdc_option.xml
 // Set DCDCCTL5[3:0]=0x0, Turn off optimization of DCDC regulator in TX
 (uint32_t)0x000088D3,
 // Set DCDCCTL5[3:0]=0x0, Turn off optimization of DCDC regulator in RX
 (uint32_t)0x000088C3,
 (uint32_t)0xFFFFFFFF,                                   \
};
#else
/* Register Overrides */
uint32_t RF_pOverrides_IEEE250kbpsDSSS[] =
{
    0x00354038, /* Synth: Set RTRIM (POTAILRESTRIM) to 5 */                    \
    0x4001402D, /* Synth: Correct CKVD latency setting (address) */            \
    0x00608402, /* Synth: Correct CKVD latency setting (value) */              \
    0x4001405D, /* Synth: Set ANADIV DIV_BIAS_MODE to PG1 (address) */         \
    0x1801F800, /* Synth: Set ANADIV DIV_BIAS_MODE to PG1 (value) */           \
    0x000784A3, /* Synth: Set FREF = 3.43 MHz (24 MHz / 7) */                  \
    0xA47E0583, /* Synth: Set loop bandwidth after lock to 80 kHz (K2) */      \
    0xEAE00603, /* Synth: Set loop bandwidth after lock to 80 kHz (K3, LSB) */ \
    0x00010623, /* Synth: Set loop bandwidth after lock to 80 kHz (K3, MSB) */ \
    0x000288A3, /* ID: Adjust RSSI offset by 2 dB */                           \
    0x000F8883, /* XD and ID: Force LNA IB to maximum */                       \
    0x002B50DC, /* Adjust AGC DC filter */                                     \
    0x05000243, /* Increase synth programming timeout */                       \
    0x002082C3, /* Set Rx FIFO threshold to avoid overflow */                  \
    0x00018063, /* Disable pointer check */                                    \
    0xFFFFFFFF  /* End of override list */                                     \
};
#endif

/* Radio Setup command */
rfc_CMD_RADIO_SETUP_t RF_cmdRadioSetup =
{
    .commandNo = 0x0802,
    .status = 0x0000,
    .pNextOp = 0x00000000,
    .startTime = 0x00000000,
    .startTrigger.triggerType = 0x0,
    .startTrigger.bEnaCmd = 0x0,
    .startTrigger.triggerNo = 0x0,
    .startTrigger.pastTrig = 0x0,
    .condition.rule = 0x1,
    .condition.nSkip = 0x0,
    .mode = 0x01,
#ifndef DeviceFamily_CC26X2
    .__dummy0 = 0x00,
#endif
    .config.frontEndMode = 0x0,
    .config.biasMode = 0x0,
    .config.bNoFsPowerUp = 0x0,
    .txPower = 0x9330,
    .pRegOverride = RF_pOverrides_IEEE250kbpsDSSS,
};

/* Frequency Synthesizer Command */
rfc_CMD_FS_t RF_cmdFs =
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
    .frequency = 0x09ab,
    .fractFreq = 0x0000,
    .synthConf.bTxMode = 0x1,
    .synthConf.refFreq = 0x0,
    .__dummy0 = 0x00,
    .__dummy1 = 0x00,
    .__dummy2 = 0x00,
    .__dummy3 = 0x0000,
};

/* IEEE TX Command */
rfc_CMD_IEEE_TX_t RF_cmdIEEETx =
{
    .commandNo                  = 0x2C01,
    .status                     = 0x0000,
    .pNextOp                    = 0, /*INSERT APPLICABLE POINTER: (uint8_t*)&xxx */
    .startTime                  = 0x00000000,
    .startTrigger.triggerType   = 0x0,
    .startTrigger.bEnaCmd       = 0x0,
    .startTrigger.triggerNo     = 0x0,
    .startTrigger.pastTrig      = 0x0,
    .condition.rule             = 0x1,
    .condition.nSkip            = 0x0,
    .txOpt.bIncludePhyHdr       = 0x0,
    .txOpt.bIncludeCrc          = 0x0,
    .txOpt.payloadLenMsb        = 0x0,
    .pPayload                   = 0x00000000,
    .payloadLen                 = 0x80,
    .timeStamp                  = 0x00000000,
};

/* IEEE RX Command */
rfc_CMD_IEEE_RX_t RF_cmdIEEERx =
{
    .commandNo                  = 0x2801,
    .status                     = 0x0000,
    .pNextOp                    = 0, /*INSERT APPLICABLE POINTER: (uint8_t*)&xxx */
    .startTime                  = 0x00000000,
    .startTrigger.triggerType   = 0x0,
    .startTrigger.bEnaCmd       = 0x0,
    .startTrigger.triggerNo     = 0x0,
    .startTrigger.pastTrig      = 0x0,
    .condition.rule             = 0x1,
    .condition.nSkip            = 0x0,
    .channel                    = 0x00,
    .rxConfig.bAutoFlushCrc     = 0x0,
    .rxConfig.bAutoFlushIgn     = 0x0,
    .rxConfig.bIncludePhyHdr    = 0x1,
    .rxConfig.bIncludeCrc       = 0x0,
    .rxConfig.bAppendRssi       = 0x1,
    .rxConfig.bAppendCorrCrc    = 0x1,
    .rxConfig.bAppendSrcInd     = 0x0,
    .rxConfig.bAppendTimestamp  = 0x1,
    .pRxQ                       = 0x00000000,
    .pOutput                    = 0x00000000,
    .frameFiltOpt.frameFiltEn   = 0x0,
    .frameFiltOpt.frameFiltStop = 0x0,
    .frameFiltOpt.autoAckEn     = 0x0,
    .frameFiltOpt.slottedAckEn  = 0x0,
    .frameFiltOpt.autoPendEn    = 0x0,
    .frameFiltOpt.defaultPend   = 0x0,
    .frameFiltOpt.bPendDataReqOnly  = 0x0,
    .frameFiltOpt.bPanCoord         = 0x0,
    .frameFiltOpt.maxFrameVersion   = 0x3,
    .frameFiltOpt.fcfReservedMask   = 0x0,
    .frameFiltOpt.modifyFtFilter    = 0x0,
    .frameFiltOpt.bStrictLenFilter  = 0x0,
    .frameTypes.bAcceptFt0Beacon    = 0x1,
    .frameTypes.bAcceptFt1Data      = 0x1,
    .frameTypes.bAcceptFt2Ack       = 0x1,
    .frameTypes.bAcceptFt3MacCmd    = 0x1,
    .frameTypes.bAcceptFt4Reserved  = 0x1,
    .frameTypes.bAcceptFt5Reserved  = 0x1,
    .frameTypes.bAcceptFt6Reserved  = 0x1,
    .frameTypes.bAcceptFt7Reserved  = 0x1,
    .ccaOpt.ccaEnEnergy         = 0x0,
    .ccaOpt.ccaEnCorr           = 0x0,
    .ccaOpt.ccaEnSync           = 0x0,
    .ccaOpt.ccaCorrOp           = 0x1,
    .ccaOpt.ccaSyncOp           = 0x1,
    .ccaOpt.ccaCorrThr          = 0x0,
    .ccaRssiThr                 = 0x64,
    .__dummy0                   = 0x0,
    .numExtEntries              = 0x00,
    .numShortEntries            = 0x00,
    .pExtEntryList              = 0x00000000,
    .pShortEntryList            = 0x00000000,
    .localExtAddr               = 0x0000000000000000,
    .localShortAddr             = 0x0000,
    .localPanID                 = 0x0000,
    .__dummy1                   = 0x0000,
    .__dummy2                   = 0x00,
    .endTrigger.triggerType     = 0x1,
    .endTrigger.bEnaCmd         = 0x0,
    .endTrigger.triggerNo       = 0x0,
    .endTrigger.pastTrig        = 0x0,
    .endTime                    = 0x00000000,
};

/***************************************************************************//**
* @brief      This function initializes and opens the RF driver for cc26xx
*
* @param[in]   phyMode  - Physical modulation type
*
* @return      Initialization status
******************************************************************************/
HAL_Status_t rf_cc26xx_init(HAL_PhyMode_t phyMode)
{
    HAL_Status_t status = HAL_ERR;
    rfc_dataEntryGeneral_t *pDataEntry;

    RF_Params_init(&RF_params);
    RF_params.nInactivityTimeout = 100; //In us
#if DMM_ENABLE && CONCURRENT_STACKS && DMM_FOR_6_TISCH
    RF_driverHandle = DMMSch_rfOpen(&RF_object, &RF_mode,
                       (RF_RadioSetup*)&RF_cmdRadioSetup, &RF_params);
#else //DMM_ENABLE
    RF_driverHandle = RF_open(&RF_object, &RF_mode,
                       (RF_RadioSetup*)&RF_cmdRadioSetup, &RF_params);
#endif //DMM_ENABLE
    if (RF_driverHandle != NULL)
    {
         /* Set up the data entery queue*/
        pDataEntry = (rfc_dataEntryGeneral_t*)RF_rxDataEntryBuffer;
        pDataEntry->status = DATA_ENTRY_PENDING;
        pDataEntry->config.type = DATA_ENTRY_TYPE_GEN;
        pDataEntry->config.lenSz = 0;
        pDataEntry->length = RF_MAX_PACKET_LENGTH + RF_MAX_APPEND_BYTES;
        pDataEntry->pNextEntry = (uint8_t *)RF_rxDataEntryBuffer;

        RF_dataQueue.pCurrEntry = (uint8_t*)pDataEntry;
        RF_dataQueue.pLastEntry = NULL;

        /* Set the Data Entity queue for received data */
        RF_cmdIEEERx.pRxQ    = &RF_dataQueue;
        /* Set rx statistics output struct */
        RF_cmdIEEERx.pOutput  = &RF_rxStatistics;

        /* Set to active power state initially*/
        Power_setConstraint(PowerCC26XX_SD_DISALLOW);
        Power_setConstraint(PowerCC26XX_IDLE_PD_DISALLOW);
//        Power_setConstraint(PowerCC26XX_SB_DISALLOW);

        status = HAL_SUCCESS;
    }
    return status;
}

/***************************************************************************//**
* @brief      This function transmits cc26xx radio packets.
*
* @param[in]   pCb - Pointer to transmit callback function
* @param[in]   *pBuf - Pointer to transmit packet buffer
* @param[in]   bufLen - Buffer length
* @param[in]   startTime - Relative time to begin packet transmission in unit
*                            of microseconds (0 for immediate).
*
* @return      Transmit status.
******************************************************************************/
HAL_Status_t rf_cc26xx_transmit(RF_Callback pCb, uint8_t* pBuf, uint16_t bufLen,
                                uint32_t startTime)
{
    HAL_Status_t status = HAL_ERR;
//    RF_EventMask result;
    RF_CmdHandle cmdHndl;

    if ((pBuf == NULL) || (bufLen > RF_MAX_PACKET_LENGTH))
        return status;

     /* Clear the TX status */
    RF_cmdIEEETx.status = 0;

    /* Set the TX buffer*/
    RF_cmdIEEETx.payloadLen = bufLen;
    RF_cmdIEEETx.pPayload = pBuf;

    if(startTime != 0)
    {
        /* Start in the future*/
        RF_cmdIEEETx.startTrigger.triggerType =  TRIG_ABSTIME;
        RF_cmdIEEETx.startTrigger.pastTrig = 1;
        RF_cmdIEEETx.startTime = startTime;
    }
    else
    {
        /* Immediate start*/
        RF_cmdIEEETx.startTrigger.triggerType = TRIG_NOW;
        RF_cmdIEEETx.startTime = 0;
    }

     /* Post IEEE TX command*/
#if DMM_ENABLE && CONCURRENT_STACKS && DMM_FOR_6_TISCH
    cmdHndl = DMMSch_rfPostCmd(RF_driverHandle,(RF_Op*)&RF_cmdIEEETx, RF_PriorityNormal,
                        pCb,RF_EventLastFGCmdDone);
#else //DMM_ENABLE
    cmdHndl = RF_postCmd(RF_driverHandle,(RF_Op*)&RF_cmdIEEETx, RF_PriorityNormal,
                        pCb,RF_EventLastFGCmdDone);
#endif //DMM_ENABLE
    /* Error when the returned command handle is less than 0*/
    if(cmdHndl >= 0)
    {
        status = HAL_SUCCESS;
    }

//    result = RF_pendCmd(RF_driverHandle, cmdHnd,  (RF_EventLastFGCmdDone |
//                                            RF_EventCmdError));
//
//    if (result & RF_EventLastFGCmdDone)
//    {
//        status = HAL_SUCCESS;
//    }
//
   return status;
}

/***************************************************************************//**
* @brief      This function turns cc26xx radio receive on.
*
* @param[in]   pCb - Pointer to receuve callback function.
* @param[in]   startTime - Relative time to turn radio on in unit
*                            of microseconds (0 for immediate).
*
* @return      Radio on status
******************************************************************************/
HAL_Status_t rf_cc26xx_receiveOn(RF_Callback pCb, uint32_t startTime)
{
    HAL_Status_t status = HAL_ERR;

    rfc_dataEntryGeneral_t *pDataEntry;

    /* Set data entry to pending status */
    pDataEntry = (rfc_dataEntryGeneral_t*)RF_rxDataEntryBuffer;
    pDataEntry->status = DATA_ENTRY_PENDING;

    /* Clear the Rx status */
    RF_cmdIEEERx.status = 0;

    if(startTime != 0)
    {
        /* Start in the future*/
        RF_cmdIEEERx.startTrigger.triggerType = TRIG_ABSTIME;
        RF_cmdIEEERx.startTime = startTime;
    }
    else
    {
        /* Immediate start*/
        RF_cmdIEEERx.startTrigger.triggerType = TRIG_NOW;
        RF_cmdIEEERx.startTime = 0;
    }
    
    /* RX will not end until command cancel */
    RF_cmdIEEERx.startTrigger.pastTrig = 1;
    RF_cmdIEEERx.endTrigger.triggerType = TRIG_NEVER;
    RF_cmdIEEERx.endTime = 0;
    
    /* Clear the Rx statistics structure */
    memset(&RF_rxStatistics, 0, sizeof(rfc_ieeeRxOutput_t));

    /* Post IEEE RX command*/
#if DMM_ENABLE && CONCURRENT_STACKS && DMM_FOR_6_TISCH
    RF_rxCmdHndl = DMMSch_rfPostCmd(RF_driverHandle, (RF_Op*)&RF_cmdIEEERx,
                              RF_PriorityNormal, pCb,
                              RF_EventMdmSoft|RF_EventRxOk);
#else //DMM_ENABLE
    RF_rxCmdHndl = RF_postCmd(RF_driverHandle, (RF_Op*)&RF_cmdIEEERx,
                              RF_PriorityNormal, pCb,
                              RF_EventMdmSoft|RF_EventRxOk);
#endif //DMM_ENABLE
    /* Error when the returned command handle is less than 0*/
    if(RF_rxCmdHndl >= 0)
    {
        status = HAL_SUCCESS;
    }

    return status;
}

/***************************************************************************//**
* @brief      This function sets cc26xx 2.4 GHz radio channel.
*
* @param[in]   channel - radio channel
*
* @return      Radio channel set status.
******************************************************************************/
HAL_Status_t rf_cc26xx_setChannel(uint16_t channel)
{
    /* 2.4 GHz 802.15.4 channels ranges from 11 to 26*/
    if(channel < 11 || channel > 26)
         return HAL_INVALID_PARAMETER;

    /* Each channel is 5 MHz apart, starting with 2405 MHz*/
    RF_cmdFs.status = 0;
    RF_cmdFs.frequency = 2405 + (channel - 11) * 5;
    RF_cmdFs.fractFreq = 0;

    /* Post FS command*/
#if DMM_ENABLE && CONCURRENT_STACKS && DMM_FOR_6_TISCH
    DMMSch_rfPostCmd(RF_driverHandle, (RF_Op*)&RF_cmdFs, RF_PriorityNormal,
                        NULL, RF_EventLastCmdDone);
#else //DMM_ENABLE
    RF_runCmd(RF_driverHandle, (RF_Op*)&RF_cmdFs, RF_PriorityNormal,
                        NULL, RF_EventLastCmdDone);
#endif //DMM_ENABLE
    return HAL_SUCCESS;
}
