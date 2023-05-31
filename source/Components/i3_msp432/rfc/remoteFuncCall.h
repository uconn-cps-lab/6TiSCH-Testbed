/*
 * Copyright (c) 2015-2016, Texas Instruments Incorporated
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

#ifndef REMOTEFUNCCALL_H_
#define REMOTEFUNCCALL_H_

#include <stdbool.h>

#define MAX_RFC_RESPONSE_SIZE    200
#define SPI_BUFFER_SIZE          8
#define MAX_IPC_PACKET_SIZE      240
#define IPC_MSG_BUFFER_SIZE      (MAX_IPC_PACKET_SIZE*6)
#define TIRTOS_HIGHEST_TASK_PRIORITY    (Task_numPriorities - 1)
#define TIRTOS_LOWEST_TASK_PRIORITY     (1)

enum enumRemoteId
{
   REMOTE_MSP432,
   REMOTE_CC2650,
};

enum enumRfcMsgId
{
   RFC_MSG_RESET,
   RFC_MSG_NULL,
   RFC_MSG_ERROR,
   RFC_MSG_FUNCTION_NAME,
   RFC_MSG_FUNCTION_PARAMETER,
   RFC_MSG_FUNCTION_RUN,
   RFC_MSG_FUNCTION_RETUN_VALUE,
   RFC_MSG_PLEASE_WAIT,
};

enum enumRfFuncId
{
   RFC_FUNC_ID_MSP432HCT_GET_CONFIG,
   RFC_FUNC_ID_MSP432HCT_GET_SENSOR_DATA,
   RFC_FUNC_ID_DTLS_WRITE,
   RFC_FUNC_ID_DTLS_HANDLE_READ,
   RFC_FUNC_ID_DTLS_HANDLE_RETRANSMISSION,
   RFC_FUNC_ID_DTLS_GET_PEER_INFO,
   RFC_FUNC_ID_DTLS_FREE_CONTEXT,
   RFC_FUNC_ID_DTLS_INIT,
   RFC_FUNC_ID_DTLS_APP_DATA_FROM_PEER,
   RFC_FUNC_ID_DTLS_ENCRYPTED_DATA_TO_PEER,
   RFC_FUNC_ID_DTLS_SET_RETRANSMIT_ETIMER,
   RFC_FUNC_ID_DTLS_RETRANSMIT_ETIMER_STOP,
   RFC_FUNC_ID_DTLS_BSP_AES_INIT,
   RFC_FUNC_ID_DTLS_BSP_AES_CCM_AUTH_ENCRYPT,
   RFC_FUNC_ID_DTLS_BSP_AES_CCM_AUTH_DECRYPT,
   RFC_FUNC_ID_DTLS_CLOCK_TIME,
   RFC_FUNC_ID_DTLS_RANDOM_RAND,
   RFC_FUNC_ID_DTLS_CONNECT_PEER,
   RFC_FUNC_ID_DTLS_ENC_KEY,
   RFC_FUNC_ID_DTLS_ACTIVATE_KEY,
   RFC_FUNC_ID_DTLS_HANDSHAKE_FAILED,
   RFC_FUNC_ID_COAP_SET_NOTIFIER_ETIMER,
   RFC_FUNC_ID_COAP_QUERY_NOTIFIER_ETIMER_EXPIRED,
   RFC_FUNC_ID_COAP_NOTIFIER_ETIMER_RESET,
   RFC_FUNC_ID_COAP_QUERY_RETRANSMIT_ETIMER_EXPIRED,
   RFC_FUNC_ID_COAP_SET_RETRANSMIT_ETIMER,
   RFC_FUNC_ID_COAP_INIT,
   RFC_FUNC_ID_COAP_TIMER_EVENT,
   RFC_FUNC_ID_COAP_COAP_EVENT,
   RFC_FUNC_ID_COAP_DELETE_ALL_OBSERVERS,
   RFC_FUNC_ID_COAP_GET_SCHEDULE,
   RFC_FUNC_ID_COAP_SCHEDULE_ADD_SLOT,
   RFC_FUNC_ID_COAP_SCHEDULE_REMOVE_SLOT,
   RFC_FUNC_ID_COAP_WEBMSG_UPDATE,
   RFC_FUNC_ID_COAP_LED_SET,
   RFC_FUNC_ID_COAP_GET_DIAGNOSIS_MESSAGE,
   RFC_FUNC_ID_COAP_PUT_DIAGNOSIS_MESSAGE,
   RFC_FUNC_ID_COAP_GET_NVM_PARAMS,
   RFC_FUNC_ID_COAP_PUT_NVM_PARAMS,
   RFC_FUNC_ID_UIP_UDP_PACKET_SEND,
   RFC_FUNC_ID_REBOOT,
   RFC_FUNC_ID_PULL,
   RFC_FUNC_ID_NEW_SCAN,
};

enum enumRfFuncRetVal
{
   RFC_FUNC_RETV_ENCRYPTED_DATA_TO_PEER_OK = 0x89,
   RFC_FUNC_RETV_WRITE_OK = 0x9A,
};

enum enumIpcMessageId
{
   IPC_MSG_ID_DTLS_HANDSHAKE_MESSAGE,
   IPC_MSG_ID_DTLS_HANDLE_RETRANSMIT,
   IPC_MSG_ID_DTLS_HANDLE_FREE_CONTEXT,
   IPC_MSG_ID_DTLS_HANDLE_INIT,
   IPC_MSG_ID_DTLS_HANDLE_CONNECT,
};

struct structRfcMsgheader
{
   uint16_t msgLength;
   uint16_t dummy;  //Added to deal with the MSP432 C compiler memset() function bug that zeros out the byte following the structure if the structure has only two bytes
};

typedef bool (*typeRapiCallbackFunc)(void *cbCtxt_p, struct structRfcMsgheader *rspHdr_p, uint8_t *resp_p);
typedef bool (*typeRfcCallHandler)();

struct structRfcFunctionDispatchTableEntry
{
   uint16_t name;
   typeRfcCallHandler handler;
};

typedef struct MsgObj {  //IPC message
    uint32_t  id;             /* writer task id */
    int32_t  val;            /* message value */
} MsgObj, *Msg;

extern uint8_t commandBuffer[IPC_MSG_BUFFER_SIZE];
extern uint16_t commandBufferWriteIdx;

bool RFC_call(uint16_t remoteId, uint8_t funcId, uint8_t numArgs, uint16_t *argLength_p, uint8_t **args_pp, typeRapiCallbackFunc rspHandler, void *cbCtxt_p);
bool RFC_dispatchHandler(uint16_t remoteId, uint16_t fname);
bool RFC_open(uint16_t remoteId);
bool RFC_powerSave(uint16_t remoteId);
bool RFC_registerDispatchTable(uint16_t remoteId, const struct structRfcFunctionDispatchTableEntry *rfcDispatchTable, uint16_t tableSize);
void RFC_getTransmitBuffer(uint16_t remoteId, uint8_t **rfcXmtBuf_pp, uint16_t *rfcXmtBufSize_p);
void RFC_getReceiveBuffer(uint16_t remoteId, uint8_t **rfcRcvBuf_pp, uint16_t *rfcRcvBufSize_p);
bool RFC_sendResponse(uint16_t remoteId, uint8_t rspSeNo, uint8_t *respBuf, uint16_t respLen);
bool RFC_getFunctionName(uint16_t remoteId, uint8_t *fname_p);
bool RFC_getFunctionParameter(uint16_t remoteId, uint16_t *paramLength_p, uint8_t *paramBuf_p, uint16_t paramBufSize);
bool RFC_getFunctionRunCommand(uint16_t remoteId);
void RFC_set_dispatch_table();  //To be supplied by the app
void  post_to_lp_thread(uint32_t msgId, uint32_t msgVal);
#endif //REMOTEFUNCCALL_H_
