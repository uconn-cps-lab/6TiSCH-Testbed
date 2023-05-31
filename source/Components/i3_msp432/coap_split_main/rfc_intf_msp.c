/*
 * Copyright (c) 2018, Texas Instruments Incorporated
 */
#if FEATURE_DTLS || FEATURE_COAP
/* Standard Header files */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/cfg/global.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Semaphore.h>

/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/SPI.h>

/* Board Header files */
#include "Board.h"
#include "SensorI2C.h"

#include "remoteFuncCall.h"
#include "dtls_client.h"
#include "dtls/dtls.h"
#include "config.h"
#include "resource.h"
#include "coap_host_app.h"

struct structRtvType1
{
   unsigned char * pMsg;
   uint16_t *pSize;
};

extern uint16_t led_status;
uint8_t uip_trx_data[256];

static bool put_diag_msg_callback(void *cbCtxt_p, struct structRfcMsgheader *msgHdr_p, uint8_t *resp_p)
{
   uint8_t status = 1;
   *(uint8_t *)cbCtxt_p = 0;

   if (msgHdr_p->msgLength > 0)
   {
      *(uint8_t *)cbCtxt_p = resp_p[0];
   }

   return(status);
}

#define MAX_PULL_NEST_LEVEL (0)
uint8_t pullNestLevel;
int mmbxPull;
int mmbxPendCnt;
int mmbxPostCnt;
void  post_to_lp_thread(uint32_t msgId, uint32_t msgVal)
{
   MsgObj ipcMsg;
   ipcMsg.id = msgId;
   ipcMsg.val = msgVal;
   Mailbox_post(slowFuncTaskMailBox, &ipcMsg, BIOS_NO_WAIT);
   mmbxPostCnt++;
   while (1)
   {
      if (Mailbox_pend(mainTaskMailBox, &ipcMsg, 4) == 0)
      {
#if (MAX_PULL_NEST_LEVEL > 0)
         if (pullNestLevel < MAX_PULL_NEST_LEVEL)
         {
             mmbxPull++;
             pullNestLevel++;
             RFC_call(REMOTE_CC2650, RFC_FUNC_ID_PULL, 0, NULL, NULL, NULL, NULL);
             pullNestLevel--;
         }
#endif //(MAX_PULL_NEST_LEVEL > 0)
         Task_sleep(8);
      }
      else
      {
         mmbxPendCnt++;
         break;
      }
   }
}

#if FEATURE_DTLS
/*******************************************************************************
* @fn
*
* @brief  Handles packet received from the DTLS peer
*
* input parameters
*
* @param
*
* output parameters
*
* @param
*
* @return
*/
bool RFC_dtls_handle_read_handler()
{
   bool status = 0;
   bool parameterOk = 1;
   uint8_t rspBuf[1] = {0};
   uint16_t uip_datalen;
   uip_ip6addr_t srcipaddr;
   uint16_t srcport;
   uint8_t dtls_context_idx;
   uint16_t paramLength;

   //Get the DTLS_dtls_context_idx as the 1st function parameter
   status = RFC_getFunctionParameter(REMOTE_CC2650, &paramLength, &dtls_context_idx, 1);
   parameterOk = status && parameterOk && (paramLength > 0) && (dtls_context_idx < MAX_NUMBER_DTLS_CONTEXT);

   if (status)
   {
      //Get the uip_trx_data as the 2nd function parameter
      status = RFC_getFunctionParameter(REMOTE_CC2650, &uip_datalen, uip_trx_data, sizeof(uip_trx_data));
      parameterOk = status && parameterOk && (uip_datalen > 0);
   }

   if (status)
   {
      //Get the srcipaddr_p as the 3rd function parameter
      status = RFC_getFunctionParameter(REMOTE_CC2650, &paramLength, srcipaddr.u8, sizeof(srcipaddr.u8));
      parameterOk = status && parameterOk && (paramLength == sizeof(srcipaddr.u8));
   }

   if (status)
   {
      //Get the srcport as the 4th function parameter
      status = RFC_getFunctionParameter(REMOTE_CC2650, &paramLength, (uint8_t *)&srcport, sizeof(srcport));
      parameterOk = status && parameterOk && (paramLength == sizeof(srcport));
   }

   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_CC2650);
   }

   if (status && parameterOk)  //Handle the call
   {
       dtls_context_t *ctx = dtls_get_context(dtls_context_idx);
       if (ctx != NULL)
       {
           rspBuf[0] = dtls_handle_read(ctx, (uint8_t *)&uip_trx_data, uip_datalen, &srcipaddr, srcport);
       }
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_CC2650, 0, rspBuf, 1);
   }

   return(status && parameterOk);
}

/*******************************************************************************
* @fn
*
* @brief
*
* input parameters
*
* @param
*
* output parameters
*
* @param
*
* @return
*/

bool RFC_dtls_write_handler()
{
   bool status = 0;
   bool parameterOk = 1;
   uint8_t rspBuf[1] = {0};
   uint16_t uip_datalen;
   uip_ip6addr_t srcipaddr;
   uint16_t srcport;
   uint8_t dtls_context_idx;
   uint16_t paramLength;
   int rtv;

   //Get the DTLS_dtls_context_idx as the 1st function parameter
   status = RFC_getFunctionParameter(REMOTE_CC2650, &paramLength, &dtls_context_idx, 1);
   parameterOk = status && parameterOk && (paramLength > 0) && (dtls_context_idx < MAX_NUMBER_DTLS_CONTEXT);

   if (status)
   {
      //Get the uip_trx_data as the 2nd function parameter
      status = RFC_getFunctionParameter(REMOTE_CC2650, &uip_datalen, uip_trx_data, sizeof(uip_trx_data));
      parameterOk = status && parameterOk && (uip_datalen > 0);
   }

   if (status)
   {
      //Get the srcipaddr_p as the 3rd function parameter
      status = RFC_getFunctionParameter(REMOTE_CC2650, &paramLength, srcipaddr.u8, sizeof(srcipaddr.u8));
      parameterOk = status && parameterOk && (paramLength == sizeof(srcipaddr.u8));
   }

   if (status)
   {
      //Get the srcport as the 4th function parameter
      status = RFC_getFunctionParameter(REMOTE_CC2650, &paramLength, (uint8_t *)&srcport, sizeof(srcport));
      parameterOk = status && parameterOk && (paramLength == sizeof(srcport));
   }

   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_CC2650);
   }

   if (status && parameterOk)  //Handle the call
   {
       dtls_context_t *ctx;
      session_t session;
      dtls_peer_t *peer;

      memset(&session, 0, sizeof(session));
      memcpy(&(session.addr), &srcipaddr, sizeof(session.addr));
      session.port = srcport;
      session.size = sizeof(session.addr) + sizeof(session.port);

      ctx = dtls_get_context(dtls_context_idx);
      if (ctx != NULL)
      {
          peer = dtls_get_peer(ctx, &session);

          if (peer != NULL && peer->state == DTLS_STATE_CONNECTED)
          {
             rtv = dtls_write(ctx, &session, uip_trx_data, uip_datalen);

             if (rtv >= 0)
             {
                rspBuf[0] = RFC_FUNC_RETV_WRITE_OK;
             }
          }
      }
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_CC2650, 0, rspBuf, 1);
   }

   return(status && parameterOk);
}

/*******************************************************************************
* @fn
*
* @brief
*
* input parameters
*
* @param
*
* output parameters
*
* @param
*
* @return
*/

bool RFC_dtls_handle_retransmission_handler()
{
   bool status = 0;
   bool parameterOk = 1;
   uint8_t rspBuf[1] = {0};
   uint8_t dtls_context_idx;
   uint16_t paramLength;

   //Get the DTLS_dtls_context_idx as the 1st function parameter
   status = RFC_getFunctionParameter(REMOTE_CC2650, &paramLength, &dtls_context_idx, 1);
   parameterOk = status && parameterOk && (paramLength > 0) && (dtls_context_idx < MAX_NUMBER_DTLS_CONTEXT);

   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_CC2650);
   }

   if (status && parameterOk)  //Handle the call
   {
      post_to_lp_thread(IPC_MSG_ID_DTLS_HANDLE_RETRANSMIT, dtls_context_idx);
      rspBuf[0] = 1;
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_CC2650, 0, rspBuf, 1);
   }

   return(status && parameterOk);
}

/*******************************************************************************
* @fn
*
* @brief
*
* input parameters
*
* @param
*
* output parameters
*
* @param
*
* @return
*/

bool RFC_dtls_free_context_handler()
{
   bool status = 0;
   bool parameterOk = 1;
   uint8_t rspBuf[1] = {0};
   uint8_t dtls_context_idx;
   uint16_t paramLength;

   //Get the DTLS_dtls_context_idx as the function parameter
   status = RFC_getFunctionParameter(REMOTE_CC2650, &paramLength, &dtls_context_idx, 1);
   parameterOk = status && parameterOk && (paramLength > 0) && (dtls_context_idx < MAX_NUMBER_DTLS_CONTEXT);

   if (status)
   {
      status = RFC_getFunctionRunCommand(REMOTE_CC2650);
   }

   if (status && parameterOk)  //Handle the call
   {
      post_to_lp_thread(IPC_MSG_ID_DTLS_HANDLE_FREE_CONTEXT, dtls_context_idx);
      rspBuf[0] = 0;
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_CC2650, 0, rspBuf, 1);
   }

   return(status && parameterOk);
}

/*******************************************************************************
* @fn
*
* @brief
*
* input parameters
*
* @param
*
* output parameters
*
* @param
*
* @return
*/
uint16_t dtlsInitCount;
bool RFC_dtls_init_handler()
{
   bool status = 0;
   uint8_t rspBuf[1] = {0};
   bool parameterOk = 1;
   uint8_t dtls_context_idx;
   uint16_t paramLength;

   //Get the DTLS_dtls_context_idx as the function parameter
   status = RFC_getFunctionParameter(REMOTE_CC2650, &paramLength, &dtls_context_idx, 1);
   parameterOk = status && parameterOk && (paramLength > 0) && (dtls_context_idx < MAX_NUMBER_DTLS_CONTEXT);


   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_CC2650);
   }

   if (status && parameterOk)  //Handle the call
   {
      post_to_lp_thread(IPC_MSG_ID_DTLS_HANDLE_INIT, dtls_context_idx);
      rspBuf[0] = 1;
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_CC2650, 0, rspBuf, 1);
   }

   return(status && parameterOk);
}
/*******************************************************************************
* @fn
*
* @brief
*
* input parameters
*
* @param
*
* output parameters
*
* @param
*
* @return
*/

bool RFC_dtls_get_peer_info_handler()
{
   bool status = 0;
   bool parameterOk = 1;
   uint8_t *payloadBuf_p;
   uint16_t payloadBufSize;
   uip_ip6addr_t srcipaddr;
   uint16_t srcport;
   uint8_t dtls_context_idx;
   uint16_t rspLen = 1;
   uint16_t paramLength;

   //Get the DTLS_dtls_context_idx as the 1st function parameter
   status = RFC_getFunctionParameter(REMOTE_CC2650, &paramLength, &dtls_context_idx, 1);
   parameterOk = status && parameterOk && (paramLength > 0) && (dtls_context_idx < MAX_NUMBER_DTLS_CONTEXT);

   if (status)
   {
      //Get the srcipaddr_p as the 2nd function parameter
      status = RFC_getFunctionParameter(REMOTE_CC2650, &paramLength, srcipaddr.u8, sizeof(srcipaddr.u8));
      parameterOk = status && parameterOk && (paramLength == sizeof(srcipaddr.u8));
   }

   if (status)
   {
      //Get the srcport as the 3rd function parameter
      status = RFC_getFunctionParameter(REMOTE_CC2650, &paramLength, (uint8_t *)&srcport, sizeof(srcport));
      parameterOk = status && parameterOk && (paramLength == sizeof(srcport));
   }

   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_CC2650);
   }

   RFC_getTransmitBuffer(REMOTE_CC2650, &payloadBuf_p, &payloadBufSize);
   payloadBuf_p[0] = 0;
   rspLen = 1;

   if (status && parameterOk)  //Handle the call
   {
       dtls_context_t *ctx;
      session_t session;
      dtls_peer_t *peer;

      memset(&session, 0, sizeof(session));
      memcpy(&(session.addr), &srcipaddr, sizeof(session.addr));
      session.port = srcport;
      session.size = sizeof(session.addr) + sizeof(session.port);
      ctx = dtls_get_context(dtls_context_idx);
      if (ctx != NULL)
      {
          peer = dtls_get_peer(ctx, &session);

          if (peer != NULL)
          {
             int idx = 0;

             payloadBuf_p[idx++] = 1;
             payloadBuf_p[idx++] = peer->role;
             payloadBuf_p[idx++] = peer->state;
             rspLen = idx;
          }
      }
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_CC2650, 0, payloadBuf_p, rspLen);
   }

   return(status && parameterOk);
}

/*******************************************************************************
* @fn
*
* @brief
*
* input parameters
*
* @param
*
* output parameters
*
* @param
*
* @return
*/

bool RFC_dtls_connect_peer_handler()
{
   bool status = 0;
   bool parameterOk = 1;
   uip_ip6addr_t srcipaddr;
   uint16_t srcport;
   uint8_t dtls_context_idx;
   uint16_t paramLength;
   uint8_t rsp[1] = {0};

   //Get the DTLS_dtls_context_idx as the 1st function parameter
   status = RFC_getFunctionParameter(REMOTE_CC2650, &paramLength, &dtls_context_idx, 1);
   parameterOk = status && parameterOk && (paramLength > 0) && (dtls_context_idx < MAX_NUMBER_DTLS_CONTEXT);

   if (status)
   {
      //Get the srcipaddr_p as the 2nd function parameter
      status = RFC_getFunctionParameter(REMOTE_CC2650, &paramLength, srcipaddr.u8, sizeof(srcipaddr.u8));
      parameterOk = status && parameterOk && (paramLength == sizeof(srcipaddr.u8));
   }

   if (status)
   {
      //Get the srcport as the 3rd function parameter
      status = RFC_getFunctionParameter(REMOTE_CC2650, &paramLength, (uint8_t *)&srcport, sizeof(srcport));
      parameterOk = status && parameterOk && (paramLength == sizeof(srcport));
   }

   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_CC2650);
   }

   if (status && parameterOk)  //Handle the call
   {
       static session_t connect;

      memcpy(&(connect.addr), &srcipaddr, sizeof(connect.addr));
      connect.port = srcport;
      connect.size = sizeof(connect.addr) + sizeof(connect.port);
      connect.ifindex = dtls_context_idx;
#if DTLS_LEDS_ON
      GPIO_write(Board_LED_RED, 1);
      GPIO_write(Board_LED_GREEN, 0);
#endif
      post_to_lp_thread(IPC_MSG_ID_DTLS_HANDLE_CONNECT, (uint32_t)(&connect));
      rsp[0] = 0;
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_CC2650, 0, rsp, 1);
   }

   return(status && parameterOk);
}

#endif //FEATURE_DTLS

#if FEATURE_COAP
/*******************************************************************************
* @fn
*
* @brief
*
* input parameters
*
* @param
*
* output parameters
*
* @param
*
* @return
*/
bool RFC_coap_reboot_handler()
{
   extern void ResetCtl_initiateHardReset(void);
   bool status = 0;
   uint8_t rspBuf[1] = {1};

   //Get the RUN message
   status = RFC_getFunctionRunCommand(REMOTE_CC2650);

   if (status)
   {
      status = RFC_sendResponse(REMOTE_CC2650, 0, rspBuf, 1);
   }

   Task_sleep(1);
   ResetCtl_initiateHardReset();
   while(1);
}
/*******************************************************************************
* @fn
*
* @brief
*
* input parameters
*
* @param
*
* output parameters
*
* @param
*
* @return
*/

bool RFC_coap_init_handler()
{
   bool status = 0;
   bool parameterOk = 1;
   uint16_t paramLength;
   uint8_t rspBuf[1] = {0};

   //Get the coap_resource_check_time as the 1st function parameter
   status = RFC_getFunctionParameter(REMOTE_CC2650, &paramLength, (uint8_t*)&coapConfig.coap_resource_check_time, sizeof(coapConfig.coap_resource_check_time));
   parameterOk = status && parameterOk && (paramLength == sizeof(coapConfig.coap_resource_check_time));

   if (status)
   {
      //Get the coapConfig.coap_obs_max_non as the 2nd function parameter
      status = RFC_getFunctionParameter(REMOTE_CC2650, &paramLength, (uint8_t*)&coapConfig.coap_obs_max_non, sizeof(coapConfig.coap_obs_max_non));
      parameterOk = status && parameterOk && (paramLength == sizeof(coapConfig.coap_obs_max_non));
   }

   if (status)
   {
      //Get the coapConfig.coap_default_response_timeout as the 3rd function parameter
      status = RFC_getFunctionParameter(REMOTE_CC2650, &paramLength, (uint8_t*)&coapConfig.coap_default_response_timeout, sizeof(coapConfig.coap_default_response_timeout));
      parameterOk = status && parameterOk && (paramLength == sizeof(coapConfig.coap_default_response_timeout));
   }

   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_CC2650);
   }

   if (status && parameterOk)  //Handle the call
   {
      coap_init();
      rspBuf[0] = 1;
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_CC2650, 0, rspBuf, 1);
   }

   return(status && parameterOk);
}

/*******************************************************************************
* @fn
*
* @brief
*
* input parameters
*
* @param
*
* output parameters
*
* @param
*
* @return
*/

bool RFC_coap_timer_event_handler()
{
   bool status = 0;
   uint8_t rspBuf[1] = {0};

   //Get the RUN message
   status = RFC_getFunctionRunCommand(REMOTE_CC2650);

   if (status)  //Handle the call
   {
      coap_timer_event();

      rspBuf[0] = 1;
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_CC2650, 0, rspBuf, 1);
   }

   return(status);
}

/*******************************************************************************
* @fn
*
* @brief
*
* input parameters
*
* @param
*
* output parameters
*
* @param
*
* @return
*/

bool RFC_coap_coap_event_handler()
{
   bool status = 0;
   bool parameterOk = 1;
   uint8_t rspBuf[1] = {0};
   uint16_t uip_datalen;
   uip_ip6addr_t srcipaddr;
   uint16_t srcport;
   uip_ip6addr_t destipaddr;
   uint16_t destport;
   uint16_t paramLength;


   //Get the uip_trx_data as the 1st function parameter
   status = RFC_getFunctionParameter(REMOTE_CC2650, &uip_datalen, uip_trx_data, sizeof(uip_trx_data));
   parameterOk = status && parameterOk && (uip_datalen > 0);

   if (status)
   {
      //Get the srcipaddr_p as the 2nd function parameter
      status = RFC_getFunctionParameter(REMOTE_CC2650, &paramLength, srcipaddr.u8, sizeof(srcipaddr.u8));
      parameterOk = status && parameterOk && (paramLength == sizeof(srcipaddr.u8));
   }

   if (status)
   {
      //Get the srcport as the 3rd function parameter
      status = RFC_getFunctionParameter(REMOTE_CC2650, &paramLength, (uint8_t *)&srcport, sizeof(srcport));
      parameterOk = status && parameterOk && (paramLength == sizeof(srcport));
   }

   if (status)
   {
      //Get the destipaddr_p as the 4th function parameter
      status = RFC_getFunctionParameter(REMOTE_CC2650, &paramLength, destipaddr.u8, sizeof(destipaddr.u8));
      parameterOk = status && parameterOk && (paramLength == sizeof(destipaddr.u8));
   }

   if (status)
   {
      //Get the destport as the 5th function parameter
      status = RFC_getFunctionParameter(REMOTE_CC2650, &paramLength, (uint8_t *)&destport, sizeof(destport));
      parameterOk = status && parameterOk && (paramLength == sizeof(destport));
   }


   if (status)
   {
      //Get the RUN message
      status = RFC_getFunctionRunCommand(REMOTE_CC2650);
   }

   if (status && parameterOk)  //Handle the call
   {
      coap_event_handler(uip_trx_data, uip_datalen, &srcipaddr, srcport, &destipaddr, destport);
      rspBuf[0] = 1;
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_CC2650, 0, rspBuf, 1);
   }

   return(status && parameterOk);
}

/*******************************************************************************
* @fn
*
* @brief
*
* input parameters
*
* @param
*
* output parameters
*
* @param
*
* @return
*/

bool RFC_coap_delete_all_observers_handler()
{
   bool status = 0;
   uint8_t rspBuf[1] = {0};

   //Get the RUN message
   status = RFC_getFunctionRunCommand(REMOTE_CC2650);

   if (status)  //Handle the call
   {
      coap_delete_all_observers();
      rspBuf[0] = 1;
   }

   if (status)
   {
      status = RFC_sendResponse(REMOTE_CC2650, 0, rspBuf, 1);
   }

   return(status);
}

static bool getSchedule_callback(void *cbCtxt_p, struct structRfcMsgheader *msgHdr_p, uint8_t *resp_p)
{
   uint8_t status = 1;

   struct structRtvType1 *retv_p = (struct structRtvType1 *)cbCtxt_p;

   *(retv_p->pSize) = 0;

   if (msgHdr_p->msgLength > 1 && resp_p != NULL)
   {
      *(retv_p->pSize) = msgHdr_p->msgLength - 1;
      memcpy(retv_p->pMsg, resp_p + 1, *(retv_p->pSize));
   }

   return status;
}


int TSCH_DB_getSchedule(uint16_t startLinkIdx, uint8_t *buf, int buf_size)
{
   uint8_t *arg[1];
   uint16_t argLen[1];
   uint8_t status;
   struct structRtvType1 rtstruct;
   uint16_t retv;

   rtstruct.pMsg = buf;
   rtstruct.pSize = &retv;

   argLen[0] = sizeof(startLinkIdx);
   arg[0] = (uint8_t *)&startLinkIdx;

   status = RFC_call(REMOTE_CC2650, RFC_FUNC_ID_COAP_GET_SCHEDULE, 1, argLen, arg, getSchedule_callback, &rtstruct);

   if(!status)
   {
      volatile int errloop = 0;

      while(1)
      {
         errloop++;
      }
   }

   return(retv);
}

static bool scheduleAddSlot_callback(void *cbCtxt_p, struct structRfcMsgheader *msgHdr_p, uint8_t *resp_p)
{
   uint8_t status = 1;
   uint16_t *retv_p = (uint16_t *)cbCtxt_p;

   *retv_p = 0;

   if (msgHdr_p->msgLength > 0 && resp_p != NULL)
   {
      *retv_p = resp_p[0];
   }

   return status;
}

uint16_t NHL_scheduleAddSlot(uint16_t slotOffset, uint16_t channelOffset, uint8_t linkOptions, uint16_t nodeAddr)
{
   uint8_t *arg[4];
   uint16_t argLen[4];
   uint8_t status;
   uint16_t retv;

   argLen[0] = sizeof(slotOffset);
   arg[0] = (uint8_t *)&slotOffset;
   argLen[1] = sizeof(channelOffset);
   arg[1] = (uint8_t *)&channelOffset;
   argLen[2] = sizeof(linkOptions);
   arg[2] = (uint8_t *)&linkOptions;
   argLen[3] = sizeof(nodeAddr);
   arg[3] = (uint8_t *)&nodeAddr;

   status = RFC_call(REMOTE_CC2650, RFC_FUNC_ID_COAP_SCHEDULE_ADD_SLOT, 4, argLen, arg, scheduleAddSlot_callback, &retv);

   if(!status)
   {
      volatile int errloop = 0;

      while(1)
      {
         errloop++;
      }
   }

   return(retv);
}

uint16_t NHL_scheduleRemoveSlot(uint16_t slotOffset, uint16_t nodeAddr)
{
   uint8_t *arg[2];
   uint16_t argLen[2];
   uint8_t status;
   uint16_t retv;

   argLen[0] = sizeof(slotOffset);
   arg[0] = (uint8_t *)&slotOffset;
   argLen[1] = sizeof(nodeAddr);
   arg[1] = (uint8_t *)&nodeAddr;

   status = RFC_call(REMOTE_CC2650, RFC_FUNC_ID_COAP_SCHEDULE_REMOVE_SLOT, 2, argLen, arg, scheduleAddSlot_callback, &retv);

   if(!status)
   {
      volatile int errloop = 0;

      while(1)
      {
         errloop++;
      }
   }

   return(retv);
}

void webmsg_update(unsigned char *pMsg, unsigned char *pSize, uint8_t seq)
{
   uint8_t *arg[2];
   uint16_t argLen[2];
   uint8_t status;
   struct structRtvType1 rtstruct;
   uint16_t retv;

   rtstruct.pMsg = pMsg;
   rtstruct.pSize = &retv;

   argLen[0] = sizeof(seq);
   arg[0] = &seq;
   argLen[1] = sizeof(led_status);
   arg[1] = (uint8_t *)&led_status;

   status = RFC_call(REMOTE_CC2650, RFC_FUNC_ID_COAP_WEBMSG_UPDATE, 2, argLen, arg, getSchedule_callback, &rtstruct);
   *pSize = retv;

   if(!status)
   {
      volatile int errloop = 0;

      while(1)
      {
         errloop++;
      }
   }
}

void LED_set(uint32_t pinId, uint8_t value)
{
   uint8_t *arg[2];
   uint16_t argLen[2];
   uint8_t status;
   uint16_t retv;

   argLen[0] = sizeof(pinId);
   arg[0] = (uint8_t *)&pinId;
   argLen[1] = sizeof(value);
   arg[1] = (uint8_t *)&value;

   status = RFC_call(REMOTE_CC2650, RFC_FUNC_ID_COAP_LED_SET, 2, argLen, arg, scheduleAddSlot_callback, &retv);

   if(!status)
   {
      volatile int errloop = 0;

      while(1)
      {
         errloop++;
      }
   }
}

void getDiagnosisMessage(uint8_t *diagNosisMsg, uint16_t *msgSize_p)
{
   uint8_t *arg[1];
   uint16_t argLen[1];
   uint8_t status;
   struct structRtvType1 rtstruct;
   extern uint16_t coapObserveDis;

   rtstruct.pMsg = (unsigned char *)diagNosisMsg;
   rtstruct.pSize = msgSize_p;

   argLen[0] = sizeof(coapObserveDis);
   arg[0] = (uint8_t *)&coapObserveDis;

   status = RFC_call(REMOTE_CC2650, RFC_FUNC_ID_COAP_GET_DIAGNOSIS_MESSAGE, 1, argLen, arg, getSchedule_callback, &rtstruct);

   if(!status)
   {
      volatile int errloop = 0;

      while(1)
      {
         errloop++;
      }
   }
}

void putDiagnosisMessage(uint8_t *data, uint16_t size)
{
   uint8_t *arg[1];
   uint16_t argLen[1];
   uint8_t status;
   uint8_t retv;

   argLen[0] = size;
   arg[0] = data;

   status = RFC_call(REMOTE_CC2650, RFC_FUNC_ID_COAP_PUT_DIAGNOSIS_MESSAGE, 1, argLen, arg, put_diag_msg_callback, &retv);

   if(!status)
   {
      volatile int errloop = 0;

      while(1)
      {
         errloop++;
      }
   }
}

void coap_notifier_etimer_set(clock_time_t interval)
{
   retimer_set(RFC_FUNC_ID_COAP_SET_NOTIFIER_ETIMER, interval);
}

int coap_notifier_etimer_expired()
{
   return(retimer_expired(RFC_FUNC_ID_COAP_QUERY_NOTIFIER_ETIMER_EXPIRED));
}

int coap_retransmit_etimer_expired()
{
   return(retimer_expired(RFC_FUNC_ID_COAP_QUERY_RETRANSMIT_ETIMER_EXPIRED));
}

void coap_retransmit_etimer_set(clock_time_t interval)
{
   retimer_set(RFC_FUNC_ID_COAP_SET_RETRANSMIT_ETIMER, interval);
}
void coap_notify_etimer_reset()
{
   retimer_stop_reset(RFC_FUNC_ID_COAP_NOTIFIER_ETIMER_RESET);
}

void getNvmParams(uint16_t chunkIdx, uint8_t *nvmParamsMsg, uint16_t *msgSize_p)
{
   bool status = 0;
   uint8_t *arg[1];
   uint16_t argLen[1];
   struct structRtvType1 rtstruct;

   rtstruct.pMsg = (unsigned char *)nvmParamsMsg;
   rtstruct.pSize = msgSize_p;

   argLen[0] = sizeof(chunkIdx);
   arg[0] = (uint8_t *)&chunkIdx;

   status = RFC_call(REMOTE_CC2650, RFC_FUNC_ID_COAP_GET_NVM_PARAMS, 1, argLen, arg, getSchedule_callback, &rtstruct);

   if(!status)
   {
      volatile int errloop = 0;

      while(1)
      {
         errloop++;
      }
   }
}

int putNvmParams(uint8_t *tlvdata, uint16_t size)
{
   bool status = 0;
   uint8_t *arg[1];
   uint16_t argLen[1];
   uint8_t retv;

   argLen[0] = size;
   arg[0] = tlvdata;

   status = RFC_call(REMOTE_CC2650, RFC_FUNC_ID_COAP_PUT_NVM_PARAMS, 1, argLen, arg, put_diag_msg_callback, &retv);

   if(!status)
   {
      volatile int errloop = 0;

      while(1)
      {
         errloop++;
      }
   }

   return(retv);
}

void uip_udp_packet_send(void *data, int len, const uip_ipaddr_t *toaddr, uint16_t toport)
{
   uint8_t *arg[3];
   uint16_t argLen[3];
   uint8_t status;
   uint8_t retv;

   argLen[0] = len;
   arg[0] = (uint8_t*)data;
   argLen[1] = sizeof(toaddr->u8);
   arg[1] = (uint8_t*)(toaddr->u8);
   argLen[2] = sizeof(toport);
   arg[2] = (uint8_t*) & (toport);

   status = RFC_call(REMOTE_CC2650, RFC_FUNC_ID_UIP_UDP_PACKET_SEND, 3, argLen, arg, put_diag_msg_callback, &retv);

   if(!status)
   {
      volatile int errloop = 0;

      while(1)
      {
         errloop++;
      }
   }
}
#endif //FEATURE_COAP

void clock_init()
{

}

static bool clock_time_callback(void *cbCtxt_p, struct structRfcMsgheader *msgHdr_p, uint8_t *resp_p)
{
   uint8_t status = 1;
   memcpy(cbCtxt_p, resp_p, sizeof(clock_time_t));
   return(status);
}


clock_time_t clock_time()
{
   uint8_t status;
   clock_time_t retv;

   status = RFC_call(REMOTE_CC2650, RFC_FUNC_ID_DTLS_CLOCK_TIME, 0, 0, NULL, clock_time_callback, &retv);

   if(!status)
   {
      volatile int errloop = 0;

      while(1)
      {
         errloop++;
      }
   }

   return(retv);
}

#define MAX32BITINT     (0xFFFFFFFF)
#define HALFMAX32BITINT (0x80000000)

long clock_delta(clock_time_t t1, clock_time_t t2)
{
   long rtv;
   uint32_t v1 = t1 & MAX32BITINT;
   uint32_t v2 = t2 & MAX32BITINT;
   uint32_t dv;

   if (v1 < v2)
   {
      dv = v2 - v1;
      if (dv >= HALFMAX32BITINT)
      {
         rtv = MAX32BITINT - dv + ((dv > HALFMAX32BITINT) ? 1 : 0);
      }
      else
      {
         rtv = -dv;
      }
   }
   else
   {
      dv = v1 - v2;
      if (dv >= HALFMAX32BITINT)
      {
         rtv = -(MAX32BITINT - dv) - ((dv > HALFMAX32BITINT) ? 1 : 0);
      }
      else
      {
         rtv = dv;
      }
   }

   return(rtv);
}

static bool retimer_callback(void *cbCtxt_p, struct structRfcMsgheader *msgHdr_p, uint8_t *resp_p)
{
   uint8_t status = 1;
   int16_t *retv_p = (int16_t *)cbCtxt_p;

   if (retv_p != NULL && msgHdr_p->msgLength >= sizeof(*retv_p))
   {
      *retv_p = *(int16_t *)resp_p;
   }

   return(status);
}

void retimer_set(uint16_t cmdCode, clock_time_t interval)
{
   uint8_t *arg[1];
   uint16_t argLen[1];
   uint8_t status;

   argLen[0] = sizeof(interval);
   arg[0] = (uint8_t *)&interval;

   status = RFC_call(REMOTE_CC2650, cmdCode, 1, argLen, arg, retimer_callback, NULL);

   if(!status)
   {
      volatile int errloop = 0;

      while(1)
      {
         errloop++;
      }
   }
}
/*---------------------------------------------------------------------------*/

int retimer_expired(uint16_t cmdCod)
{
   uint8_t status;
   int16_t retv;

   status = RFC_call(REMOTE_CC2650, cmdCod, 0, NULL, NULL, retimer_callback, (uint8_t*)&retv);

   if(!status)
   {
      volatile int errloop = 0;

      while(1)
      {
         errloop++;
      }
   }

   return(retv);
}

/*---------------------------------------------------------------------------*/

void retimer_stop_reset(uint16_t cmdCod)
{
   uint8_t status;

   status = RFC_call(REMOTE_CC2650, cmdCod, 0, NULL, NULL, retimer_callback, NULL);

   if(!status)
   {
      volatile int errloop = 0;

      while(1)
      {
         errloop++;
      }
   }
}


#endif //FEATURE_DTLS || FEATURE_COAP

