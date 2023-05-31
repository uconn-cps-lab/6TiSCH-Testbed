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
 *  ====================== nhldb-tsch-cmm.c =============================================
 *  
 */
#include "lib/memb.h"
#include "nhldb-tsch-cmm.h"

/*---------------------------------------------------------------------------*/
#if IS_ROOT
#define NUM_COMMAND_MSGS  15
#else
#define NUM_COMMAND_MSGS  5
#endif


#if defined (__IAR_SYSTEMS_ICC__) || defined (__TI_COMPILER_VERSION__)
// IAR compiler has problem of 2 bytes alignment
// So, we needed to add data alignment pragma for memb
// So, we coped the following declarations from memb.h
static char command_msg_memb_memb_count[NUM_COMMAND_MSGS];
static NHLDB_TSCH_CMM_cmdmsg_t command_msg_memb_memb_mem[NUM_COMMAND_MSGS];
static struct memb command_msg_memb =
{sizeof(NHLDB_TSCH_CMM_cmdmsg_t), NUM_COMMAND_MSGS, command_msg_memb_memb_count, (void *) command_msg_memb_memb_mem};
#else /* IAR_SYSTEMS_ICC__ */
MEMB(command_msg_memb, NHLDB_TSCH_CMM_cmdmsg_t, NUM_COMMAND_MSGS);
#endif /* IAR_SYSTEMS_ICC__ */

/**************************************************************************************************//**
 *
 * @brief       initialize the TSCH command message memory
 *
 *
 * @param[in]
 *
 *
 * @param[out]
 *
 * @return
 *
 ***************************************************************************************************/
void
NHLDB_TSCH_CMM_init(void)
{
  // init memory pool for NHL commands
  memb_init(&command_msg_memb);
}

/**************************************************************************************************//**
 *
 * @brief       allocate the TSCH command message buffer
 *
 *
 * @param[in]
 *
 *
 * @param[out]
 *
 * @return      pointer to allocated command message  buffer
 *
 ***************************************************************************************************/
NHLDB_TSCH_CMM_cmdmsg_t* NHLDB_TSCH_CMM_alloc_cmdmsg(uint8_t owner)
{
  NHLDB_TSCH_CMM_cmdmsg_t* cmsg;

  uint32_t key = Hwi_disable();

  cmsg = memb_alloc(&command_msg_memb);
  Hwi_restore(key);

  if(!cmsg) {
    ULPSMAC_Dbg.numCmdMsgAllocError++;
    return NULL;
  }
  cmsg->owner = owner;
  
  return cmsg;
}

/**************************************************************************************************//**
 *
 * @brief       free allocated command message memory
 *
 *
 * @param[in]   cmsg -- pointer of previous allocated command message memory
 *
 *
 * @param[out]
 *
 * @return
 *
 ***************************************************************************************************/
int8_t NHLDB_TSCH_CMM_free_cmdmsg(NHLDB_TSCH_CMM_cmdmsg_t* cmsg)
{
  int8_t res;

  res = memb_free(&command_msg_memb, cmsg);

  return res;
}

