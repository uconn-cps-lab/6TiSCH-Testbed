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
 *  ====================== mac_pib_pvt.h =============================================
 *  
 */

#ifndef MAC_PIB_PVT_H
#define MAC_PIB_PVT_H

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


/* ------------------------------------------------------------------------------------------------
 *                                          Defines
 * ------------------------------------------------------------------------------------------------
 */

// Robert copy from mac_api.h
/* Special address values */
#define MAC_ADDR_USE_EXT                    0xFFFE  /* Short address value indicating extended address is used */
#define MAC_SHORT_ADDR_BROADCAST            0xFFFF  /* Broadcast short address */
#define MAC_SHORT_ADDR_NONE                 0xFFFF  /* Short address when there is no short address */


/**
 * FCFG Base.
 *  for chip's Extended address
 */
#define EXTADDR_OFFSET                      0x2F0     // in FCFG; LSB..MSB

/* ------------------------------------------------------------------------------------------------
 *                                           Constants
 * ------------------------------------------------------------------------------------------------
 */

#define TSCH_DEVICE_EUI_LEN                                           (8)


/* ------------------------------------------------------------------------------------------------
 *                                           Typedefs
 * ------------------------------------------------------------------------------------------------
 */

/* PIB access and min/max table type */
typedef struct
{
  uint16_t      offset;
  uint8_t       len;
  uint8_t       min;
  uint8_t       max;

} MAC_PIB_TBL_t;

/* TSCH MAC PIB type
  MAC generic PIB is define 2006

  TSCH specific in 2012 Table 52b-f
  TSCH MAC PIB is defined in Table 52b
  TSCH MAC PIB is defined in Table 52c

*/

typedef struct _tsch_mac_pib_
{
  uint8_t                           bsn;
  uint8_t                           dsn;
  uint8_t                           maxFrameRetries;
  uint8_t                           minBe;
  uint8_t                           maxBe;

  uint16_t                          shortAddress;
  uint16_t                          panId;

  /* MAC coordinator short Address */
  uint16_t                          macCoordinatorShortAddress;
  bool                              securityEnabled;
  bool                              PanCoordinator;

  /* coordinator extended address */
  uint8_t                           coordinatorEUI[TSCH_DEVICE_EUI_LEN];

  /* device EUI */
  uint8_t                           devEUI[TSCH_DEVICE_EUI_LEN];
  /*

    All TSCH Specific PIBs are implemented in the DB or link manager
    at this moment, we still keep these PIBs in DB
    later, we might migrate all these PIBs to this module
  */

  /*
    security related PIBs are defined in mac_secuirity_pib.c
  */

} TSCH_MAC_PIB_t;


/* ------------------------------------------------------------------------------------------------
 *                                           Global Variables
 * ------------------------------------------------------------------------------------------------
 */
extern TSCH_MAC_PIB_t TSCH_MAC_Pib;


/* ------------------------------------------------------------------------------------------------
 *                                           Function Prototypes
 * ------------------------------------------------------------------------------------------------
 */

void TSCH_PIB_init(void);
uint16_t TSCH_PIB_getIndex(uint16_t pibAttribute);
uint16_t TSCH_MlmeGetReqSize( uint16_t pibAttribute );



/**************************************************************************************************
*/

#endif /* MAC_PIB_PVT_H */

