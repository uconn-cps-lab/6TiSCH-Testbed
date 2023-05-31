#if (CC26XX_DEVICES || CC13XX_DEVICES)
#include "Board.h"
#include "uip-conf.h"
#include "hal_api.h"
#include "hct_msp432/nwp/nwp_cc2650.h"
#include "hct_msp432/serialbus/BusSPI.h"
#include "../../../../../source/Components/apps/hct_msp432/hct/msp432hct.h"
#else
#ifdef TARGET_IS_MSP432P4XX
/* Standard Header files */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

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
#include "BusSPI.h"
#endif
#endif

#include "remoteFuncCall.h"

static uint8_t rfc_lock_flag;
static uint16_t rfc_transaction_nest_level;
static uint8_t spiReceiveBuffer[SPI_BUFFER_SIZE];
static uint8_t spiTransmitBuffer[SPI_BUFFER_SIZE];
uint8_t RFC_rsp[MAX_RFC_RESPONSE_SIZE];
//uint8_t RFC_req[MAX_RFC_RESPONSE_SIZE];
uint16_t sizeOfRfcFunctionDispatchTable;
const struct structRfcFunctionDispatchTableEntry *rfc_function_dispatch_table;

#ifdef TARGET_IS_MSP432P4XX
#define WAIT_LOOP_COUNT_1_MS    (40000)
#endif

#if (CC26XX_DEVICES || CC13XX_DEVICES)
#define WAIT_LOOP_COUNT_1_MS    (10000)
#endif

void RFC_fatalErrorHandler()
{
#if (DEBUG_HALT_ON_ERROR)
   volatile int errLoop = 0;

   while (1)
   {
      errLoop++;
   }
#endif
}

static void RFC_lock()
{
   while (rfc_lock_flag)
   {
      Task_sleep(2);
   }

   rfc_lock_flag = 1;
}

static void RFC_unlock()
{
   rfc_lock_flag = 0;
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
static uint32_t RFC_disableTaskPreemption()
{
   uint32_t key;
   key = Task_setPri(Task_self(), TIRTOS_HIGHEST_TASK_PRIORITY);
   return(key);
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
static uint32_t RFC_restoreTaskPreemption(uint32_t prevPri)
{
   return(Task_setPri(Task_self(), prevPri));
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
static void RFC_task_idle(uint32_t idleCount)
{
   uint32_t key = Task_setPri(Task_self(), TIRTOS_LOWEST_TASK_PRIORITY);  //Set the task to the lowest priority to allow other tasks to run
   volatile int i = 0;

   while (i < idleCount)
   {
      i++;
   }

   Task_setPri(Task_self(), key);  //Restore the original task priority
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
static void RFC_spiSelectCs()
{
#ifdef TARGET_IS_MSP432P4XX
   volatile int i = 0;

   /* Wait to ensure CC2650 enters SPI_writeRead() first*/
   while (i < 10000)
   {
      i++;
   }

   BusSPI_select(Board_SPI_CS);
#endif
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
static void RFC_spiDeselectCs()
{
#ifdef TARGET_IS_MSP432P4XX
   BusSPI_deselect(Board_SPI_CS);
#endif
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
static uint32_t RFC_peerSync(uint16_t remoteId, uint8_t disableTaskPreemption)
{
   bool status = 0;
   uint8_t rxbuf[2] = {0xB6, 0x6B};
   uint8_t txbuf[2] = {0xA5, 0x5A};
   uint32_t key = 0;

   if (remoteId == REMOTE_CC2650 ||
         remoteId == REMOTE_MSP432)
   {
      while (1)
      {
         if (disableTaskPreemption)
         {
            key = RFC_disableTaskPreemption();  //Disable the task preemption to make sure the slave will be waiting on the SPI read/write when the master assert the SPI CS and clock
         }

         RFC_spiSelectCs();
         status = BusSPI_writeRead(txbuf, 2, rxbuf, 2);
         RFC_spiDeselectCs();

         if (status && rxbuf[0] == txbuf[0] && rxbuf[1] == txbuf[1])
         {
            break;
         }

         if (disableTaskPreemption)
         {
            key = RFC_restoreTaskPreemption(key);
         }

         RFC_task_idle(100 * WAIT_LOOP_COUNT_1_MS);
      }
   }

   return(key);
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
static bool RFC_synchronizedSpiReadWrite(uint16_t remoteId, uint8_t xmtBuf[], uint16_t xmtLen, uint8_t rcvBuf[], uint16_t rcvLen)
{
   bool status = 0;
   uint32_t key;

   if (remoteId == REMOTE_CC2650 ||
         remoteId == REMOTE_MSP432)
   {
      key = RFC_peerSync(remoteId, 1);     //Task preemption is disabled
      RFC_spiSelectCs();
      status = BusSPI_writeRead(xmtBuf, xmtLen, rcvBuf, rcvLen);  //Do the actual SPI transaction with the task switch disabled
      RFC_spiDeselectCs();
      RFC_restoreTaskPreemption(key);   //Re-enable the task preemption
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

static bool RFC_sendMsg(uint16_t remoteId, uint16_t msgLen, uint8_t *msgBuf_p)
{
   bool status = 0;

   if (msgBuf_p != NULL)
   {
      uint8_t *src_p;
      uint16_t segLen;

      if (remoteId == REMOTE_MSP432 ||
            remoteId == REMOTE_CC2650)
      {
         spiTransmitBuffer[0] = msgLen & 0xFF;  //LE
         spiTransmitBuffer[1] = (msgLen >> 8) & 0xFF;
         status = RFC_synchronizedSpiReadWrite(remoteId, spiTransmitBuffer, 2, spiReceiveBuffer, 2); //Send the message header

         src_p = msgBuf_p;

         while (status && msgLen > 0)
         {
            segLen = (msgLen > SPI_BUFFER_SIZE) ? SPI_BUFFER_SIZE : msgLen;
            status = RFC_synchronizedSpiReadWrite(remoteId, src_p, segLen, spiReceiveBuffer, segLen); //Send the message segment
            msgLen -= segLen;
            src_p += segLen;
         }
      }
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
bool bufferOverflow = 0;
static bool RFC_getMsg(uint16_t remoteId, struct structRfcMsgheader *msgHdr_p, uint8_t *msgBuf_p, uint16_t msgBufSize)
{
   bool status = 0;
   uint16_t msgLen = 0;
   uint16_t respLen;
   uint16_t segLen;
   uint16_t copyLen;

   if (msgHdr_p != NULL)
   {
      memset(msgHdr_p, 0, sizeof(msgHdr_p));

      if (remoteId == REMOTE_CC2650 ||
            remoteId == REMOTE_MSP432)
      {
         status = RFC_synchronizedSpiReadWrite(remoteId, spiTransmitBuffer, 2, spiReceiveBuffer, 2); //received the header of response message: uint16_t response length

         if (status)
         {
            respLen = spiReceiveBuffer[1];
            respLen = (respLen << 8) + spiReceiveBuffer[0];  //Response length in the LE format
            msgHdr_p->msgLength = respLen;

            while(status && respLen > 0)
            {
               segLen = (respLen > SPI_BUFFER_SIZE) ? SPI_BUFFER_SIZE : respLen;
               status = RFC_synchronizedSpiReadWrite(remoteId, spiTransmitBuffer, segLen, spiReceiveBuffer, segLen); //Receive the message segment

               if (status)
               {
                  copyLen = msgBufSize - msgLen;

                  if (segLen <= copyLen)
                  {
                     copyLen = segLen;
                  }
                  else
                  {
                     bufferOverflow++;
                  }

                  if (copyLen > 0 && msgBuf_p != NULL)
                  {
                     memcpy(msgBuf_p + msgLen, spiReceiveBuffer, copyLen);
                     msgLen += copyLen;
                  }

                  respLen -= segLen;
               }
            }
         }
      }
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
bool RFC_call(uint16_t remoteId, uint8_t funcId, uint8_t numArgs, uint16_t *argLength_p, uint8_t **args_pp, typeRapiCallbackFunc rspHandler, void *cbCtxt_p)
{
   //This function must be reentrant as nested function calls are expected.
   bool status = 0;
   bool transacDone = 0;
   uint8_t txBuf[2];
   struct structRfcMsgheader rspHdr;

   RFC_lock();

   rfc_transaction_nest_level++;

   memset(&rspHdr, 0, sizeof(rspHdr));

   if (remoteId == REMOTE_MSP432  ||
         remoteId == REMOTE_CC2650)
   {
#if (CC26XX_DEVICES || CC13XX_DEVICES)

      if (rfc_transaction_nest_level == 1)
      {
         wakeUpMSP432();
      }

#endif
      txBuf[0] = RFC_MSG_FUNCTION_NAME;
      txBuf[1] = funcId;
      status = RFC_sendMsg(remoteId, 2, txBuf);

      if (status)
      {
         int i;

         for (i = 0; i < numArgs; i++)
         {
            txBuf[0] = RFC_MSG_FUNCTION_PARAMETER;
            txBuf[1] = i;
            status = RFC_sendMsg(remoteId, 2, txBuf);

            if (!status)
            {
               break;
            }

            status = RFC_sendMsg(remoteId, argLength_p[i], args_pp[i]);

            if (!status)
            {
               break;
            }
         }
      }

      if (status)
      {
         txBuf[0] = RFC_MSG_FUNCTION_RUN;
         status = RFC_sendMsg(remoteId, 2, txBuf);
      }

      while (status && !transacDone)
      {
         status = RFC_getMsg(remoteId, &rspHdr, RFC_rsp, sizeof(RFC_rsp));

         if (status)
         {
            switch (RFC_rsp[0])
            {
               case RFC_MSG_PLEASE_WAIT:
                  RFC_task_idle(100 * WAIT_LOOP_COUNT_1_MS);
                  break;

               case RFC_MSG_FUNCTION_RETUN_VALUE:
                  status = RFC_getMsg(remoteId, &rspHdr, RFC_rsp, sizeof(RFC_rsp)); //Get the separate payload of the return value message

                  if (status)
                  {
                     if (rspHandler != NULL)
                     {
                        status = rspHandler(cbCtxt_p, &rspHdr, RFC_rsp);
                     }
                  }

                  transacDone = 1;
                  break;

               case RFC_MSG_FUNCTION_NAME:
                  RFC_dispatchHandler(remoteId, RFC_rsp[1]);
                  break;

               case RFC_MSG_FUNCTION_PARAMETER:
                  status = RFC_getMsg(remoteId, &rspHdr, RFC_rsp, sizeof(RFC_rsp));  //Get the separate payload of the messages.  This message should never happen here
                  break;

               case RFC_MSG_RESET:
               case RFC_MSG_ERROR:
                  transacDone = 1;
                  break;

               case RFC_MSG_NULL:
               case RFC_MSG_FUNCTION_RUN:  //This message should never happen here
               default:
                  break;
            }
         }
      }
   }

   rfc_transaction_nest_level--;
   RFC_unlock();

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
bool RFC_dispatchHandler(uint16_t remoteId, uint16_t fname)
{
   bool status = 0;
   int i;

   rfc_transaction_nest_level++;

   for (i = 0; i < sizeOfRfcFunctionDispatchTable; i++)
   {
      if (rfc_function_dispatch_table[i].name == fname)
      {
         if (rfc_function_dispatch_table[i].handler != NULL)
         {
            status = rfc_function_dispatch_table[i].handler();
            if (!status)
            {
               RFC_fatalErrorHandler();
            }
         }

         break;
      }
   }

   if (i == sizeOfRfcFunctionDispatchTable)
   {
      RFC_fatalErrorHandler();
   }

   rfc_transaction_nest_level--;

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

bool RFC_open(uint16_t remoteId)
{
   bool status = 0;

   if (remoteId == REMOTE_CC2650)
   {
      /* Initialize SPI */
      status = BusSPI_open_master(Board_SPI);
   }
   else if (remoteId == REMOTE_MSP432)
   {
#if (CC26XX_DEVICES || CC13XX_DEVICES)
      /* Initialize SPI */
      status = BusSPI_open_slave(Board_SPI);
#endif
   }

   RFC_set_dispatch_table();
   rfc_transaction_nest_level = 0;

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
bool RFC_powerSave(uint16_t remoteId)
{
   bool status = 0;

   if (remoteId == REMOTE_CC2650)
   {
#ifdef TARGET_IS_MSP432P4XX
      /* disable SPI to minimize consumption */
      MSP_EXP432P401R_disableSPI();
      MSP_EXP432P401R_prepareSPI();
      status = 1;
#endif
   }
   else if (remoteId == REMOTE_MSP432)
   {
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
bool RFC_registerDispatchTable(uint16_t remoteId, const struct structRfcFunctionDispatchTableEntry *rfcDispatchTable, uint16_t tableSize)
{
   bool status = 0;

   if (remoteId == REMOTE_CC2650 ||
         remoteId == REMOTE_MSP432)
   {
      rfc_function_dispatch_table = rfcDispatchTable;
      sizeOfRfcFunctionDispatchTable = tableSize;
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
void RFC_getTransmitBuffer(uint16_t remoteId, uint8_t **rfcXmtBuf_pp, uint16_t *rfcXmtBufSize_p)
{
   *rfcXmtBuf_pp = RFC_rsp;
   *rfcXmtBufSize_p = MAX_RFC_RESPONSE_SIZE;
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
void RFC_getReceiveBuffer(uint16_t remoteId, uint8_t **rfcRcvBuf_pp, uint16_t *rfcRcvBufSize_p)
{
   *rfcRcvBuf_pp = RFC_rsp;
   *rfcRcvBufSize_p = MAX_RFC_RESPONSE_SIZE;
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
bool RFC_sendResponse(uint16_t remoteId, uint8_t rspSeNo, uint8_t *respBuf, uint16_t respLen)
{
   bool status = 0;
   uint8_t rspBuf[2];

   RFC_lock();

   rspBuf[0] = RFC_MSG_FUNCTION_RETUN_VALUE;
   rspBuf[1] = rspSeNo;
   status = RFC_sendMsg(REMOTE_CC2650, 2, rspBuf);

   if (status)
   {
      status = RFC_sendMsg(REMOTE_CC2650, respLen, respBuf);
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
bool RFC_getFunctionName(uint16_t remoteId, uint8_t *fname_p)
{
   struct structRfcMsgheader msgHdr;
   uint8_t rcvBuf[2];
   bool status;

   status = RFC_getMsg(remoteId, &msgHdr, rcvBuf, sizeof(rcvBuf));  //Get the FUNCTION NAME message

   if (status && rcvBuf[0] == RFC_MSG_FUNCTION_NAME)
   {
      *fname_p = rcvBuf[1];
   }
   else
   {
       status = 0;
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
bool RFC_getFunctionParameter(uint16_t remoteId, uint16_t *paramLength_p, uint8_t *paramBuf_p, uint16_t paramBufSize)
{
   struct structRfcMsgheader msgHdr;
   uint8_t rcvBuf[2];
   bool status;

   status = RFC_getMsg(remoteId, &msgHdr, rcvBuf, sizeof(rcvBuf));  //Get the FUNCTION PARAMETER message

   if (status && rcvBuf[0] == RFC_MSG_FUNCTION_PARAMETER)
   {
      status = RFC_getMsg(remoteId, &msgHdr, paramBuf_p, paramBufSize);  //Get the parameter as the function parameter message payload
      *paramLength_p = msgHdr.msgLength;
   }
   else
   {
      status = 0;
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
bool RFC_getFunctionRunCommand(uint16_t remoteId)
{
   struct structRfcMsgheader msgHdr;
   uint8_t rcvBuf[2];
   bool status;

   status = RFC_getMsg(remoteId, &msgHdr, rcvBuf, sizeof(rcvBuf));  //Get the RUN message
   status =  (status && rcvBuf[0] == RFC_MSG_FUNCTION_RUN);
   RFC_unlock();
   return(status);
}


