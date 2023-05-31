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
 *  ====================== nhl.h =============================================
 *  
 */

#ifndef __NHL_H__
#define __NHL_H__

#include "ulpsmac.h"
#include "stat802154e.h"
#include "ulpsmacaddr.h"

#define NHL_TRUE    ULPSMAC_TRUE
#define NHL_FALSE   ULPSMAC_FALSE

#define NHL_CTRL_TYPE_SHORTADDR  1

#define TASK_EVENT_MSG 			0
#define TASK_EVENT_HCT                  1

typedef enum
{
  /* generic status code */
  NHL_STAT_SUCCESS                  = STAT802154E_SUCCESS,

  /* Lower MAC status code */
  NHL_STAT_ERR                      = STAT802154E_ERR,
  NHL_STAT_ERR_FATAL                = STAT802154E_ERR_FATAL,
  NHL_STAT_ERR_BUSY                 = STAT802154E_ERR_BUSY,

} NHL_status_e;

/** Callback for getting notified of incoming packet. */
typedef void (*nhl_recv_callback_t) (void);
typedef void (*nhl_sent_callback_t) (uint8_t status, uint8_t num_tx);
#if WITH_UIP6
typedef void (*nhl_ctrl_callback_t) (uint16_t ctrl_type, void* val_ptr);
#endif

struct nhl_driver {
  char *name;

  /** Initialize the ULPSMAC_NHL driver by main maybe don't need*/
  void (* init) (void);

#if WITH_UIP6
  void (* setup_node) (nhl_ctrl_callback_t ctrl_cb);
#else
  /* set node as pan coordinator or not by app */
  void (* setup_node) (nhl_recv_callback_t recv_cb, nhl_sent_callback_t sent_cb);
#endif

  void (* send) (uint8_t dest_addr_mode, ulpsmacaddr_t* dest_addr);

  uint8_t (* has_ded_link)(ulpsmacaddr_t *addr);
};


#endif /*__NHL_H__*/
