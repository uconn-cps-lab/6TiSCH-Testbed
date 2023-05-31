/******************************************************************************

 @file dmm_policy.h

 @brief dmm policy Header

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
/*!****************************************************************************
 *  @file  dmm_policy.h
 *
 *  @brief      Dual Mode Policy Manager
 *
 *  The DMMPolicy interface provides a service to for the stack applications to
 *  update the stack states, which is then used to make scheduling decisions
 *
 *
 *  # Usage #
 *
 *  To use the DMMPolicy module to set the scheduling policy, the application
 *  calls the following APIs:
 *    - DMMPolicy_init(): Initialize the DMMPolicy module/task.
 *    - DMMPolicy_Params_init():  Initialize a DMMPolicy_Params structure
 *      with default values.  Then change the parameters from non-default
 *      values as needed.
 *    - DMMPolicy_open():  Open an instance of the DMMPolicy module,
 *      passing the initialized parameters.
 *    - Stack A/B application - DMMPolicy_updateStackState: Update the policy
 *                              used by DMMSch for scheduling RF commands from
 *                              stack A and B
 *
 *   An example of a policy table (define in a use case specific policy header,
 *   such as dmm_policy_blesp_wsnnode.h)
 *
 *   \code
 *   // The supported stack states bit map for BLE Simple Peripheral
 *   #define DMMPOLICY_STACKSTATE_BLEPERIPH_ADV          0x00000001 // State for BLE Simple Peripheral when trying to connect
 *   #define DMMPOLICY_STACKSTATE_BLEPERIPH_CONNECTING   0x00000002 // State for BLE Simple Peripheral when connect
 *   #define DMMPOLICY_STACKSTATE_BLEPERIPH_CONNECTED    0x00000004 // State for BLE Simple Peripheral when connect
 *   #define DMMPOLICY_STACKSTATE_BLEPERIPH_ANY          0xFFFFFFFF // Allow any policy
 *
 *   // The supported stack states bit map for Wsn Node
 *   #define DMMPOLICY_STACKSTATE_WSNNODE_SLEEPING   0x00000001 // State for Wsn Node when sleeping
 *   #define DMMPOLICY_STACKSTATE_WSNNODE_TX         0x00000002 // State for Wsn Node when transmitting
 *   #define DMMPOLICY_STACKSTATE_WSNNODE_RX         0x00000004 // State for Wsn Node when receiving
 *   #define DMMPOLICY_STACKSTATE_WSNNODE_ANY        0xFFFFFFFF // Allow any policy
 *
 *   // The policy table for the BLE Simple Peripheral and WSN Node use case
 *   DMMPolicy_policyTableEntry_t DMMPolicy_wsnNodeBleSpPolicyTable[] = {
 *   //State 1: BleSp connectable advertisements and Wsn Node TX or Ack: BleSp  = Low Priority | Time None Critical, WsnNode = High Priority | Time Critical
 *       {
 *         {DMMPolicy_StackType_BlePeripheral, DMMPolicy_StackType_WsnNode},
 *         {(DMMPOLICY_STACKSTATE_BLEPERIPH_ADV) , (DMMPOLICY_STACKSTATE_WSNNODE_SLEEPING | DMMPOLICY_STACKSTATE_WSNNODE_TX | DMMPOLICY_STACKSTATE_WSNNODE_RX)},
 *         {DMMPOLICY_PRIORITY_LOW, DMMPOLICY_TIME_NONE_CRITICAL, //BLE SP Stack
 *          DMMPOLICY_PRIORITY_HIGH, DMMPOLICY_TIME_NONE_CRITICAL} //WSN NODE Stack
 *       },
 *   //State 2: BleSp connecting or connected and Wsn Node Tx: BleSp  = High Priority | Time Critical, WsnNode = Low Priority | Time Critical
 *       {
 *         {DMMPolicy_StackType_BlePeripheral, DMMPolicy_StackType_WsnNode},
 *         {(DMMPOLICY_STACKSTATE_BLEPERIPH_CONNECTING | DMMPOLICY_STACKSTATE_BLEPERIPH_CONNECTED) , (DMMPOLICY_STACKSTATE_WSNNODE_TX)},
 *         {DMMPOLICY_PRIORITY_HIGH, DMMPOLICY_TIME_CRITICAL, //BLE SP Stack
 *          DMMPOLICY_PRIORITY_LOW, DMMPOLICY_TIME_NONE_CRITICAL} //WSN NODE Stack
 *       },
 *   //State 2: BleSp connecting or connected and Wsn Node Tx: BleSp  = High Priority | Time Critical, WsnNode = Low Priority | Time Critical
 *       {
 *         {DMMPolicy_StackType_BlePeripheral, DMMPolicy_StackType_WsnNode},
 *         {(DMMPOLICY_STACKSTATE_BLEPERIPH_CONNECTING | DMMPOLICY_STACKSTATE_BLEPERIPH_CONNECTED) , (DMMPOLICY_STACKSTATE_WSNNODE_RX)},
 *         {DMMPOLICY_PRIORITY_HIGH, DMMPOLICY_TIME_CRITICAL, //BLE SP Stack
 *          DMMPOLICY_PRIORITY_LOW, DMMPOLICY_TIME_NONE_CRITICAL} //WSN NODE Stack
 *       },
 *   //Default State: If matching state is not found default to: BleSp  = High Priority | Time Critical, WsnNode = Low Priority | none Time Critical
 *       {
 *         {DMMPolicy_StackType_BlePeripheral, DMMPolicy_StackType_WsnNode},
 *         {DMMPOLICY_STACKSTATE_BLEPERIPH_ANY, DMMPOLICY_STACKSTATE_WSNNODE_ANY},
 *         {DMMPOLICY_PRIORITY_HIGH, DMMPOLICY_TIME_CRITICAL, //BLE SP Stack
 *         DMMPOLICY_PRIORITY_LOW, DMMPOLICY_TIME_NONE_CRITICAL} //WSN NODE Stack
 *       },
 *   };
 *
 *   // The policy table size for the BLE Simple Peripheral and WSN Node use case
 *   uint32_t DMMPolicy_wsnNodeBleSpPolicyTableSize = (sizeof(DMMPolicy_wsnNodeBleSpPolicyTable) / sizeof(DMMPolicy_policyTableEntry_t));
 *   \endcode
 *
 ********************************************************************************/

#ifndef DMMPolicy_H_
#define DMMPolicy_H_

#include "stdint.h"
#include <ti/drivers/rf/RF.h>

//! \brief number of stacks supported by this policy manager
#define DMMPOLICY_NUM_STACKS    2

//! \brief stack priotiy
#define DMMPOLICY_PRIORITY_LOW     RF_PriorityNormal
#define DMMPOLICY_PRIORITY_HIGH    RF_PriorityHigh

//! \brief stack timing
#define DMMPOLICY_TIME_NONE_CRITICAL    0
#define DMMPOLICY_TIME_CRITICAL         1

//! \brief the stack types supported
typedef enum
{
    DMMPolicy_StackType_invalid = 0,          //!< invalid stack type
    DMMPolicy_StackType_BlePeripheral,  //!< stack type for a BLE Simple Peripheral
    DMMPolicy_StackType_WsnNode,              //!< stack type for an EasyLink Wireless Sensor Network Node
} DMMPolicy_StackType;

//! \brief Structure used to decide the policy for a particular stack state
typedef struct
{
  RF_Priority     priority;      //!< 0 being highest priority
  uint16_t     timingConstrant;  //!< 0=Time critical and cannot be changed, 0xFFFF=No timing constraint, 1-0xFFFE timing constraint in ms units
} DMMPolicy_Policy;

//! \brief policy table entry
typedef struct
{
    DMMPolicy_StackType stackType[DMMPOLICY_NUM_STACKS];
    uint32_t stackStateBitMask[DMMPOLICY_NUM_STACKS];
    DMMPolicy_Policy policy[DMMPOLICY_NUM_STACKS];
} DMMPolicy_PolicyTableEntry;


/** @brief RF parameter struct
 *  DMM Scheduler parameters are used with the DMMPolicy_open() and DMMPolicy_Params_init() call.
 */
typedef struct {
    DMMPolicy_PolicyTableEntry *policyTable; //!< policy table to be used for the DMM use case
    uint32_t numPolicyTableEntries;            //!< entries in policy table
} DMMPolicy_Params;

/** @brief Status codes for various DMM Policy functions.
 *
 *  RF_Stat is reported as return value for DMM Policy functions.
 */
typedef enum {
    DMMPolicy_StatusError,          ///< Error
    DMMPolicy_StatusNoPolicyError,  ///< Error with policy table
    DMMPolicy_StatusParamError,     ///< Parameter Error
    DMMPolicy_StatusSuccess         ///< Function finished with success
} DMMPolicy_Status;

/** @brief  Function to initialize the DMMPolicy_Params struct to its defaults
 *
 *  @param  params      An pointer to RF_Params structure for
 *                      initialization
 *
 *  Defaults values are:
 */
extern void DMMPolicy_Params_init(DMMPolicy_Params *params);

/** @brief  Function that initializes the DMMPolicy module
 *
 */
extern void DMMPolicy_init(void);

/** @brief  Function to open the DMMPolicy module
 *
 *  @param  params      An pointer to RF_Params structure for initialization
 *
 *  @return DMMPolicy_Stat status
 */
extern DMMPolicy_Status DMMPolicy_open(DMMPolicy_Params *params);

/** @brief  Updates the policy used to make scheduling decisions
 *
 *  @param  stackType     The stack type that has changed state
 *  @param  newState      The state the stack has changed to
 *
 *  @return DMMPolicy_Stat status
 */
extern DMMPolicy_Status DMMPolicy_updateStackState(DMMPolicy_StackType stackType, uint32_t newState);

#endif /* DMMPolicy_H_ */
