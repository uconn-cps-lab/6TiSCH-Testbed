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
 *  ====================== nhldb-tsch-sched.c =============================================
 *  
 */

/*******************************************************************************
* INCLUDES
*/
#include "lib/memb.h"
#include "nhldb-tsch-sched.h"
#include "mac_config.h"

/*******************************************************************************
* MACROS
*/
#define RF_CHANNLE_OFFSET               (11)
//Tao: It's a minor bug here. Channel offset ranges [0,15]. And this 
//macro is only used when assigning shared slots. Which means shared slots are
//all allocated at offset 11 instead of 0
#define MAX_SCHED_PERIOD                (32)

/*
Bit 0: Indicates if current timeslot is assigned
Bit 1-3: Link type of assigned timeslot
Bit 4-11: Period offset of assigned timeslot
Bit 12-15: Unused
*/
static uint16_t sched_ts_bitmap[NHLDB_TSCH_SCHED_SLOTFRAME_SIZE_0];
static uint8_t sched_period_offset_limit[4];

/*******************************************************************************
* GLOBAL VARIABLES
*/


/**************************************************************************************************//**
*
* @brief       This function initializes the scheduler related data structures.
*
* @param[in]   pSlotframeSize --pointer to the scheduler’s slotframe size array
*
* @return
***************************************************************************************************/
void NHLDB_TSCH_SCHED_init(uint16_t* pSlotframeSize)
{
    uint16_t sf_iter;

    memset(sched_ts_bitmap,0x0,sizeof(sched_ts_bitmap));

#if TSCH_PER_SCHED
    /* Beacon Link*/
    sched_period_offset_limit[NHLDB_TSCH_SCHED_ADV_LINK]= TSCH_MACConfig.beacon_period_sf;
    /* Share Link*/
    sched_period_offset_limit[NHLDB_TSCH_SCHED_SH_LINK]= 1;
    /* Dedicated Link*/
    sched_period_offset_limit[NHLDB_TSCH_SCHED_DED_UP_LINK]= TSCH_MACConfig.uplink_data_period_sf;
    sched_period_offset_limit[NHLDB_TSCH_SCHED_DED_DOWN_LINK]= TSCH_MACConfig.downlink_data_period_sf;
#else
     memset(sched_period_offset_limit,0x0,sizeof(sched_period_offset_limit));
#endif

    // time slot : 0xFFFF mark it is not used
    for(sf_iter=0; sf_iter<NHLDB_TSCH_SCHED_NUM_SLOTFRAMES; sf_iter++)
    {
        pSlotframeSize[sf_iter] = TSCH_MACConfig.slotframe_size;
    }
}

/**************************************************************************************************//**
*
* @brief       This function assigns timeslot and channel offset in link argument.The corresponding
*              bit in the bitmap is set. It indicates this link is assigned.
*
* @param[in]    sf_idx -- slotframe index
*               linktype -- beacon, share, dedicated link
*
* @param[out]  timeslot_out -- pointer to assigned timeslot
*              channel_out  -- pointer to assigned channel offset
*              period_offset_out  -- pointer to assigned period offset
*
* @return
***************************************************************************************************/
uint8_t NHLDB_TSCH_SCHED_find_schedule(uint8_t sf_idx, uint8_t linktype,
                                       uint16_t *timeslot_out,uint16_t *channel_out,uint8_t *period_offset_out)
{
    uint16_t i;
    uint8_t timeslot_assigned;
    uint8_t timeslot_linktype;
    uint8_t timeslot_period_offset;

    for (i=0;i<TSCH_MACConfig.slotframe_size;i++)
    {
        timeslot_assigned = sched_ts_bitmap[i]& (0x1);
        timeslot_linktype = (sched_ts_bitmap[i]>>1)&(0x3);
        timeslot_period_offset = (sched_ts_bitmap[i]>>3)&(0xFF);

        if ((timeslot_assigned == NHL_FALSE) ||
            ((timeslot_linktype == linktype) &&
             (timeslot_period_offset < sched_period_offset_limit[linktype]))
           )
        {
            *timeslot_out = i;
            *period_offset_out = timeslot_period_offset++;
            *channel_out = RF_CHANNLE_OFFSET;
            sched_ts_bitmap[i] = (NHL_TRUE + (linktype<<1) +
                                  (timeslot_period_offset<<3));

            return NHL_STAT_SUCCESS;
        }
    }

    return NHL_STAT_ERR;
}

/**************************************************************************************************//**
*
* @brief       This function deletes the assigned link from the bitmap
*
*
* @param[in]   timeslot: assigned timeslot
*
* @return
***************************************************************************************************/
void NHLDB_TSCH_SCHED_del_schedule_link(uint16_t timeslot)
{
    uint8_t timeslot_assigned;
    uint8_t timeslot_linktype;
    uint8_t timeslot_period_offset;


    timeslot_assigned = sched_ts_bitmap[timeslot]& (0x1);
    timeslot_linktype = (sched_ts_bitmap[timeslot]>>1)&(0x3);
    timeslot_period_offset = (sched_ts_bitmap[timeslot]>>3)&(0xFF);

    if ((timeslot_assigned == NHL_TRUE) ||
        (timeslot_period_offset > 0))
    {
        timeslot_period_offset--;

        if (timeslot_period_offset==0)
        {
            timeslot_assigned = NHL_FALSE;
            timeslot_linktype =0;
        }

        sched_ts_bitmap[timeslot] = (timeslot_assigned + (timeslot_linktype<<1) +
                              (timeslot_period_offset<<3));
    }
}

/**************************************************************************************************//**
*
* @brief       This function deletes the assigned share link from the bitmap
*
*
* @param[in]
*
* @return
***************************************************************************************************/
void NHLDB_TSCH_SCHED_del_sh_link(void)
{
    uint8_t timeslot_assigned;
    uint8_t timeslot_linktype;
    uint16_t i;
    uint16_t count;

    count=0;

    for (i=0;i<TSCH_MACConfig.slotframe_size;i++)
    {
         timeslot_assigned = sched_ts_bitmap[i]& (0x1);
         timeslot_linktype = (sched_ts_bitmap[i]>>1)&(0x3);

          if ((timeslot_assigned == NHL_TRUE) &&
              (timeslot_linktype == NHLDB_TSCH_SCHED_SH_LINK))
          {
            if (count >0)
            {
            NHLDB_TSCH_SCHED_del_schedule_link(i);
            }

            count++;
          }
    }
}

/**************************************************************************************************//**
*
* @brief       This function updates the period offset limit of the specified link type
*
*
* @param[in]  linktype: link type
* @param[in]  newPeriodOffsetLimit: new limit
*
* @return
***************************************************************************************************/
void NHLDB_TSCH_SCHED_update_periodOffset(uint8_t linktype, uint8_t newPeriodOffsetLimit)
{
    sched_period_offset_limit[linktype]=newPeriodOffsetLimit;
}

