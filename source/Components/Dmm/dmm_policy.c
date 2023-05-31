/******************************************************************************

 @file dmm_policy.c

 @brief Dual Mode Manager Policy

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

/***** Includes *****/
#include "dmm/dmm_policy.h"
#include "dmm/dmm_scheduler.h"

/***** Defines *****/

/***** Variable declarations *****/
DMMPolicy_Params DMMPolicyParams;

/***** Private function definitions *****/


/***** Public function definitions *****/

/** @brief  Function to initialize the DMMPolicy_Params struct to its defaults
 *
 *  @param  params      An pointer to RF_Params structure for
 *                      initialization
 *
 */
void DMMPolicy_Params_init(DMMPolicy_Params *params)
{
    params->policyTable = NULL;
    params->numPolicyTableEntries = 0;
}

/** @brief  Function that initializes the DMMPolicy module
 *
 */
void DMMPolicy_init(void)
{

}

/** @brief  Function to open the DMMPolicy module
 *
 *  @param  params      An pointer to RF_Params structure for initialization
 *
 *  @return DMMPolicy_Stat status
 */
DMMPolicy_Status DMMPolicy_open(DMMPolicy_Params *params)
{
    DMMPolicy_Status status = DMMPolicy_StatusError;

    /* Populate default RF parameters if not provided */
    if (params != NULL)
    {
        memcpy(&DMMPolicyParams, params, sizeof(DMMPolicy_Params));
        status = DMMPolicy_StatusSuccess;
    }
    else
    {
        status = DMMPolicy_StatusParamError;
    }

    return status;
}

/** @brief  Updates the policy used to make scheduling decisions
 *
 *  @param  stackType     The stack type that has changed state
 *  @param  newState      The state the stack has changed to
 *
 *  @return DMMPolicy_Stat status
 */
DMMPolicy_Status DMMPolicy_updateStackState(DMMPolicy_StackType stackType, uint32_t newState)
{
    DMMPolicy_Status status = DMMPolicy_StatusNoPolicyError;
    uint8_t policyIdx;
    uint8_t stackTypeIdx;
    static uint32_t currentStackStateMask[DMMPOLICY_NUM_STACKS] = {0xFF, 0xFF};
    uint8_t stackStateTemp;
    bool localStateUdated = false;

    /* index through policy's and update local state*/
    for(policyIdx = 0; (policyIdx < DMMPolicyParams.numPolicyTableEntries) && !localStateUdated; policyIdx++)
    {
        //find matching stack type;
        for(stackTypeIdx = 0; stackTypeIdx < DMMPOLICY_NUM_STACKS; stackTypeIdx++)
        {
            if(DMMPolicyParams.policyTable[policyIdx].stackType[stackTypeIdx] == stackType)
            {
                //Store incase we can not find any matching policies
                stackStateTemp = currentStackStateMask[stackTypeIdx];

                //update local stack state
                currentStackStateMask[stackTypeIdx] =  newState;
                localStateUdated = true;

                break;
            }
        }
    }

    if(localStateUdated)
    {
        bool policyFound = false;

        /* index through policy's and update local state*/
        for(policyIdx = 0; policyIdx < DMMPolicyParams.numPolicyTableEntries; policyIdx++)
        {
            if( (DMMPolicyParams.policyTable[policyIdx].stackStateBitMask[0] & currentStackStateMask[0]) &&
                (DMMPolicyParams.policyTable[policyIdx].stackStateBitMask[1] & currentStackStateMask[1]) )
            {
                /* valid policy found, update scheduler */
                policyFound = true;
                DMMSch_updatePolicy(DMMPolicyParams.policyTable[policyIdx]);

                status = DMMPolicy_StatusSuccess;
                break;
            }
        }

        if(!policyFound)
        {
            /* restore the previous stack state */
            currentStackStateMask[stackTypeIdx] = stackStateTemp;
        }
    }

    return status;
}
