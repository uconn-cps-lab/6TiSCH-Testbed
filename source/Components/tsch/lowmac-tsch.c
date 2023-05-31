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
*  ====================== lowmac-tsch.c =============================================
*  
*/
#include "ulpsmac.h"
#include "ulpsmacbuf.h"

#include "lowmac-tsch.h"
#include "tsch-db.h"
#include "tsch-acm.h"
#include "tsch-txm.h"
#include "tsch-rxm.h"
#include <ti/sysbios/family/arm/m3/Hwi.h>

/*---------------------------------------------------------------------------*/
static
void init(void) 
{
  LOWMAC_status_e res;
  TSCH_DB_handle_t* db_hnd_ptr;
  
  db_hnd_ptr = TSCH_DB_init();
  if (!db_hnd_ptr) 
  {
    return;
  }
  res = TSCH_ACM_init(db_hnd_ptr);
  if (res != LOWMAC_STAT_SUCCESS) 
  {
    return;
  }
  
  res = TSCH_TXM_init(db_hnd_ptr);
  if (res != LOWMAC_STAT_SUCCESS) 
  {
    return;
  }
  
  res = TSCH_RXM_init(db_hnd_ptr);
  if (res != LOWMAC_STAT_SUCCESS) 
  {
    return;
  }
}

/*---------------------------------------------------------------------------*/
static int16_t start(void)
{
  return ((int16_t) TSCH_ACM_start());
}

/*---------------------------------------------------------------------------*/
static int16_t stop(void) 
{
  return ((int16_t) TSCH_ACM_stop());
}

/*---------------------------------------------------------------------------*/
static void send_ulpsmac(BM_BUF_t *usm_buf, LOWMAC_txcb_t sent, void *ptr) 
{
  TSCH_TXM_que_pkt_thread_safe(usm_buf, sent, ptr);
//  uint32_t key;
//  key = Hwi_disable();
//  TSCH_TXM_que_pkt(usm_buf, sent, ptr);
//  Hwi_restore(key);
}

/*---------------------------------------------------------------------------*/
static void input_ulpsmac(BM_BUF_t* usm_buf) 
{
  TSCH_RXM_recv_pkt(usm_buf);
}

/*---------------------------------------------------------------------------*/
static uint16_t set_pan_coord(void) {
  
  return ((uint16_t) TSCH_DB_set_pan_coord());
}

/*---------------------------------------------------------------------------*/
static uint16_t get_db(uint16_t attrib_id, void* in_value, uint16_t* in_size,
                       LOWMAC_dbcb_t db_callback) 
{
  return ((uint16_t) TSCH_DB_get_db(attrib_id, in_value, in_size));
}

/*---------------------------------------------------------------------------*/
static uint16_t set_db(uint16_t attrib_id, void* in_val, uint16_t* in_size,
                       LOWMAC_dbcb_t db_callback) 
{
  
  return ((uint16_t) TSCH_DB_set_db(attrib_id, in_val, in_size));
}

/*---------------------------------------------------------------------------*/
static uint16_t delete_db(uint16_t attrib_id, void* in_val, uint16_t* in_size,
                          LOWMAC_dbcb_t db_callback) 
{
  return ((uint16_t) TSCH_DB_delete_db(attrib_id, in_val, in_size));
}

/*---------------------------------------------------------------------------*/
static uint16_t modify_db(uint16_t attrib_id, void* in_val, uint16_t* in_size,
                          LOWMAC_dbcb_t db_callback) 
{
  return ((uint16_t) TSCH_DB_modify_db(attrib_id, in_val, in_size, db_callback));
}

/*---------------------------------------------------------------------------*/
const struct lowmac_driver lowmac_tsch_driver = 
{ 
  "lowmac-tsch", 
  init,
  send_ulpsmac, 
  input_ulpsmac, 
  start, stop, 
  set_pan_coord, 
  get_db, 
  set_db,
  delete_db, 
  modify_db,
};
