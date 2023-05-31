/******************************************************************************

 @file  remote_display_gatt_profile.h

 @brief This file contains the Simple GATT profile definitions and prototypes
        prototypes.

 Group: WCS, BTS
 Target Device: CC13xx

 ******************************************************************************
 
 Copyright (c) 2010-2018, Texas Instruments Incorporated
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

#ifndef REMOTEDISPLAYGATTPROFILE_H
#define REMOTEDISPLAYGATTPROFILE_H

#ifdef __cplusplus
extern "C"
{
#endif

   /*********************************************************************
 * INCLUDES
 */

   /*********************************************************************
 * CONSTANTS
 */

// Profile Parameters
#define RDPROFILE_NODE_PAN_ID_CHAR             0  // RW uint8 - Profile node PAN ID initial value
#define RDPROFILE_NODE_DATA_CHAR               1  // RW uint8 - Profile node data initial value
#define RDPROFILE_NODE_ADDR_CHAR               2  // RW uint8[] - Profile EUI initial value
#define RDPROFILE_NODE_CHARPARAM3_CHAR         3  // RW uint8[] - Profile data interval initial value
#define RDPROFILE_NODE_CHARPARAM4_CHAR         4  // RW uint8[] - Profile sensor ID initial value
  
// Remote Display Profile Service UUID
#define RDPROFILE_SERV_UUID               0x1180
#define RDPROFILE_SERV_UUID_BASE128(uuid) 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
    0x00, 0xB0, 0x00, 0x40, 0x51, 0x04, LO_UINT16(uuid), HI_UINT16(uuid), 0x00, \
    0xF0

// Remote Display UUID
#define RDPROFILE_CHAR_UUID_BASE128(uuid) 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
    0x00, 0xB0, 0x00, 0x40, 0x51, 0x04, LO_UINT16(uuid), HI_UINT16(uuid), 0x00, \
    0xF0

#define RDPROFILE_NODE_ADDR_CHAR_UUID              0x1181
#define RDPROFILE_NODE_PAN_ID_CHAR_UUID            0x1182
#define RDPROFILE_NODE_CHARPARAM3_CHAR_UUID        0x1183
#define RDPROFILE_NODE_CHARPARAM4_CHAR_UUID        0x1184
#define RDPROFILE_NODE_DATA_CHAR_UUID              0x1185
  
// Remote Display Profile Services bit fields
#define RDPROFILE_SERVICE               0x00000001

// Length of Characteristic 5 in bytes
#define RDPROFILE_NODE_ADDR_CHAR_LEN           8

   /*********************************************************************
 * TYPEDEFS
 */

  
   /*********************************************************************
 * MACROS
 */

   /*********************************************************************
 * Profile Callbacks
 */

// Callback when a characteristic value has changed
   typedef void (*remoteDisplayProfileChange_t)( uint8 paramID );

   typedef struct
   {
  remoteDisplayProfileChange_t        pfnRemoteDispalyProfileChange;  // Called when characteristic value changes
   } remoteDisplayProfileCBs_t;

    

   /*********************************************************************
 * API FUNCTIONS 
 */


   /*
 * RemoteDisplay_AddService- Initializes the Simple GATT Profile service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 */

   extern bStatus_t RemoteDisplay_AddService( uint32 services );

   /*
 * RemoteDisplay_RegisterAppCBs - Registers the application callback function.
 *                    Only call this function once.
 *
 *    appCallbacks - pointer to application callbacks.
 */
   extern bStatus_t RemoteDisplay_RegisterAppCBs( remoteDisplayProfileCBs_t *appCallbacks );

   /*
 * RemoteDisplay_SetParameter - Set a Remote Display GATT Profile parameter.
 *
 *    param - Profile parameter ID
 *    len - length of data to right
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 */
   extern bStatus_t RemoteDisplay_SetParameter( uint8 param, uint8 len, void *value );
  
   /*
 * RemoteDisplay_GetParameter - Get a Remote Display GATT Profile parameter.
 *
 *    param - Profile parameter ID
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 */
   extern bStatus_t RemoteDisplay_GetParameter( uint8 param, void *value );


   /*********************************************************************
   *********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* REMOTEDISPLAYGATTPROFILE_H */
