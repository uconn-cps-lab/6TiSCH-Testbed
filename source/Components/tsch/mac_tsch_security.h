/*
* Copyright (c) 2017 Texas Instruments Incorporated
*
* All rights reserved not granted herein.
* Limited License.
*
* Texas Instruments Incorporated grants a world-wide, royalty-free,
* non-exclusive license under copyrights and patents it now or hereafter
* owns or controls to make, have made, use, import, offer to sell and sell ("Utilize")
* this software subject to the terms herein.  With respect to the foregoing patent
*license, such license is granted  solely to the extent that any such patent is necessary
* to Utilize the software alone.  The patent license shall not apply to any combinations which
* include this software, other than combinations with devices manufactured by or for TI (â€œTI Devicesâ€�).
* No hardware patent is licensed hereunder.
*
* Redistributions must preserve existing copyright notices and reproduce this license (including the
* above copyright notice and the disclaimer and (if applicable) source code license limitations below)
* in the documentation and/or other materials provided with the distribution
*
* Redistribution and use in binary form, without modification, are permitted provided that the following
* conditions are met:
*
*       * No reverse engineering, decompilation, or disassembly of this software is permitted with respect to any
*     software provided in binary form.
*       * any redistribution and use are licensed by TI for use only with TI Devices.
*       * Nothing shall obligate TI to provide you with source code for the software licensed and provided to you in object code.
*
* If software source code is provided to you, modification and redistribution of the source code are permitted
* provided that the following conditions are met:
*
*   * any redistribution and use of the source code, including any resulting derivative works, are licensed by
*     TI for use only with TI Devices.
*   * any redistribution and use of any object code compiled from the source code and any resulting derivative
*     works, are licensed by TI for use only with TI Devices.
*
* Neither the name of Texas Instruments Incorporated nor the names of its suppliers may be used to endorse or
* promote products derived from this software without specific prior written permission.
*
* DISCLAIMER.
*
* THIS SOFTWARE IS PROVIDED BY TI AND TIâ€™S LICENSORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
* BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL TI AND TIâ€™S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
/*
 *  ====================== mac_tsch_security.h =============================================
 *
 */
#ifndef __MAC_TSCH_SECURITY_H__
#define __MAC_TSCH_SECURITY_H__

#ifdef FEATURE_MAC_SECURITY
#include "frame802154e.h"
#include "ulpsmacaddr.h"
#include "nhl_device_table.h"

#define MAX_MAC_KEY_TABLE_SIZE                        (1)
#define MAX_MAC_DEVICE_TABLE_SIZE                     NODE_MAX_NUM
#define MAC_MAX_FRAME_COUNTER                         (0xFFFFFFFF)
#define IEEE_EXT_ADDRESS_LEN                          (8)
#define MAC_SEC_KEY_SIZE                              (16)

#define MAC_KEY_USAGE_NEGOTIATING                     0x01
#define MAC_KEY_USAGE_RX                              0x02
#define MAC_KEY_USAGE_TX                              0x04

#ifndef MAC_SEC_MAX_DTLS_CONNECT_ATTEMPT
#define MAC_SEC_MAX_DTLS_CONNECT_ATTEMPT              (6)
#endif

enum enumMacSecLevel
{
   MAC_SEC_LEVEL_NONE = 0,
   MAC_SEC_LEVEL_MIC_32,
   MAC_SEC_LEVEL_MIC_64,
   MAC_SEC_LEVEL_MIC_128,
   MAC_SEC_LEVEL_ENC,
   MAC_SEC_LEVEL_ENC_MIC_32,
   MAC_SEC_LEVEL_ENC_MIC_64,
   MAC_SEC_LEVEL_ENC_MIC_128
};

enum enumMacKeyIdMode
{
   MAC_KEY_ID_MODE_NONE,
   MAC_KEY_ID_MODE_1,
   MAC_KEY_ID_MODE_2,
   MAC_KEY_ID_MODE_3
};

enum enumMacKeyIdLen
{
   MAC_KEY_ID_4_LEN = 5,
   MAC_KEY_ID_8_LEN = 9
};

enum enumMacFrameCntSupress
{
   MAC_FRAME_CNT_NOSUPPRESS,
   MAC_FRAME_CNT_SUPPRESS,
};

enum enumMacFrameCntSize
{
   MAC_FRAME_CNT_4_BYTES,
   MAC_FRAME_CNT_5_BYTES,
};

enum enumMacpanCoordAddress
{
   MAC_PAN_COORD_SHORT_ADDRESS,
   MAC_PAN_COORD_EXTENDED_ADDRESS,
};

typedef struct structMacSecurityDbg
{
   uint8_t encrypt_security_error;
   uint8_t encrypt_counter_error;
   uint8_t encrypt_framesize_error;
   uint8_t encrypt_transform_error;
} MACSECURITY_DBG_s;

struct structKeyIdLookupDescKm0Short
{
   uint16_t DevicePANId;
   uint16_t DeviceAddress;
   uint8_t DeviceAddrMode;
};

struct structKeyIdLookupDescKm0Ext
{
   uint16_t DevicePANId;
   uint8_t DeviceAddress[IEEE_EXT_ADDRESS_LEN];
   uint8_t DeviceAddrMode;
};

struct structKeyIdLookupDescKm1_3
{
   uint8_t KeySource[8];
   uint8_t KeyIndex;
};

struct structKeyIdLookupDescKm2
{
   uint8_t KeySource[4];
   uint8_t KeyIndex;
};

struct structKeyIdLookupDescriptor
{
   uint8_t KeyIdMode;
   union
   {
      struct structKeyIdLookupDescKm0Short     Km0s;
      struct structKeyIdLookupDescKm0Ext       Km0e;
      struct structKeyIdLookupDescKm1_3        Km1_3;
      struct structKeyIdLookupDescKm2          Km2;
   } LookupData;
   uint8_t len;
};

struct structSecurityPolicy
{
   uint8_t SecurityMinimum;
   uint8_t DeviceOverrideSecurityMinimum;
   uint8_t AllowedSecurityLevels;    //bits corresponding to the security levels:  bit 1: level 1, ...
};

struct structGenericDescriptor
{
   uint8_t id;
   struct structSecurityPolicy secPolicy;
};

struct structKeyUsageDescriptorCmd
{
   uint8_t CommandFrameIdentifier;
   struct structSecurityPolicy secPolicy;
};

struct structShortLookupAddress
{
   uint16_t DeviceAddress;
};

struct structExtLookupAddress
{
   uint8_t DeviceAddress[IEEE_EXT_ADDRESS_LEN];
};

struct structDevLookupDescriptor
{
   uint16_t DevicePANId;
   union
   {
      struct structShortLookupAddress   sAddr;
      struct structExtLookupAddress     eAddr;
   } data;
   uint8_t len;
};

struct structDeviceDescriptor
{
   uint32_t handshakeStartTime; //JIRA39
   uint32_t FrameCounter;
   uint8_t FrameCounterExt;
   uint8_t Exempt;
   uint16_t DevicePANId;
   uint16_t shortAddress;
   uint8_t pairwiseKey[MAC_SEC_KEY_SIZE];   //JIRA39
   uint8_t keyUsage;          //JIRA39
   uint8_t connectAttemptCount; //JIRA39
};

#define structSecurityLevelDescriptor structGenericDescriptor

struct structSecurityLevelDescriptorList
{
   const struct structGenericDescriptor *SevLevListFrameType;
   const uint8_t *SevLevListCmd;
   uint8_t FrameTypeListLen;
   uint8_t CommandFrameIdentifierListLen;
};

struct structKeyDescriptor
{
   const struct structKeyIdLookupDescriptor *KeyIdLookupList;
   const struct structGenericDescriptor *KeyUsageListFrameType;
   const uint8_t *KeyUsageListCmd;
   uint8_t Key[MAC_SEC_KEY_SIZE];
   uint8_t KeyIdLookupListLen;
   uint8_t FrameTypeListLen;
   uint8_t CommandFrameIdentifierListLen;
};

struct structMacSecurityPib
{
#if (MAX_MAC_KEY_TABLE_SIZE > 0)
   uint16_t macKeyTableLen;
   struct structKeyDescriptor macKeyTable[MAX_MAC_KEY_TABLE_SIZE];
#endif
#if (MAX_MAC_DEVICE_TABLE_SIZE > 0)
   uint16_t macDeviceTableLen;
   struct structDeviceDescriptor macDeviceTable[MAX_MAC_DEVICE_TABLE_SIZE];
#endif
   struct structSecurityLevelDescriptorList macSecurityLevelTable;
   uint32_t macFrameCounterMode;
   uint32_t macFrameCounter;
   uint8_t macFrameCounterExt;
   uint8_t macAutoRequestSecurityLevel;
   uint8_t macAutoRequestKeyIdMode;
   uint8_t macAutoRequestKeySource[8];
   uint8_t macAutoRequestKeyIndex;
   uint8_t macDefaultKeySource[8];
};

struct structKeyLookupInput
{
   uint8_t frameType;
   uint8_t KeyIdMode;
   uint8_t *KeySource_p;
   uint8_t KeyIndex;
   uint8_t deviceAddressingMode;
   uint16_t devicePANId;
   uint8_t *deviceAddress_p;
};

extern struct structMacSecurityPib macSecurityPib;

void MAC_MlmeGetSecurityReq(int pibId, uint8_t *pibVal);
void MAC_MlmeSetSecurityReq(int pibId, uint8_t *pibVal);
void MAC_SecurityDeviceTableUpdate(ulpsmacaddr_panid_t ulpsmacaddr_panid_node, uint16_t *destShortAddr_p, ulpsmacaddr_t *destExtAddr_p);
void MAC_SecurityKeyIdUpate(ulpsmacaddr_panid_t ulpsmacaddr_panid_node, uint8_t *my_shortaddr_p, ulpsmacaddr_t *ulpsmacaddr_long_node_p);
void MAC_SecurityInit();
uint8_t macOutgoingFrameSecurity(uint16_t shortAddr, frame802154e_t *frame_p, uint8_t **pKeyTx_p, uint8_t **pFrameCounterExt_p, uint32_t **pFrameCounter_p);
uint8_t macIncomingFrameSecurity(uint16_t shortAddr, frame802154e_t *frame_p, uint8_t **pKeyRx_p, struct structDeviceDescriptor **devDesc_pp);
uint8_t macCcmStarInverseTransform(uint8_t *pKeyRx, uint8_t authlen, uint8_t *nonce, uint8_t *pCData, uint8_t cDataLen, uint8_t *pAData, uint8_t aDataLen );
uint8_t macCcmStarTransform(uint8_t *pKeyTx, uint8_t authlen, uint8_t *nonce,  uint8_t *pAData, uint8_t aDataLen, uint8_t *pMData, uint8_t mDataLen);
uint8_t macSecurityDeviceAdd(uint16_t shortAddr);
uint8_t macSecurityDeviceRemove(uint16_t shortAddr);
uint8_t macSecurityDeviceUpdate(uint16_t oldShortAddr, uint16_t newShortAddr);
void macSecurityDeviceReset(uint16_t shortAddr);
void macSecurityStartKeyExchangeWithParent(uip_ipaddr_t *destipaddr_p);
uint8_t macSecurityKeyExchangeWithParent();
void macSecurityDeletePairwiseKey(uint16_t shortAddr);
void macSecurityAddPairwiseKey(uint16_t shortAddr, uint8_t *key);
void macSecurityActivatePairwiseKey(uint16_t shortAddr);
#endif //#ifdef FEATURE_MAC_SECURITY
#endif /*__MAC_TSCH_SECURITY_H__*/
