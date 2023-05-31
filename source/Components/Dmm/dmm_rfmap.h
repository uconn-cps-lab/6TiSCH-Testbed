/******************************************************************************

 @file dmm_rfmap.h

 @brief remaps the RF driver API to DMM

 Group: WCS LPC
 Target Device: CC13xx

 ******************************************************************************
 
 Copyright (c) 2016-2018, Texas Instruments Incorporated
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 *  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 *  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 *  Neither the name of Texas Instruments Incorporated nor the names of
    its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ******************************************************************************
 Release Name: simplelink_cc13x2_sdk_2_10_00_
 Release Date: 2018-04-09 15:16:26
 *****************************************************************************/
/*!****************************************************************************
 *  @file  dmm_rfmap.h
 *
 *  @brief      Dual Mode Manager RF API remapping
 *
 *  The dmm_rfmap header file remaps select functions in the RF driver API the
 *  DMM equivalents that are defined in the dmm_schedular module so that
 *  priorities and timing can be set according to the dual mode manager policy
 *
 ********************************************************************************/

#ifndef dmm_rfmap_H_
#define dmm_rfmap_H_

#include <ti/drivers/rf/RF.h>
#include <dmm/dmm_scheduler.h>
#if DMM_FOR_BLE  //Feng: DMM is useless for 6tisch, won't work period if the WSN radio is busy,  cancels the BLE beacon 100%
#define RF_open             DMMSch_rfOpen
#define RF_postCmd          DMMSch_rfPostCmd
#define RF_runCmd           DMMSch_rfRunCmd
#define RF_scheduleCmd      DMMSch_rfScheduleCmd
#define RF_runScheduleCmd   DMMSch_rfRunScheduleCmd
#define RF_cancelCmd        DMMSch_rfCancelCmd
#define RF_flushCmd         DMMSch_rfFlushCmd
#define RF_runImmediateCmd  DMMSch_rfRunImmediateCmd
#define RF_runDirectCmd     DMMSch_rfRunDirectCmd
#endif  //DMM_FOR_BLE
#endif /* dmm_rfmap_H_ */
