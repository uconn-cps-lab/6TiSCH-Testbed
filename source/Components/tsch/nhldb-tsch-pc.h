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
 *  ====================== nhldb-tsch-nonpc.h =============================================
 *  
 */
#ifndef __NHLDB_TSCH_PC_H__
#define __NHLDB_TSCH_PC_H__

#include "ulpsmacaddr.h"
#include "prmtv802154e.h"

#define NUM_DEFAULT_LINKS_IN_ASSOC       1   // Beacon
#define NUM_DEDICATED_LINKS_IN_ASSOC     2   // TX and RX
#define NUM_LINKS_IN_ASSOC               (NUM_DEFAULT_LINKS_IN_ASSOC + NUM_DEDICATED_LINKS_IN_ASSOC)

#define ASSIGN_LINK_EXIST 0
#define ASSIGN_NEW_LINK 1        
#define ASSIGN_NO_LINK 2

#define NHLDB_TSCH_PC_NUM_SLOTFRAMES     NHLDB_TSCH_SCHED_NUM_SLOTFRAMES
#define NHLDB_TSCH_PC_MAX_NUM_LINKS      128

/*---------------------------------------------------------------------------*/
extern void NHLDB_TSCH_PC_init();
extern uint16_t NHLDB_TSCH_PC_set_def_sflink(void);
extern uint8_t NHLDB_TSCH_PC_create_advie(uint8_t* buf, uint8_t bufLen);
extern void NHLDB_TSCH_PC_request_assocresp_ie(uint16_t parentShrtAddr, 
                                        uint16_t childShrtAddr,
                                        uint16_t nextHopShortAddr);
extern void NHLDB_TSCH_PC_parse_disassocie(uint8_t rm_my_link, 
                                           ulpsmacaddr_t* coordShrtAddr, 
                                           ulpsmacaddr_t* devExtAddr,
                                           uint8_t* ie_buf, uint8_t ie_buflen);
extern uint16_t NHLDB_TSCH_mofify_adv(void);
extern uint8_t NHLDB_TSCH_find_link_handle(uint8_t slotframeHandle, 
                                           tschMlmeSetLinkReq_t* setLinkArg);
extern void NHLDB_TSCH_delete_link_handle(uint8_t slotframeHandle, 
                                          uint16_t linkHandle);
extern uint8_t NHLDB_TSCH_assign_link_handle(uint8_t slotframeHandle, 
                                             tschMlmeSetLinkReq_t* setLinkArg);
/*---------------------------------------------------------------------------*/
#endif /* __NHLDB_TSCH_PC_H__ */

