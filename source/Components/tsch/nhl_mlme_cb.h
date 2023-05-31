/*
* Copyright (c) 2015 Texas Instruments Incorporated
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
 *  ====================== nhl_mlme_cb.h =============================================
 *  
 */


#ifndef NHL_MLME_CB_H
#define NHL_MLME_CB_H

#include "prmtv802154e.h"
#include "mac_config.h"

//*****************************************************************************
// typedefs
//*****************************************************************************
//! \ MAC Command Type
//!
typedef enum {
    COMMAND_NONE = 0x0,
    COMMAND_BEACON_REQUEST,
    COMMAND_DATA_REQUEST,
    COMMAND_DATA_CONFIRM,
    COMMAND_DATA_INDICATION,
    COMMAND_BEACON_INDICATION,
    COMMAND_ASSOCIATE_INDICATION,
    COMMAND_ASSOCIATE_RESPONSE_IE,
    COMMAND_ASSOCIATE_CONFIRM,
    COMMAND_DISASSOCIATE_INDICATION,
    COMMAND_DISASSOCIATE_CONFIRM,
    COMMAND_COMM_STATUS_INDICATION,
    COMMAND_SCAN_CONFIRM,
#if ULPSMAC_L2MH
    COMMAND_ASSOCIATE_INDICATION_L2MH,
    COMMAND_ASSOCIATE_CONFIRM_L2MH,
    COMMAND_DISASSOCIATE_INDICATION_L2MH,
    COMMAND_DISASSOCIATE_CONFIRM_L2MH,
#endif
    COMMAND_RESTART,
    COMMAND_JOIN_INDICATION,
    COMMAND_SHARE_SLOT_MODIFICATION,
    COMMAND_TRANSMIT_PACKET,
    COMMAND_SEND_KA_PACKET, //JIRA51
    COMMAND_ADJUST_KA_TIMER, //JIRA51
    COMMAND_ASTAT_REQ_PROC, //JIRA52
    NUM_COMMANDS,
} command_set_e;

typedef struct cbMsg{
  uint8_t cb_type;
  uint8_t msg_type;
  uint8_t arg_size;
} cbMsg_t;


bool nhl_ipc_msg_post(uint8_t msgType, uint32_t v1, uint32_t v2, uint32_t v3); //JIRA51
#endif /* NHL_MLME_CB_H */
