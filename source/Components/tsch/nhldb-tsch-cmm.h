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
 *  ====================== nhldb-tsch-cmm.h =============================================
 *  
 */

#ifndef __NHLDB_TSCH_CMM_H__
#define __NHLDB_TSCH_CMM_H__

#include "prmtv802154e.h"
#include "ulpsmacaddr.h"
#include "ulpsmacbuf.h"

/*---------------------------------------------------------------------------*/
#define NHLDB_TSCH_CMM_SPMEM_SIZE   128
struct __NHLDB_TSCH_CMM_spmem {
  uint8_t mem[NHLDB_TSCH_CMM_SPMEM_SIZE];
};
typedef struct __NHLDB_TSCH_CMM_spmem NHLDB_TSCH_CMM_spmem_t;

/*---------------------------------------------------------------------------*/
#define NHLDB_TSCH_CMM_CMDMSG_ARG_SIZE  64
typedef struct __NHLDB_TSCH_CMM_cmdmsg {
  uint8_t type;
  uint8_t len;
  uint8_t arg[NHLDB_TSCH_CMM_CMDMSG_ARG_SIZE];
  uint8_t buf[ULPSMACBUF_DATA_SIZE];
  uint8_t event_type;
  uint8_t owner;
  uint16_t recv_asn; // Jiachen
} NHLDB_TSCH_CMM_cmdmsg_t;

/*---------------------------------------------------------------------------*/
extern void NHLDB_TSCH_CMM_init(void);
/*---------------------------------------------------------------------------*/
extern NHLDB_TSCH_CMM_cmdmsg_t* NHLDB_TSCH_CMM_alloc_cmdmsg(uint8_t);
/*---------------------------------------------------------------------------*/
extern int8_t NHLDB_TSCH_CMM_free_cmdmsg(NHLDB_TSCH_CMM_cmdmsg_t* cmsg);
/*---------------------------------------------------------------------------*/
#endif /* __NHLDB_TSCH_CMM_H__ */

