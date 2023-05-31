/******************************************************************************

 @file dmm_policy_blesp_wsnnode.c

 @brief Dual Mode Manager Policy for WsnNode and BLE SP

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

#include <dmm/dmm_policy_blesp_wsnnode.h>

/***** Defines *****/

//! \brief The policy table for the BLE Simple Peripheral and WSN Node use case
DMMPolicy_PolicyTableEntry DMMPolicy_wsnNodeBleSpPolicyTable[] =
{
//State 1: BleSp connectable advertisements and Wsn Node TX or Ack: BleSp  = Low Priority | Time None Critical, WsnNode = High Priority | Time Critical
    {
      {DMMPolicy_StackType_BlePeripheral, DMMPolicy_StackType_WsnNode},
      {(DMMPOLICY_STACKSTATE_BLEPERIPH_ADV) , (DMMPOLICY_STACKSTATE_WSNNODE_SLEEPING | DMMPOLICY_STACKSTATE_WSNNODE_TX | DMMPOLICY_STACKSTATE_WSNNODE_RX)},
      {
         DMMPOLICY_PRIORITY_LOW, DMMPOLICY_TIME_NONE_CRITICAL, //BLE SP Stack
         DMMPOLICY_PRIORITY_HIGH, DMMPOLICY_TIME_NONE_CRITICAL
      } //WSN NODE Stack
    },
//State 2: BleSp connecting or connected and Wsn Node Tx: BleSp  = High Priority | Time Critical, WsnNode = Low Priority | Time None Critical
    {
      {DMMPolicy_StackType_BlePeripheral, DMMPolicy_StackType_WsnNode},
      {(DMMPOLICY_STACKSTATE_BLEPERIPH_CONNECTING | DMMPOLICY_STACKSTATE_BLEPERIPH_CONNECTED) , (DMMPOLICY_STACKSTATE_WSNNODE_TX)},
      {
         DMMPOLICY_PRIORITY_HIGH, DMMPOLICY_TIME_NONE_CRITICAL, //BLE SP Stack
         DMMPOLICY_PRIORITY_LOW, DMMPOLICY_TIME_NONE_CRITICAL
      } //WSN NODE Stack
    },
//State 3: BleSp connecting or connected and Wsn Node Ack Rx: BleSp  = High Priority | Time Critical, WsnNode = Low Priority | Time None Critical
    {
      {DMMPolicy_StackType_BlePeripheral, DMMPolicy_StackType_WsnNode},
      {(DMMPOLICY_STACKSTATE_BLEPERIPH_CONNECTING | DMMPOLICY_STACKSTATE_BLEPERIPH_CONNECTED) , (DMMPOLICY_STACKSTATE_WSNNODE_RX)},
      {
         DMMPOLICY_PRIORITY_HIGH, DMMPOLICY_TIME_NONE_CRITICAL, //BLE SP Stack
         DMMPOLICY_PRIORITY_LOW, DMMPOLICY_TIME_NONE_CRITICAL
      } //WSN NODE Stack
    },
//Default State: If matching state is not found default to: BleSp  = High Priority | Time Critical, WsnNode = Low Priority | none Time None Critical
    {
      {DMMPolicy_StackType_BlePeripheral, DMMPolicy_StackType_WsnNode},
      {DMMPOLICY_STACKSTATE_BLEPERIPH_ANY, DMMPOLICY_STACKSTATE_WSNNODE_ANY},
      {
         DMMPOLICY_PRIORITY_HIGH, DMMPOLICY_TIME_NONE_CRITICAL, //BLE SP Stack
         DMMPOLICY_PRIORITY_LOW, DMMPOLICY_TIME_NONE_CRITICAL
      } //WSN NODE Stack
    },
};

//! \brief The policy table size for the BLE Simple Peripheral and WSN Node use case
uint32_t DMMPolicy_wsnNodeBleSpPolicyTableSize = (sizeof(DMMPolicy_wsnNodeBleSpPolicyTable) / sizeof(DMMPolicy_PolicyTableEntry));
