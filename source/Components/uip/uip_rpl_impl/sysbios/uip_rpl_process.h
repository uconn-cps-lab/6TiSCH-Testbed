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
 *  uip process thread related functions implementation in SYSBIOS
 */

#ifndef __UIP_RPL_PROCESS_H__
#define  __UIP_RPL_PROCESS_H__

#include "uip-conf.h"
#include "net/uip.h"
#include "net/udp-simple-socket.h"

#include "uip_rpl_impl/uip_rpl_attributes.h"


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

typedef struct __TCPIP_cmdmsg
{
    //!!!!The size of this data structure (8 bytes right now)
    // must agree with the mailbox definition in the .cfg file.  If this data structure is modified,
    // make sure the .cfg file is modified as well.
    // Please note, there are two mailboxes in the .cfg file using this data structure:
    //  coap_mailbox and tcpip_mailbox
    //They both must be modified to agree with the size of this data structure.
   uint8_t event_type;
   uint8_t index;
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

void uip_rpl_init();
void tcpip_process_post_func(process_event_t event, process_data_t data);
void rpl_process_post_func(process_event_t event, process_data_t data);
void tcpip_task(UArg arg0, UArg arg1);
void prefix_setup(uint8_t *pAddr);
void ctrl_proc_cb(uint16_t ctrl_type, void* ctrl_val);
void uip_ds6_tsch_notification_callback(int event, void* route);
void uip_setup_appConnCallback(uip_connection_callback callBack);
void uip_rpl_clean(void);
uint8_t uip_rpl_get_rank(void);
void uip_udp_packet_send_post(struct udp_simple_socket *s, void *data, int len,
                              const uip_ipaddr_t *toaddr, uint16_t toport);

#endif
