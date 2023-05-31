/*******************************************************************************
 *  Copyright (c) 2017 Texas Instruments Inc.
 *  All Rights Reserved This program is the confidential and proprietary
 *  product of Texas Instruments Inc.  Any Unauthorized use, reproduction or
 *  transfer of this program is strictly prohibited.
 *******************************************************************************
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
 *******************************************************************************
 * FILE PURPOSE:
 *
 *******************************************************************************
 * DESCRIPTION:
 *
 *******************************************************************************
 * HISTORY:
 *
 ******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>
#include <pthread.h>
#include <comm.h>
#include <pltfrm_mbx.h>
#include <proj_assert.h>
#include <crc16.h>
#include <util.h>
#include <time.h>
#include <unistd.h>

#include "uip_rpl_process.h"
#include "rime/rimeaddr.h"
#include "net/uip.h"
#include "net/uip-ds6-nbr.h"

#include "mac-dep.h"
#include "bm_api.h"
#include "net/packetbuf.h"

#include "device.h"
#include "nv_params.h"  

#define DEV_SYNC_TIMEOUT_MSG   (0xAAAAAAAA)
#define MAX_RETRY              (5)
static int devSyncMsg;

uint8_t *rootTschScheduleBuffer_p;
uint16_t rootTschScheduleBufferSize;
uint16_t rootTschScheduleLen;
/*****************************************************************************
*  FUNCTION:
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*  RETURNS:
*
*****************************************************************************/
int DEV_waitforsync(UINT8 msgid)
{
   int timeOutCounter = 100;

   while (devSyncMsg == DEV_SYNC_TIMEOUT_MSG && timeOutCounter > 0)
   {
      usleep(100000);
      timeOutCounter--;
   }

   if (timeOutCounter == 0)
   {
     return -1;
   }
   else
   {
      return 1;
   }
}
/*****************************************************************************
*  FUNCTION:
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*  RETURNS:
*
*****************************************************************************/
void DEV_resetSys(void)
{
   COMM_TX_msg_s msg;
   struct timespec ts;
   int rc = -1;
   int numRetry = 0;

   devSyncMsg = DEV_SYNC_TIMEOUT_MSG;

   printf("reset the device\n");
   
   msg.id = RESET_DEVICE;
   msg.info.resetSys.rsttype = 0;
   
   do
   {
     rc = COMM_sendMsg(&msg);
     if (rc > 0)
     {
       rc = DEV_waitforsync(RESET_DEVICE);
     }
     
     if (rc <= 0)
     {
       sleep(5);
       printf("Reset failed. Resending the command\n");
       numRetry++;
       if (numRetry > MAX_RETRY)
       {
         printf("Reach the max retry limit\n");
         proj_assert(0);
       }
     }
   } while (rc <= 0);
   
   /* give the device enough time to reset */
   printf("Pausing to allow device to reset.\n");
   ts.tv_sec = 3;
   ts.tv_nsec = 0;
   nanosleep(&ts, NULL);

   return;
}

/******************************************************************************
* FUNCTION NAME: Shut_Down_Handler
*
* DESCRIPTION:
*       HCT SHUT_DOWN handler.
*
* Return Value:       none
* Input Parameters:   UINT8 *message payload
*                     UINT32 message payload length
* Output Parameters:  none
******************************************************************************/
void DEV_shutDown_Handler(UINT8 *msg, UINT32 msglen)
{
   UINT16 status;

   devSyncMsg = RESET_DEVICE;
   usleep(10000);  

   status = *((UINT16 *)(&msg[0]));

   switch (status)
   {
      case 0:
         printf("%s: SHUT_DOWN returned success.\n", __FUNCTION__);
         break;
      default:
         printf("%s: ERROR! SHUT_DOWN returned %#x.\n", __FUNCTION__, status);
         break;
   }

   return;
}

/******************************************************************************
* FUNCTION NAME: DEV_SetSystemConfig
*
* DESCRIPTION:
*     Set system configuration (example on using LOAD_SYSTEM_CONFIG)
*
* Return Value:       none
* Input Parameters:   none
* Output Parameters:  none
******************************************************************************/
void DEV_setSysConfig(COMM_setSysCfg_s *psetSysCfg)
{
   COMM_TX_msg_s msg;
   int rc = -1;

   devSyncMsg = DEV_SYNC_TIMEOUT_MSG;
   
   printf("Load system config to root node\n"); 

   msg.id = SET_SYS_INFO;
   memcpy(&(msg.info.setSysCfg), psetSysCfg, sizeof(COMM_setSysCfg_s));
 
   do
   {
     rc = COMM_sendMsg(&msg);
     if (rc > 0)
     {
       rc = DEV_waitforsync(SET_SYS_INFO);
     }
     
     if (rc <= 0)
     {
       printf("Load system config failed. Resending the command\n");
       proj_assert(0);
     }
   } while (rc <= 0);

   return;
}

/******************************************************************************
* FUNCTION NAME: Set_System_Config_Handler
*
* DESCRIPTION:
*       HCT LOAD_SYSTEM_CONFIG handler.
*
* Return Value:       none
* Input Parameters:   UINT8 *message payload
*                     UINT32 message payload length
* Output Parameters:  none
******************************************************************************/
void DEV_setSysConfig_Handler(UINT8 *msg, UINT32 msglen)
{
   UINT16 status;

   devSyncMsg = SET_SYS_INFO;
   usleep(10000); 

   msg = UTIL_readTwoBytes(msg, &status);

   switch (status)
   {
      case 0:
         printf("Load system config returned success.\n");
         break;
      default:
         printf("Load system config returned %#x.\n", status);
         break;
   }

   return;
}

/******************************************************************************
* FUNCTION NAME: DEV_getMACPib
*
* DESCRIPTION:
*     Set system configuration (example on using LOAD_SYSTEM_CONFIG)
*
* Return Value:       none
* Input Parameters:   none
* Output Parameters:  none
******************************************************************************/
void DEV_getMACPib(UINT32 attrId, UINT16 attrIdx)
{
   COMM_TX_msg_s msg;
   COMM_getMacPib_s *preq;
   int rc = -1;
   
   devSyncMsg = DEV_SYNC_TIMEOUT_MSG;

   msg.id = MAC_PIB_GET;
   preq = &(msg.info.getMacPib);

   preq->numItems = 1;
   preq->attrId[0] = attrId;
   preq->attrIdx[0] = attrIdx;
   
   do
   {
     rc = COMM_sendMsg(&msg);
     if (rc > 0)
     {
       rc = DEV_waitforsync(MAC_PIB_GET);
     }
     
     if (rc <= 0)
     {
      sleep(1);
      printf("Get MAC PIB failed. Resending the get pib command\n");
     }
   } while (rc <= 0);

   return;
}

/******************************************************************************
* FUNCTION NAME: DEV_setMACPib
*
* DESCRIPTION:
*
*
* Return Value:       none
* Input Parameters:
* Output Parameters:
******************************************************************************/
void DEV_setMACPib(UINT32 attrId, UINT16 attrIdx, uint8_t *val, uint16_t len)  //fengmo NVM
{
   COMM_TX_msg_s msg;
   COMM_setMacPib_s *preq;
   int rc = -1;

   msg.id = MAC_PIB_SET;
   preq = &(msg.info.setMacPib);

   preq->numItems = 1;
   preq->attrId[0] = attrId;
   preq->attrIdx[0] = attrIdx;
   preq->attrLen[0] = len;
   preq->attrVal_p[0] = (UINT16 *)val;
   
   do
   {
     rc = COMM_sendMsg(&msg);
  
     if (rc <= 0)
     {
      sleep(1);
      printf("Set MAC PIB failed. Resending the set pib command\n");
     }
   } while (rc <= 0);
   
   return;
}

/******************************************************************************
* FUNCTION NAME: DEV_getMACPib_Reply_Handler
*
* DESCRIPTION:
*       HCT GET_SYSTEM_INFO handler.
*
* Return Value:       none
* Input Parameters:   UINT8 *message payload
*                     UINT32 message payload length
* Output Parameters:  none
******************************************************************************/
void DEV_getMACPib_Reply_Handler(void)
{
   devSyncMsg = MAC_PIB_GET;
   usleep(10000);  
}

/******************************************************************************
* FUNCTION NAME: DEV_NHLScheduleAddSlot
*
* DESCRIPTION: execute NHL_scheduleAddSlot function
*
* Return Value:       none
* Input Parameters:   none
* Output Parameters:  none
******************************************************************************/
void DEV_NHLScheduleAddSlot(uint16_t slotOffset, uint16_t channelOffset, uint8_t linkOptions, uint16_t nodeAddr)
{
   COMM_TX_msg_s msg;
   COMM_NHLSchedule_s *preq;
   int rc=-1;

   msg.id = NHL_SCHEDULE;
   preq = &(msg.info.NHLSched);

   preq->operation = 1;
   preq->slotOffset = slotOffset;
   preq->channelOffset = channelOffset;
   preq->linkOptions = linkOptions;
   preq->nodeAddr = nodeAddr;
     
   do
   {
     rc = COMM_sendMsg(&msg);
     
     if (rc <= 0)
     {
       sleep(1);
       printf("Add slot failed. Resending the add slot command\n");
     }
   } while (rc <= 0);

   return;
}

/******************************************************************************
* FUNCTION NAME: DEV_NHLScheduleRemoveSlot
*
* DESCRIPTION: execute NHL_scheduleRemoveSlot function
*
* Return Value:       none
* Input Parameters:
* Output Parameters:  none
******************************************************************************/
void DEV_NHLScheduleRemoveSlot(uint16_t slotOffset, uint16_t node)
{
   COMM_TX_msg_s msg;
   COMM_NHLSchedule_s *preq;
   int rc=-1;

   msg.id = NHL_SCHEDULE;
   preq = &(msg.info.NHLSched);

   preq->operation = 2;
   preq->slotOffset = slotOffset;
   preq->nodeAddr = node;

       
   do
   {
     rc = COMM_sendMsg(&msg);
     
     if (rc <= 0)
     {
       sleep(1);
       printf("Remove slot failed. Resending the remove slot command\n");
     }
   } while (rc <= 0);
   
   return;
}

/*******************************************************************************
 * @fn
 *
 * @brief
 *
 * @param
 *
 * @return      none
 *******************************************************************************
*/
void NVM_init()
{
   DEV_getMACPib(MAC_ATTR_ID_BCN_CHAN,0);
   DEV_getMACPib(MAC_ATTR_ID_MAC_PSK_07,0);
   DEV_getMACPib(MAC_ATTR_ID_MAC_PSK_8F,0);
   DEV_getMACPib(MAC_ATTR_ID_ASSOC_REQ_TIMEOUT_SEC,0);
   DEV_getMACPib(MAC_ATTR_ID_SLOTFRAME_SIZE,0);
   DEV_getMACPib(MAC_ATTR_ID_KEEPALIVE_PERIOD,0);
   DEV_getMACPib(MAC_ATTR_ID_COAP_RESOURCE_CHECK_TIME,0);
   DEV_getMACPib(MAC_ATTR_ID_COAP_DEFAULT_PORT,0);
   DEV_getMACPib(MAC_ATTR_ID_COAP_DEFAULT_DTLS_PORT,0);
   DEV_getMACPib(MAC_ATTR_ID_TX_POWER,0);
   DEV_getMACPib(MAC_ATTR_ID_BCN_CH_MODE,0);
   DEV_getMACPib(MAC_ATTR_ID_SCAN_INTERVAL,0);
   DEV_getMACPib(MAC_ATTR_ID_NUM_SHARED_SLOT,0);
   DEV_getMACPib(MAC_ATTR_ID_FIXED_CHANNEL_NUM,0);
   DEV_getMACPib(MAC_ATTR_ID_RPL_DIO_DOUBLINGS,0);
   DEV_getMACPib(MAC_ATTR_ID_COAP_OBS_MAX_NON,0);
   DEV_getMACPib(MAC_ATTR_ID_COAP_DEFAULT_RESPONSE_TIMEOUT,0);
   DEV_getMACPib(MAC_ATTR_ID_DEBUG_LEVEL,0);
   DEV_getMACPib(MAC_ATTR_ID_PHY_MODE,0);
}

/*******************************************************************************
 * @fn
 *
 * @brief
 *
 * @param
 *
 * @return      none
 *******************************************************************************
 */
void NVM_update()
{
   DEV_setMACPib(MAC_ATTR_ID_PAN_ID,0, (uint8_t*)&(nvParams.panid), sizeof(nvParams.panid));
   DEV_setMACPib(MAC_ATTR_ID_BCN_CHAN,0, &(nvParams.bcn_chan_0), sizeof(nvParams.bcn_chan_0));
   DEV_setMACPib(MAC_ATTR_ID_MAC_PSK_07,0,nvParams.mac_psk, 8);
   DEV_setMACPib(MAC_ATTR_ID_MAC_PSK_8F,0,nvParams.mac_psk+8, 8);
   DEV_setMACPib(MAC_ATTR_ID_ASSOC_REQ_TIMEOUT_SEC,0, (uint8_t *)&(nvParams.assoc_req_timeout_sec), sizeof(nvParams.assoc_req_timeout_sec));
   DEV_setMACPib(MAC_ATTR_ID_SLOTFRAME_SIZE,0, (uint8_t *)&(nvParams.slotframe_size), sizeof(nvParams.slotframe_size));
   DEV_setMACPib(MAC_ATTR_ID_KEEPALIVE_PERIOD,0, (uint8_t *)&(nvParams.keepAlive_period), sizeof(nvParams.keepAlive_period));
   DEV_setMACPib(MAC_ATTR_ID_COAP_RESOURCE_CHECK_TIME,0, (uint8_t *)&(nvParams.coap_resource_check_time), sizeof(nvParams.coap_resource_check_time));
   coapConfig.coap_resource_check_time = nvParams.coap_resource_check_time;
   DEV_setMACPib(MAC_ATTR_ID_COAP_DEFAULT_PORT,0, (uint8_t *)&(nvParams.coap_port), sizeof(nvParams.coap_port));
   coapConfig.coap_port = nvParams.coap_port;
   DEV_setMACPib(MAC_ATTR_ID_COAP_DEFAULT_DTLS_PORT,0, (uint8_t *)&(nvParams.coap_dtls_port), sizeof(nvParams.coap_dtls_port));
   coapConfig.coap_dtls_port = nvParams.coap_dtls_port;
   DEV_setMACPib(MAC_ATTR_ID_TX_POWER,0, (uint8_t *)&(nvParams.tx_power), sizeof(nvParams.tx_power));
   DEV_setMACPib(MAC_ATTR_ID_BCN_CH_MODE,0, &(nvParams.bcn_ch_mode), sizeof(nvParams.bcn_ch_mode));
   DEV_setMACPib(MAC_ATTR_ID_SCAN_INTERVAL,0, &(nvParams.scan_interval), sizeof(nvParams.scan_interval));
   DEV_setMACPib(MAC_ATTR_ID_NUM_SHARED_SLOT,0, &(nvParams.num_shared_slot), sizeof(nvParams.num_shared_slot));
   DEV_setMACPib(MAC_ATTR_ID_FIXED_CHANNEL_NUM,0, &(nvParams.fixed_channel_num), sizeof(nvParams.fixed_channel_num));
   DEV_setMACPib(MAC_ATTR_ID_RPL_DIO_DOUBLINGS,0, &(nvParams.rpl_dio_doublings), sizeof(nvParams.rpl_dio_doublings));
   rplConfig.rpl_dio_doublings = nvParams.rpl_dio_doublings;
   DEV_setMACPib(MAC_ATTR_ID_COAP_OBS_MAX_NON,0, &(nvParams.coap_obs_max_non), sizeof(nvParams.coap_obs_max_non));
   coapConfig.coap_obs_max_non = nvParams.coap_obs_max_non;
   DEV_setMACPib(MAC_ATTR_ID_COAP_DEFAULT_RESPONSE_TIMEOUT,0, &(nvParams.coap_default_response_timeout), sizeof(nvParams.coap_default_response_timeout));
   coapConfig.coap_default_response_timeout = nvParams.coap_default_response_timeout;
   DEV_setMACPib(MAC_ATTR_ID_DEBUG_LEVEL,0, &(nvParams.debug_level), sizeof(nvParams.debug_level));
   DEV_setMACPib(MAC_ATTR_ID_PHY_MODE,0, &(nvParams.phy_mode), sizeof(nvParams.phy_mode));
   DEV_setMACPib(MAC_ATTR_ID_FIXED_PARENT,0, &(nvParams.fixed_parent), sizeof(nvParams.fixed_parent));
   DEV_setMACPib(MAC_ATTR_ID_NVM,0, NULL, 0);
}

/*******************************************************************************
 * @fn   DEV_NHLScheduleGet
 *
 * @brief  Retrieve all the links assigned to the root node
 * @param[in]   startLinkIdx -- start index of the links to be retrieved (the first link of the first slot frame has index of 0)
 *              buf    -- pointer to buffer to hold the retrieved link structures
 *              buf_size   -- size of the buffer, should not fill beyond this size if there are more links than can be held in the buffer
 *
 *
 * @return      number of bytes retrieved and stored in the buffer
 *
 * @note        The first two-bytes of the buffer contains an LE 16-bit integer indicating the total number of links having index
 *              greater than or equal to startLinkIdx.  The second two-bytes of the buffer contains an LE 16-bit integer indicating the
 *              actual number of retrieved links.  The two numbers differ if the buffer does not have enough space to hold all the
 *              retrievable links.
 *******************************************************************************/
int DEV_NHLScheduleGet(uint16_t startLinkIdx, uint8_t *buf, int bufSize)
{
   rootTschScheduleBuffer_p = buf;
   rootTschScheduleBufferSize = bufSize;

   DEV_getMACPib(MAC_ATTR_ID_ROOT_SCHEDULE, startLinkIdx);

   return(rootTschScheduleLen);
}
#if (FEATURE_MAC_SECURITY)
/*******************************************************************************
 * @fn
 *
 * @brief
 * @param[in]
 *
 * @return
 *
 * @note
 *******************************************************************************/
static uint16_t shortAddr;
void DEV_sendPairwiseKey(uip_ip6addr_t *ipaddr_p, uint8_t *key)
{
   uip_lladdr_t *lladdr_p = uip_ds6_nbr_lladdr_from_ipaddr(ipaddr_p);
   shortAddr = lladdr_p->addr[1] + lladdr_p->addr[0]*256;  //BE

   printf("Set Pairwise Key: %d\n", shortAddr);
   DEV_setMACPib(MAC_ATTR_ID_MAC_PAIR_KEY_ADDR, 0, (uint8_t*)&shortAddr, 2);
   DEV_setMACPib(MAC_ATTR_ID_MAC_PAIR_KEY_07,0, key, 8);
   DEV_setMACPib(MAC_ATTR_ID_MAC_PAIR_KEY_8F,0, key+8, 8);
}
/*******************************************************************************
 * @fn
 *
 * @brief
 * @param[in]
 *
 * @return
 *
 * @note
 *******************************************************************************/
void DEV_deletePairwiseKey(uip_ip6addr_t *ipaddr_p)
{
   uip_lladdr_t *lladdr_p = uip_ds6_nbr_lladdr_from_ipaddr(ipaddr_p);
   shortAddr = lladdr_p->addr[1] + lladdr_p->addr[0]*256;  //BE

   printf("Delete Pairwise Key: %d\n", shortAddr);
   DEV_setMACPib(MAC_ATTR_ID_MAC_DELETE_PAIR_KEY, 0, (uint8_t*)&shortAddr, 2);
}
#endif //FEATURE_MAC_SECURITY
