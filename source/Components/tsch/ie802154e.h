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
 *  ====================== ie802154e.h =============================================
 *  
 */

/* Includes */
#ifndef IE_802154E_H
#define IE_802154E_H

#include "ulpsmac.h"
#include "pib-802154e.h"

/* Macros & Defines */
#define IE_PACKING(var,size,position)   (((uint16_t)var&(((uint16_t)1<<size)-1))<<position)
#define IE_UNPACKING(var,size,position) (((uint16_t)var>>position)&(((uint16_t)1<<size)-1))
#define GET_BYTE(var,bytenum)           ((var>>(bytenum*8))&0xFF)
#define MAKE_UINT16(low,high)           ((low&0x00FF)|((high&0x00FF)<<8))

#define IE_TYPE_HEADER                  0
#define IE_TYPE_PAYLOAD                 1
#define IE_TYPE_S                       1
#define IE_TYPE_P                       15

// Header IE for TSCH
#define IE_ID_ACKNACK_TIME_CORRECTION   0x1e
#define IE_ID_LIST_TERM1                0x7e
#define IE_ID_LIST_TERM2                0x7f
#define IE_ID_LIST_TERM                 0x7f
#define IE_ID_HDR_UNMG_L                0x00
#define IE_ID_HDR_UNMG_H                0x19
#define IE_ID_H_S                       8
#define IE_ID_H_P                       7
#define IE_LEN_H_S                      7
#define IE_LEN_H_P                      0

// Payload IE for TSCH
// Payload IE Group ID
#define IE_GROUP_ID_ESDU                0x0
#define IE_GROUP_ID_MLME                0x1
#define IE_GROUP_ID_UNMG_L              0x2
#define IE_GROUP_ID_UNMG_H              0x9
#define IE_GROUP_ID_TERM                0xf
#define IE_GROUP_ID_S                   4
#define IE_GROUP_ID_P                   11
#define IE_LEN_P_S                      11
#define IE_LEN_P_P                      0

#define IE_ID_TSCH_PERIODIC_SCHED       0x42
#define IE_ID_TSCH_LINK_PERIOD          0x43
#define IE_ID_TSCH_COMPRESS_LINK        0x44
#define IE_ID_TSCH_SUGGEST_PARENT	0x45

// MLME IEs for TSCH
#define IE_ID_TSCH_SYNCH                0x1a
#define IE_ID_TSCH_SLOTFRAMELINK        0x1b
#define IE_ID_TSCH_TIMESLOT             0x1c
#define IE_ID_CHANNELHOPPING            0x09
#define IE_ID_EB_FILTER                 0x1e
#define IE_ID_SHORT_UNMG_L              0x40
#define IE_ID_SHORT_UNMG_H              0x7f
#define IE_ID_LONG_UNMG_L               0x00
#define IE_ID_LONG_UNMG_H               0x08
#define IE_SUBTYPE_SHORT                0
#define IE_SUBTYPE_LONG                 1
#define IE_SUBTYPE_S                    1
#define IE_SUBTYPE_P                    15
#define IE_ID_SHORT_S                   7
#define IE_ID_SHORT_P                   8
#define IE_SUBLEN_SHORT_S               8
#define IE_SUBLEN_SHORT_P               0
#define IE_ID_LONG_S                    4
#define IE_ID_LONG_P                    11
#define IE_SUBLEN_LONG_S                11
#define IE_SUBLEN_LONG_P                0

// Header IE for
#define MIN_HEADER_IE_LEN               2
#define MAX_HEADER_IE_LEN               127
#define IE_ACKNACK_TIME_CORRECTION_LEN  7

// PAYLOAD IEs for TSCH
#define MIN_PAYLOAD_IE_LEN                 2
#define MAX_PAYLOAD_IE_LEN                 127
#define IE_TSCH_SYNCH_LEN                  6
#define IE_SLOTFRAME_LINK_INFO_LEN         1
#define IE_SLOTFRAME_INFO_LEN              4
#define IE_LINK_INFO_LEN                   5
#define IE_TIMESLOT_TEMPLATE_LEN_FULL      25
#define IE_TIMESLOT_TEMPLATE_LEN_ID_ONLY   1
#define IE_HOPPING_TIMING_LEN              1
#define IE_EB_FILTER_LEN_MIN               1

#define MAC_LINK_OPTION_TX         0x01
#define MAC_LINK_OPTION_TX_COAP    0x21
#define MAC_LINK_OPTION_RX         0x02
#define MAC_LINK_OPTION_SH         0x04
#define MAC_LINK_OPTION_TK         0x08

// private configuration for TX LINK
#define MAC_LINK_OPTION_WAIT_CONFIRM        0x10

#define MAC_LINK_TYPE_NORMAL 0x0
#define MAC_LINK_TYPE_ADV    0x1


typedef enum
{
  IE_BEACON_LINK,
  IE_SH_LINK,
  IE_DED_UP_LINK,
  IE_DED_DOWN_LINK,
  IE_NUM_PERIOD_LINK_TYPE
} IE_PERIOD_LINK_TYPE_e;


// TODO: Check the endianess
//       Currently do not include content length
static inline void
ie802154e_set16(uint8_t* buf_ptr, uint16_t value)
{
  buf_ptr[0] = value & 0xff;
  buf_ptr[1] = (value >> 8) & 0xff;
}

static inline uint16_t
ie802154e_get16(uint8_t* buf_ptr)
{
  return (
           ((uint16_t) buf_ptr[0]      ) |
           ((uint16_t) buf_ptr[1] <<  8)
         );
}

static inline void
ie802154e_set32(uint8_t* buf_ptr, uint32_t value)
{
  buf_ptr[0] = value & 0xff;
  buf_ptr[1] = (value >> 8)   & 0xff;
  buf_ptr[2] = (value >> 16 ) & 0xff;
  buf_ptr[3] = (value >> 24 ) & 0xff;
}

static inline uint32_t
ie802154e_get32(uint8_t* buf_ptr)
{
  return (
           ((uint32_t) buf_ptr[0]      ) |
           ((uint32_t) buf_ptr[1] <<  8) |
           ((uint32_t) buf_ptr[2] << 16) |
           ((uint32_t) buf_ptr[3] << 24)
         );
}

static inline void
ie802154e_set_asn(uint8_t* buf_ptr, uint64_t value)
{
  buf_ptr[0] = value & 0xff;
  buf_ptr[1] = (value >> 8)   & 0xff;
  buf_ptr[2] = (value >> 16 ) & 0xff;
  buf_ptr[3] = (value >> 24 ) & 0xff;
  buf_ptr[4] = (value >> 32 ) & 0xff;
}

static inline uint64_t
ie802154e_get_asn(uint8_t* buf_ptr)
{
  return (
           ((uint64_t) buf_ptr[0]      ) |
           ((uint64_t) buf_ptr[1] <<  8) |
           ((uint64_t) buf_ptr[2] << 16) |
           ((uint64_t) buf_ptr[3] << 24)
           //((uint64_t) buf_ptr[4] << 32)
         );
}

/**
 * \brief Defines the bitfields of IEEE 802.15.4e Header IE.
 * Section 5.2.4.2
 */
typedef struct {
  uint8_t  ie_type;                     /**< 1 bit, type of IE */
  uint8_t  ie_elem_id;                  /**< 8 bits, Element ID */
  uint8_t  ie_content_length;           /**< 7 bits, IE Content Length */
  uint8_t* ie_content_ptr;              /** max size 127 bytes */
} ie802154e_hie_t;

/**
 * \brief Defines the bitfields of IEEE 802.15.4e Payload IE.
 * Section 7.2.4.1
 */
typedef struct {
  uint8_t  ie_type;                     /**< 1 bit, type of IE */
  uint8_t  ie_group_id;                 /**< 4 bits, group id */
  uint16_t ie_content_length;           /**< 11 bits, IE Content Length */
  uint8_t  outer_descriptor;            /**< flag which outer descriptor is needed for packing */
  uint8_t  ie_sub_type;                 /**< 1 bit, sub type of MLME IE */
  uint8_t  ie_elem_id;                  /**< 7 bits or 4 bits, Element ID */
  uint16_t ie_sub_content_length;       /**< 8 bits or 11 bits, Sub IE Content Length */
  uint8_t* ie_content_ptr;              /** IE content or First Sub IE Content */
} ie802154e_pie_t;

typedef struct {
  uint8_t acknack;
  int16_t time_synch;
  uint8_t rxRssi;
  uint32_t rxTotal;
} ie_acknack_time_correction_t;

// TSCH Synchronization IE Payload
typedef struct{
  uint64_t macASN;          /** 5 bytes, uint64_t for holding 5 bytes */
  uint8_t  macJoinPriority; /** 1 byte Join Priority */
} ie_tsch_synch_t;

// Per link link information field
typedef struct{
  uint16_t  macTimeslot;                /** 2 bytes to indicate timeslot in slotframe*/
  uint16_t  macChannelOffset;           /** 2 bytes */
  uint8_t  macLinkOption;               /** 1 byte, Tx, Rx or Shared link */
} ie_link_info_field_t;

#define IE_SLOTFRAMELINK_MAX_LINK      10
#define IE_SLOTFRAMELINK_MAX_SLOTFRAME 1
typedef struct{
  uint8_t  macSlotframeHandle;          /** 1 byte for slotframe handle */
  uint8_t  numLinks;                    /** 1 byte for number of links */
  uint16_t macSlotframeSize;            /** 2 bytes for size of slotframe in bytes */
  ie_link_info_field_t link_info_list[IE_SLOTFRAMELINK_MAX_LINK];
} ie_slotframe_info_field_t;

// TSCH Slotframe and Link IE Payload
typedef struct{
  uint8_t numSlotframes;                /** 1 byte, Number of slotframes in list*/
  ie_slotframe_info_field_t slotframe_info_list[IE_SLOTFRAMELINK_MAX_SLOTFRAME]; /** */
} ie_tsch_slotframelink_t;

// TSCH Channel Hopping IE Payload : This IE will exceed the payload limit
typedef struct{
  uint8_t macHoppingSequenceID;        /** 1 byte, Hopping sequence ID */
//  uint8_t channelPage;
//  uint16_t numChannels;
//  uint32_t phyConfiguration;
//  uint32_t extendedBitmap;
//  uint16_t hoppingSequenceLen;
//  uint16_t hoppingSequenceList[128];
//  uint16_t currentHop;
  // currently we will assume the default hopping sequence id of 0 and
  // therefore we do not need other fields in this IE
} ie_hopping_t;

// TSCH Timeslot parameters, see Section 7.2.4.3.10
typedef struct{
  uint8_t timeslot_id;
  uint8_t reserve;
  uint16_t macTsCcaOffset;
  uint16_t TsCCA;
  uint16_t TsTxOffset;
  uint16_t TsRxOffset;
  uint16_t TsRxAckDelay;
  uint16_t TsTxAckDelay;
  uint16_t TsRxWait;
  uint16_t TsAckWait;
  uint16_t TsRxTx;
  uint16_t TsMaxAck;
  uint16_t TsMaxTx;
  uint16_t TsTimeslotLength;
} ie_timeslot_template_t;

/* Figure 48ll - EB Filter IE */
typedef struct {
  uint8_t permit_join_on;               /* 1 bit: Permit Joining On */
  uint8_t inc_lq_filter;                /* 1 bit: Include Link Quality Filter */
  uint8_t inc_perc_filter;              /* 1 bits: Include Percent Filter */
  uint8_t num_entries_pib_id;           /* 2 bits: number of entries in PIB Identifier list */
  uint8_t lq;                           /* 1 octet : */
  uint8_t perc_filter;                  /* 1 octet : */
  uint8_t pib_ids[4];                   /* 4 octets : */
} ie_eb_filter_t;

/* Periodic Schduler IE Payload */
typedef struct{
  uint8_t period;
  uint8_t numFragPackets;
  uint8_t numHops;
} ie_tsch_periodic_scheduler_t;

/* Link Period IE */
typedef struct{
  uint8_t period[IE_SLOTFRAMELINK_MAX_LINK];
  uint8_t periodOffset[IE_SLOTFRAMELINK_MAX_LINK];
} ie_tsch_link_period_t;

/* Link Compression IE */
typedef struct{
  uint8_t linkType;
  uint8_t startTimeslot;
  uint8_t numContTimeslot;
} ie_tsch_link_comp_t;

uint8_t ie802154e_add_hdrie(ie802154e_hie_t* ie_info, uint8_t* buf, uint8_t buf_len);
uint8_t ie802154e_add_hdrie_term(uint8_t* buf, uint8_t buf_len);

uint8_t ie802154e_find_in_hdrielist(uint8_t* buf, uint8_t buf_len, ie802154e_hie_t* find_ie);
void    ie802154e_parse_acknack_time_correction(uint8_t* buf, ie_acknack_time_correction_t* arg);

// Adds an IE to a list of payload IEs
// Usage: Adds payload IEs in front of the payload (or null payload) data stored in buf
// Runs in the context of the NHL
// Input:  Buffer pointer *buf
//		   Payload IE structure pointer ie_info
//
// Output: Modified ulpsmac buffer *buf
//		   length is incremented by the length of the added payload IE
uint8_t ie802154e_add_payloadie(ie802154e_pie_t* ie_info, uint8_t* buf, uint8_t buf_len);
uint8_t ie802154e_add_payloadie_term(uint8_t* buf, uint8_t buf_len);

// Parses payload IE list and extracts the first IE from the list
// Called by NHL
// Runs in the NHL process protothread
// For motes that are not PAN coordinators, this
// sets PAN and Lower MAC Databases (DBs) according
// to the information contained in the IE list.
// Input: Ulpsmac buffer pointer *buf
// Output: Payload IE structure pointed by payload_ie
//		   length: running tally of overall payload IE list
//         Ajusts ulpsmacbuf metadata
uint8_t ie802154e_find_in_payloadielist(uint8_t* buf, uint8_t buf_len, ie802154e_pie_t* find_ie);
uint8_t ie802154e_payloadielist_len(uint8_t* buf, uint8_t buf_len);
uint8_t ie802154e_parse_payloadie(uint8_t* buf, uint8_t buf_len, ie802154e_pie_t* find_ie);
uint8_t ie802154e_parse_payloadsubie(uint8_t* buf, uint8_t buf_len, ie802154e_pie_t* find_ie);
void    ie802154e_parse_tsch_synch(uint8_t* buf, ie_tsch_synch_t* arg);
void    ie802154e_parse_timeslot_template(ie802154e_pie_t* pie, ie_timeslot_template_t* ie_tt);
void    ie802154e_parse_hopping_timing(ie802154e_pie_t* pie, ie_hopping_t* ie_hop);
void    ie802154e_parse_tsch_slotframelink(uint8_t* buf, ie_tsch_slotframelink_t* arg);
void    ie802154e_parse_eb_filter(uint8_t* buf, ie_eb_filter_t* ie_eb_filter);
void    ie802154e_parse_tsch_periodic_information(uint8_t* buf, ie_tsch_periodic_scheduler_t* arg);
void    ie802154e_parse_link_period(ie802154e_pie_t* pIE, uint16_t len, ie_tsch_link_period_t* arg);
void    ie802154e_parse_link_compression(ie802154e_pie_t* pIE, ie_tsch_link_comp_t* arg);

uint8_t ie802154e_hdrielist_len(uint8_t* buf, uint8_t buf_len);

#endif /* IE_802154E_H */
