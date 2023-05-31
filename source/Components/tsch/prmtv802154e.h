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
 *  ====================== prmtv802154e.h =============================================
 *  
 */

#ifndef __PRMTV802154E_H__
#define __PRMTV802154E_H__

#include "ulpsmac.h"
#include "ulpsmacbuf.h"
#include "bm_api.h"

#include "stat802154e.h"
#include "ie802154e.h"
#include "frame802154e.h"

#include "tsch_api.h"

/*---------------------------------------------------------------------------*/
#define MAX_EB_PAYLOAD_SIZE         80
#define MAX_BCN_RECV_ALLOW_IN_SCAN  4

#define SET_SLOTFRAME_ADD     0
#define SET_SLOTFRAME_DELETE  2
#define SET_SLOTFRAME_MODIFY  3

#define SET_LINK_ADD          0
#define SET_LINK_DELETE       2
#define SET_LINK_MODIFY       3

#define SCAN_TYPE_PASSIVE     0
#define SCAN_TYPE_ACTIVE      1

#define TSCH_MODE_OFF         0
#define TSCH_MODE_ON          1

#define BEACON_TYPE_NORMAL     0x00
#define BEACON_TYPE_ENHANCED   0x01

#define ASSOC_STATUS_SUCCESS         0x00
#define ASSOC_STATUS_PAN_AT_CAPA     0x01
#define ASSOC_STATUS_PAN_ACC_DENIED  0x02
#define ASSOC_STATUS_PAN_ALT_PARENT  0x03

#define DISASSOC_STATUS_ACK          0x00
#define DISASSOC_STATUS_NACK         0x01

#define BEACONINFO_STATUS_SUCCESS    0x00
#define BEACONINFO_STATUS_FAIL       0x01

/* Reserved bit 5 in Capacity Information Field. See section 5.3.1.2 - 2012*/
#define ASSOC_CAPINFO_ASSIGN_DED_LINK_SHIFT 5
#define ASSOC_CAPINFO_ASSIGN_DED_LINK_MASK  (0x01 << ASSOC_CAPINFO_ASSIGN_DED_LINK_SHIFT)
#define ASSOC_CAPINFO_ASSIGN_DED_LINK_TRUE  (0x01 << ASSOC_CAPINFO_ASSIGN_DED_LINK_SHIFT)
#define ASSOC_CAPINFO_ASSIGN_DED_LINK_FALSE (0x00 << ASSOC_CAPINFO_ASSIGN_DED_LINK_SHIFT)

#define ASSOC_CAPINFO_DEV_TYPE_SHIFT    1
#define ASSOC_CAPINFO_DEV_TYPE_MASK     (0x01 << ASSOC_CAPINFO_DEV_TYPE_SHIFT)
#define ASSOC_CAPINFO_DEV_TYPE_FFD      (0x01 << ASSOC_CAPINFO_DEV_TYPE_SHIFT)
#define ASSOC_CAPINFO_DEV_TYPE_RFD      (0x00 << ASSOC_CAPINFO_DEV_TYPE_SHIFT)

#define ASSOC_CAPINFO_ASSIGN_ADDR_SHIFT 7
#define ASSOC_CAPINFO_ASSIGN_ADDR_MASK  (0x01 << ASSOC_CAPINFO_ASSIGN_ADDR_SHIFT)
#define ASSOC_CAPINFO_ASSIGN_ADDR_TRUE  (0x01 << ASSOC_CAPINFO_ASSIGN_ADDR_SHIFT)
#define ASSOC_CAPINFO_ASSIGN_ADDR_FALSE (0x00 << ASSOC_CAPINFO_ASSIGN_ADDR_SHIFT)

#define DISASSOC_REASON_BY_COORD     0x01
#define DISASSOC_REASON_BY_DEVICE    0x02

#endif /* ___PRMTV802154E_H__ */
