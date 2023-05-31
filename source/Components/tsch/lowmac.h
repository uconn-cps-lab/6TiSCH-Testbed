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
 *  ====================== lowmac.h =============================================
 *  
 */

#ifndef __LOWMAC_H__
#define __LOWMAC_H__

#include "rtimer.h"
#include "ulpsmac.h"
#include "bm_api.h"

#define  LOWMAC_FALSE  ULPSMAC_FALSE
#define  LOWMAC_TRUE   ULPSMAC_TRUE

typedef void (* LOWMAC_txcb_t)(void *ptr, uint16_t status, uint8_t transmissions);
typedef void (* LOWMAC_dbcb_t)(void *pib_ptr, uint16_t status);

typedef enum
{
  /* generic status code */
  LOWMAC_STAT_SUCCESS                  = UPMAC_STAT_SUCCESS,

  LOWMAC_STAT_ERR                      = UPMAC_STAT_ERR,
  LOWMAC_STAT_ERR_FATAL                = UPMAC_STAT_ERR_FATAL,
  LOWMAC_STAT_ERR_BUSY                 = UPMAC_STAT_ERR_BUSY,

  LOWMAC_STAT_FRAME_TOO_LONG           = UPMAC_STAT_FRAME_TOO_LONG,
  //LOWMAC_STAT_INVALID_ADDRESS          = UPMAC_STAT_INVALID_ADDRESS,
  LOWMAC_STAT_TX_NOACK                 = UPMAC_STAT_NO_ACK,

  LOWMAC_STAT_INVALID_PARAMETER        = UPMAC_STAT_INVALID_PARAMETER,
  //LOWMAC_STAT_NO_SHORT_ADDRESS         = UPMAC_NO_SHORT_ADDRESS,

  LOWMAC_STAT_SLOTFRAME_NOT_FOUND      = UPMAC_STAT_SLOTFRAME_NOT_FOUND,
  LOWMAC_STAT_MAX_SLOTFRAMES_EXCEEDED  = UPMAC_STAT_MAX_SLOTFRAMES_EXCEEDED,

  LOWMAC_STAT_UNKNOWN_LINK             = UPMAC_STAT_UNKNOWN_LINK,
  LOWMAC_STAT_MAX_LINKS_EXCEEDED       = UPMAC_STAT_MAX_LINKS_EXCEEDED,

  LOWMAC_STAT_MAX_CHHOP_EXCEEDED       = UPMAC_STAT_MAX_CHHOP_EXCEEDED,

  LOWMAC_STAT_NO_SYNC                  = UPMAC_STAT_NO_SYNC,

  LOWMAC_STAT_DB_BAD_ATTRIB_ID         = UPMAC_STAT_PIB_BAD_ATTRIB_ID,
  LOWMAC_STAT_DB_BAD_VALUE             = UPMAC_STAT_PIB_BAD_VALUE,
  LOWMAC_STAT_DB_RDONLY                = UPMAC_STAT_PIB_RDONLY,
  //LOWMAC_STAT_PIB_INVALID_INDEX        = UPMAC_STAT_INVALID_INDEX,

  LOWMAC_STAT_NO_SLOTFRAME             = UPMAC_STAT_INVALID_GTS,
  LOWMAC_STAT_NO_LINK                  = UPMAC_STAT_INVALID_GTS,
  LOWMAC_STAT_NO_QUEUE                 = UPMAC_STAT_TRANSACTION_OVERFLOW,

  LOWMAC_STAT_TX_COLLISION             = UPMAC_STAT_CHANNEL_ACCESS_FAILURE,
  LOQMAC_STAT_RX_NO_DATA               = UPMAC_STAT_NO_DATA,
/*==================================*/

  /* Lower MAC status code */

  //LOWMAC_STAT_ADDRESS_NOT_FOUND        = 0xB1,

}LOWMAC_status_e;


/**
 * The structure of a LOWMAC driver for ulpsmac (in addition to Contiki NETSTACK).
 */
struct lowmac_driver {
  char *name;

  /** Initialize the ULPSMAC_LOWMAC driver */
  void (* init)(void);

  /** Send a packet from the ulpsmac buffer  */
  void (* send)(BM_BUF_t * usm_buf, LOWMAC_txcb_t sent_callback, void *ptr);

  /** Callback for getting notified of incoming packet. */
  void (* input)(BM_BUF_t* usm_buf);

  /** Turn the ULPSMAC_LOWMAC layer on. */
  int16_t (* on)(void);

  /** Turn the ULPSMAC_LOWMAC layer off. */
  int16_t (* off)(void);

  /** set pan coordinator */
  uint16_t (* set_pan_coord) (void);

  /** get_db */
  uint16_t (* get_db)(uint16_t attrib_id, void* in_value, uint16_t* in_size, LOWMAC_dbcb_t db_callback);

  /** set_db */
  uint16_t (* set_db)(uint16_t attrib_id, void* in_value, uint16_t* in_size, LOWMAC_dbcb_t db_callback);

  /** delete_db */
  uint16_t (*delete_db)(uint16_t attrib_id, void* in_val, uint16_t* in_size, LOWMAC_dbcb_t db_callback);

  /** modify_db */
  uint16_t (*modify_db)(uint16_t attrib_id, void* in_val, uint16_t* in_size, LOWMAC_dbcb_t db_callback);

};

#endif /* __LOWMAC_H__ */
