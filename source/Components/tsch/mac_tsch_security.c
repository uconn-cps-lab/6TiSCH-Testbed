/*
* Copyright (c) 2010-2014 Texas Instruments Incorporated
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
* include this software, other than combinations with devices manufactured by or for TI (Ã¢â‚¬Å“TI DevicesÃ¢â‚¬ï¿½).
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
* THIS SOFTWARE IS PROVIDED BY TI AND TIÃ¢â‚¬â„¢S LICENSORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
* BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL TI AND TIÃ¢â‚¬â„¢S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
/*
 *  ====================== mac_tsch_security.c =============================================
 *
 */
#ifdef FEATURE_MAC_SECURITY
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26xx.h>
#include "mac_tsch_security.h"
#include "lowmac.h"
#include "mac_pib_pvt.h"
#include "mac_pib.h"
#include "nhl_device_table.h"
#include "bsp_aes.h"
#include "led.h"

static const struct structKeyIdLookupDescriptor keyIdLookupList_1[] =
{
   {
      .KeyIdMode = MAC_KEY_ID_MODE_1,
      .LookupData =
      {
         .Km1_3 =
         {
            .KeyIndex = 0,
            .KeySource = {1, 1, 1, 1, 1, 1, 1, 1},   //Must match the macSecurityPib.macDefaultKeySource below.
         }
      },
      .len = sizeof(struct structKeyIdLookupDescKm1_3),
   },
   {
      .KeyIdMode = MAC_KEY_ID_MODE_2,
      .LookupData =
      {
         .Km2 =
         {
            .KeyIndex = 0,
            .KeySource = {2, 2, 2, 2},
         }
      },
      .len = sizeof(struct structKeyIdLookupDescKm2),
   },
   {
      .KeyIdMode = MAC_KEY_ID_MODE_3,
      .LookupData =
      {
         .Km1_3 =
         {
            .KeyIndex = 0,
            .KeySource = {2, 2, 2, 2, 2, 2, 2, 2},
         }
      },
      .len = sizeof(struct structKeyIdLookupDescKm1_3),
   },
};
#define KEY_ID_LIST_1_SIZE (sizeof(keyIdLookupList_1)/sizeof(keyIdLookupList_1[0]))

static const struct structGenericDescriptor frameTypeList1[] =
{
   {.id = MAC_FRAME_TYPE_BEACON, .secPolicy = {.SecurityMinimum = 0, .DeviceOverrideSecurityMinimum = 0, .AllowedSecurityLevels = 0xFF}},
   {.id = MAC_FRAME_TYPE_DATA, .secPolicy = {.SecurityMinimum = 0, .DeviceOverrideSecurityMinimum = 0, .AllowedSecurityLevels = 0xFF}},
   {.id = MAC_FRAME_TYPE_ACK, .secPolicy = {.SecurityMinimum = 0, .DeviceOverrideSecurityMinimum = 0, .AllowedSecurityLevels = 0xFF}},
   {.id = MAC_FRAME_TYPE_MULTIPURPOSE, .secPolicy = {.SecurityMinimum = 0, .DeviceOverrideSecurityMinimum = 0, .AllowedSecurityLevels = 0xFF}},
   {.id = MAC_FRAME_TYPE_COMMAND, .secPolicy = {.SecurityMinimum = 0, .DeviceOverrideSecurityMinimum = 0, .AllowedSecurityLevels = 0xFF}},
};
#define FRAME_TYPE_LIST1_SIZE (sizeof(frameTypeList1)/sizeof(frameTypeList1[0]))
/*  Strictly speaking 802.15.4 allows policy down to the individual commands. This commented out code does that. This can take up lots of memory.
 * For now, all the commands share a single policy
struct structGenericDescriptor commandList1[] =
{
   {.id = MAC_COMMAND_ASSOC_REQ, .secPolicy = {.SecurityMinimum = 0, .DeviceOverrideSecurityMinimum = 0, .AllowedSecurityLevels = 0xFF}},
   {.id = MAC_COMMAND_ASSOC_RESP, .secPolicy = {.SecurityMinimum = 0, .DeviceOverrideSecurityMinimum = 0, .AllowedSecurityLevels = 0xFF}},
   {.id = MAC_COMMAND_DISASSOC_NOT, .secPolicy = {.SecurityMinimum = 0, .DeviceOverrideSecurityMinimum = 0, .AllowedSecurityLevels = 0xFF}},
   {.id = MAC_COMMAND_DATA_REQ, .secPolicy = {.SecurityMinimum = 0, .DeviceOverrideSecurityMinimum = 0, .AllowedSecurityLevels = 0xFF}},
   {.id = MAC_COMMAND_PANID_CONFLICT, .secPolicy = {.SecurityMinimum = 0, .DeviceOverrideSecurityMinimum = 0, .AllowedSecurityLevels = 0xFF}},
   {.id = MAC_COMMAND_ORPHAN_NOT, .secPolicy = {.SecurityMinimum = 0, .DeviceOverrideSecurityMinimum = 0, .AllowedSecurityLevels = 0xFF}},
   {.id = MAC_COMMAND_COORD_REALIGNMENT, .secPolicy = {.SecurityMinimum = 0, .DeviceOverrideSecurityMinimum = 0, .AllowedSecurityLevels = 0xFF}},
   {.id = MAC_COMMAND_ASSOC_REQ_TI_L2MH, .secPolicy = {.SecurityMinimum = 0, .DeviceOverrideSecurityMinimum = 0, .AllowedSecurityLevels = 0xFF}},
   {.id = MAC_COMMAND_ASSOC_RESP_TI_L2MH, .secPolicy = {.SecurityMinimum = 0, .DeviceOverrideSecurityMinimum = 0, .AllowedSecurityLevels = 0xFF}},
   {.id = MAC_COMMAND_DISASSOC_NOT_TI_L2MH, .secPolicy = {.SecurityMinimum = 0, .DeviceOverrideSecurityMinimum = 0, .AllowedSecurityLevels = 0xFF}},
   {.id = MAC_COMMAND_DISASSOC_ACK_TI_L2MH, .secPolicy = {.SecurityMinimum = 0, .DeviceOverrideSecurityMinimum = 0, .AllowedSecurityLevels = 0xFF}},
   {.id = MAC_COMMAND_BEACON_INFO_TI_REQ, .secPolicy = {.SecurityMinimum = 0, .DeviceOverrideSecurityMinimum = 0, .AllowedSecurityLevels = 0xFF}},
   {.id = MAC_COMMAND_BEACON_INFO_TI_RESP, .secPolicy = {.SecurityMinimum = 0, .DeviceOverrideSecurityMinimum = 0, .AllowedSecurityLevels = 0xFF}},
};
*/
const uint8_t commandList1[] =
{
   MAC_COMMAND_ASSOC_REQ,
   MAC_COMMAND_ASSOC_RESP,
   MAC_COMMAND_DISASSOC_NOT,
   MAC_COMMAND_DATA_REQ,
   MAC_COMMAND_PANID_CONFLICT,
   MAC_COMMAND_ORPHAN_NOT,
   MAC_COMMAND_COORD_REALIGNMENT,
   MAC_COMMAND_ASSOC_REQ_TI_L2MH,
   MAC_COMMAND_ASSOC_RESP_TI_L2MH,
   MAC_COMMAND_DISASSOC_NOT_TI_L2MH,
   MAC_COMMAND_DISASSOC_ACK_TI_L2MH,
   MAC_COMMAND_BEACON_INFO_TI_REQ,
   MAC_COMMAND_BEACON_INFO_TI_RESP,
};
#define COMMAND_LIST1_SIZE (sizeof(commandList1)/sizeof(commandList1[0]))

struct structMacSecurityPib macSecurityPib =
{
#if (MAX_MAC_KEY_TABLE_SIZE > 0)
   .macKeyTableLen = 1,
   .macKeyTable =
   {
      {
         //macKeyTable[0]
         .KeyIdLookupList = keyIdLookupList_1,
         .KeyIdLookupListLen = KEY_ID_LIST_1_SIZE,
         .KeyUsageListFrameType = frameTypeList1,
         .FrameTypeListLen = FRAME_TYPE_LIST1_SIZE,
         .KeyUsageListCmd = commandList1,
         .CommandFrameIdentifierListLen = COMMAND_LIST1_SIZE,
         .Key =
         {
            0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF
         },
      }, //macKeyTable[0]
   },
#endif
#if (MAX_MAC_DEVICE_TABLE_SIZE > 0)
   .macDeviceTableLen = 0,
   .macDeviceTable =
   {
      {
         //macDeviceTable[0]
         .FrameCounter = 0,
         .FrameCounterExt = 0,
         .Exempt = 0,
         .DevicePANId = 0,
         .shortAddress = 0, 
      }, //macDeviceTable[0]
   },
#endif
   .macSecurityLevelTable =
   {
      .SevLevListFrameType = frameTypeList1,
      .FrameTypeListLen = FRAME_TYPE_LIST1_SIZE,
      .SevLevListCmd = commandList1,
      .CommandFrameIdentifierListLen = COMMAND_LIST1_SIZE,
   },
   .macFrameCounter = 0,
   .macFrameCounterExt = 0,
   .macAutoRequestSecurityLevel = 0x06,
   .macAutoRequestKeyIdMode = 0,
   .macAutoRequestKeySource = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
   .macAutoRequestKeyIndex = 0xFF,
   .macDefaultKeySource = {1, 1, 1, 1, 1, 1, 1, 1},
};

void MACSEC_fatalErrorHandler()
{
#if (DEBUG_HALT_ON_ERROR)
   volatile int errorCounter = 0;
   while (1)  //fmfmdebugdebug
   {
      errorCounter ++;
   }
#endif
}
/***************************************************************************//**
 * @fn          KeyLookupDataComposeKm0
 *
 * @brief      Build the look up data structure for the KeyDescriptor table search in KeyId mode 0 per 802.15.4-2011
 *
 * @param[in]  keyLookupInput_p - Input of the KeyDesciptor look up procedure
 *
 * @param[Out] LookupDesc_p - Data structure for efficient search of the table
 * @return     MAC_SUCCESS if successful
 ******************************************************************************/

static uint8_t KeyLookupDataComposeKm0(struct structKeyLookupInput *keyLookupInput_p, struct structKeyIdLookupDescriptor *LookupDesc_p)
{
   uint8_t retVal = MAC_SUCCESS;

   if (keyLookupInput_p->deviceAddressingMode == MAC_ADDR_MODE_NULL)
   {
      if (keyLookupInput_p->frameType == MAC_FRAME_TYPE_BEACON || TSCH_MAC_Pib.macCoordinatorShortAddress == 0xFFFE)
      {
         memcpy(LookupDesc_p->LookupData.Km0e.DeviceAddress, TSCH_MAC_Pib.coordinatorEUI, TSCH_DEVICE_EUI_LEN);
         LookupDesc_p->LookupData.Km0e.DevicePANId = TSCH_MAC_Pib.panId;
         LookupDesc_p->len = sizeof(struct structKeyIdLookupDescKm0Ext);
      }
      else if (TSCH_MAC_Pib.macCoordinatorShortAddress <= 0xFFFD)
      {
         LookupDesc_p->LookupData.Km0s.DeviceAddress = TSCH_MAC_Pib.macCoordinatorShortAddress;
         LookupDesc_p->LookupData.Km0s.DevicePANId = TSCH_MAC_Pib.panId;
         LookupDesc_p->len = sizeof(struct structKeyIdLookupDescKm0Short);
      }
      else
      {
         retVal = MAC_INVALID_PARAMETER;
      }
   }
   else if (keyLookupInput_p->deviceAddressingMode == MAC_ADDR_MODE_SHORT)
   {
      LookupDesc_p->LookupData.Km0s.DeviceAddress = *(uint16_t *)keyLookupInput_p->deviceAddress_p;
      LookupDesc_p->LookupData.Km0s.DevicePANId = keyLookupInput_p->devicePANId;
      LookupDesc_p->len = sizeof(struct structKeyIdLookupDescKm0Short);
   }
   else if (keyLookupInput_p->deviceAddressingMode == MAC_ADDR_MODE_EXT)
   {
      memcpy(LookupDesc_p->LookupData.Km0e.DeviceAddress, keyLookupInput_p->deviceAddress_p, IEEE_EXT_ADDRESS_LEN);
      LookupDesc_p->LookupData.Km0e.DevicePANId = keyLookupInput_p->devicePANId;
      LookupDesc_p->len = sizeof(struct structKeyIdLookupDescKm0Ext);
   }
   else
   {
      retVal = MAC_INVALID_PARAMETER;
   }

   return(retVal);
}

/***************************************************************************//**
 * @fn          KeyLookupDataComposeKm1_2_3
 *
 * @brief      Build the look up data structure for the KeyDescriptor table search in KeyId mode 1,2,3 per 802.15.4-2011
 *
 * @param[in]  keyLookupInput_p - Input of the KeyDesciptor look up procedure
 *
 * @param[Out] LookupDesc_p - Data structure for efficient search of the table
 * @return     MAC_SUCCESS if successful
 ******************************************************************************/
static uint8_t KeyLookupDataComposeKm1_2_3(struct structKeyLookupInput *keyLookupInput_p, struct structKeyIdLookupDescriptor *LookupDesc_p)
{
   uint8_t retVal = MAC_INVALID_PARAMETER;
   uint8_t *source_p = (keyLookupInput_p->KeyIdMode == 1) ?
                       macSecurityPib.macDefaultKeySource : keyLookupInput_p->KeySource_p;

   if (source_p != NULL)
   {
      LookupDesc_p->KeyIdMode = keyLookupInput_p->KeyIdMode;

      if (keyLookupInput_p->KeyIdMode == 1 || keyLookupInput_p->KeyIdMode == 3)
      {
         LookupDesc_p->LookupData.Km1_3.KeyIndex = keyLookupInput_p->KeyIndex;
         memcpy(LookupDesc_p->LookupData.Km1_3.KeySource, source_p, 8);
         LookupDesc_p->len = sizeof(struct structKeyIdLookupDescKm1_3);
      }
      else
      {
         LookupDesc_p->LookupData.Km2.KeyIndex = keyLookupInput_p->KeyIndex;
         memcpy(LookupDesc_p->LookupData.Km2.KeySource, source_p, 4);
         LookupDesc_p->len = sizeof(struct structKeyIdLookupDescKm2);
      }

      retVal = MAC_SUCCESS;
   }

   return(retVal);
}

/***************************************************************************//**
 * @fn          KeyLookup
 *
 * @brief      KeyDescriptor table search per 802.15.4-2011
 *
 * @param[in]  LookupDesc_p - Input of the table search procedure
 *
 * @param[Out] keyDesc_pp - Points to a KeyDescriptor data structure if the search is successful, undefined otherwise.
 * @return     MAC_SUCCESS if successful
 ******************************************************************************/
static uint8_t KeyLookup(struct structKeyIdLookupDescriptor *LookupDesc_p, struct structKeyDescriptor **keyDesc_pp)
{
   uint8_t retVal = MAC_UNAVAILABLE_KEY;

   int i, j;

   for (i = 0; i < macSecurityPib.macKeyTableLen; i++)
   {
      for (j = 0; j < macSecurityPib.macKeyTable[i].KeyIdLookupListLen; j++)
      {
         if ((LookupDesc_p->KeyIdMode == macSecurityPib.macKeyTable[i].KeyIdLookupList[j].KeyIdMode) &&
               (memcmp(&(LookupDesc_p->LookupData),
                       &(macSecurityPib.macKeyTable[i].KeyIdLookupList[j].LookupData),
                       LookupDesc_p->len) == 0))
         {
            *keyDesc_pp = &(macSecurityPib.macKeyTable[i]);
            retVal = MAC_SUCCESS;
            break;
         }
      }

      if (retVal == MAC_SUCCESS)
      {
         break;
      }
   }

   return(retVal);
}

/***************************************************************************//**
 * @fn          KeyDescriptorLookupProcedure
 *
 * @brief      KeyDescriptor Look Up procedure as defined by 802.15.4-2011
 *
 * @param[in]  keyLookupInput_p - Input of the procedure
 *
 * @param[Out] keyDesc_pp - Points to a KeyDescriptor data structure if the look up is successful, undefined otherwise.
 * @return     MAC_SUCCESS if successful
 ******************************************************************************/
static uint8_t KeyDescriptorLookupProcedure(struct structKeyLookupInput *keyLookupInput_p, struct structKeyDescriptor **keyDesc_pp)
{
   uint8_t retVal = MAC_UNAVAILABLE_KEY;
   struct structKeyIdLookupDescriptor LookupDesc;

   memset(&LookupDesc, 0, sizeof(LookupDesc));
   LookupDesc.KeyIdMode = keyLookupInput_p->KeyIdMode;

   if (keyLookupInput_p->KeyIdMode == 0)
   {
      retVal = KeyLookupDataComposeKm0(keyLookupInput_p, &LookupDesc);
   }
   else if (keyLookupInput_p->KeyIdMode <= 3)
   {
      retVal = KeyLookupDataComposeKm1_2_3(keyLookupInput_p, &LookupDesc);
   }

   if (retVal == MAC_SUCCESS)
   {
      retVal = KeyLookup(&LookupDesc, keyDesc_pp);
   }

   return(retVal);
}

/***************************************************************************//**
 * @fn          macSecurityKeyLookup
 *
 * @brief      Wrapper for the KeyDescriptor Look Up procedure
 *
 * @param[in]  frame_p - The 802.15.4 frame to be processed
 *
 * @param[Out] keyDesc_pp - Points to a KeyDescriptor data structure if the look up is successful, undefined otherwise.
 * @return     MAC_SUCCESS if successful
 ******************************************************************************/
static uint8_t macSecurityKeyLookup(uint16_t shortAddress, frame802154e_t *frame_p, struct structKeyDescriptor **keyDesc_pp, uint8_t isIncoming)
{
   uint8_t retVal = MAC_SUCCESS;
   struct structKeyLookupInput keyLookupInput;
   uint8_t keyIdx = 0;
   static struct structKeyDescriptor pairWiseKeyDesc;

   keyLookupInput.KeyIdMode = frame_p->aux_hdr.security_control.key_id_mode;
   keyLookupInput.deviceAddress_p = frame_p->dest_addr;
   keyLookupInput.deviceAddressingMode = frame_p->fcf.dest_addr_mode;
   keyLookupInput.devicePANId = frame_p->dest_pid;
   keyLookupInput.frameType = frame_p->fcf.frame_type;

   if (keyLookupInput.KeyIdMode != 0)
   {
      keyIdx = keyLookupInput.KeyIndex = frame_p->aux_hdr.key[(keyLookupInput.KeyIdMode - 1) << 2];
      keyLookupInput.KeySource_p = frame_p->aux_hdr.key;
   }

   if (keyLookupInput.KeyIdMode == MAC_KEY_ID_MODE_1 && keyLookupInput.KeyIndex == 0xFF && shortAddress != 0)
   {
      //pairwise key
      keyLookupInput.KeyIndex = 0;
   }
   retVal = KeyDescriptorLookupProcedure(&keyLookupInput, keyDesc_pp);

   if (retVal == MAC_SUCCESS && keyLookupInput.KeyIdMode == MAC_KEY_ID_MODE_1 && keyIdx == 0xFF && shortAddress != 0)
   {
      //pairwise key
      int i;
      for (i = 0; i < macSecurityPib.macDeviceTableLen; i++)
      {
         if (shortAddress == macSecurityPib.macDeviceTable[i].shortAddress)
         {
            if ((macSecurityPib.macDeviceTable[i].keyUsage & MAC_KEY_USAGE_RX) &&
                  (isIncoming || (macSecurityPib.macDeviceTable[i].keyUsage & MAC_KEY_USAGE_TX)))
            {
            memcpy(&pairWiseKeyDesc, *keyDesc_pp, sizeof(pairWiseKeyDesc));
            memcpy(pairWiseKeyDesc.Key, macSecurityPib.macDeviceTable[i].pairwiseKey, sizeof(pairWiseKeyDesc.Key));
            *keyDesc_pp = &pairWiseKeyDesc;
               if (isIncoming && ((macSecurityPib.macDeviceTable[i].keyUsage & MAC_KEY_USAGE_TX) == 0))
               {
                  //The peer has the pairwise key, so we can use it to send
                  macSecurityPib.macDeviceTable[i].keyUsage |= MAC_KEY_USAGE_TX;
               }
            }
            else
            {
               retVal = MAC_INVALID_INDEX;
            }
            break;
         }
      }
   }

   return(retVal);
}

/***************************************************************************//**
 * @fn          DevLookup
 *
 * @brief      Security Device Look Up procedure as defined by 802.15.4-2011
 *
 * @param[in]  LookupDesc_p - Device look up input
 *
 * @param[Out] devDesc_pp - Points to a Device Descriptor data structure if the look up is successful, undefined otherwise.
 * @return     MAC_SUCCESS if successful
 ******************************************************************************/
static uint8_t DevLookup(struct structDevLookupDescriptor *LookupDesc_p, struct structDeviceDescriptor **devDesc_pp)
{
   uint8_t retVal = MAC_UNAVAILABLE_DEVICE;
   int i;
   uint16_t shortAddress;

   for (i = 0; i < macSecurityPib.macDeviceTableLen; i++)
   {
      shortAddress = macSecurityPib.macDeviceTable[i].shortAddress;

      if (macSecurityPib.macDeviceTable[i].DevicePANId == LookupDesc_p->DevicePANId && shortAddress != 0)
      {
         if (LookupDesc_p->len == sizeof(struct structExtLookupAddress))
         {
            ulpsmacaddr_t extAddr = NHL_deviceTableGetLongAddr(shortAddress);
            if (memcmp(extAddr.u8, LookupDesc_p->data.eAddr.DeviceAddress, IEEE_EXT_ADDRESS_LEN) == 0)
            {
               *devDesc_pp = &(macSecurityPib.macDeviceTable[i]);
               retVal = MAC_SUCCESS;
               break;
            }
         }
         else if (shortAddress == LookupDesc_p->data.sAddr.DeviceAddress)
         {
            *devDesc_pp = &(macSecurityPib.macDeviceTable[i]);
            retVal = MAC_SUCCESS;
            break;
         }
      }
   }

   return(retVal);
}

/***************************************************************************//**
 * @fn          macSecurityDeviceLookup
 *
 * @brief      Wrapper for the Security Device Look Up procedure as defined by 802.15.4-2011
 *
 * @param[in]  frame_p - 802.15.4 frame to be processed
 *
 * @param[Out] devDesc_pp - Points to a Device Descriptor data structure if the look up is successful, undefined otherwise.
 * @return     MAC_SUCCESS if successful
 ******************************************************************************/
static uint8_t macSecurityDeviceLookup(frame802154e_t *frame_p, struct structDeviceDescriptor **devDesc_pp)
{
   uint8_t retVal = MAC_SUCCESS;
   struct structDevLookupDescriptor LookupDesc;
   uint8_t *deviceAddress_p = frame_p->src_addr;
   uint8_t deviceAddressingMode = frame_p->fcf.src_addr_mode;
   uint16_t devicePANId = frame_p->src_pid;

   memset(&LookupDesc, 0, sizeof(LookupDesc));

   if (deviceAddressingMode == MAC_ADDR_MODE_NULL)
   {
      LookupDesc.DevicePANId = TSCH_MAC_Pib.panId;

      if (TSCH_MAC_Pib.macCoordinatorShortAddress == 0xFFFE)
      {
         memcpy(LookupDesc.data.eAddr.DeviceAddress, TSCH_MAC_Pib.coordinatorEUI, IEEE_EXT_ADDRESS_LEN);
         LookupDesc.len = sizeof(struct structExtLookupAddress);
      }
      else if (TSCH_MAC_Pib.macCoordinatorShortAddress <= 0xFFFD)
      {
         LookupDesc.data.sAddr.DeviceAddress = TSCH_MAC_Pib.macCoordinatorShortAddress;
         LookupDesc.len = sizeof(struct structShortLookupAddress);
      }
      else
      {
         retVal = MAC_INVALID_PARAMETER;
      }
   }
   else if (deviceAddressingMode == MAC_ADDR_MODE_SHORT)
   {
      LookupDesc.data.sAddr.DeviceAddress = *(uint16_t *)deviceAddress_p;
      LookupDesc.DevicePANId = devicePANId;
      LookupDesc.len = sizeof(struct structShortLookupAddress);
   }
   else if (deviceAddressingMode == MAC_ADDR_MODE_EXT)
   {
      memcpy(LookupDesc.data.eAddr.DeviceAddress, deviceAddress_p, IEEE_EXT_ADDRESS_LEN);
      LookupDesc.DevicePANId = devicePANId;
      LookupDesc.len = sizeof(struct structExtLookupAddress);
   }
   else
   {
      retVal = MAC_INVALID_PARAMETER;
   }

   if (retVal == MAC_SUCCESS)
   {
      retVal = DevLookup(&LookupDesc, devDesc_pp);
   }

   return(retVal);
}
/***************************************************************************//**
 * @fn          macSecurityLevelLookup
 *
 * @brief      MAC security level look up procedure per IEEE 802.15.4-2011
 *
 * @param[in]  frame_p - 802.15.4 frame to be processed
 *
 * @param[Out] secLevelDesc_pp - Security Level Desc, if the procedure is successful, undefined otherwise
 * @return     MAC_SUCCESS if successful
 ******************************************************************************/
static uint8_t macSecurityLevelLookup(frame802154e_t *frame_p, struct structSecurityLevelDescriptor **secLevelDesc_pp)
{
   uint8_t retVal = MAC_UNAVAILABLE_SECURITY_LEVEL;
   uint8_t frameType = frame_p->fcf.frame_type;
   uint8_t cmdFrameId = *((uint8_t *)frame_p->payload + frame_p->payloadie_len);
   int i;

   for (i = 0; i < macSecurityPib.macSecurityLevelTable.FrameTypeListLen; i++)
   {
      if (frameType == macSecurityPib.macSecurityLevelTable.SevLevListFrameType[i].id)
      {
         if (frameType != MAC_FRAME_TYPE_COMMAND)
         {
            *secLevelDesc_pp = (struct structSecurityLevelDescriptor *) & (macSecurityPib.macSecurityLevelTable.SevLevListFrameType[i]);
            return(MAC_SUCCESS);
         }
         else
         {
            int j;

            for (j = 0; j < macSecurityPib.macSecurityLevelTable.CommandFrameIdentifierListLen; j++)
            {
               if (cmdFrameId == macSecurityPib.macSecurityLevelTable.SevLevListCmd[j])
               {
                  *secLevelDesc_pp = (struct structSecurityLevelDescriptor *) & (macSecurityPib.macSecurityLevelTable.SevLevListFrameType[i]);
                  return(MAC_SUCCESS);
               }
            }
         }
      }
   }

   return(retVal);
}
/***************************************************************************//**
 * @fn          macSecurityLevelCheck
 *
 * @brief      MAC security level check procedure per IEEE 802.15.4-2011
 *
 * @param[in]  frame_p - 802.15.4 frame to be processed,
 *             secLevelDesc_p - Security Level Desc to check with
 *
 * @param[Out] N/A
 * @return     MAC_SUCCESS if successful
 ******************************************************************************/
static uint8_t macSecurityLevelCheck(frame802154e_t *frame_p, struct structSecurityLevelDescriptor *secLevelDesc_p)
{
   uint8_t retVal = MAC_IMPROPER_SECURITY_LEVEL;
   uint8_t sec1Lev = frame_p->aux_hdr.security_control.security_level;
   uint8_t sec1b2 = sec1Lev & 0x04;
   uint8_t sec1b1b0 = sec1Lev & 0x03;

   if (secLevelDesc_p->secPolicy.AllowedSecurityLevels == 0)
   {
      uint8_t sec2Lev = secLevelDesc_p->secPolicy.SecurityMinimum;

      if ((sec1b2 >= (sec2Lev & 0x04)) && (sec1b1b0 >= (sec2Lev & 0x03)))
      {
         retVal = MAC_SUCCESS;
      }
   }
   else
   {
      uint8_t mask = 0x01;
      uint8_t i;

      for (i = 0; i < 8; i++)
      {
         if ((secLevelDesc_p->secPolicy.AllowedSecurityLevels & mask) && (sec1Lev == i))
         {
            retVal = MAC_SUCCESS;
            break;
         }

         mask <<= 1;
      }
   }

   if (retVal != MAC_SUCCESS && sec1Lev == 0 && secLevelDesc_p->secPolicy.DeviceOverrideSecurityMinimum)
   {
      retVal = MAC_CONDITIONAL_PASS;
   }

   return(retVal);
}
/***************************************************************************//**
 * @fn          macFrameCounterCheck
 *
 * @brief      MAC frame counter check procedure per IEEE 802.15.4-2011
 *
 * @param[in]  frame_p - 802.15.4 frame to be processed,
 *             devDesc_pdevDesc_p - Security Device Desc to check with
 *
 * @param[Out] N/A
 * @return     MAC_SUCCESS if successful
 ******************************************************************************/
static uint8_t macFrameCounterCheck(frame802154e_t *frame_p, struct structDeviceDescriptor *devDesc_p)
{
   uint8_t retVal = MAC_SUCCESS;

   if ((frame_p->aux_hdr.frame_counter == 0xFFFFFFFF) &&
         ((frame_p->aux_hdr.security_control.frame_counter_size == MAC_FRAME_CNT_4_BYTES) ||
          (frame_p->aux_hdr.frame_counter_ext == 0xFF)))
   {
      retVal = MAC_COUNTER_ERROR;
   }
   else
   {
      uint8_t counterExt1 = frame_p->aux_hdr.frame_counter_ext;
      uint8_t counterExt2 = devDesc_p->FrameCounterExt;

      if (frame_p->aux_hdr.security_control.frame_counter_size == MAC_FRAME_CNT_4_BYTES)
      {
         if (frame_p->aux_hdr.frame_counter < devDesc_p->FrameCounter)
         {
            retVal = MAC_COUNTER_ERROR;
         }
      }
      else if ((counterExt1 < counterExt2) ||
               (counterExt1 ==  counterExt2 && frame_p->aux_hdr.frame_counter < devDesc_p->FrameCounter))
      {
         retVal = MAC_COUNTER_ERROR;
      }
   }

   return(retVal);
}
/***************************************************************************//**
 * @fn          macKeyUsagePolicyCheck
 *
 * @brief      MAC key usage check procedure per IEEE 802.15.4-2011
 *
 * @param[in]  frame_p - 802.15.4 frame to be processed,
 *             keyDesc_p - Key Desc to check with
 *
 * @param[Out] N/A
 * @return     MAC_SUCCESS if successful
 ******************************************************************************/
static uint8_t macKeyUsagePolicyCheck(frame802154e_t *frame_p, struct structKeyDescriptor *keyDesc_p)
{
   uint8_t retVal = MAC_IMPROPER_KEY_TYPE;
   uint8_t frameType = frame_p->fcf.frame_type;
   uint8_t cmdFrameId = *((uint8_t *)frame_p->payload + frame_p->payloadie_len);
   int i;

   for (i = 0; i < keyDesc_p->FrameTypeListLen; i++)
   {
      if (frameType == keyDesc_p->KeyUsageListFrameType[i].id)
      {
         if (frameType != MAC_FRAME_TYPE_COMMAND)
         {
            return(MAC_SUCCESS);
         }
         else
         {
            int j;

            for (j = 0; j < keyDesc_p->CommandFrameIdentifierListLen; j++)
            {
               if (cmdFrameId == keyDesc_p->KeyUsageListCmd[j])
               {
                  return(MAC_SUCCESS);
               }
            }
         }
      }
   }

   return(retVal);
}

//=====================================================
//
//
//    API functions
//=====================================================
/***************************************************************************//**
 * @fn          MAC_SecurityDeviceTableUpdate
 *
 * @brief
 *
 * @param[in]
 *
 * @param[Out]
 * @return
 ******************************************************************************/
void MAC_SecurityDeviceTableUpdate(ulpsmacaddr_panid_t ulpsmacaddr_panid_node, uint16_t *destShortAddr_p, ulpsmacaddr_t *destExtAddr_p)
{
   int i;
   uint16_t shortAddress;

   for (i = 0; i < macSecurityPib.macDeviceTableLen; i++)
   {
      shortAddress = macSecurityPib.macDeviceTable[i].shortAddress;

      if (shortAddress != 0)
      {
         ulpsmacaddr_t extAddr = NHL_deviceTableGetLongAddr(shortAddress);
         if (memcmp(extAddr.u8, destExtAddr_p, IEEE_EXT_ADDRESS_LEN) == 0)
         {
            macSecurityPib.macDeviceTable[i].DevicePANId = ulpsmacaddr_panid_node;
            break;
         }
      }
   }
}
/***************************************************************************//**
 * @fn          MAC_MlmeSetSecurityReq
 *
 * @brief
 *
 * @param[in]
 *
 * @param[Out]
 * @return
 ******************************************************************************/
void MAC_MlmeSetSecurityReq(int pibId, uint8_t *pibVal)
{
   uint32_t key;
   switch (pibId)
   {
      case MAC_FRAME_COUNTER_MODE:
         key = Task_disable();
         macSecurityPib.macFrameCounterMode = *pibVal;
         Task_restore(key);
         break;

      case MAC_PAN_COORD_SHORT_ADDRESS:
         key = Task_disable();
         memcpy((uint8_t *) & (TSCH_MAC_Pib.macCoordinatorShortAddress), pibVal, 2);
         Task_restore(key);
         break;

      case MAC_PAN_COORD_EXTENDED_ADDRESS:
         key = Task_disable();
         memcpy(TSCH_MAC_Pib.coordinatorEUI, pibVal, IEEE_EXT_ADDRESS_LEN);
         Task_restore(key);
         break;

      default:
         break;
   }
}

/***************************************************************************//**
 * @fn          MAC_MlmeGetSecurityReq
 *
 * @brief
 *
 * @param[in]
 *
 * @param[Out]
 * @return
 ******************************************************************************/
void MAC_MlmeGetSecurityReq(int pibId, uint8_t *pibVal)
{
   uint32_t key;
   switch (pibId)
   {
      case MAC_FRAME_COUNTER_MODE:
         key = Task_disable();
         *pibVal = macSecurityPib.macFrameCounterMode;
         Task_restore(key);
         break;

      case MAC_PAN_COORD_SHORT_ADDRESS:
         key = Task_disable();
         memcpy(pibVal, (uint8_t *) & (TSCH_MAC_Pib.macCoordinatorShortAddress), 2);
         Task_restore(key);
         break;

      case MAC_PAN_COORD_EXTENDED_ADDRESS:
         key = Task_disable();
         memcpy(pibVal, TSCH_MAC_Pib.coordinatorEUI, IEEE_EXT_ADDRESS_LEN);
         Task_restore(key);
         break;

      default:
         break;
   }
}
/***************************************************************************//**
 * @fn          MAC_SecurityKeyIdUpate
 *
 * @brief
 *
 * @param[in]
 *
 * @param[Out]
 * @return
 ******************************************************************************/
void MAC_SecurityKeyIdUpate(ulpsmacaddr_panid_t ulpsmacaddr_panid_node, uint8_t *my_shortaddr_p, ulpsmacaddr_t *ulpsmacaddr_long_node_p)
{
   uint32_t key = Task_disable();
   TSCH_MAC_Pib.panId = ulpsmacaddr_panid_node;
   Task_restore(key);
   key = Task_disable();
   memcpy(TSCH_MAC_Pib.coordinatorEUI, ulpsmacaddr_long_node_p->u8, IEEE_EXT_ADDRESS_LEN);
   Task_restore(key);
   key = Task_disable();
   memcpy((uint8_t *) & (TSCH_MAC_Pib.macCoordinatorShortAddress), my_shortaddr_p, 2);
   Task_restore(key);
}
/***************************************************************************//**
 * @fn          MAC_SecurityInit
 *
 * @brief
 *
 * @param[in]
 *
 * @param[Out]
 * @return
 ******************************************************************************/
#define MAC_SECURITY_TEST_1 0
#define MAC_SECURITY_TEST_2 0
#define MAC_SECURITY_TEST_3 0
#define MAC_SECURITY_TEST_4 0
#define MAC_SECURITY_TEST_5 0
void MAC_SecurityInit()
{
#if MAC_SECURITY_TEST_1
   uint8_t pKeyTx[16] = {0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF};
   uint8_t authlen = 8;
   uint8_t securityLevel = 2;
   uint32_t FrameCounter = 0x05;
   uint8_t pAData[] = {0x08, 0xD0, 0x84, 0x21, 0x43, 0x01, 0x00, 0x00, 0x00, 0x00, 0x48, 0xDE, 0xAC, 0x02, 0x05,
                       0x00, 0x00, 0x00, 0x55, 0xCF, 0x00, 0x00, 0x51, 0x52, 0x53, 0x54, 0, 0, 0, 0, 0, 0, 0, 0
                      };
   uint8_t aDataLen = sizeof(pAData) - 8;
   uint8_t srcAddr[] = {0xac, 0xde, 0x48, 0x00, 0x00, 0x00, 0x00, 0x01};
   uint8_t expectedMac[8] = {0x22, 0x3B, 0xC1, 0xEC, 0x84, 0x1A, 0xB5, 0x53};
   uint8_t *mac_p = pAData + aDataLen;
   uint8_t nonce[13];
   int i;

   memcpy(nonce, srcAddr, 8);

   for (i = 11; i >= 8; i--)
   {
      nonce[i] = FrameCounter & 0xFF;
      FrameCounter >>= 8;
   }

   nonce[12] = securityLevel;

   macCcmStarTransform(pKeyTx, authlen, nonce, pAData, aDataLen, pAData + aDataLen, 0);

   if (memcmp(mac_p, expectedMac, 8) != 0)
   {
      MACSEC_fatalErrorHandler();
   }

   //expected pAData here:
   //08 D0 84 21 43 01 00 00 00 00 48 DE AC || 02 05 00 00 00 || 55 CF 00 00 51 52 53 54 22 3B C1 EC 84 1A
   // B5 53

   if (macCcmStarInverseTransform(pKeyTx, authlen, nonce, pAData + aDataLen, authlen, pAData, aDataLen ) != MAC_SUCCESS)
   {
      MACSEC_fatalErrorHandler();
   }

#endif

#if MAC_SECURITY_TEST_2
   uint8_t pKeyTx[16] = {0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF};
   uint8_t authlen = 0;
   uint8_t securityLevel = 4;
   uint32_t FrameCounter = 0x05;
   uint8_t pAData[] = {0x69, 0xDC, 0x84, 0x21, 0x43, 0x02, 0x00, 0x00, 0x00, 0x00, 0x48, 0xDE, 0xAC, 0x01, 0x00, 0x00,
                       0x00, 0x00, 0x48, 0xDE, 0xAC, 0x04, 0x05, 0x00, 0x00, 0x00, 0x61, 0x62, 0x63, 0x64
                      };
   uint8_t pdataLen = 4;
   uint8_t aDataLen = sizeof(pAData) - pdataLen - authlen;
   uint8_t srcAddr[] = {0xac, 0xde, 0x48, 0x00, 0x00, 0x00, 0x00, 0x01};
   uint8_t nonce[13];
   int i;
   uint8_t rtv;

   memcpy(nonce, srcAddr, 8);

   for (i = 11; i >= 8; i--)
   {
      nonce[i] = FrameCounter & 0xFF;
      FrameCounter >>= 8;
   }

   nonce[12] = securityLevel;

   rtv = macCcmStarTransform(pKeyTx, authlen, nonce, pAData, aDataLen, pAData + aDataLen, pdataLen);
   //expected pAData here:
   //69 DC 84 21 43 02 00 00 00 00 48 DE AC 01 00 00 00 00 48 DE AC || 04 05 00 00 00 || D4 3E 02 2B
   if (rtv != MAC_SUCCESS)
   {
      MACSEC_fatalErrorHandler();
   }

   nonce[12] = securityLevel;

   //pAData[aDataLen + pdataLen] ^= 0x01;

   rtv = macCcmStarInverseTransform(pKeyTx, authlen, nonce, pAData + aDataLen, pdataLen + authlen, pAData, aDataLen );

   if (rtv != MAC_SUCCESS)
   {
      MACSEC_fatalErrorHandler();
   }

#endif

#if MAC_SECURITY_TEST_3
   volatile int ell = 0;
   uint8_t pKeyTx[16] = {0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF};
   uint8_t authlen = 8;
   uint8_t securityLevel = 6;
   uint32_t FrameCounter = 0x05;
   uint8_t pAData[] = { 0x2B, 0xDC, 0x84, 0x21, 0x43, 0x02, 0x00, 0x00, 0x00, 0x00, 0x48, 0xDE, 0xAC, 0xFF, 0xFF, 0x01,
                        0x00, 0x00, 0x00, 0x00, 0x48, 0xDE, 0xAC, 0x06, 0x05, 0x00, 0x00, 0x00, 0x01, 0xCE, 0, 0, 0, 0, 0, 0, 0, 0
                      };
   uint8_t pdataLen = 1;
   uint8_t aDataLen = sizeof(pAData) - pdataLen - authlen;
   uint8_t srcAddr[] = {0xac, 0xde, 0x48, 0x00, 0x00, 0x00, 0x00, 0x01};
   uint8_t nonce[13];
   int i;
   uint8_t rtv;

   memcpy(nonce, srcAddr, 8);

   for (i = 11; i >= 8; i--)
   {
      nonce[i] = FrameCounter & 0xFF;
      FrameCounter >>= 8;
   }

   nonce[12] = securityLevel;

   rtv = macCcmStarTransform(pKeyTx, authlen, nonce, pAData, aDataLen, pAData + aDataLen, pdataLen);
   //Expected pAData after the transform:
   //2B DC 84 21 43 02 00 00 00 00 48 DE AC FF FF 01 00 00 00 00 48 DE AC || 06 05 00 00 00 || 01 D8 4F DE
   // 52 90 61 F9 C6 F1
   if (rtv != MAC_SUCCESS)
   {
      MACSEC_fatalErrorHandler();
   }

   i = 1;
   i++;

   //pAData[aDataLen + pdataLen] ^= 0x01;

   rtv = macCcmStarInverseTransform(pKeyTx, authlen, nonce, pAData + aDataLen, pdataLen + authlen, pAData, aDataLen );
   //Expected pAData here:
   //0x2B, 0xDC, 0x84, 0x21, 0x43, 0x02, 0x00, 0x00, 0x00, 0x00, 0x48, 0xDE, 0xAC, 0xFF, 0xFF, 0x01,
   // 0x00, 0x00, 0x00, 0x00, 0x48, 0xDE, 0xAC, 0x06, 0x05, 0x00, 0x00, 0x00, 0x01, 0xCE
   if (rtv != MAC_SUCCESS)
   {
      MACSEC_fatalErrorHandler();
   }
   
   while (1)
   {
     ell++;
   }

#endif

#if MAC_SECURITY_TEST_4
   static volatile int ell = 0;
   static uint8_t pKeyTx[16] = {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF};
   static uint8_t authlen = 4;
   static uint8_t pAData[] =   {0x69, 0xA8, 0x78, 0xAB, 0xAB, 0x01, 0x00, 0x03, 0x00, 0x4D, 0x4D, 0xA7, 0x02, 0x00, 0x00, 0x00,
         0x7A, 0x33, 0x3A, 0x9B, 0x02, 0xBF, 0x9D, 0x1E, 0x00, 0x00, 0xF1, 0x05, 0x12, 0x01, 0x80, 0x20,
         0x01, 0x0D, 0xB8, 0x12, 0x34, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFE, 0x00, 0x00, 0x03, 0x06, 0x14,
         0x00, 0x00, 0x00, 0x78, 0x20, 0x01, 0x0D, 0xB8, 0x12, 0x34, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFE, 0x00, 0x00, 0x01,
                        0, 0, 0, 0};  //73
   static uint8_t pdataLen = 53;
   static uint8_t aDataLen ;
   static uint8_t nonce[13] = {0x40, 0xC4, 0x99, 0x18, 0x00, 0x4B, 0x12, 0x00, 0x00, 0x02, 0xA7, 0x4D, 0x00};
   uint8_t rtv;
   
   aDataLen = sizeof(pAData) - pdataLen - authlen;  //73-53-4 = 16
   
    rtv = macCcmStarTransform(pKeyTx, authlen, nonce, pAData, aDataLen, pAData + aDataLen, pdataLen);
   //Expected pAData after the transform:
   // 69 A8 78 AB AB 01 00 03 00 4D 4D A7 02 00 00 00
   // 8F F8 55 25 EE 68 EE C4 C2 E8 16 CD D3 5B 52 9B
   // 47 6F 00 90 81 04 D2 5E 00 90 5A A6 4C 25 4F 5A B5 7F 92 7D C7 69 D2 35 DE 5C 02 01 D9 2D D8 B9
   // 8B 38 FD 85 0C
   // 7B FE C8 D2

   if (rtv != MAC_SUCCESS)
   {
      MACSEC_fatalErrorHandler();
   }

   rtv = macCcmStarInverseTransform(pKeyTx, authlen, nonce, pAData + aDataLen, pdataLen + authlen, pAData, aDataLen );
   if (rtv != MAC_SUCCESS)
   {
      MACSEC_fatalErrorHandler();
   }

   while (1)
   {
     ell++;
   }

#endif

#if MAC_SECURITY_TEST_5
   static volatile int ell = 0;
   static uint8_t key[16] = {30, 120, 22, 196, 85, 141, 167, 14, 31, 165, 241, 97, 166, 230, 68, 0};
   static uint8_t aad[13] =   {0, 1, 0, 0, 0, 0, 0, 0, 22, 254, 253, 0, 24};
   static uint8_t buf[32] =   {189, 126, 176, 175, 103, 63, 53, 104,
                               151, 241, 217, 200, 43, 126, 218, 195,
                               140, 245,  97,  95, 4,  149, 123, 177,
                               99,  144,  74, 81, 111,  34, 134,  79};
   static uint8_t length = 32;
   static uint8_t la = 13;
   static uint8_t nounce[12] = {12, 142, 139, 221, 0, 1, 0, 0, 0, 0, 0, 0};
   static uint8_t mic[8];
   int status;

#ifdef DeviceFamily_CC26X2
   status = bspAesCcmDecryptAuth((uint8_t *)key, 8, (uint8_t *)nounce,  (uint8_t *)buf, length,
                                  (uint8_t *)aad, la, 3, (uint8_t *)mic);
#else
   status = bspAesCcmDecryptAuth((const char*)key,
                            CRYPTOCC26XX_KEY_ANY,//defaultkeylocationdec,
                                8,//mLen,                  //depending on security level
                                (char *)nounce,
                                (char *)buf, //payload
                                length,
                                (char *)aad,     //header
                                la,
                                3,//defaultfieldlen,                 //2
                                (char *)mic);
#endif
//Expected decoded buf (24 bytes) here:
//{20,0,0,12,0,5,0,0,0,0,0,12,121,55,120,72,95,91,130,147,0,78,126,192}
   while (1)
   {
     ell++;
   }

#endif
}
/***************************************************************************//**
 * @fn          macOutgoingFrameSecurity
 *
 * @brief
 *
 * @param[in]
 *
 * @param[Out]
 * @return
 ******************************************************************************/
uint8_t macOutgoingFrameSecurity(uint16_t shortAddr, frame802154e_t *frame_p, uint8_t **pKeyTx_p, uint8_t **pFrameCounterExt_p, uint32_t **pFrameCounter_p)
{
   struct structKeyDescriptor *keyDesc_p;
   uint8_t retVal;

    retVal = macSecurityKeyLookup(shortAddr, frame_p, &keyDesc_p, 0);

   if (retVal == MAC_SUCCESS)
   {
      *pKeyTx_p = keyDesc_p->Key;
      *pFrameCounterExt_p = &(macSecurityPib.macFrameCounterExt);
      *pFrameCounter_p = &(macSecurityPib.macFrameCounter);
   }

   return(retVal);
}
/***************************************************************************//**
 * @fn          macIncomingFrameSecurity
 *
 * @brief
 *
 * @param[in]
 *
 * @param[Out]
 * @return
 ******************************************************************************/
uint8_t macIncomingFrameSecurity(uint16_t shortAddr, frame802154e_t *frame_p, uint8_t **pKeyRx_p, struct structDeviceDescriptor **devDesc_pp)
{
   uint8_t retVal = MAC_SUCCESS;
   struct structKeyDescriptor *keyDesc_p;
   struct structDeviceDescriptor *devDesc_p;
   struct structSecurityLevelDescriptor *secLevelDesc_p;


   if (frame_p->fcf.frame_version == 0)
   {
      retVal = MAC_UNSUPPORTED_LEGACY;
   }
   else if (frame_p->aux_hdr.security_control.security_level == 0)
   {
      retVal = MAC_UNSUPPORTED_SECURITY;
   }
   else
   {
      retVal = macSecurityKeyLookup(shortAddr, frame_p, &keyDesc_p, 1);
   }

   if (retVal == MAC_SUCCESS)
   {
      *pKeyRx_p = keyDesc_p->Key;
      retVal = macSecurityDeviceLookup(frame_p, &devDesc_p);
   }

   if (retVal == MAC_SUCCESS)
   {
      *devDesc_pp = devDesc_p;
      retVal = macSecurityLevelLookup(frame_p, &secLevelDesc_p);
   }

   if (retVal == MAC_SUCCESS)
   {
      uint8_t scRetVal;
      scRetVal = macSecurityLevelCheck(frame_p, secLevelDesc_p);

      if (scRetVal == MAC_CONDITIONAL_PASS)
      {
         if (!devDesc_p->Exempt)
         {
            retVal = MAC_IMPROPER_SECURITY_LEVEL;
         }
      }
      else if (scRetVal == MAC_SUCCESS)
      {
         retVal = macFrameCounterCheck(frame_p, devDesc_p);

         if (retVal == MAC_SUCCESS)
         {
            retVal = macKeyUsagePolicyCheck(frame_p, keyDesc_p);
         }
      }
   }

   return(retVal);
}
/***************************************************************************//**
 * @fn          macCcmStarInverseTransform
 *
 * @brief
 *
 * @param[in]
 *
 * @param[Out]
 * @return
 ******************************************************************************/
uint8_t macCcmStarInverseTransform(uint8_t *pKeyRx, uint8_t authlen,
                                   uint8_t *nonce, uint8_t *pCData,
                                   uint8_t cDataLen, uint8_t *pAData,
                                   uint8_t aDataLen)
{
    uint8_t regTag[16];
    int status;

#ifdef DeviceFamily_CC26X2
    status = bspAesCcmDecryptAuth( pKeyRx, authlen, nonce, pCData, cDataLen,
                                  pAData, aDataLen, 2, regTag);

    if (status != AESCCM_STATUS_SUCCESS)
    {
        return (MAC_SECURITY_ERROR);
    }

#else
    status = bspAesCcmDecryptAuth((const char *)pKeyRx, CRYPTOCC26XX_KEY_ANY, authlen, (char *)nonce, (char *)pCData, cDataLen,
            (char *)pAData, aDataLen, 2, (char *)regTag);

    if (status != CRYPTOCC26XX_STATUS_SUCCESS)
    {
        return (MAC_SECURITY_ERROR);
    }
#endif

    return (MAC_SUCCESS);
}
/***************************************************************************//**
 * @fn          macCcmStarTransform
 *
 * @brief
 *
 * @param[in]
 *
 * @param[Out]
 * @return
 ******************************************************************************/
uint8_t macCcmStarTransform(uint8_t *pKeyTx, uint8_t authlen, uint8_t *nonce,
                            uint8_t *pAData, uint8_t aDataLen, uint8_t *pMData,
                            uint8_t mDataLen)
{

    int status;

#ifdef DeviceFamily_CC26X2
    status = bspAesCcmAuthEncrypt( pKeyTx, authlen, nonce,  pMData, mDataLen,
                                   pAData, aDataLen, 2, pMData + mDataLen);
    if (status != AESCCM_STATUS_SUCCESS)
    {
        return (MAC_SECURITY_ERROR);
    }

#else
    status = bspAesCcmAuthEncrypt((const char *)pKeyTx, CRYPTOCC26XX_KEY_ANY, authlen, (char *)nonce, (char *)pMData, mDataLen,
            (char *)pAData, aDataLen, 2, (char *)pMData + mDataLen);

    if (status != CRYPTOCC26XX_STATUS_SUCCESS)
    {
        return (MAC_SECURITY_ERROR);
    }
#endif

    return (MAC_SUCCESS);
}
/***************************************************************************//**
 * @fn          macSecurityDeviceReset
 *
 * @brief      API function to reset a device in the MAC security device table
 *
 * @param[in]  shortAddr: > 0: the short address of the device,  0: reset all devices in the table
 *
 * @param[Out] N/A
 * @return     N/A
 ******************************************************************************/
void macSecurityDeviceReset(uint16_t shortAddr)
{
   int i;

   if (shortAddr == 0)
   {
      macSecurityPib.macDeviceTableLen = 0;
      memset(macSecurityPib.macDeviceTable, 0, sizeof(macSecurityPib.macDeviceTable));
   }
   else
   {
      for (i = 0; i < macSecurityPib.macDeviceTableLen; i++)
      {
         if (macSecurityPib.macDeviceTable[i].shortAddress == shortAddr)
         {
            macSecurityPib.macDeviceTable[i].FrameCounter = macSecurityPib.macDeviceTable[i].FrameCounterExt = 0;
            macSecurityPib.macDeviceTable[i].keyUsage = 0;
            break;
         }
      }
   }
}
/***************************************************************************//**
 * @fn          macSecurityDeviceAdd
 *
 * @brief      API function to add a new device to the MAC security device table
 *
 * @param[in]  short address: NHL device short address
 *
 * @param[Out] N/A
 * @return     MAC_SUCCESS if successful
 ******************************************************************************/
uint8_t macSecurityDeviceAdd(uint16_t shortAddr)
{
   uint8_t retVal = MAC_DEV_TABLE_FULL;
   int i;

   for (i = 0; i < macSecurityPib.macDeviceTableLen; i++)
   {
      if (macSecurityPib.macDeviceTable[i].shortAddress == shortAddr)
      {
         break;
      }
   }

   if (i == macSecurityPib.macDeviceTableLen && i < MAX_MAC_DEVICE_TABLE_SIZE)
   {
      macSecurityPib.macDeviceTableLen++;
   }

   if (i < MAX_MAC_DEVICE_TABLE_SIZE)
   {
      memset(&(macSecurityPib.macDeviceTable[i]), 0, sizeof(struct structDeviceDescriptor));
      macSecurityPib.macDeviceTable[i].DevicePANId = TSCH_MAC_Pib.panId;
      macSecurityPib.macDeviceTable[i].shortAddress = shortAddr;

      retVal = MAC_SUCCESS;
   }

   return(retVal);
}
/***************************************************************************//**
 * @fn          macSecurityDeviceRemove
 *
 * @brief      API function to remove a device from the MAC security device table
 *
 * @param[in]  shortAddr - Short address of the NHL device to be removed
 *
 * @param[Out] N/A
 * @return     MAC_SUCCESS if successful
 ******************************************************************************/
uint8_t macSecurityDeviceRemove(uint16_t shortAddr)
{
   uint8_t retVal = MAC_SUCCESS;
   uint16_t i;

   for (i = 0; i < macSecurityPib.macDeviceTableLen; i++)
   {
      if (macSecurityPib.macDeviceTable[i].shortAddress == shortAddr)
      {
         uint16_t j;

         macSecurityPib.macDeviceTableLen--;

         for (j = i; j < macSecurityPib.macDeviceTableLen; j++)
         {
            macSecurityPib.macDeviceTable[j] = macSecurityPib.macDeviceTable[j + 1];
         }

         memset(&(macSecurityPib.macDeviceTable[j]), 0, sizeof(struct structDeviceDescriptor));
         break;
      }
   }

   return(retVal);
}
/***************************************************************************//**
 * @fn          macSecurityDeviceUpdate
 *
 * @brief      API function to update the short address of a device in the MAC security device table
 *
 * @param[in]  oldShortAddr - old short address,
 *             newShortAddr - new short address
 *
 * @param[Out] N/A
 * @return     MAC_SUCCESS if successful
 ******************************************************************************/
uint8_t macSecurityDeviceUpdate(uint16_t oldShortAddr, uint16_t newShortAddr)
{
   uint8_t retVal = MAC_SUCCESS;
   uint16_t i;

   for (i = 0; i < macSecurityPib.macDeviceTableLen; i++)
   {
      if (macSecurityPib.macDeviceTable[i].shortAddress == oldShortAddr)
      {
         macSecurityPib.macDeviceTable[i].shortAddress = newShortAddr;
         break;
      }
   }

   return(retVal);
}
#endif // FEATURE_MAC_SECURITY
