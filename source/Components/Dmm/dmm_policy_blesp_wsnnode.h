/******************************************************************************

 @file dmm_policy_blesp_wsnnode.h

 @brief Policy for the BLE SImple Peripheral and WSN Node DMM use case

 Group: WCS LPC
 Target Device: CC13xx

 ******************************************************************************
 
 Copyright (c) 2017-2018, Texas Instruments Incorporated
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
#ifndef dmm_policy_blesp_wsnnode__H_
#define dmm_policy_blesp_wsnnode__H_

#include <dmm/dmm_policy.h>

/***** Defines *****/
//! \brief The supported stack states bit map for BLE Simple Peripheral
#define DMMPOLICY_STACKSTATE_BLEPERIPH_ADV          0x00000001 //!< State for BLE Simple Peripheral when advertising connectable
#define DMMPOLICY_STACKSTATE_BLEPERIPH_CONNECTING   0x00000002 //!< State for BLE Simple Peripheral when in the process of connecting to a master device
#define DMMPOLICY_STACKSTATE_BLEPERIPH_CONNECTED    0x00000004 //!< State for BLE Simple Peripheral when connected to a master device
#define DMMPOLICY_STACKSTATE_BLEPERIPH_ANY          0xFFFFFFFF //!< Allow any policy

//! \brief The supported stack states bit map for Wsn Node
#define DMMPOLICY_STACKSTATE_WSNNODE_SLEEPING   0x00000001 //!< State for Wsn Node when sleeping
#define DMMPOLICY_STACKSTATE_WSNNODE_TX         0x00000002 //!< State for Wsn Node when transmitting
#define DMMPOLICY_STACKSTATE_WSNNODE_RX         0x00000004 //!< State for Wsn Node when receiving
#define DMMPOLICY_STACKSTATE_WSNNODE_ANY        0xFFFFFFFF //!< Allow any policy

//! \brief The policy table for the BLE Simple Peripheral and WSN Node use case
extern DMMPolicy_PolicyTableEntry DMMPolicy_wsnNodeBleSpPolicyTable[];

//! \brief The policy table size for the BLE Simple Peripheral and WSN Node use case
extern uint32_t DMMPolicy_wsnNodeBleSpPolicyTableSize;

#endif //dmm_policy_blesp_wsnnode__H_
