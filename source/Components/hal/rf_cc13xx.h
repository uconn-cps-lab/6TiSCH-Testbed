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
 *  ====================== rf_cc13xx.h =============================================
 *  Header file cc13xx specfic RF command functions.
 */

#ifndef RF__CC13XX__API
#define RF__CC13XX__API

#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
 * INCLUDES
 */
#include <driverlib/rf_common_cmd.h>
#include <driverlib/rf_mailbox.h>
#include <driverlib/rf_prop_cmd.h>
#include <driverlib/rf_data_entry.h>
#include <rf_patches/rf_patch_cpe_genfsk.h>
#include <rf_patches/rf_patch_rfe_genfsk.h>
#include <ti/drivers/rf/RF.h>

#include <vims.h>
#include <hw_memmap.h>

#include "hal_api.h"

/*******************************************************************************
* DEFINES
*/
#define RF_MAX_PACKET_LENGTH      (128)
#define RF_MAX_APPEND_BYTES       (10)

/*******************************************************************************
* EXTERNS
*/
extern RF_Handle RF_driverHandle;
extern RF_CmdHandle RF_rxCmdHndl;
extern uint8_t RF_rxDataEntryBuffer[1+sizeof(rfc_dataEntryGeneral_t)
                                   +RF_MAX_PACKET_LENGTH + RF_MAX_APPEND_BYTES];

/*******************************************************************************
 * FUNCTIONS
 */
HAL_Status_t rf_cc13xx_init(HAL_PhyMode_t phyMode);
HAL_Status_t rf_cc13xx_transmit(RF_Callback pCb, uint8_t* pBuf, uint16_t bufLen,
                                uint32_t startTime);
HAL_Status_t rf_cc13xx_receiveOn(RF_Callback pCb, uint32_t startTime);
HAL_Status_t rf_cc13xx_setChannel(uint16_t channel);

#ifdef __cplusplus
}
#endif

#endif //RF__CC13XX__API