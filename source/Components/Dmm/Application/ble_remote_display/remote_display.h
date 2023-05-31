/******************************************************************************

 @file  remote_display.h

 @brief This file contains the Remote Display BLE application
        definitions and prototypes.

 Group: WCS, BTS
 Target Device: CC13xx

 ******************************************************************************
 
 Copyright (c) 2013-2018, Texas Instruments Incorporated
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

#ifndef REMOTE_DISPLAY_H
#define REMOTE_DISPLAY_H

#ifdef __cplusplus
extern "C"
{
#endif

//! \brief Callback function type for updating the node report interval
   typedef void (*RemoteDisplay_setReportIntervalCb_t)(uint8_t reportInterval);

//! \brief Callback function type for updating the Concentrator LED Toggle
   typedef void (*RemoteDisplay_setConcLedCb_t)(uint8_t ConcLed);

//! \brief Callback function type for updating the node address
   typedef void (*RemoteDisplay_setNodeAddressCb_t)(uint8_t reportInterval);

//! \brief Structure for node callbacks
   typedef struct
   {
    RemoteDisplay_setReportIntervalCb_t setReportIntervalCb;
    RemoteDisplay_setConcLedCb_t setConcLedCb;
    RemoteDisplay_setNodeAddressCb_t setNodeAddressCb;
   } RemoteDisplay_nodeCbs_t;

//! \brief Structure for node stats
   typedef struct
   {
  uint16_t dataSendSuccess;
  uint16_t dataSendFail;
  uint16_t dataTxSchError;
  uint16_t ackRxAbort;
  uint16_t ackRxTimeout;
  uint16_t ackRxSchError;
   } RemoteDisplay_nodeWsnStats_t;

   /*********************************************************************
 *  @fn      RemoteDisplay_registerNodeCbs
 *
 * @brief   Register the wsn node callbacks
 */
   extern void RemoteDisplay_registerNodeCbs(RemoteDisplay_nodeCbs_t nodeCbs);

   /*********************************************************************
 *  @fn      RemoteDisplay_createTask
 *
 * @brief   Task creation function for the Remote Display Peripheral.
 */
   extern void RemoteDisplay_createTask(void);

   /*********************************************************************
 *  @fn      RemoteDisplay_setNodeAddress
 *
 * @brief   Sets the nodes address characteristic
 */
   extern void RemoteDisplay_setNodeAddress(uint8_t nodeAddress);

   /*********************************************************************
 *  @fn      RemoteDisplay_setNodeSensorReading
 *
 * @brief   Sets the nodes sensor reading characteristic
 */
   extern void RemoteDisplay_updateNodeData(uint8_t sensorData);

   /*********************************************************************
 *  @fn      RemoteDisplay_setNodeSensorReading
 *
 * @brief   Sets the nodes sensor reading characteristic
 */
   extern void RemoteDisplay_updateNodeWsnStats(RemoteDisplay_nodeWsnStats_t stats);

   /*********************************************************************
 *  @fn      RemoteDisplay_bleFastStateUpdateCb
 *
 * @brief   Callback from BLE link layer to indicate a state change
 */
   extern void RemoteDisplay_bleFastStateUpdateCb(uint32_t stackType, uint32_t stackState);

   /*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* REMOTE_DISPLAY_H */
