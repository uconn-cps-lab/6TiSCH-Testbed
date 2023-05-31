/*******************************************************************************
Filename:       tsch-leaf.c
Revised:
Revision:

Description:    This file contains main functions and application task to send
data periodically.


Copyright 2012-2016 Texas Instruments Incorporated. All rights reserved.

IMPORTANT: Your use of this Software is limited to those specific rights
granted under the terms of a software license agreement between the user
who downloaded the software, his/her employer (which must be your employer)
and Texas Instruments Incorporated (the "License").  You may not use this
Software unless you agree to abide by the terms of the License. The License
limits your use, and you acknowledge, that the Software may not be modified,
copied or distributed unless embedded on a Texas Instruments microcontroller
or used solely and exclusively in conjunction with a Texas Instruments radio
frequency transceiver, which is integrated into your product.  Other than for
the foregoing purpose, you may not use, reproduce, copy, prepare derivative
works of, modify, distribute, perform, display or sell this Software and/or
its documentation for any purpose.

YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

Should you have any questions regarding your right to use this Software,
contact Texas Instruments Incorporated at www.TI.com.
*******************************************************************************/

/* -----------------------------------------------------------------------------
*  Includes
* ------------------------------------------------------------------------------
*/
#include "stdio.h"
#include <string.h>

#include "Board.h"
#include "led.h"

#include "simple-udp/simple-udp-sensors.h"

#include "mac_config.h"
#include "bsp_aes.h"
#include "nhl_api.h"
#include "nhl_setup.h"

#if SENSORTAG || I3_MOTE
#include "SensorI2C.h"
#include "SensorBatMon.h"
#include "SensorTmp007.h"
#include "SensorHDC1000.h"
#include "SensorOpt3001.h"
#include "SensorBmp280.h"
#include "SensorMpu9250.h"
#include "SensorLIS2HH12.h"
#include "SensorLIS2DW12.h"
#endif


/* -----------------------------------------------------------------------------
*  Local Variables
* ------------------------------------------------------------------------------
*/
static Semaphore_Handle semHandle;

/*********************************************************************//********
* @fn         exceptionHandler
*
* @brief      Default exception handler function register in .cfg
*
* @param       none
*
* @return      none
********************************************************************************
*/
void exceptionHandler()
{
  while(1){}
}

/************************************************************************//***
* @fn          NHL_dataConfCb
*
* @brief       Application data confirm callback function.
*
* @param[in]   pDataConf -- pointer to data confirmation result
*
* @return      none
******************************************************************************
*/
void NHL_dataConfCb(NHL_McpsDataConf_t *pDataConf)
{
  return;
}

/************************************************************************//***
* @fn          NHL_dataIndCb
*
* @brief       Data Indication Callback function 
*
* @param       pDataInd -- pointer to received data structure
*
* @return      none
*******************************************************************************/
void NHL_dataIndCb(NHL_McpsDataInd_t *pDataInd)
{
  BM_free_6tisch(pDataInd->pMsdu);
  return;
}

/***************************************************************************//**
* @fn          APP_preparePacket
*
* @brief       Fill in the data request prameters 
*
* @param[out]  nhlMcpsDataReq  -- Structure of the McpsDataReq
*
* @return      true or false
*
******************************************************************************/
bool APP_preparePacket(NHL_McpsDataReq_t* nhlMcpsDataReq)
{
  NHL_DataReq_t*      nhlDataReq;
  NHL_Sec_t*   secReq;
  uint8_t destIsBcast;
  ulpsmacaddr_t ulpsmacDestAddr;
  memset(ulpsmacDestAddr.u8,0,sizeof(ulpsmacaddr_t));
  
  nhlDataReq = &nhlMcpsDataReq->dataReq;
  if (nhlMcpsDataReq->pMsdu == NULL)
  {
    return false;
  }
  
  ulpsmacDestAddr.u8[0] = 1;
  
  nhlDataReq->dstAddrMode = ulpsmacaddr_addr_mode(&ulpsmacDestAddr);
  nhlDataReq->dstAddr = ulpsmacDestAddr;
  
  nhlDataReq->txOptions = 1;
  if (nhlDataReq->dstAddrMode == ULPSMACADDR_MODE_NULL)
  {
    destIsBcast = 1;
  } else
  {
    destIsBcast = 0;
  }
  
  if (destIsBcast)
  {
    nhlDataReq->dstAddrMode = ULPSMACADDR_MODE_SHORT;
    nhlDataReq->txOptions = 0;
    ulpsmacaddr_copy(nhlDataReq->dstAddrMode,&nhlDataReq->dstAddr,
                     &ulpsmacaddr_broadcast);
  }
  
  if (ulpsmacaddr_short_node == ulpsmacaddr_null_short)
  {
    nhlDataReq->srcAddrMode = ULPSMACADDR_MODE_LONG;
  }
  else
  {
    nhlDataReq->srcAddrMode = ULPSMACADDR_MODE_SHORT;
  }
  
  NHL_getPIB(TSCH_MAC_PIB_ID_macPANId, &nhlDataReq->dstPanId);
  
  nhlDataReq->fco.iesIncluded = 0;
  nhlDataReq->pIEList = NULL;
  nhlDataReq->fco.seqNumSuppressed = 0;
 
  secReq = &nhlMcpsDataReq->sec;
#ifdef FEATURE_MAC_SECURITY
  if (((nhlDataReq->dstAddrMode == ULPSMACADDR_MODE_LONG)
       && !ulpsmacaddr_long_cmp(&nhlDataReq->dstAddr,&ulpsmacaddr_broadcast)
         && !ulpsmacaddr_long_cmp(&nhlDataReq->dstAddr,&ulpsmacaddr_null)) ||
      ((nhlDataReq->dstAddrMode == ULPSMACADDR_MODE_SHORT)
       && !ulpsmacaddr_short_cmp(&nhlDataReq->dstAddr,&ulpsmacaddr_broadcast)
         && !ulpsmacaddr_short_cmp(&nhlDataReq->dstAddr,&ulpsmacaddr_null)))
  {
	  NHL_setMacSecurityParameters(secReq);
  }
  else
  {
    secReq->securityLevel = MAC_SEC_LEVEL_NONE;
  }
#else
  secReq->securityLevel = MAC_SEC_LEVEL_NONE;
#endif
  
  return true;
}

/*******************************************************************************
* @fn          APP_packetSendTask
*
* @brief       Application task to send the packet
*
* @param       arg0 - meaningless
* @param       arg1 - meaningless
*
* @return      none
*******************************************************************************/
void APP_packetSendTask(UArg arg0, UArg arg1)
{
  NHL_McpsDataReq_t nhlMcpsDataReq;
  uint8_t msgLen;
  SampleMsg_t msg;
  bool status;
  NHL_MlmeScanReq_t nhlMlmeScanReq;
  
  status = SensorBatmon_init();
  
#if SENSORTAG
   status = SensorI2C_open();
   status = SensorTmp007_init();
   status = SensorOpt3001_init();
   status = SensorHdc1000_init();
   status = SensorBmp280_init();
   status = SensorMpu9250_init();
#elif I3_MOTE
   status = SensorI2C_open();
   status = SensorOpt3001_init();
   status = SensorBmp280_init();
 
#if CC13XX_DEVICES
   status = SensorLIS2DW12_init();
#elif CC26XX_DEVICES
   status = SensorTmp007_init();
   status = SensorHdc1000_init();
#endif
#endif

  Semaphore_Params semParams;
  Semaphore_Params_init(&semParams);
  semParams.mode = Semaphore_Mode_BINARY;
  semHandle = Semaphore_create(0, &semParams, NULL);
  
  Task_sleep(CLOCK_SECOND / 4);
  
  NHL_setupScanArg(&nhlMlmeScanReq);
  NHL_startScan(&nhlMlmeScanReq,NULL);

  
  while (1)
  {
    Semaphore_pend(semHandle,BIOS_WAIT_FOREVER);
    
    /* Do not start sending data until the device is associated*/
    if (!TSCH_checkConnectionStatus())
    {
      continue;
    }
    
    sample_msg_create(&msg);
    
    msgLen = sizeof(msg);
    nhlMcpsDataReq.pMsdu = BM_alloc(10);
    memcpy(nhlMcpsDataReq.pMsdu->buf_aligned,&msg,msgLen);
    nhlMcpsDataReq.pMsdu->datalen = msgLen;
    status = APP_preparePacket(&nhlMcpsDataReq);
    if (status == true)
    {
      NHL_dataReq(&nhlMcpsDataReq,NULL);
    }
  }
}

/*******************************************************************************
* @fn          APP_periodicPost
*
* @brief       Post function to application task when periodic clock expires.
*
* @param       none
*
* @return      none
*******************************************************************************/
void APP_periodicPost(void)
{
  Semaphore_post(semHandle);
}

/**************************************************************************//***
 * @fn          main
 *
 * @brief       Main function
 *
 * @param       none
 *
 * @return      none
 *******************************************************************************
 */
void main()
{
  Task_Params taskParams;
  
  /* Periodic application clock */
  Clock_Params clockParams;
  Clock_Params_init(&clockParams);
  clockParams.period = 2*CLOCK_SECOND;
  clockParams.startFlag = true;
  Clock_create((Clock_FuncPtr)APP_periodicPost,10*CLOCK_SECOND, &clockParams, NULL);
  
  /* Crate application task to send data periodically*/
  Task_Params_init(&taskParams);
  taskParams.priority = 1;
  taskParams.stackSize = 768;
  
  Task_create(APP_packetSendTask, &taskParams, NULL);
  
  PIN_init(BoardGpioInitTable);
  LED_init();
  
#ifdef FEATURE_MAC_SECURITY
  bspAesInit(0);
#endif
  
  set_defaultTschMacConfig();
  
  /* Enable interrupts and start sysbios */
  BIOS_start();
}