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
* include this software, other than combinations with devices manufactured by or for TI (“TI Devices”). 
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
* THIS SOFTWARE IS PROVIDED BY TI AND TI’S LICENSORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
* BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL TI AND TI’S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
/*
 *  ====================== mac_pib.h =============================================
 *  
 */

#ifndef MAC_PIB_H
#define MAC_PIB_H

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */
 
#if defined (__IAR_SYSTEMS_ICC__) || defined(__TI_COMPILER_VERSION__)
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <stdio.h>
#include <string.h>
#endif 

#include "tsch_api.h"

/* ------------------------------------------------------------------------------------------------
 *                                          Defines
 * ------------------------------------------------------------------------------------------------
 */

// Robert copy from mac_api.h
/* Special address values */
#define MAC_ADDR_USE_EXT                    0xFFFE  /* Short address value indicating extended address is used */
#define MAC_SHORT_ADDR_BROADCAST            0xFFFF  /* Broadcast short address */
#define MAC_SHORT_ADDR_NONE                 0xFFFF  /* Short address when there is no short address */


/*
  802.15.4-2006 Spec MAC-PIB attributes in table 86
*/

#define TSCH_MAC_PIB_ID_macBSN                                        (0x49)
#define TSCH_MAC_PIB_ID_PAN_COORDINATOR_EUI                           (0x4a)
#define TSCH_MAC_PIB_ID_macPANCoordinatorShortAddress                 (0x4b)
#define TSCH_MAC_PIB_ID_macDSN                                        (0x4C)
#define TSCH_MAC_PIB_ID_rootTschSchedule                              (0x4D)
#define TSCH_MAC_PIB_ID_minBE                                         (0x4F)
#define TSCH_MAC_PIB_ID_macPANId                                      (0x50)
#define TSCH_MAC_PIB_ID_ASSOC_REQ_TIMEOUT_SEC                   (0x51)
#define TSCH_MAC_PIB_ID_SLOTFRAME_SIZE                          (0x52)
#define TSCH_MAC_PIB_ID_macShortAddress                               (0x53)
#define TSCH_MAC_PIB_ID_BCN_CHAN                                      (0x54)  //fengmo NVM
#define TSCH_MAC_PIB_ID_MAC_PSK_07                                    (0x55)  //fengmo NVM
#define TSCH_MAC_PIB_ID_MAC_PSK_8F                                    (0x56)  //fengmo NVM
#define TSCH_MAC_PIB_ID_maxBE                                         (0x57)
#define TSCH_MAC_PIB_ID_NVM                                           (0x58)  //fengmo NVM
#define TSCH_MAC_PIB_ID_macMaxFrameRetries                            (0x59)
#define TSCH_MAC_PIB_ID_KEEPALIVE_PERIOD                        (0x5A)
#define TSCH_MAC_PIB_ID_COAP_RESOURCE_CHECK_TIME                (0x5B)
#define TSCH_MAC_PIB_ID_COAP_PORT                               (0x5C)
#define TSCH_MAC_PIB_ID_macSecurityEnabled                            (0x5D)
#define MAC_SECURITY_ENABLED                                          (0x5E)
#define MAC_FRAME_COUNTER_MODE                                        (0x5F)
#define TSCH_MAC_PIB_ID_COAP_DTLS_PORT                          (0x60)
#define TSCH_MAC_PIB_ID_TX_POWER                                (0x61)
#define TSCH_MAC_PIB_ID_BCN_CH_MODE                             (0x62)
#define TSCH_MAC_PIB_ID_SCAN_INTERVAL                           (0x63)
#define TSCH_MAC_PIB_ID_NUM_SHARED_SLOT                         (0x64)
#define TSCH_MAC_PIB_ID_FIXED_CHANNEL_NUM                       (0x65)
#define TSCH_MAC_PIB_ID_RPL_DIO_DOUBLINGS                       (0x66)
#define TSCH_MAC_PIB_ID_COAP_OBS_MAX_NON                        (0x67)
#define TSCH_MAC_PIB_ID_COAP_DEFAULT_RESPONSE_TIMEOUT           (0x68)
#define TSCH_MAC_PIB_ID_DEBUG_LEVEL                             (0x69)
#define TSCH_MAC_PIB_ID_PHY_MODE                                (0x6A)
#define TSCH_MAC_PIB_ID_FIXED_PARENT                            (0x6B)
#define TSCH_MAC_PIB_ID_MAC_PAIR_KEY_ADDR                       (0x6C)
#define TSCH_MAC_PIB_ID_MAC_PAIR_KEY_07                         (0x6D)
#define TSCH_MAC_PIB_ID_MAC_PAIR_KEY_8F                         (0x6E)
#define TSCH_MAC_PIB_ID_MAC_DELETE_PAIR_KEY                     (0x6F)
#define TSCH_MAC_PIB_ID_PANCoordinator                                (0xF0)
#define TSCH_MAC_PIB_ID_DEV_EUI                                       (0xF1)




/* ------------------------------------------------------------------------------------------------
 *                                           Constants
 * ------------------------------------------------------------------------------------------------
 */



/* ------------------------------------------------------------------------------------------------
 *                                           Typedefs
 * ------------------------------------------------------------------------------------------------
 */


/* TSCH MAC PIB type
  MAC genral PIB is define 2006

  TSCH specific in 2012 Table 52b-f
  TSCH MAC PIB is defined in Table 52b 
  TSCH MAC PIB is defined in Table 52c

*/


/* TSCH MAC PIB function call status */
typedef enum
{
  TSCH_MAC_PIB_STATUS_SUCCESS                                       =0,
  TSCH_MAC_PIB_STATUS_UNSUPPORTED_ATTRIBUTE                         =1,
  TSCH_MAC_PIB_STATUS_READ_ONLY                                     =2,
  TSCH_MAC_PIB_STATUS_INVALID_PARAMETER                             =3,
  TSCH_MAC_PIB_STATUS_CHANGE_NOT_ALLOWED                            =4,
  TSCH_MAC_PIB_STATUS_MAX,
  TSCH_MAC_PIB_STATUS_INVALID_INDEX,
} TschMacPibStatus_t;

/* ------------------------------------------------------------------------------------------------
 *                                           Global Variables
 * ------------------------------------------------------------------------------------------------
 */ 
#define pMacPib (&TSCH_MAC_Pib)

/* ------------------------------------------------------------------------------------------------
 *                                           Function Prototypes
 * ------------------------------------------------------------------------------------------------
 */

uint16_t TSCH_PIB_getIndex(uint16_t pibAttribute);
uint16_t TSCH_MlmeGetReqSize(uint16_t pibAttribute );

uint16_t TSCH_MlmeGetReq(uint16_t pibAttribute, void *pValue);
uint16_t TSCH_MlmeSetReq(uint16_t pibAttribute, void *pValue);


/**************************************************************************************************
*/

#endif /* TSCH_MAC_PIB_H */


