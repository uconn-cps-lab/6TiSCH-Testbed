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
 *  ====================== pib-802154e.h =============================================
 *  
 */

#ifndef __PIB_802154E_H__
#define __PIB_802154E_H__

#include "tsch-pib.h"

#define PIB_802154E_ASN_LEN       TSCH_PIB_ASN_LEN

#define PIB_802154E_DSN           0x4c
#define PIB_802154E_RESP_WAITTIME 0x5a
#define PIB_802154E_EB_IE_LIST    0x9a  // macEBIEList 

#define PIB_802154E_ASN           TSCH_PIB_ASN
#define PIB_802154E_JOIN_PRIO     TSCH_PIB_JOIN_PRIO
#define PIB_802154E_MIN_BE        TSCH_PIB_MIN_BE
#define PIB_802154E_MAX_BE        TSCH_PIB_MAX_BE
#define PIB_802154E_DISCON_TIME   TSCH_PIB_DISCON_TIME
#define PIB_802154E_FRAME_RETRIES TSCH_PIB_MAX_FRAME_RETRIES

#define PIB_802154E_CHAN_HOP      TSCH_PIB_CHAN_HOP
#define PIB_802154E_TIMESLOT      TSCH_PIB_TIMESLOT
#define PIB_802154E_SLOTFRAME     TSCH_PIB_SLOTFRAME
#define PIB_802154E_LINK          TSCH_PIB_LINK 

typedef struct __TSCH_PIB_channel_hopping   PIB_802154E_channel_hopping_t;
typedef struct __TSCH_PIB_timeslot_template PIB_802154E_timeslot_template_t;
typedef struct __TSCH_PIB_slotframe         PIB_802154E_slotframe_t;
typedef struct __TSCH_PIB_link              PIB_802154E_link_t; 

#define PIB_802154E_set_asn       TSCH_PIB_set_asn
#define PIB_802154E_get_asn       TSCH_PIB_get_asn

#endif /*__PIB_802154E_H__*/
