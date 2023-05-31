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
 *  ====================== nhldb-tsch-sched.h =============================================
 *  
 */

#ifndef __NHLDB_TSCH_SCHED_H__
#define __NHLDB_TSCH_SCHED_H__

#define ENABLE_BITMAP_IMPLEMENTATION

/*******************************************************************************
 * INCLUDES
 */

#include "ulpsmacaddr.h"
#include "nhldb-tsch-pc.h"

/*******************************************************************************
 * MACROS
 */
#define NHLDB_TSCH_SCHED_NUM_SLOTFRAMES     (1)
#define NHLDB_TSCH_SCHED_SLOTFRAME_SIZE_0   (256)

#define NHLDB_TSCH_SCHED_ADV_LINK       (0)
#define NHLDB_TSCH_SCHED_SH_LINK        (1)
#define NHLDB_TSCH_SCHED_DED_UP_LINK    (2)
#define NHLDB_TSCH_SCHED_DED_DOWN_LINK  (3)

/*******************************************************************************
 * APIs
 */
extern void NHLDB_TSCH_SCHED_init(uint16_t* sf_size_out);
extern uint8_t NHLDB_TSCH_SCHED_find_schedule(uint8_t sf_idx, uint8_t linktype,
 uint16_t *timeslot_out,uint16_t *channel_out,uint8_t *period_offset_out);
extern void NHLDB_TSCH_SCHED_del_schedule_link(uint16_t timeslot);
extern void NHLDB_TSCH_SCHED_del_sh_link(void);
extern void
NHLDB_TSCH_SCHED_update_periodOffset(uint8_t linktype, uint8_t newPeriodOffsetLimit);
#endif /* __NHLDB_TSCH_SCHED_H__ */

