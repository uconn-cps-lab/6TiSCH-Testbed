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
 *  ====================== ehest.c =============================================
 *  Energy harvesting estimation module 
 */

/* -----------------------------------------------------------------------------
*  Includes
* ------------------------------------------------------------------------------
*/

#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>

#include "Board.h"
#include "hal_api.h"
#include "sys/clock.h"

/* Pin driver handles */
static PIN_Handle vBATOKPinHandle;

/* Global memory storage for a PIN_Config table */
static PIN_State vBATOKPinState;

PIN_Config vBATOKPinTable[] = {
    Board_VBAT_OK | PIN_INPUT_EN | PIN_NOPULL | PIN_IRQ_BOTHEDGES,
    PIN_TERMINATE
};

static uint8_t numOn=0;
static clock_time_t prevOnTime,curOnTime,offTime;
static clock_time_t totalTime,ehTime;
static uint16_t insEhDutyCycle, avgEhDutyCycle;
static uint8_t prevVal=0;

/* -----------------------------------------------------------------------------
*  Public functions
* ------------------------------------------------------------------------------
*/

/***************************************************************************//**
* @brief      This function calculates the energy harvesting duty cycle
*
* @param[in]   handle - driver pin handle function 
* @param[in]   pinID - pin ID
*
* @return      none
******************************************************************************/
void vBATOKCallbackFxn(PIN_Handle handle, PIN_Id pinId) 
{
    uint8_t val=PIN_getInputValue(pinId);
    
    if (prevVal == val)
    {
      numOn = 0;
      curOnTime = 0;
      prevOnTime = 0;
      offTime = 0;
    }
    prevVal = val;
      
    if (val == 1)
    {
        prevOnTime = curOnTime;
        curOnTime = clock_milisceconds();
        
        /*number of rising edge*/
        numOn++;
        
        if (numOn%2 == 0)
        {          
          totalTime += (curOnTime - prevOnTime);
          ehTime += (offTime - prevOnTime);  
          numOn=0;
        }
    }
    else
    {        
        offTime = clock_milisceconds();
    }
}

/***************************************************************************//**
* @brief      This function returns the energy harvesting duty cycle
*
* @param[in]   none
* @param[in]   none
*
* @return      none
******************************************************************************/
void ehest_getDutyCycle(uint8_t *pInsEhDutyCycle, uint8_t* pAvgEhDutyCycle)
{    
    insEhDutyCycle = 0;
    
    /* Corner case that EH is always primary voltage source*/
    if ((ehTime == 0) && (prevVal == 1))
    {
      insEhDutyCycle = 10000;
    }
    else if (ehTime < totalTime)
    {
      insEhDutyCycle = (uint16_t)(ehTime*10000/totalTime);
    }
    
    if ((avgEhDutyCycle == 0) && (insEhDutyCycle != 0))
    {
      avgEhDutyCycle = insEhDutyCycle;
    }
    else
    {
      avgEhDutyCycle = (uint16_t)(((uint32_t)insEhDutyCycle 
                                   + (uint32_t)(99*avgEhDutyCycle))/100);
    }
    
    *pInsEhDutyCycle = (uint8_t)(insEhDutyCycle/100);
    *pAvgEhDutyCycle = (uint8_t)(avgEhDutyCycle/100);
    
    /* restart the duty cyce calculation*/
    ehTime = 0;
    totalTime = 0;
}

/***************************************************************************//**
* @brief      This function enables vBATOK pins and setup callback function.
*
* @param[in]   none

* @return      none
******************************************************************************/
void ehest_enable(void)
{
  vBATOKPinHandle = PIN_open(&vBATOKPinState, vBATOKPinTable);
  
  if(!vBATOKPinHandle) {
    System_abort("Error initializing vBATOK pins\n");
  }

  /* Setup callback for vBATOK pins */
  if (PIN_registerIntCb(vBATOKPinHandle, &vBATOKCallbackFxn) != 0) {
    System_abort("Error registering vBATOK callback function");
  }
   
  numOn = 0;
  ehTime =  0;
  totalTime = 0;
  avgEhDutyCycle = 0;
}
