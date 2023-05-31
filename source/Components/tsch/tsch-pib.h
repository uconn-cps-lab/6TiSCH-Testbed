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
 *  ====================== tsch-pib.h =============================================
 *  
 */

#ifndef __TSCH_PIB_H__
#define __TSCH_PIB_H__

#if defined (__IAR_SYSTEMS_ICC__) || defined(__TI_COMPILER_VERSION__)
#include <stdint.h>
#endif

#define TSCH_PIB_ASN                   0x91
#define TSCH_PIB_JOIN_PRIO             0x92   
#define TSCH_PIB_MIN_BE                0x4f
#define TSCH_PIB_MAX_BE                0x57
#define TSCH_PIB_DISCON_TIME           0x8f
#define TSCH_PIB_MAX_FRAME_RETRIES     0x59

#define TSCH_PIB_CHAN_HOP              0x90
#define TSCH_PIB_TIMESLOT              0x70
#define TSCH_PIB_SLOTFRAME             0x64
#define TSCH_PIB_LINK                  0x69

#define TSCH_PIB_TI_KEEPALIVE          0x01

#define TSCH_PIB_ASN_LEN                5
#define TSCH_PIB_CHAN_HOP_BITMAP_LEN    2
#define TSCH_PIB_CHAN_HOP_SEQ_LIST_LEN  132

#define TSCH_PIB_LINK_OPTIONS_TX        0x01
#define TSCH_PIB_LINK_OPTIONS_TX_COAP   0x21
#define TSCH_PIB_LINK_OPTIONS_RX        0x02  
#define TSCH_PIB_LINK_OPTIONS_SH        0x04  
#define TSCH_PIB_LINK_OPTIONS_TK        0x08  

#define TSCH_PIB_LINK_TYPE_NORMAL       0x0   
#define TSCH_PIB_LINK_TYPE_ADV          0x1   

static inline void
TSCH_PIB_set_asn(uint8_t* asn_buf, uint64_t asn)
{
  asn_buf[0] = (asn      ) & 0xff;
  asn_buf[1] = (asn >>  8) & 0xff;
  asn_buf[2] = (asn >> 16) & 0xff;
  asn_buf[3] = (asn >> 24) & 0xff;
  asn_buf[4] = (asn >> 32) & 0xff;
}

static inline uint64_t
TSCH_PIB_get_asn(uint8_t* asn_buf)
{
  return (
           ((uint64_t) asn_buf[0] )      |
           ((uint64_t) asn_buf[1] <<  8) |
           ((uint64_t) asn_buf[2] << 16) |
           ((uint64_t) asn_buf[3] << 24) |
           ((uint64_t) asn_buf[4] << 32)
         );
}

typedef struct __TSCH_PIB_channel_hopping {

  uint8_t  hopping_sequence_id;
  uint8_t  channel_page;
  uint16_t number_of_channels;
  uint32_t phy_configuration;
  uint16_t hopping_sequence_length;
  uint16_t hopping_sequence_list[TSCH_PIB_CHAN_HOP_SEQ_LIST_LEN];

} TSCH_PIB_channel_hopping_t;


typedef struct __TSCH_PIB_timeslot_template {

  uint8_t  timeslot_id;
  uint8_t  reserve;
  uint16_t mac_ts_cca_offset;
  uint16_t ts_cca;
  uint16_t ts_tx_offset;
  uint16_t ts_rx_offset;
  uint16_t ts_rx_ack_delay;
  uint16_t ts_tx_ack_delay;
  uint16_t ts_rx_wait;
  uint16_t ts_ack_wait;
  uint16_t ts_rx_tx;
  uint16_t ts_max_ack;
  uint16_t ts_max_tx;
  uint16_t ts_timeslot_length;

} TSCH_PIB_timeslot_template_t;

typedef struct __TSCH_PIB_slotframe 
{
  uint8_t    slotframe_handle;
  uint8_t    reserve;
  uint16_t   slotframe_size;
} TSCH_PIB_slotframe_t;

typedef struct __TSCH_PIB_link {

  uint8_t    slotframe_handle;
  uint8_t    link_option;
  uint8_t    link_type;
  uint8_t    period;
  uint8_t    periodOffset;
  uint16_t   link_id;
  uint16_t   timeslot;
  uint16_t   channel_offset;
  uint16_t   peer_addr;
} TSCH_PIB_link_t;

typedef struct __TSCH_PIB_ti_keepalive {

  uint16_t  period;
  uint16_t  dst_addr_short;
} TSCH_PIB_ti_keepalive_t;

#endif /* __TSCH_PIB_H__ */
