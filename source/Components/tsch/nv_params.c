/**
@file  nv_params.c
@brief NVM (flash) parameters read/write driver code

<!--
Copyright 2017 Texas Instruments Incorporated. All rights reserved.

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
PROVIDED ``AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
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
-->
*/
/*******************************************************************************
* INCLUDES
*/
#include "NVS.h"
#include "NVSCC26XX.h"
#include "nv_params.h"
#include "mac_config.h"
#if FEATURE_MAC_SECURITY
#include "mac_security_api.h"
#endif
/*******************************************************************************
* CONSTANTS
*/
#define NVS_BLOCK_SIZE              (sizeof(struct structNvParams))
#define NVS_USE_RAM_COPY_BLOCK      1
#define NV_OFFSET                   (0)   //NV paramater offset in # of bytes
#ifdef DeviceFamily_CC26X2
#define NVS_block                   (0x54000)
#define NVS_block_2                 (0x50000)
#define NVS_copy_block              (0x56000)  //Not used and should not be used, CCA
#else
#define NVS_block                   (0x1E000)
#define NVS_copy_block              (0x1F000)  //Not used and should not be used, CCA
#endif

/*
 *  ========================== NVS begin ====================================
 *  NOTE:  NVM
 */
NVSCC26XX_Object  NVSCC26XX_object[] =
{
   {.opened = 0},
#ifdef DeviceFamily_CC26X2
   {.opened = 0}
#endif  //DeviceFamily_CC26X2
};

#ifndef DeviceFamily_CC26X2
#if NVS_USE_RAM_COPY_BLOCK
uint32_t NVS_ram_copy_block[sizeof(struct structNvParams)/4];
#endif
#endif

const NVSCC26XX_HWAttrs NVSCC26XX_hwAttrs[] =
{
     {
#ifdef DeviceFamily_CC26X2
    .regionBase = (void *)NVS_block,
    .regionSize = 8192,
#else //DeviceFamily_CC26X2
   .block = (void *)NVS_block,
   .blockSize = NVS_BLOCK_SIZE,   //Must be less than or equal to 4096 (page size)
#if NVS_USE_RAM_COPY_BLOCK
   .copyBlock = (void *)NVS_ram_copy_block,
   .isRam = 1
#else  //NVS_USE_RAM_COPY_BLOCK
   .copyBlock = (void *)NVS_copy_block,
   .isRam = 0
#endif //NVS_USE_RAM_COPY_BLOCK
#endif  //DeviceFamily_CC26X2
     },
#ifdef DeviceFamily_CC26X2
     {
      .regionBase = (void *)NVS_block_2,
      .regionSize = 8192*2,
     }
#endif  //DeviceFamily_CC26X2
};

const NVS_Config NVS_config[] =
{
   {
      .fxnTablePtr = &NVSCC26XX_fxnTable,
        .object = &(NVSCC26XX_object[0]),
        .hwAttrs = &(NVSCC26XX_hwAttrs[0])
   },
   {
#ifdef DeviceFamily_CC26X2
    .fxnTablePtr = &NVSCC26XX_fxnTable,
        .object = &(NVSCC26XX_object[1]),
        .hwAttrs = &(NVSCC26XX_hwAttrs[1])
#else //DeviceFamily_CC26X2
    .fxnTablePtr = NULL,
    .object = NULL,
    .hwAttrs = NULL
#endif  //DeviceFamily_CC26X2
   }
};

uint8_t nvmLock;

/*******************************************************************************
* MACROS
*/
#define min(a,b)  ((a) < (b) ? (a) : (b))

NVS_Handle nvh;

/*******************************************************************************
* @fn          NVM_checkSum
*
* @brief       Calculate the checksum of the NVM parameter block
*
* input parameters
*
* @param       struct structNvParams *, pointer to the NVM parameter data 
* structure.
*
* output parameters
*
* @param       None.
*
* @return      The 32-bit checksum.
*/
static uint32_t NVM_checkSum(struct structNvParams *nvParams_p)
{
   uint32_t chkSum;
   int i;
   uint8_t *bptr = (uint8_t *)nvParams_p;
   
   chkSum = 0xFFFFFFFF - NV_PARAMS_VERSION;
   
   for (i = 0; i < (sizeof(*nvParams_p) - sizeof(uint32_t)); i++)
   {
      chkSum -= bptr[i];
   }
   
   return(chkSum);
}
/*******************************************************************************
* @fn          NVM_writeParams
*
* @brief       Write the NVM parameters to the flash
*
* input parameters
*
* @param       struct structNvParams *, pointer to the NVM parameter data 
* structure to be written to the flash.
*
* output parameters
*
* @param       None.
*
* @return      >= 0:  the number of bytes successfully written to the flash,
*              -1:    write to the flash failed.
*/

int NVM_writeParams(struct structNvParams *nvParams_p)
{
   int i;
   uint32_t nvword;
   uint32_t param;  
   int retVal;

   while (nvmLock)
   {
   }

   nvmLock = 1;

   retVal = sizeof(*nvParams_p);
   
   nvParams_p->checkSum = NVM_checkSum(nvParams_p);
   
   NVS_write(nvh, NV_OFFSET, (uint8_t *)nvParams_p, sizeof(*nvParams_p), NVS_WRITE_ERASE);
   for (i = 0; i < sizeof(*nvParams_p)/sizeof(uint32_t); i++)
   {
      NVS_read(nvh, NV_OFFSET+i*sizeof(nvword), &nvword, sizeof(nvword));
      param = *((uint32_t *)nvParams_p + i);
      if (nvword != param)
      {
        retVal = -1;
      }
      break;
   }
   nvmLock = 0;
   return(retVal);
}

/*******************************************************************************
* @fn          NVM_readParams
*
* @brief       Retrievethe NVM parameters from the flash
*
* input parameters
*
* @param       none
*
* output parameters
*
* @param       struct structNvParams *, pointer to the NVM parameter data 
* structure to be filled.
*
* @return      >= 0:  the number of bytes successfully retrievd from the flash,
*              -1:    retrieve from the flash failed.
*/

int NVM_readParams(struct structNvParams *nvParams_p)
{
   int retVal;
   uint32_t checkSum;
   struct structNvParams *nvs_params_p = (struct structNvParams *)(NVS_block + NV_OFFSET);
   
   while (nvmLock)
   {
   }

   nvmLock = 1;
   checkSum = NVM_checkSum(nvs_params_p);
   if (checkSum != nvs_params_p->checkSum)  //If the data in the flash is not valid, don't read
   {
      retVal = -1;
   }
   else
   {
      retVal = NVS_read(nvh, NV_OFFSET, (uint32_t *)nvParams_p, sizeof(*nvParams_p));    
      if (retVal != 0)
      {
         retVal = -1;
      }
      else
      {
         retVal = sizeof(*nvParams_p);
      }
   }
   nvmLock = 0;
   return(retVal);
}
/*******************************************************************************
* @fn          NVM_init
*
* @brief       Initialize the NVM module
*
* input parameters
*
* @param       None
*
* output parameters
*
* @param       None
*
* @return      None
*/
#if FEATURE_MAC_SECURITY
extern struct structMacSecurityPib macSecurityPib;
#endif

extern TSCH_MAC_CONFIG_t TSCH_MACConfig;

void NVM_sync()
{
   if (NVM_readParams(&nvParams) < 0)
   {
      nvParams.bcn_chan_0                     = TSCH_MACConfig.bcn_chan[0];
      nvParams.panid                          = TSCH_MACConfig.panID;
      nvParams.assoc_req_timeout_sec          = TSCH_MACConfig.assoc_req_timeout_sec;
      nvParams.slotframe_size                 = TSCH_MACConfig.slotframe_size;
      nvParams.keepAlive_period               = TSCH_MACConfig.keepAlive_period;
      nvParams.coap_resource_check_time       = coapConfig.coap_resource_check_time;
      nvParams.coap_port                      = coapConfig.coap_port;
      nvParams.coap_dtls_port                 = coapConfig.coap_dtls_port;
      nvParams.tx_power                       = TSCH_MACConfig.tx_power;
      nvParams.bcn_ch_mode                    = TSCH_MACConfig.bcn_ch_mode;
      nvParams.scan_interval                  = TSCH_MACConfig.scan_interval;
      nvParams.num_shared_slot                = TSCH_MACConfig.num_shared_slot;
      nvParams.fixed_channel_num              = TSCH_MACConfig.fixed_channel_num;
      nvParams.rpl_dio_doublings              = rplConfig.rpl_dio_doublings;
      nvParams.coap_obs_max_non               = coapConfig.coap_obs_max_non;
      nvParams.coap_default_response_timeout  = coapConfig.coap_default_response_timeout;
      nvParams.debug_level                    = TSCH_MACConfig.debug_level;
      nvParams.phy_mode                       = TSCH_MACConfig.phy_mode;
      nvParams.fixed_parent                   = TSCH_MACConfig.restrict_to_node;

#if FEATURE_MAC_SECURITY
      for (int i = 0; i < 16; i++)
      {
         nvParams.mac_psk[i] = macSecurityPib.macKeyTable[0].Key[i];
      }
#endif
      //commented out per JIRA KILBYIIOT-14:  NVM_writeParams(&nvParams);
   }
   else
   {
      TSCH_MACConfig.bcn_chan[0]             = nvParams.bcn_chan_0;
      TSCH_MACConfig.panID                   = nvParams.panid;
      TSCH_MACConfig.restrict_to_pan         = TSCH_MACConfig.panID;
      TSCH_MACConfig.assoc_req_timeout_sec   = nvParams.assoc_req_timeout_sec;
      TSCH_MACConfig.slotframe_size          = nvParams.slotframe_size;
      TSCH_MACConfig.keepAlive_period        = nvParams.keepAlive_period;
      coapConfig.coap_resource_check_time    = nvParams.coap_resource_check_time;
      coapConfig.coap_port                   = nvParams.coap_port;
      coapConfig.coap_dtls_port              = nvParams.coap_dtls_port;
      TSCH_MACConfig.tx_power                = nvParams.tx_power;
      TSCH_MACConfig.bcn_ch_mode             = nvParams.bcn_ch_mode;
      TSCH_MACConfig.scan_interval           = nvParams.scan_interval;
      TSCH_MACConfig.num_shared_slot         = nvParams.num_shared_slot;
      TSCH_MACConfig.fixed_channel_num       = nvParams.fixed_channel_num;
      rplConfig.rpl_dio_doublings            = nvParams.rpl_dio_doublings;
      coapConfig.coap_obs_max_non            = nvParams.coap_obs_max_non;
      coapConfig.coap_default_response_timeout = nvParams.coap_default_response_timeout;
      TSCH_MACConfig.debug_level             = nvParams.debug_level;
      TSCH_MACConfig.phy_mode                = nvParams.phy_mode;
      TSCH_MACConfig.restrict_to_node        = nvParams.fixed_parent;
#if FEATURE_MAC_SECURITY
      for (int i = 0; i < 16; i++)
      {
         macSecurityPib.macKeyTable[0].Key[i] = nvParams.mac_psk[i];
      }
#endif
   }
}

void NVM_init()
{
   nvmLock = 0;
   NVS_init();  
   nvh = NVS_open(0, NULL);
   NVM_sync();
}


void NVM_update()
{
   NVM_writeParams(&nvParams);

}

