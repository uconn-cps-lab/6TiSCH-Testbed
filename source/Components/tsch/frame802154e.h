/*
 *  Copyright (c) 2008, Swedish Institute of Computer Science
 *  All rights reserved.
 *
 *  Additional fixes for AVR contributed by:
 *        Colin O'Flynn coflynn@newae.com
 *        Eric Gnoske egnoske@gmail.com
 *        Blake Leverett bleverett@gmail.com
 *        Mike Vidales mavida404@gmail.com
 *        Kevin Brown kbrown3@uccs.edu
 *        Nate Bohlmann nate@elfwerks.com
 *
 *  Additional fixes for MSP430 contributed by:
 *        Joakim Eriksson
 *        Niclas Finne
 *        Nicolas Tsiftes
 *
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the copyright holders nor the names of
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
*/
/**
 *    \addtogroup frame802154
 *    @{
 */
/**
 *  \file
 *  \brief 802.15.4 frame creation and parsing functions
 *
 *  This file converts to and from a structure to a packed 802.15.4
 *  frame.
 *
 *    $Id: frame802154.h,v 1.3 2010/02/18 21:00:28 adamdunkels Exp $
*/

 /******************************************************************************
 *
 * Copyright (c) 2010-2014 Texas Instruments Inc.  All rights reserved.
 *
 * DESCRIPTION: Modified for 802.15.4e
 *
 * HISTORY:
 *
 *
 ******************************************************************************/
#include "mac_config.h"

/* Includes */
#ifndef FRAME_802154E_H
#define FRAME_802154E_H

/* Macros & Defines */
//#define MAC_FCF_FRAME_VERSION_B00  0x00      /* Not supported */
//#define MAC_FCF_FRAME_VERSION_B01  0x01      /* Not supported */
#define MAC_FCF_FRAME_VERSION_B10  0x02

/* MAC address mode bitmaps */
#define MAC_ADDR_MODE_NULL      0x00
#define MAC_ADDR_MODE_SHORTEST  0x01
#define MAC_ADDR_MODE_SHORT     0x02
#define MAC_ADDR_MODE_EXT       0x03

/* MAC frame field lengths in bytes */

#define MAC_FCF_FIELD_LEN               2       /* frame control field */
#define MAC_SEQ_NUM_FIELD_LEN           1       /* sequence number  */
#define MAC_PAN_ID_FIELD_LEN            2       /* PAN ID  */
#define MAC_EXT_ADDR_FIELD_LEN          8       /* Extended address */
#define MAC_SHORT_ADDR_FIELD_LEN        2       /* Short address */
#define MAC_SHORTEST_ADDR_FIELD_LEN	1	/* 1 byte address 15.4e */
#define MAC_FCS_FIELD_LEN               2       /* FCS field */


/* Frame control field bit masks 
 * Used to create the Frame Control Field by 
 * the create() function */

#define MAC_FCF_FRAME_TYPE_MASK         0x0007
#define MAC_FCF_SEC_ENABLED_MASK        0x0008
#define MAC_FCF_FRAME_PENDING_MASK      0x0010
#define MAC_FCF_ACK_REQUEST_MASK        0x0020
#define MAC_FCF_INTRA_PAN_MASK          0x0040
#define MAC_FCF_SEQ_NUM_SUP_MASK	0x0100	
#define MAC_FCF_IE_LIST_MASK		0x0200	
#define MAC_FCF_DST_ADDR_MODE_MASK      0x0C00
#define MAC_FCF_FRAME_VERSION_MASK      0x3000
#define MAC_FCF_SRC_ADDR_MODE_MASK      0xC000


/* Frame control field bit positions */
#define MAC_FCF_FRAME_TYPE_POS          0
#define MAC_FCF_SEC_ENABLED_POS         3
#define MAC_FCF_FRAME_PENDING_POS       4
#define MAC_FCF_ACK_REQUEST_POS         5
#define MAC_FCF_INTRA_PAN_POS           6
#define MAC_FCF_SEQ_NUM_SUP_POS		8	
#define MAC_FCF_IE_LIST_POS		9	
#define MAC_FCF_DST_ADDR_MODE_POS       10
#define MAC_FCF_FRAME_VERSION_POS       12
#define MAC_FCF_SRC_ADDR_MODE_POS       14


/* Frame type */
#define MAC_FRAME_TYPE_BEACON           0
#define MAC_FRAME_TYPE_DATA             1
#define MAC_FRAME_TYPE_ACK              2
#define MAC_FRAME_TYPE_COMMAND          3
#define MAC_FRAME_TYPE_MULTIPURPOSE	5
#define MAC_FRAME_TYPE_MAX_VALID        MAC_FRAME_TYPE_MULTIPURPOSE

// MAC command frame identifiers
#define MAC_COMMAND_ASSOC_REQ         0x01
#define MAC_COMMAND_ASSOC_RESP        0x02
#define MAC_COMMAND_DISASSOC_NOT      0x03
#define MAC_COMMAND_DATA_REQ          0x04
#define MAC_COMMAND_PANID_CONFLICT    0x05
#define MAC_COMMAND_ORPHAN_NOT        0x06
#define MAC_COMMAND_COORD_REALIGNMENT 0x07

#if ULPSMAC_L2MH
// Command frame ID is reserved 0x0a ~ 0xff (table 82)
#define MAC_COMMAND_ASSOC_REQ_TI_L2MH     0x11
#define MAC_COMMAND_ASSOC_RESP_TI_L2MH    0x12
#define MAC_COMMAND_DISASSOC_NOT_TI_L2MH  0x13
#define MAC_COMMAND_DISASSOC_ACK_TI_L2MH  0x14
#endif
#define MAC_COMMAND_BEACON_INFO_TI_REQ    0x15
#define MAC_COMMAND_BEACON_INFO_TI_RESP   0x16

// MAC command frame maximum size
#define MAX_COMMAND_MESSAGE_SIZE 20


#define FRAME802154_SECURITY_LEVEL_NONE (0)
#define FRAME802154_SECURITY_LEVEL_128  (3)


/**
 *   @brief  The IEEE 802.15.4e frame has a number of constant/fixed fields that
 *            can be counted to make frame construction and max payload
 *            calculations easier.
 *
 *            These include:
 *            1. FCF                  - 2 bytes       - Fixed
 *            2. Sequence number      - 1 byte        - Fixed
 *            3. Addressing fields    - 4 - 20 bytes  - Variable
 *            4. Aux security header  - 0 - 14 bytes  - Variable
 *            5. CRC                  - 2 bytes       - Fixed
*/

/**
 * \brief Defines the bitfields of the frame control field (FCF).
 */
typedef struct {
  uint8_t frame_type;        /**< 3 bit. Frame type field, see 802.15.4 */
  uint8_t security_enabled;  /**< 1 bit. True if security is used in this frame */
  uint8_t frame_pending;     /**< 1 bit. True if sender has more data to send */
  uint8_t ack_required;      /**< 1 bit. Is an ack frame required? */
  uint8_t panid_compression; /**< 1 bit. Is this a compressed header? */
  //uint8_t reserved; 	     /**< 1 bit. Unused bits */
  uint8_t seq_num_suppress;  /**< 1 bit, is sequence number suppressed? */
  uint8_t ie_list_present;   /**< 1 bit, Is IE list present? */
  uint8_t dest_addr_mode;    /**< 2 bit. Destination address mode, 802.15.4e */
  uint8_t frame_version;     /**< 2 bit. 802.15.4e frame version */
  uint8_t src_addr_mode;     /**< 2 bit. Source address mode, see 802.15.4e */
} frame802154e_fcf_t;

/** \brief 802.15.4 security control bitfield.  See section 7.6.2.2.1 in 802.15.4 specification */
typedef struct {
  uint8_t  security_level; /**< 3 bit. security level      */
  uint8_t  key_id_mode;    /**< 2 bit. Key identifier mode */
#ifndef FEATURE_MAC_SECURITY  
  uint8_t  reserved;       /**< 3 bit. Reserved bits       */
#else
  uint8_t  frame_counter_suppression; /**< 1 bit. has frame counter in header?       */
  uint8_t  frame_counter_size;        /**< 1 bit. 0:4-byte, 1:5-byte frame counter   */
  uint8_t  reserved;                  /**< 1 bit. Reserved bit                       */
#endif
} frame802154_scf_t;

/** \brief 802.15.4 Aux security header */
typedef struct {
  frame802154_scf_t security_control;  /**< Security control bitfield */
#ifdef FEATURE_MAC_SECURITY
  uint8_t frame_counter_ext; /**< Frame counter for 5-bytes, used for security */
#endif   
  uint32_t frame_counter;   /**< Frame counter, used for security */
  uint8_t  key[9];          /**< The key itself, or an index to the key */
} frame802154_aux_hdr_t;

/** \brief Parameters used by the frame802154e_create() function.  These
 *  parameters are used in the 802.15.4e frame header.  See the 802.15.4e
 *  specification for details.
 */

typedef struct {
  frame802154e_fcf_t fcf;         /**< Frame control field  */
  uint8_t seq;                    /**< Sequence number */
  uint16_t dest_pid;              /**< Destination PAN ID */
  uint8_t dest_addr[8];           /**< Destination address */
  uint16_t src_pid;               /**< Source PAN ID */
  uint8_t src_addr[8];            /**< Source address */
  frame802154_aux_hdr_t aux_hdr;  /**< Aux security header */
  uint8_t aux_hdr_offset;
  uint8_t* hdrie;                 /**< header ie pointer */
  uint8_t hdrie_len;              /**< header ie length */
  uint8_t* payloadie;             /**< payload ie pointer */
  uint8_t payloadie_len;          /**< payload ie length */
  uint8_t *payload;               /**< Pointer to 802.15.4 frame payload */
  uint8_t payload_len;            /**< Length of payload field */
  uint16_t fcs;
} frame802154e_t;

/**
 *  \brief Structure that contains the lengths of the various addressing and security fields
 *  in the 802.15.4 header.  This structure is used in \ref frame802154_create()
 */
typedef struct {
  uint8_t seq_num_len;     /**< Length in bytes of sequence number field */
  uint8_t dest_pid_len;    /**< Length (in bytes) of destination PAN ID field */
  uint8_t dest_addr_len;   /**< Length in bytes of destination address field */
  uint8_t src_pid_len;     /**<  Length (in bytes) of source PAN ID field */
  uint8_t src_addr_len;    /**<  Length (in bytes) of source address field */
  uint8_t aux_sec_len;     /**<  Length in bytes of aux security header field */
  uint8_t hdrie_len;   /**< Length in bytes of header IEs */
  uint8_t payloadie_len;  /**< Length in bytes of payload IEs */
} frame802154e_flen_t;

/* Prototypes */
/*----------------------------------------------------------------------------*/
extern uint8_t 
frame802154e_hdrlen(frame802154e_t *p);
/*----------------------------------------------------------------------------*/
extern uint8_t 
frame802154e_hdrlen2(frame802154e_t *p, frame802154e_flen_t* flen);
/*----------------------------------------------------------------------------*/
extern uint8_t 
frame802154e_check_fcf(frame802154e_fcf_t* fcf);
/*----------------------------------------------------------------------------*/
extern uint8_t 
frame802154e_create(frame802154e_t *p, uint8_t *buf, uint8_t buf_len);
/*----------------------------------------------------------------------------*/
extern uint8_t 
frame802154e_create2(frame802154e_t *p, frame802154e_flen_t* flen, uint8_t *buf, uint8_t buf_len);
/*----------------------------------------------------------------------------*/
extern uint8_t 
frame802154e_parse_header(uint8_t *buf, uint8_t buf_len, frame802154e_t *pf);
/*----------------------------------------------------------------------------*/

/** @} */
#endif /* FRAME_802154E_H */
