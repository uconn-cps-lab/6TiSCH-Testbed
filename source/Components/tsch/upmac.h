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
 *  ====================== upmac.h =============================================
 *  
 */


#ifndef __UPMAC_H__
#define __UPMAC_H__

#include "ulpsmac.h"
#include "stat802154e.h"

#include "bm_api.h"

#define UPMAC_FALSE   ULPSMAC_FALSE
#define UPMAC_TRUE    ULPSMAC_TRUE

typedef enum
{
  /* generic status code */
  UPMAC_STAT_SUCCESS                  = STAT802154E_SUCCESS,

  UPMAC_STAT_ERR                      = STAT802154E_ERR,
  UPMAC_STAT_ERR_FATAL                = STAT802154E_ERR_FATAL,
  UPMAC_STAT_ERR_BUSY                 = STAT802154E_ERR_BUSY,

  UPMAC_STAT_FRAME_TOO_LONG           = STAT802154E_FRAME_TOO_LONG,
  UPMAC_STAT_INVALID_ADDRESS          = STAT802154E_INVALID_ADDRESS,
  UPMAC_STAT_NO_ACK                   = STAT802154E_NO_ACK,

  UPMAC_STAT_NO_DATA                  = STAT802154E_NO_DATA,

  UPMAC_STAT_INVALID_PARAMETER        = STAT802154E_INVALID_PARAMETER,
  UPMAC_STAT_NO_SHORT_ADDRESS         = STAT802154E_NO_SHORT_ADDRESS,

  UPMAC_STAT_SCAN_IN_PROGRESS         = STAT802154E_SCAN_IN_PROGRESS,

  UPMAC_STAT_SLOTFRAME_NOT_FOUND      = STAT802154E_SLOTFRAME_NOT_FOUND,
  UPMAC_STAT_MAX_SLOTFRAMES_EXCEEDED  = STAT802154E_MAX_SLOTFRAMES_EXCEEDED,

  UPMAC_STAT_UNKNOWN_LINK             = STAT802154E_UNKNOWN_LINK,
  UPMAC_STAT_MAX_LINKS_EXCEEDED       = STAT802154E_MAX_LINKS_EXCEEDED,

  UPMAC_STAT_NO_SYNC                  = STAT802154E_NO_SYNC,

  UPMAC_STAT_CHANNEL_ACCESS_FAILURE   = STAT802154E_CHANNEL_ACCESS_FAILURE,

  UPMAC_STAT_PIB_BAD_ATTRIB_ID        = STAT802154E_UNSUPPORTED_ATTRIBUTE,
  UPMAC_STAT_PIB_BAD_VALUE            = STAT802154E_INVALID_PARAMETER,
  UPMAC_STAT_PIB_RDONLY               = STAT802154E_READ_ONLY,
  UPMAC_STAT_PIB_INVALID_INDEX        = STAT802154E_INVALID_INDEX,

  UPMAC_STAT_MAX_CHHOP_EXCEEDED       = STAT802154E_INVALID_INDEX,

  UPMAC_STAT_INVALID_GTS              = STAT802154E_INVALID_GTS,
  UPMAC_STAT_TRANSACTION_OVERFLOW     = STAT802154E_TRANSACTION_OVERFLOW,
/*==================================*/

} UPMAC_status_e;


/**
 * The structure of a LOWMAC driver for ulpsmac (in addition to Contiki NETSTACK).
 */
struct upmac_driver {
  char *name;
  /** Initialize the ULPSMAC_UPMAC driver */
  void (* init) (void);
  /** Callback for getting notified of incoming packet. */
  void (* input)(BM_BUF_t * usm_buf);
};

#endif /*__UPMAC_H__*/
