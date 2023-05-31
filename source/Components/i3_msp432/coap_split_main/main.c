/*
 * Copyright (c) 2017, Universitat Oberta de Catalunya (UOC)
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
 * *  Neither the name of Universitat Oberta de Catalunya nor the names of
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
 *
 * @authors:
 *         Xavier Vilajosana (xvilajosana@uoc.edu)
 *         Pere Tuset        (peretuset@uoc.edu)
 *         Borja Martinez    (bmartinezh@uoc.edu)
 *
 *
 * This demo software aims to exemplify the use of the Fusion processor (MSP432)
 * to sample data from the sensors in the I3Mote and communicate the sampled
 * data to the Network processor. The inter-processor communication is carried out
 * using the HCT protocol and wrapper functions are provided for that. In addition
 * the program exemplifies the sequence of message exchanges between the Fusion processor
 * and the Network processor.
 */

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
#include "BusSPI.h"

/* Sensor Driver files*/
#include "SensorTmp007.h"
#include "SensorHdc1000.h"
#include "SensorOpt3001.h"
#include "SensorBmp280.h"
#include "SensorLIS2HH12.h"

/* NWP Header files */
#include "nwp_msp432.h"
#include "sensors.h"
#include "tlv.h"

/* Power Estimation file */
#include "pwrest.h"
#include "remoteFuncCall.h"
#if FEATURE_DTLS
#include "dtls_client.h"
#endif
#if FEATURE_COAP
#include "config.h"
#include "coap.h"
#include "coap_host_app.h"
#include "coap_common_app.h"
#endif //FEATURE_COAP

/* For debugging purposes, set it to 1 */
#define LEDS_ON                             1

#define TASK_STACK_SIZE                   (1024*3)
#define MAX_TLVS                           10
#define DEFAULT_COAP_OBSERVE_PERIOD     10000
#define DEFAULT_SENSOR_SAMPLING_PERIOD     40 // in TI-RTOS tics -> 40*250 = 10000

Task_Struct task0Struct;
Char task0Stack[TASK_STACK_SIZE];

Task_Struct task1Struct;
Char task1Stack[TASK_STACK_SIZE];

Task_Struct task2Struct;
Char task2Stack[TASK_STACK_SIZE];

static Semaphore_Struct semaphore;
static Semaphore_Params semParams;

static Semaphore_Struct mutex;
static Semaphore_Params mutexParams;

//static UInt fastTick = 10;
//static UInt slowTick = 1000;

/* select the sensors that need to be sampled */
SensorCommand_t sensorCommand = { .bmp280 = true,
                                  .hdc1000 = true,
                                  .lis2hh12 = true,
                                  .opt3001 = true,
                                  .tmp007 = false
                                };

static HCT_TLV_t tlvs[MAX_TLVS];

/*shared variable to store read data */
static SensorDataPack_t sensorData;
static bool sensorStatus;

static void wakeUpInterruptFromCc2650(unsigned int index);

static bool RFC_msp432HctGet_handler();
static bool RFC_new_scan_handler();
#if FEATURE_DTLS
extern bool RFC_dtls_handle_retransmission_handler();
extern bool RFC_dtls_write_handler();
extern bool RFC_dtls_handle_read_handler();
extern bool RFC_dtls_get_peer_info_handler();
extern bool RFC_dtls_free_context_handler();
extern bool RFC_dtls_init_handler();
extern bool RFC_dtls_connect_peer_handler();
#endif  //FEATURE_DTLS

#if FEATURE_COAP
extern bool RFC_coap_init_handler();
extern bool RFC_coap_timer_event_handler();
extern bool RFC_coap_coap_event_handler();
extern bool RFC_coap_delete_all_observers_handler();
extern bool RFC_coap_reboot_handler();
#endif  //FEATURE_COAP

const struct structRfcFunctionDispatchTableEntry rfc_function_dispatch_table_x[] =
{
#if FEATURE_DTLS
   {RFC_FUNC_ID_DTLS_HANDLE_RETRANSMISSION,        RFC_dtls_handle_retransmission_handler},
   {RFC_FUNC_ID_DTLS_WRITE,                        RFC_dtls_write_handler},
   {RFC_FUNC_ID_DTLS_HANDLE_READ,                  RFC_dtls_handle_read_handler},
   {RFC_FUNC_ID_DTLS_GET_PEER_INFO,                RFC_dtls_get_peer_info_handler},
   {RFC_FUNC_ID_DTLS_FREE_CONTEXT,                 RFC_dtls_free_context_handler},
   {RFC_FUNC_ID_DTLS_INIT,                         RFC_dtls_init_handler},
   {RFC_FUNC_ID_DTLS_CONNECT_PEER,                 RFC_dtls_connect_peer_handler},
#endif  //FEATURE_DTLS
   {RFC_FUNC_ID_MSP432HCT_GET_SENSOR_DATA,         RFC_msp432HctGet_handler},
   {RFC_FUNC_ID_MSP432HCT_GET_CONFIG,              RFC_msp432HctGet_handler},
   {RFC_FUNC_ID_NEW_SCAN,                          RFC_new_scan_handler},
#if FEATURE_COAP
   {RFC_FUNC_ID_COAP_INIT,                         RFC_coap_init_handler},
   {RFC_FUNC_ID_COAP_TIMER_EVENT,                  RFC_coap_timer_event_handler},
   {RFC_FUNC_ID_COAP_COAP_EVENT,                   RFC_coap_coap_event_handler},
   {RFC_FUNC_ID_COAP_DELETE_ALL_OBSERVERS,         RFC_coap_delete_all_observers_handler},
   {RFC_FUNC_ID_REBOOT,                            RFC_coap_reboot_handler},
#endif  //FEATURE_COAP
};
#define DISPATCH_TABLE_SIZE   (sizeof(rfc_function_dispatch_table_x) / sizeof(rfc_function_dispatch_table_x[0]))


void RFC_set_dispatch_table()
{
   RFC_registerDispatchTable(REMOTE_CC2650, (const struct structRfcFunctionDispatchTableEntry *)rfc_function_dispatch_table_x, DISPATCH_TABLE_SIZE);
}

void MSP_fatalErrorHandler()
{
#if (DEBUG_HALT_ON_ERROR)
   volatile int errorCounter = 0;
   while (1)  //fmfmdebugdebug
   {
      errorCounter ++;
   }
#endif
}

static bool RFC_new_scan_handler()
{
   bool status;
   uint8_t rspBuf[1] = {1};

   //Get the RUN message
   status = RFC_getFunctionRunCommand(REMOTE_CC2650);

   if (status)  //Handle the call
   {
#if DTLS_LEDS_ON
      GPIO_write(Board_LED_RED, 1);
      GPIO_write(Board_LED_GREEN, 1);
#endif
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
static bool msp432HctGetConfigAndData_handler(uint8_t *rcvPket)
{
   static bool isConfigured = false;

   bool status = 0;
   HCT_Frame_t * p_hctHeader;
   uint8_t * p_payload;
   uint16_t payloadLength;
   uint16_t lenTLVs = 0;
   uint8_t numTLVs = 0;
   uint16_t pktLen = 0;
   uint8_t org, rpy;
   uint16_t coapObservePeriod = DEFAULT_COAP_OBSERVE_PERIOD;
   uint8_t hctSequenceNumber = 0;
   bool firstWakeup = true;
   /* power estimation measurement*/
   PWREST_PwrMeasurement_t pwrMeasure;
   uint32_t gpsenActivePwr, gpsenSleepPwr;
   uint16_t pwr[4];
   SensorDataPack_t localData;
   uint8_t *xmtBuf_p;
   uint16_t xmtBufSize;

   RFC_getTransmitBuffer(REMOTE_CC2650, &xmtBuf_p, &xmtBufSize);

   /* Parse received header and check CRC */
   status = HCTRetrieveHeader(rcvPket, &p_hctHeader, &p_payload, &payloadLength);

   if (status)
   {
      /* Check header */
      HCTRetrieveFlags(p_hctHeader->flags, &org, &rpy, &hctSequenceNumber);

      if (org != HCT_ORG_NETPROC)
      {
         /* The origin is not the CC2650.
            this cannot happen, avoid parsing the message */
         status = false;
      }

      if (!status)
      {
         /* if something went wrong go to the ERROR status */
         p_hctHeader->type = HCT_CMD_ERROR;
         MSP_fatalErrorHandler();

      }
      else
      {
         /* Decide action based on transaction type */
         switch (p_hctHeader->type)
         {
               /* Send configuration to CC2650 Network Processor */
            case HCT_CMD_CONFIG_GET:
               {
                  // if (rpy == HCT_RPY_ACK_REQUIRED)
                  {
#if LEDS_ON
                     /* toggle green led at every config transaction */
                      GPIO_write(Board_LED_RED, 1);
                      GPIO_write(Board_LED_GREEN, 1);
#endif
                     /* Set CC2650 as configured */
                     isConfigured = true;

                     /* Create a NWP ConfigGet response */
                     pktLen = nwpConfigGet(xmtBuf_p, hctSequenceNumber, coapObservePeriod);

                     /* check error case */
                     if (pktLen > HCT_FIX_PKT_SIZE)
                     {
                        /* the received buffer is larger than the expected pkt size */
                        /* increase the HCT_FIX_PKT_SIZE */
                        while(1);
                     }

                     status = RFC_sendResponse(REMOTE_CC2650, 0, xmtBuf_p, pktLen);
                  }
               }
               break;

            case HCT_CMD_DATA_GET:
               {
                  // if (rpy == HCT_RPY_ACK_REQUIRED)
                  {
                     numTLVs = 0;
                     lenTLVs = 0;

                     /* Cannot get data if is not configured */
                     if (!isConfigured)
                     {
                        break;
                     }

                     /* perform energy consumption measurements */
                     if (firstWakeup == true)
                     {
                        firstWakeup = false;
                        PWREST_init();
                        PWREST_on(PWREST_TYPE_TOTAL);
                     }
                     else
                     {
                        uint8_t i;

                        PWREST_off(PWREST_TYPE_CPU);
                        PWREST_off(PWREST_TYPE_TOTAL);
                        PWREST_calAvgCur(&pwrMeasure);
                        PWREST_on(PWREST_TYPE_TOTAL);

                        gpsenActivePwr = 0;
                        gpsenSleepPwr = 0;

                        gpsenSleepPwr += pwrMeasure.avgSleepCur[PWREST_TYPE_SENSORS_TMP];

                        for(i = PWREST_TYPE_SENSORS_HUM; i < PWREST_TYPE_TOTAL; i++)
                        {
                           gpsenActivePwr += pwrMeasure.avgActiveCur[i];
                           gpsenSleepPwr += pwrMeasure.avgSleepCur[i];
                        }

                        /*Convert nA to uA*/
                        /* MSP432 */
                        pwr[0] = (uint16_t)(pwrMeasure.avgActiveCur[PWREST_TYPE_CPU] / 10);
                        pwr[1] = (uint16_t)(pwrMeasure.avgSleepCur[PWREST_TYPE_CPU] / 10);

                        /* General purpose sensor*/
                        pwr[2] = (uint16_t)(gpsenActivePwr / 10);
                        pwr[3] = (uint16_t)(gpsenSleepPwr / 10);

                        /* convert the power measurements into a TLV to be transmitted */
                        convertPowerMeasurement2TLVs(pwr, tlvs, &numTLVs, &lenTLVs);
                     }

                     /* At this point we pack and send the sensor data read from the other task.
                      *  Error if sensors are not correct */
                     if (sensorStatus)
                     {
                        /* mutual exclusion ON */
                        if (!Semaphore_pend(Semaphore_handle(&mutex), BIOS_WAIT_FOREVER))
                        {
                           System_printf("Error pending mutex!");
                           System_flush();

                           MSP_fatalErrorHandler();
                        }

                        memcpy(&localData, &sensorData, sizeof(sensorData));
                        Semaphore_post(Semaphore_handle(&mutex));
                        /* mutual exclusion OFF */

                        /* Convert sensor data to a TLV structure */
                        convertSensorData2TLVs(&sensorCommand, &localData, tlvs, &numTLVs, &lenTLVs);

                        /* Create the frame to be transmitted */
                        pktLen = nwpDataGet(xmtBuf_p, hctSequenceNumber, tlvs, numTLVs, lenTLVs);

                        /* check error case */
                        if (pktLen > HCT_FIX_PKT_SIZE)
                        {
                           while(1); //increase the HCT_FIX_PKT_SIZE
                        }

                        status = RFC_sendResponse(REMOTE_CC2650, 0, xmtBuf_p, pktLen);
                     }
                  }
               }
               break;

            case HCT_CMD_ERROR:
               {
                  GPIO_write(Board_LED_RED, 1);

                  while(1);

                  /* Handle error message from the CC2650. */
               }

               //break;
            default:
               break;
         }

         // GPIO_write(Board_LED_GREEN, 0);
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
static bool RFC_msp432HctGet_handler()
{
   bool status = 0;
   uint8_t *payloadBuf_p;
   uint16_t payloadBufSize;
   uint16_t paramLength;

   RFC_getReceiveBuffer(REMOTE_CC2650, &payloadBuf_p, &payloadBufSize);

   status = RFC_getFunctionParameter(REMOTE_CC2650, &paramLength, payloadBuf_p, payloadBufSize);

   if (status)
   {
      status = RFC_getFunctionRunCommand(REMOTE_CC2650);
   }

   if (status)  //Handle the HCT message
   {
      status = msp432HctGetConfigAndData_handler(payloadBuf_p);
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
static void Handle_RFC_call()
{
   bool status;
   uint8_t fname;

   status = RFC_getFunctionName(REMOTE_CC2650, &fname);

   if (status)
   {
      Semaphore_pend(Semaphore_handle(&semaphore), BIOS_NO_WAIT); //Clear the semaphore in case it is still set
      status = RFC_dispatchHandler(REMOTE_CC2650, fname);
   }
   else
   {
      MSP_fatalErrorHandler();
   }
}

/**
 * This task will execute every time the wakeUpInterruptFromCc2650 is executed
 * The interrupt is triggered by the CC2650 when turns OFF the MSP_EXP432P401R_WAKEUP line
 * When it is executed it will post a semaphore that will allow the mainTask to execute
 * When executed the mainTask will pack the sensor data into an HCT message
 * and send it back to the CC2650 through SPI
 */
Void mainTask(UArg arg0, UArg arg1)
{
   /* Initialize I2C */
   SensorI2C_open();
   /* Initialize Sensors so they are in off state */
   SensorTmp007_init();
   SensorOpt3001_init();
   SensorHdc1000_init();
   SensorBmp280_init();
   SensorLis2hh12_init();

   /* Configure the CoAP period in the CC2650 Network Processor */

   sensorStatus = false;

   RFC_open(REMOTE_CC2650);

   while (true)
   {
      PWREST_on(PWREST_TYPE_CPU);

      Handle_RFC_call();  //The first time don't wait for the interrupt from CC2650 since it may have already happened before MSP started

      RFC_powerSave(REMOTE_CC2650);
      /* Wait for CC2650 initial interrupt to configure or data-ready.
         Toggled by wake up pin (see wakeUpInterruptFromCc2650) */

      if (!Semaphore_pend(Semaphore_handle(&semaphore), BIOS_WAIT_FOREVER))
      {
         System_printf("Error pending semaphore!");
         System_flush();

         MSP_fatalErrorHandler();
      }
   }
}

/**
 * This task will execute every DEFAULT_SENSOR_SAMPLING_PERIOD
 * and will sample all the active sensors and stored the
 * data in a global shared variable.
 */
Void sensorSamplingTask(UArg arg0, UArg arg1)
{
   SensorDataPack_t localData;
   bool status;

   while (1)
   {
      /* delay Sensor sampling period */
      Task_sleep(DEFAULT_SENSOR_SAMPLING_PERIOD);
      PWREST_on(PWREST_TYPE_CPU);

      status = readSensorsData(sensorCommand, &localData);

      if (status)
      {
         /* mutual exclusion */
         if (!Semaphore_pend(Semaphore_handle(&mutex), BIOS_WAIT_FOREVER))
         {
            System_printf("Error pending mutex!");
            System_flush();

            MSP_fatalErrorHandler();
         }

         /* copy the values */
         sensorStatus = status;
         memcpy(&sensorData, &localData, sizeof(sensorData));

         /* end of mutual exclusion */
         Semaphore_post(Semaphore_handle(&mutex));
      }

      PWREST_off(PWREST_TYPE_CPU);
   }
}

/**
 * This task will execute every DEFAULT_SENSOR_SAMPLING_PERIOD
 * and will sample all the active sensors and stored the
 * data in a global shared variable.
 */
int mbxerr;
int mbxPendCnt;
int mbxPostCnt;
Void slowFunctionTask(UArg arg0, UArg arg1)
{
   while(1)
   {
      MsgObj      ipcMsg;
      dtls_context_t *ctx;

      if (Mailbox_pend(slowFuncTaskMailBox, &ipcMsg, BIOS_WAIT_FOREVER) == 0)
      {
         mbxerr++;
      }
      else
      {
         mbxPendCnt++;
         switch(ipcMsg.id)
         {
         case IPC_MSG_ID_DTLS_HANDSHAKE_MESSAGE:
            {
               uint8_t dtls_context_idx;
               session_t session;
               uint8 *msg;
               uint16_t msgLen;
               uint16_t commandBufferReadIdx = ipcMsg.val;

               memcpy(&dtls_context_idx, commandBuffer + commandBufferReadIdx, 1);
               commandBufferReadIdx += 1;
               memcpy(&session, commandBuffer + commandBufferReadIdx, sizeof(session_t));
               commandBufferReadIdx += sizeof(session_t);
               memcpy(&msgLen, commandBuffer + commandBufferReadIdx, 2);
               commandBufferReadIdx += 2;
               msg = commandBuffer + commandBufferReadIdx;

               ipcMsg.val = 0;

               ctx = dtls_get_context(dtls_context_idx);
               if (ctx != NULL)
               {
                   ipcMsg.val = dtls_handle_handshake_command_message(ctx, &session, msg, msgLen);
               }

               mbxPostCnt++;
               Mailbox_post(mainTaskMailBox, &ipcMsg, BIOS_NO_WAIT);
            }
            break;
         case IPC_MSG_ID_DTLS_HANDLE_RETRANSMIT:
            {
               ctx = dtls_get_context(ipcMsg.val);
               if (ctx != NULL)
               {
                   dtls_handle_retransmit(ctx);
               }

               mbxPostCnt++;
               Mailbox_post(mainTaskMailBox, &ipcMsg, BIOS_NO_WAIT);
            }
            break;
         case IPC_MSG_ID_DTLS_HANDLE_FREE_CONTEXT:
            {
               ctx = dtls_get_context(ipcMsg.val);
               if (ctx != NULL)
               {
                   dtls_free_context(ctx);
               }

               mbxPostCnt++;
               Mailbox_post(mainTaskMailBox, &ipcMsg, BIOS_NO_WAIT);
            }
            break;
         case IPC_MSG_ID_DTLS_HANDLE_INIT:
            {
               uint8_t idx = ipcMsg.val;
               ctx = dtls_get_context(idx);

               if (ctx != NULL)
               {
                  dtls_free_context(ctx);
               }
               init_dtls(idx);

               mbxPostCnt++;
               Mailbox_post(mainTaskMailBox, &ipcMsg, BIOS_NO_WAIT);
            }
            break;
         case IPC_MSG_ID_DTLS_HANDLE_CONNECT:
            {
               session_t *connect_p = (session_t *)(ipcMsg.val);
               uint8_t idx = connect_p->ifindex;
               connect_p->ifindex = 0;
               ctx = dtls_get_context(idx);
               if (ctx != NULL)
               {
                   dtls_connect(ctx, connect_p);
               }
               mbxPostCnt++;
               Mailbox_post(mainTaskMailBox, &ipcMsg, BIOS_NO_WAIT);
            }
            break;
         default:
            ipcMsg.val = -1;
            mbxPostCnt++;
            Mailbox_post(mainTaskMailBox, &ipcMsg, BIOS_NO_WAIT);
            break;
         }
      }
   }
}

/**
 * This function will execute when the MSP432 detects an interrupt on the MSP_EXP432P401R_WAKEUP line
 * This interrupt is triggered by the CC2650 when turns OFF the MSP_EXP432P401R_WAKEUP line
 * When it is executed it will post a semaphore that will allow the mainTask to execute
 */
static void wakeUpInterruptFromCc2650(unsigned int index)
{
   /* Clear the interrupt */
   GPIO_clearInt(index);

   /* Check that the interrupts comes from the MSP_EXP432P401R_WAKEUP line */
   if (index == MSP_EXP432P401R_WAKEUP)
   {
      Semaphore_post(Semaphore_handle(&semaphore));
   }
}

int main(void)
{
   Task_Params taskParams1, taskParams2, taskParams3;

   /* Call board init functions */
   Board_initGeneral();
   Board_initGPIO();
   Board_initI2C();
   Board_initSPI();

   /* Configure and enable CC2650 interrupt source */
   GPIO_setCallback(MSP_EXP432P401R_WAKEUP, wakeUpInterruptFromCc2650);
   GPIO_enableInt(MSP_EXP432P401R_WAKEUP);

#if FEATURE_COAP
   coap_config_init();
#endif //FEATURE_COAP

   /* Initialize semaphore */
   Semaphore_Params_init(&semParams);
   semParams.mode = Semaphore_Mode_BINARY;
   Semaphore_construct(&semaphore, 0, &semParams);

   /* mutual exclusion sensor readings*/
   Semaphore_Params_init(&mutexParams);
   mutexParams.mode = Semaphore_Mode_BINARY;
   //open by default
   Semaphore_construct(&mutex, 1, &mutexParams);

   /* Construct mainTask thread */
   Task_Params_init(&taskParams1);
   taskParams1.stackSize = TASK_STACK_SIZE;
   taskParams1.stack = &task0Stack;
   taskParams1.priority = 2;  //The main task can lower itself to priority 1 when waiting on SPI
   Task_construct(&task0Struct, (Task_FuncPtr) mainTask, &taskParams1, NULL);

   /* Construct testTask thread */
   Task_Params_init(&taskParams2);
   taskParams2.stackSize = TASK_STACK_SIZE;
   taskParams2.stack = &task1Stack;
   taskParams2.priority = 2;
   Task_construct(&task1Struct, (Task_FuncPtr) sensorSamplingTask, &taskParams2, NULL);

   /* Construct the slow processing functions thread */
   Task_Params_init(&taskParams3);
   taskParams3.stackSize = TASK_STACK_SIZE;
   taskParams3.stack = &task2Stack;
   taskParams3.priority = 1;
   Task_construct(&task2Struct, (Task_FuncPtr) slowFunctionTask, &taskParams3, NULL);

   /* Start BIOS */
   BIOS_start();

   return (0);
}

/*
 * This is the systic wrapper when using the WDT to drive the O.S timing.
 * The WDT timer wakes up the O.S with the period configured in main.cfg file.
 */
Void kernelClock(UArg arg)
{
   Clock_tick();
}
