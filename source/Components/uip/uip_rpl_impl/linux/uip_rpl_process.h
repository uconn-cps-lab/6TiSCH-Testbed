/*
* Copyright (c) 2014 Texas Instruments Incorporated
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
 *  ====================== uip_rpl_process.h =============================================
 *  uip process thread related functions implementation in Linux
 */

#ifndef __UIP_RPL_PROCESS_H__
#define  __UIP_RPL_PROCESS_H__

#include <syslog.h>
#include "uip-conf.h"

#include "uip_rpl_impl/uip_rpl_attributes.h"

#define NHL_CTRL_TYPE_SHORTADDR  1

#define UIP_UDP_SEND            0x14
#define IP_ADDR_INIT            0x15

typedef void (*uip_connection_callback) (void);

typedef struct
{
    uint16_t total_rx_packet;
    uint16_t buf_full_drop_segment;
    uint16_t frag_n_header_error;
    uint16_t unknown_header_error;
    uint16_t packet_len_small_error;
    uint16_t packet_len_big_error;
    uint16_t number_tx_pkt;
    uint16_t tx_drop_len_wrong;
    uint16_t reass_timeout;
    uint16_t frag_n;
    uint16_t frag_1;
} LowPAN_DBG_s;

typedef struct __IP_addr_init
{
    uint16_t addrType;
    void* addr;
}IP_addr_init_t;

typedef struct __TCPIP_cmdmsg
{
	uint8_t event_type;
	void *data;
} TCPIP_cmdmsg_t;

typedef struct _tcpip_debug
{
  uint16_t  tcpip_post_err;
  uint16_t  tcpip_post_app_err;
  uint16_t  tcpip_pend_err;
} TCPIP_DBG_s;

extern TCPIP_DBG_s TCPIP_Dbg;
extern LowPAN_DBG_s LOWPAN_Dbg;

int UIP_init(void);
void UIP_setMacPib(uint32_t attrId, uint16_t attrValLen, uint8_t *attrVal_p);
int UIP_address_init(void);
void uip_rpl_init();
void tcpip_process_post_func(process_event_t event, process_data_t data);
void rpl_process_post_func(process_event_t event, process_data_t data);
void nhl_ctrl_proc_cb(uint16_t ctrl_type, void* ctrl_val);
void prefix_setup(uint8_t *pAddr);
void tsch_sicslowpan_sent_cb(uint8_t status, uint8_t num_tx);
void tsch_sicslowpan_input(void);
void UIP_resetDIOTime(void);

#endif
