/******************************************************************************

 @file dmm_scheduler.c

 @brief Dual Mode Manager Scheduler

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
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <xdc/runtime/Error.h>

#include "dmm/dmm_policy.h"
#include "dmm/dmm_scheduler.h"
#include <dmm/dmm_policy_blesp_wsnnode.h>

#include <ti/drivers/dpl/HwiP.h>
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(rf_patches/rf_patch_cpe_multi_protocol.h)

/* Board Header files */
#include "board.h"

/***** Defines *****/
#define MAX_DMM_CLIENTS 2

#define MAC_CMD_CBS 8

#define RF_GET_SCHEDULE_MAP_CURR_CMD_IDX    RF_SCH_MAP_CURRENT_CMD_OFFSET
#define RF_GET_SCHEDULE_MAP_PENDING_CMD_IDX RF_SCH_MAP_PENDING_CMD_OFFSET

/***** Type declarations *****/
typedef struct
{
    Task_Handle* pTaskHndl;
    DMMPolicy_StackType stackType;
    RF_Handle clientHndl;
    RF_ClientCallback rfClientCb;
} DMMSch_Client_t;

typedef struct
{
    RF_Handle h;
    RF_CmdHandle cmdHndl;
    RF_Callback cb;
    RF_EventMask bmEvent;
} rfCmdCb;

/***** Variable declarations *****/

/* todo: consider making this a link list */
DMMSch_Client_t dmmClients[MAX_DMM_CLIENTS] = {0};
uint32_t numDmmClients = 0;

DMMPolicy_PolicyTableEntry currentPolicy;

//Mutex for locking the RF driver resource
static Semaphore_Handle DmmSchedulerMutex;

// table for holding command callbacks used to intercept
//callbacks for client when commands are aborted to take
//corrective action
static rfCmdCb cmdCbTable[MAC_CMD_CBS];

static uint8_t unSchCnt = 0;


/***** Private function definitions *****/
static void clientRfCmdCb(RF_Handle h, RF_CmdHandle ch, RF_EventMask e);
static int16_t getClientRfCmdCbSlot(void);
static DMMSch_Client_t* getClient(void);
static DMMPolicy_StackType getClientStackType(RF_Handle clientHndl);
static void setSchedule(RF_Handle h, RF_Op* pOp, RF_ScheduleCmdParams *pSchParams);
static RF_CmdHandle cmdSchedule(RF_Handle h, RF_Op* pOp, RF_ScheduleCmdParams* pSchParams, RF_Callback pCb, RF_EventMask bmEvent);
static bool schResolveConflict(RF_ScheduleMapElement* new, RF_ScheduleMapElement* old);
static bool isClientOwner(RF_Handle h, RF_ScheduleMapElement* pCmd);
static RF_ScheduleMapElement* checkForPreemption(RF_Handle h2, RF_Op* pOp_new, RF_ScheduleCmdParams *pSchParams);
static RF_Stat preemptRFCmd(RF_ScheduleMapElement* pCmdSchMapElem, RF_Priority prio);
static RF_CmdHandle rfScheduleCmdWithAbortWorkaround(RF_Handle h, RF_Op* pOp, RF_ScheduleCmdParams* pSchParams, RF_Callback pCb, RF_EventMask bmEvent);


/*
 *  This is the DMM level callback function wrapper that is passed to RF_scheduleCmd.
 *      This function wraps the regular client callback function passed from app/stack.
 *      The regular client callback function was saved in cmdCbTable in schedule time.
 *      After the rf command is finished, this wrapper callback is called. Any DMM
 *      specific actions can be done here. After that, this function looks up and
 *      call the regular client callback function.
 *
 *  Input:  h                    - RF_Handle
 *          ch                   - Command handle
 *          e                    - event mask
 *
 *  Return: void
 */
static void clientRfCmdCb(RF_Handle h, RF_CmdHandle ch, RF_EventMask e)
{
    int16_t rfCmdIdx;

    //find the callback index for this command
    for(rfCmdIdx = 0; rfCmdIdx < MAC_CMD_CBS; rfCmdIdx++)
    {
        if (cmdCbTable[rfCmdIdx].cmdHndl == ch)
        {
            /* check if client provided a command callback */
            if ((cmdCbTable[rfCmdIdx].cb) && (e & cmdCbTable[rfCmdIdx].bmEvent))
            {
                /* call clients command callback */
                cmdCbTable[rfCmdIdx].cb(h, ch, e);
            }

            if(e & RF_EventLastCmdDone)
            {
                /* last command in chain, there will be no more callbacks for this command,
                 * free table entry
                 */
                cmdCbTable[rfCmdIdx].cmdHndl = -1;
            }
        }
    }
}

/*
 *  Find an available callback function slot from the callback table.
 *      This table is used to store client callback functions
 *
 *  Input:  void                 
 *
 *  Return: uint8_t              - an empty entry in the table.
 *
 */
static int16_t getClientRfCmdCbSlot(void)
{
    int16_t rfCmdIdx;

    //find an empty slot
    for(rfCmdIdx = 0; rfCmdIdx < MAC_CMD_CBS; rfCmdIdx++)
    {
        if (cmdCbTable[rfCmdIdx].cmdHndl < 0)
        {
            break;
        }
    }

    if(rfCmdIdx >= MAC_CMD_CBS)
    {
        rfCmdIdx = -1;
    }

    return rfCmdIdx;
}

/*
 *  Find which client is running in the current task
 *
 *  Input:  void
 *
 *  Return: DMMSch_Client_t      - DMM client pointer
 *
 */
static DMMSch_Client_t* getClient(void)
{
    uint32_t clientIdx;

    /* map task handle to rfHandle returned */
    for(clientIdx = 0; clientIdx < MAX_DMM_CLIENTS; clientIdx++)
    {
        Task_Handle currentTask = Task_self();
        /* If the clients task equals this ask and there is no
         * client handle registered
         */
        if (*(dmmClients[clientIdx].pTaskHndl) == currentTask)
        {
            return &(dmmClients[clientIdx]);
        }
    }

    return NULL;
}

/*
 *  Find the given client stack type
 *
 *  Input:  clientHndl           - RF_Handle
 *
 *  Return: DMMPolicy_StackType  - DMM stack type
 *
 */
static DMMPolicy_StackType getClientStackType(RF_Handle clientHndl)
{
    uint32_t clientIdx;

    /* map task handle to rfHandle returned */
    for(clientIdx = 0; clientIdx < MAX_DMM_CLIENTS; clientIdx++)
    {
        /* If the clients task equals this ask and there is no
         * client handle registered
         */
        if (dmmClients[clientIdx].clientHndl == clientHndl)

        {
            return dmmClients[clientIdx].stackType;
        }
    }

    return DMMPolicy_StackType_invalid;
}

/*
 *  Adjust the rf command settings based on DMM policy
 *
 *  Input:  h                    - RF_Handle
 *          pOp                  - RF_Op pointer to the requesting command
 *          pSchParams           - RF schedule parameters pointer
 *  Return: void
 *
 */
static void setSchedule(RF_Handle h, RF_Op* pOp, RF_ScheduleCmdParams *pSchParams)
{
    uint8_t policyStackIdx;
    DMMPolicy_StackType clientStackType = getClientStackType(h);

    /* find the stack policy index */
    for(policyStackIdx = 0; policyStackIdx < DMMPOLICY_NUM_STACKS; policyStackIdx++)
    {
        if(clientStackType == currentPolicy.stackType[policyStackIdx])
        {
            break;
        }
    }

    if(policyStackIdx >= DMMPOLICY_NUM_STACKS)
    {
        /* can not find a valid policy, return without changing the scheduling */
        return;
    }

    /* override the priority based on the policy */
    pSchParams->priority = currentPolicy.policy[policyStackIdx].priority;

    /* override the ability to shift the command based on policy */
    if(currentPolicy.policy[policyStackIdx].timingConstrant == DMMPOLICY_TIME_CRITICAL)
    {
        pOp->startTrigger.pastTrig = 0;
        pSchParams->allowDelay = RF_AllowDelayNone;
    }
    else
    {
        pOp->startTrigger.pastTrig = 1;
        pSchParams->allowDelay = RF_AllowDelayAny;
    }


}

/*
 *  Check if the requesting command can preempt the conflicting command.
 *
 *  Input:  new                  - RF_Cmd pointer to the requesting command
 *          old                  - RF_Cmd pointer to the conflicting command
 *  Return: bool                 - Indicate if the requesting command can preempt the conflicting
 *                                 command.
 */
static bool schResolveConflict(RF_ScheduleMapElement* new, RF_ScheduleMapElement* old)
{
    return ((new->priority > old->priority ? true : false));
}

/*
 *  Check if the requesting command belongs to the given client.
 *
 *  Input:  h                  - RF handle to be checked
 *          pCmd               - RF command
 *  Return: bool               - Indicate if the command belongs to this RF handle.
 *
 */
static bool isClientOwner(RF_Handle h, RF_ScheduleMapElement* pCmd)
{
    if (pCmd && (pCmd->pClient == h))
    {
        return true;
    }
    else
    {
        return false;
    }
}
/*
 *  Check if the requesting command can be scheduled by preempting the running command
 *  or any of the commands in the queue belonging to the other client.
 *
 *  Input:  preemptionChkClient     - other client RF_Handle to be checked against
 *          pOp_new                 - RF_Op pointer to the requesting command
 *          pSchParams              - Pointer to the schedule command parameter structure
 *  Return: RF_ScheduleMapElement*  - RF_ScheduleMapElement pointer to the first command to be preempted.
 *                                     Return NULL pointer if there is no commands from the other client running/queued.
 */
static RF_ScheduleMapElement* checkForPreemption(RF_Handle preemptionChkClient, RF_Op* pOp_new, RF_ScheduleCmdParams *pSchParams)
{

     bool canBePreempted = false;
     RF_ScheduleMapElement* pPreemptCmd = NULL;
     RF_ScheduleMapElement  CmdSchMapElemNew;  // new command

     RF_InfoVal rfInfoVal;
     RF_ScheduleMapElement schMap[RF_NUM_SCHEDULE_MAP_ENTRIES] = {0};

     uint32_t kk;

     rfInfoVal.pScheduleMap = schMap;
     RF_getInfo(preemptionChkClient, RF_GET_SCHEDULE_MAP, &rfInfoVal);

    RF_ScheduleMapElement* pCmdElemCurr =
            &schMap[RF_GET_SCHEDULE_MAP_CURR_CMD_IDX];

     // for new command, only priority is needed. Other commands are not needed for preemption check.
     CmdSchMapElemNew.priority = pSchParams->priority;

    /* Per default, no commands belonging to h2 is found in the queue */
    bool noH2Cmd = true;

    /* Check if we can preempt the running command */
    if (isClientOwner(preemptionChkClient, pCmdElemCurr))
    {
        noH2Cmd = false;
        canBePreempted = schResolveConflict(&CmdSchMapElemNew, pCmdElemCurr);
        if (canBePreempted)
        {
            pPreemptCmd = pCmdElemCurr;
        }
    }

    /* Check if it is possible to preempt any of the commands in the queue */
    kk = RF_GET_SCHEDULE_MAP_PENDING_CMD_IDX;

    //schMap was memset to 0. So, if there is no command, the pClient must be zero.
    //                            If there is a command,  the pCliend must NOT be zero.
    while (schMap[kk].pClient)
    {
        if (isClientOwner(preemptionChkClient, &schMap[kk]))
        {
            noH2Cmd = false;

            canBePreempted = schResolveConflict(&CmdSchMapElemNew, &schMap[kk]);
            if (canBePreempted && !pPreemptCmd)
            {
                pPreemptCmd = &schMap[kk];
            }

            /* Any individual command found in the queue which fails on
               the preemption criterion will clear the preemption pointer. */
            if (!canBePreempted)
            {
                pPreemptCmd = NULL;
            }
        }
        kk++;
    }

    /* If there was no commands belonging to h2 in the queue,
       return with the requesting command */
    if (noH2Cmd)
    {
        pPreemptCmd = NULL;  // no preemption
    }

    return pPreemptCmd;
}


/*
 *  Preempts (flush) the preempted commands
 *  or any of the commands in the queue belonging to the other client.
 *
 *  Input:  pCmdSchMapElem          - RF commands in the command Q to be flushed
 *          prio                    - Command priority
 *  Return: RF_Stat                 - Flush command execution status
 */
static RF_Stat preemptRFCmd(RF_ScheduleMapElement* pCmdSchMapElem, RF_Priority prio)
{

    /* Abort the running command per default*/
    uint8_t stopOrAbort = 0;

    /* If the priority of the preempted command is normal, stop the running command */
    if (prio == RF_PriorityHigh)
    {
        stopOrAbort = RF_ABORT_GRACEFULLY; //RF_SCH_CMD_PRI_PREEMPT_STOP;
    }

    /* Abort multiple radio commands implicitly, mark them as preempted */
    return (RF_flushCmd(pCmdSchMapElem->pClient, pCmdSchMapElem->ch,
                        (stopOrAbort | RF_ABORT_PREEMPTION)));
}

/*
 *  A wrapper function to run scheduleCmd with callback workaround.
 */
static RF_CmdHandle rfScheduleCmdWithAbortWorkaround(RF_Handle h, RF_Op* pOp, RF_ScheduleCmdParams* pSchParams, RF_Callback pCb, RF_EventMask bmEvent)
{
    RF_CmdHandle cmdHndl = RF_SCHEDULE_CMD_ERROR;
    uint32_t key;
    int16_t ClientRfCmdCbIdx = -1;
    bool cmdSubmitted = false;

    /* enable critical section in case callback is trigger in between submitting the
     * command and storing the callback
     */
    key = HwiP_disable();

    /* check if a callback is needed */
    if(pCb)
    {
        //get a slot in the cmd call back table
        ClientRfCmdCbIdx = getClientRfCmdCbSlot();

        if ((ClientRfCmdCbIdx > -1) && (ClientRfCmdCbIdx < MAC_CMD_CBS))
        {
            RF_EventMask tmpBmEvent = bmEvent;

            cmdHndl = RF_scheduleCmd(h, pOp, pSchParams, clientRfCmdCb, bmEvent);
            if(cmdHndl >= 0)
            {
               cmdCbTable[ClientRfCmdCbIdx].h = h;
               cmdCbTable[ClientRfCmdCbIdx].cmdHndl = cmdHndl;
               cmdCbTable[ClientRfCmdCbIdx].cb = pCb;
               cmdCbTable[ClientRfCmdCbIdx].bmEvent = tmpBmEvent;

               /* flag that command has been submitted */
               cmdSubmitted = true;
            }

            /* Exit critical section. */
            HwiP_restore(key);

        }
    }

    if(!cmdSubmitted)
    {
        /* Exit critical section. */
        HwiP_restore(key);

        /* not able to intercept cmd callback, schedule command with clients callback
         * todo: When BLE stack handles schedule errors return an error*/
        cmdHndl = RF_scheduleCmd(h, pOp, pSchParams, pCb, bmEvent);
    }

    return cmdHndl;

}

/*
 *  DMM level function to start RF_scheduleCmd.
 *
 *  Input:  pCmdSchMapElem          - RF commands in the command Q to be flushed
 *          prio                    - Command priority
 *  Return: RF_Stat                 - Flush command execution status
 */
static RF_CmdHandle cmdSchedule(RF_Handle h, RF_Op* pOp, RF_ScheduleCmdParams* pSchParams, RF_Callback pCb, RF_EventMask bmEvent)
{
    RF_CmdHandle cmdHndl = RF_SCHEDULE_CMD_ERROR;
    uint32_t key;
    RF_Handle h2;
    RF_InfoVal rfInfoVal;
    RF_Stat    rfPreemptStats = RF_StatError;
    
   bmEvent |= RF_EventLastCmdDone;

    cmdHndl = rfScheduleCmdWithAbortWorkaround(h, pOp, pSchParams, pCb, bmEvent);

    if(cmdHndl < 0)
    {
        //enable critical section in case RF change the command Q while DMM processing the preemption to the queue.
        key = HwiP_disable();

        // RF_GET_CLIENT_LIST always returns successful.
        RF_getInfo(h, RF_GET_CLIENT_LIST, &rfInfoVal);

        if (h == rfInfoVal.pClientList[0])
        {
            h2 = rfInfoVal.pClientList[1];
        }
        else
        {
            h2 = rfInfoVal.pClientList[0];
        }

        RF_ScheduleMapElement* pCmdPreempted = checkForPreemption(h2, pOp, pSchParams);

        if (pCmdPreempted)
        {
            rfPreemptStats = preemptRFCmd(pCmdPreempted, pCmdPreempted->priority);
            
            /* Exit critical section after we check the RF cmd.
             * Current running command (pCurrCmd) is flushed through CM0. CM4 RF driver
             * must wait for CM0 cpe0 interrupt to clear pCurrCmd. So, we must exit
             * critical section here before re-try insertion.
             * */
            HwiP_restore(key);

            // if we preempted some commands
            if ( rfPreemptStats == RF_StatSuccess)
            {
                // try to insert command one more time.
                cmdHndl = RF_scheduleCmd(h, pOp, pSchParams, pCb, bmEvent);  // did not help (may be a little), Eric
                //cmdHndl = rfScheduleCmdWithAbortWorkaround(h, pOp, pSchParams, pCb, bmEvent);
                if(cmdHndl < 0)
                {
                    unSchCnt++;
                }
            }
        }
        else
        {
            /* Exit critical section. */
            HwiP_restore(key);
            unSchCnt++;
        }

    }
    
    return cmdHndl;
}

/***** Public function definitions *****/

/** @brief  Function to initialize the DMMSch_Params
 *  *       This function is currently a placeholder. To follow the convention,
 *          we provide this function for application level.
 *
 *  @param  params      An pointer to RF_Params structure for
 *                      initialization
 *
 *  Defaults values are:
 */
void DMMSch_Params_init(DMMSch_Params *params)
{
    // Empty Function
}

/** @brief  Function that initializes the DMMSch module
 *
 */
void DMMSch_init(void)
{
    int16_t rfCmdIdx;

    // create semaphore instance if not already created
    if (DmmSchedulerMutex == NULL)
    {
        //Create a semaphore for blocking commands
        Semaphore_Params semParams;
        Error_Block eb;

        // init params
        Semaphore_Params_init(&semParams);
        Error_init(&eb);

        DmmSchedulerMutex = Semaphore_create(0, &semParams, &eb);

        /* todo: return error
        if (DmmSchedulerMutex == NULL)
        {
            return error;
        }*/

        /* init cmd callback table */
        for(rfCmdIdx = 0; rfCmdIdx < MAC_CMD_CBS; rfCmdIdx++)
        {
            cmdCbTable[rfCmdIdx].cmdHndl = RF_SCHEDULE_CMD_ERROR;
        }

        Semaphore_post(DmmSchedulerMutex);
    }
}

/** @brief  Function to open the DMMSch module
 *          This function is currently a placeholder. To follow the convention,
 *          we provide this function for application level. In the future,
 *          when it is needed, we can add more code to this function.
 *
 *  @param  params      An pointer to RF_Params structure for initialization
 */
void DMMSch_open(DMMSch_Params *params)
{
    if (params != NULL)
    {
        //empty function.
    }
}

/** @brief  Register a DMM Scheduler client
 *
 *  @param  pTaskHndl  - Task handle that the stack is running in, used to map the
 *                       RF Client handle to a Stack type
 *
 *  @param  stackType  - Stack Type associated with Task handle
 */
void DMMSch_registerClient(Task_Handle* pTaskHndl, DMMPolicy_StackType stackType)
{
    if(numDmmClients < MAX_DMM_CLIENTS)
    {
        dmmClients[numDmmClients].pTaskHndl = pTaskHndl;
        dmmClients[numDmmClients].stackType = stackType;

        numDmmClients++;
    }
}

/** @brief  Updates the policy used to make scheduling decisions
 *
 *  @param  policy     A new policy
 */
void DMMSch_updatePolicy(DMMPolicy_PolicyTableEntry policy)
{
    Semaphore_pend(DmmSchedulerMutex, BIOS_WAIT_FOREVER);

    memcpy(&currentPolicy, &policy, sizeof(DMMPolicy_PolicyTableEntry));

    //policy updated, release mutex
    Semaphore_post(DmmSchedulerMutex);
}

/** @brief  Intercepts calls from a stack to RF_postCmd (re-mapped to DMMSch_rfOpen),
 *          The DMMSch module uses this to tie
 *
 *  @param  params      An pointer to RF_Params structure for initialization
 */
RF_Handle DMMSch_rfOpen(RF_Object *pObj, RF_Mode *pRfMode, RF_RadioSetup *pOpSetup, RF_Params *params)
{
    DMMSch_Client_t *dmmClient;
    RF_Handle clientHndl = NULL;

    /* block other callers until RF driver is opened fr this client */
    Semaphore_pend(DmmSchedulerMutex, BIOS_WAIT_FOREVER);

    /* override the RF_Mode and patches for multi-mode operation */
    pRfMode->cpePatchFxn = rf_patch_cpe_multi_protocol;
#if defined(CC13X2R1_LAUNCHXL) || defined(CC13X2P1_LAUNCHXL)  //fengmo
    pRfMode->rfMode = RF_MODE_AUTO;
#else
    pRfMode->rfMode = RF_MODE_MULTIPLE;
#endif

    dmmClient = getClient();

    if(dmmClient)
    {
        clientHndl = RF_open(pObj, pRfMode, pOpSetup, params);
        dmmClient->clientHndl = clientHndl;
    }

    //release mutex
    Semaphore_post(DmmSchedulerMutex);

    return clientHndl;
}

/**
 *  @brief  Handles calls from a stack to RF_postCmd (re-mapped to DMMSch_postCmd),
 *  adjusts timing as necessary and schedules then accordingly with RF_scheduleCmd.
 *
 *  @sa RF_pendCmd(), RF_runCmd(), RF_scheduleCmd(), RF_RF_cancelCmd(), RF_flushCmd(), RF_getCmdOp()
 *
 *  @param h         Driver handle previously returned by RF_open()
 *  @param pOp       Pointer to the RF operation command.
 *  @param ePri      Priority of this RF command (used for arbitration in multi-client systems)
 *  @param pCb       Callback function called during command execution and upon completion.
 *                   If RF_postCmd() fails, no callback is made.
 *  @param bmEvent   Bitmask of events that will trigger the callback or that can be pended on.
 *  @return          A handle to the RF command. Return value of RF_ALLOC_ERROR indicates error.
 */
RF_CmdHandle DMMSch_rfPostCmd(RF_Handle h, RF_Op* pOp, RF_Priority ePri, RF_Callback pCb, RF_EventMask bmEvent)
{
    RF_CmdHandle cmdHndl;
    RF_ScheduleCmdParams schParams = {0};

    /* block other client until command has been scheduled */
    Semaphore_pend(DmmSchedulerMutex, BIOS_WAIT_FOREVER);

    setSchedule(h, pOp, &schParams);

    cmdHndl = cmdSchedule(h, pOp, &schParams, pCb, bmEvent);

    //release mutex
    Semaphore_post(DmmSchedulerMutex);

    return cmdHndl;
}

/**
 *  @brief  Handles calls from a stack to RF_scheduleCmd (re-mapped to DMMSch_scheduleCmd),
 *          adjusts timing as necessary and schedules then accordingly with RF_scheduleCmd.
 *
 *  @param h         Handle previously returned by RF_open()
 *  @param pOp       Pointer to the #RF_Op. Must normally be in persistent and writeable memory
 *  @param pSchParams Pointer to the schedule command parameter structure
 *  @param pCb       Callback function called upon command completion (and some other events).
 *                   If RF_scheduleCmd() fails no callback is made
 *  @param bmEvent   Bitmask of events that will trigger the callback.
 *  @return          A handle to the RF command. Return value of RF_ALLOC_ERROR indicates error.
 */
RF_CmdHandle DMMSch_rfScheduleCmd(RF_Handle h, RF_Op* pOp, RF_ScheduleCmdParams *pSchParams, RF_Callback pCb, RF_EventMask bmEvent)
{
    RF_CmdHandle cmdHndl;

    /* block other client until command has been scheduled */
    Semaphore_pend(DmmSchedulerMutex, BIOS_WAIT_FOREVER);

    setSchedule(h, pOp, pSchParams);

    cmdHndl = cmdSchedule(h, pOp, pSchParams, pCb, bmEvent);

    //release mutex
    Semaphore_post(DmmSchedulerMutex);

    return cmdHndl;
}

/** @brief  Handles calls from a stack to RF_runCmd (re-mapped to DMMSch_runCmd),
 *          adjusts timing as necessary and schedules then accordingly with RF_scheduleCmd.
 *
 *  @param h         Driver handle previously returned by RF_open()
 *  @param pOp       Pointer to the RF operation command.
 *  @param ePri      Priority of this RF command (used for arbitration in multi-client systems)
 *  @param pCb       Callback function called during command execution and upon completion.
 *                   If RF_runCmd() fails, no callback is made.
 *  @param bmEvent   Bitmask of events that will trigger the callback or that can be pended on.
 *  @return          The relevant termination event.
 */
RF_EventMask DMMSch_rfRunCmd(RF_Handle h, RF_Op* pOp, RF_Priority ePri, RF_Callback pCb, RF_EventMask bmEvent)
{
    return RF_runCmd(h, pOp, ePri, pCb, bmEvent);
}

/**
 *  @brief  Handles calls from a stack to RF_runScheduleCmd (re-mapped to DMMSch_runScheduleCmd),
 *          adjusts timing as necessary and schedules then accordingly with RF_scheduleCmd.
 *
 *
 *  @param h         Handle previously returned by RF_open()
 *  @param pOp       Pointer to the #RF_Op. Must normally be in persistent and writeable memory
 *  @param pSchParams Pointer to the schedule command parameter structure
 *  @param pCb       Callback function called upon command completion (and some other events).
 *                   If RF_runScheduleCmd() fails, no callback is made.
 *  @param bmEvent   Bitmask of events that will trigger the callback.
 *  @return          The relevant command completed event.
 */
RF_EventMask DMMSch_rfRunScheduleCmd(RF_Handle h, RF_Op* pOp, RF_ScheduleCmdParams *pSchParams, RF_Callback pCb, RF_EventMask bmEvent)
{
     return RF_runScheduleCmd(h, pOp, pSchParams, pCb, bmEvent);
}

/**
 *  @brief  Abort/stop/cancel single command in command queue.
 *
 *  If command is running, aborts/stops it and posts callback for the
 *  aborted/stopped command. <br>
 *  If command has not yet run, cancels it it and posts callback for the
 *  cancelled command. <br>
 *  If command has already run or been aborted/stopped/cancelled, has no effect.<br>
 *  If RF_cancelCmd is called from a Swi context with same or higher priority
 *  than RF Driver Swi, when the RF core is powered OFF -> the cancel callback will be delayed
 *  until the next power-up cycle.<br>
 *
 *  @note Calling context : Task/SWI
 *
 *  @param h            Handle previously returned by RF_open()
 *  @param ch           Command handle previously returned by RF_postCmd().
 *  @param mode         1: Stop gracefully, 0: abort abruptly
 *  @return             RF_Stat indicates if command was successfully completed
 */
RF_Stat DMMSch_rfCancelCmd(RF_Handle h, RF_CmdHandle ch, uint8_t mode)
{
     return RF_cancelCmd(h, ch, mode);
}

/**
 *  @brief  Abort/stop/cancel command and any subsequent commands in command queue.
 *
 *  If command is running, aborts/stops it and then cancels all later commands in queue.<br>
 *  If command has not yet run, cancels it and all later commands in queue.<br>
 *  If command has already run or been aborted/stopped/cancelled, has no effect.<br>
 *  The callbacks for all cancelled commands are issued in chronological order.<br>
 *  If RF_flushCmd is called from a Swi context with same or higher priority
 *  than RF Driver Swi, when the RF core is powered OFF -> the cancel callback will be delayed
 *  until the next power-up cycle.<br>
 *
 *  @note Calling context : Task/SWI
 *
 *  @param h            Handle previously returned by RF_open()
 *  @param ch           Command handle previously returned by RF_postCmd().
 *  @param mode         1: Stop gracefully, 0: abort abruptly
 *  @return             RF_Stat indicates if command was successfully completed
 */
RF_Stat DMMSch_rfFlushCmd(RF_Handle h, RF_CmdHandle ch, uint8_t mode)
{
    return RF_flushCmd(h, ch, mode);
}

/**
 *  @brief Send any Immediate command. <br>
 *
 *  Immediate Comamnd is send to RDBELL, if radio is active and the RF_Handle points
 *  to the current client. <br>
 *  In other appropriate RF_Stat values are returned. <br>
 *
 *  @note Calling context : Task/SWI/HWI
 *
 *  @param h            Handle previously returned by RF_open()
 *  @param pCmdStruct   Pointer to the immediate command structure
 *  @return             RF_Stat indicates if command was successfully completed
*/
RF_Stat DMMSch_rfRunImmediateCmd(RF_Handle h, uint32_t* pCmdStruct)
{
    /* TODO: Need to make sure the correct client is selected or this
     * will return an error
     */
    return RF_runImmediateCmd(h, pCmdStruct);
}

/**
 *  @brief Send any Direct command. <br>
 *
 *  Direct Comamnd value is send to RDBELL immediately, if radio is active and
 *  the RF_Handle point to the current client. <br>
 *  In other appropriate RF_Stat values are returned. <br>
 *
 *  @note Calling context : Task/SWI/HWI
 *
 *  @param h            Handle previously returned by RF_open()
 *  @param cmd          Direct command value.
 *  @return             RF_Stat indicates if command was successfully completed.
*/
RF_Stat DMMSch_rfRunDirectCmd(RF_Handle h, uint32_t cmd)
{
    /* TODO: Need to make sure the correct client is selected or this
     * will return an error
     */
    return RF_runDirectCmd(h, cmd);
}

RF_Priority DMMSch_getPriorityByType(DMMPolicy_StackType clientStackType)
{
    RF_Priority priority = DMMPOLICY_PRIORITY_LOW;
    uint8_t policyStackIdx;

    /* find the stack policy index */
    for(policyStackIdx = 0; policyStackIdx < DMMPOLICY_NUM_STACKS; policyStackIdx++)
    {
        if(clientStackType == currentPolicy.stackType[policyStackIdx])
        {
            break;
        }
    }

    if(policyStackIdx < DMMPOLICY_NUM_STACKS)
    {
        priority = currentPolicy.policy[policyStackIdx].priority;
    }

    return(priority);
}

RF_Priority DMMSch_getPriority(RF_Handle h)
{
    return(DMMSch_getPriorityByType(getClientStackType(h)));
}


