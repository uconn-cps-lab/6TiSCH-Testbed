/*
 *
 *  Copyright (c) 2008, Swedish Institute of Computer Science
 *  All rights reserved.
 *
 *  Additional fixes for AVR contributed by:
 *
 *      Colin O'Flynn coflynn@newae.com
 *      Eric Gnoske egnoske@gmail.com
 *      Blake Leverett bleverett@gmail.com
 *      Mike Vidales mavida404@gmail.com
 *      Kevin Brown kbrown3@uccs.edu
 *      Nate Bohlmann nate@elfwerks.com
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
 *
 *  $Id: frame802154.c,v 1.4 2010/02/18 21:00:28 adamdunkels Exp $
*/
/*
 *  \brief This file is where the main functions that relate to frame
 *  manipulation will reside.
*/
/**
 *   \addtogroup frame802154
 *   @{
*/
/**
 *  \file
 *  \brief 802.15.4 frame creation and parsing functions
 *
 *  This file converts to and from a structure to a packed 802.15.4
 *  frame.
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

#include <string.h>

#include "frame802154e.h"
#include "ulpsmacaddr.h"
#include "ie802154e.h"

/*----------------------------------------------------------------------------*/
static uint8_t
addr_len(uint8_t mode)
{
  switch(mode) {
  case MAC_ADDR_MODE_NULL:  /* 0-bit address */
    return 0;
  case MAC_ADDR_MODE_SHORTEST: /* 8-bit address */
    return MAC_SHORTEST_ADDR_FIELD_LEN;
  case MAC_ADDR_MODE_SHORT:   /* 16-bit address */
    return MAC_SHORT_ADDR_FIELD_LEN;
  case MAC_ADDR_MODE_EXT:
    return MAC_EXT_ADDR_FIELD_LEN;
  default:
    return 0;
  }
}

/*----------------------------------------------------------------------------*/
static inline void
field_len(frame802154e_t *p, frame802154e_flen_t *flen)
{
  /* init flen to zeros */
  memset(flen, 0, sizeof(frame802154e_flen_t));

  /* Determine if Sequence number suppression is used */
  if (! (p->fcf.seq_num_suppress & 0x01)){
    flen->seq_num_len = 1;
  }

  /* determine address lengths */
  flen->dest_addr_len = addr_len(p->fcf.dest_addr_mode & 3);
  
  /* Table 7-2 IEEE 802.15.4-2015*/
  if (p->fcf.panid_compression == 1)
  {
    if ((p->fcf.src_addr_mode == ULPSMACADDR_MODE_LONG) &&
      (p->fcf.dest_addr_mode == ULPSMACADDR_MODE_LONG))
      {
        flen->dest_pid_len = 0;
        flen->src_pid_len = 0;
      }
    else 
    {
      flen->dest_pid_len = 2;
      flen->src_pid_len = 0;
    }
  }
  else
  {
    if ((p->fcf.src_addr_mode == ULPSMACADDR_MODE_LONG) &&
      (p->fcf.dest_addr_mode == ULPSMACADDR_MODE_LONG))
      {
        flen->dest_pid_len = 2;
        flen->src_pid_len = 0;
      }
    else
    {
      flen->dest_pid_len = 2;
      flen->src_pid_len = 2;
    }
  }

  /* determine address lengths */
  flen->src_addr_len = addr_len(p->fcf.src_addr_mode & 3);

  /* Aux security header */
  /* TODO: Aux security header not yet supported */
#ifdef FEATURE_MAC_SECURITY
  if(p->fcf.security_enabled & 1) {
    uint8_t frame_counter_size;
    uint8_t key_id_size;

    if(p->aux_hdr.security_control.frame_counter_suppression)
      frame_counter_size = 0;
    else if(p->aux_hdr.security_control.frame_counter_size)
      frame_counter_size = 5;
    else
      frame_counter_size = 4;

    switch(p->aux_hdr.security_control.key_id_mode) {
      case 0:
        key_id_size = 0; /* minimum value */
        break;
      case 1:
        key_id_size = 1;
        break;
      case 2:
        key_id_size = 5;
        break;
      case 3:
        key_id_size = 9;
        break;
      default:
        key_id_size = 0;
        break;
    }

    flen->aux_sec_len = 1 + frame_counter_size + key_id_size;
  }
#endif

  if(p->fcf.ie_list_present) {
    flen->hdrie_len = p->hdrie_len;
    flen->payloadie_len = p->payloadie_len;
  } else {
    flen->hdrie_len = 0;
    flen->payloadie_len = 0;
  }
}

static inline uint8_t
hdr_len(frame802154e_flen_t* flen)
{
  return MAC_FCF_FIELD_LEN + flen->seq_num_len + flen->dest_pid_len + flen->dest_addr_len
	  + flen->src_pid_len + flen->src_addr_len + flen->aux_sec_len+flen->hdrie_len;
}

/***************************************************************************//**
 * @fn          hdr_create
 *
 * @brief      Creates a frame for transmission over the air.  This function is
 *             meant to be called by a higher level function, that interfaces to a MAC
 *
 * @param[in]  finfo - Pointer to frame802154e_t struct, which specifies the
 *                     frame to send
 *             flen - Pointer to frame802154e_flen_t structure, which specifies
 *                    field length
 *             buf - Pointer to the buffer to use for the frame
 *
 * @return     legnth of header
 ******************************************************************************/
static uint8_t
hdr_create(frame802154e_t* finfo, frame802154e_flen_t* flen, uint8_t* buf)
{
  int16_t  c;
  uint8_t* tx_frame_buffer;
  uint8_t  pos;

  // statically setting frame version to '10'
  finfo->fcf.frame_version = MAC_FCF_FRAME_VERSION_B10;
#ifndef FEATURE_MAC_SECURITY  // orginal : #if 1
  finfo->fcf.security_enabled = 0;
#endif

  /* OK, now we have field lengths.  Time to actually construct */
  /* the outgoing frame, and store it in tx_frame_buffer */
  tx_frame_buffer = buf;
  tx_frame_buffer[0] = (finfo->fcf.frame_type & 7) |
    ((finfo->fcf.security_enabled & 1) << 3) |
    ((finfo->fcf.frame_pending & 1) << 4) |
    ((finfo->fcf.ack_required & 1) << 5) |
    ((finfo->fcf.panid_compression & 1) << 6);
  tx_frame_buffer[1] = (finfo->fcf.seq_num_suppress & 1) |
	 ((finfo->fcf.ie_list_present & 1) << 1) |
	  ((finfo->fcf.dest_addr_mode & 3) << 2) |
    ((finfo->fcf.frame_version & 3) << 4) |
    ((finfo->fcf.src_addr_mode & 3) << 6);
  /* marker for position in header */
  pos = MAC_FCF_FIELD_LEN;

  /* sequence number */
  if (!(finfo->fcf.seq_num_suppress)) {
    tx_frame_buffer[pos] = finfo->seq;
    pos++;
  }

  /* Destination PAN ID */
  if(flen->dest_pid_len == 2) {
    tx_frame_buffer[pos++] = finfo->dest_pid & 0xff;
    tx_frame_buffer[pos++] = (finfo->dest_pid >> 8) & 0xff;
  }

  /* Destination address */
  for(c=0; c<flen->dest_addr_len; c++) {
    tx_frame_buffer[pos++] = finfo->dest_addr[c];
  }

  /* Source PAN ID */
  if(flen->src_pid_len == 2) {
    tx_frame_buffer[pos++] = finfo->src_pid & 0xff;
    tx_frame_buffer[pos++] = (finfo->src_pid >> 8) & 0xff;
  }

  /* Source address */
  for(c=0; c<flen->src_addr_len; c++) {
    tx_frame_buffer[pos++] = finfo->src_addr[c];
  }

  /* Aux header */
  /* TODO: Aux security header not yet implemented */
#ifdef FEATURE_MAC_SECURITY  
  if(flen->aux_sec_len) {
     uint8_t key_id_size;
     tx_frame_buffer[pos++] = (finfo->aux_hdr.security_control.security_level & 7) |
                              ((finfo->aux_hdr.security_control.key_id_mode & 3) << 3) |
                              ((finfo->aux_hdr.security_control.frame_counter_suppression & 1) << 5) |
                              ((finfo->aux_hdr.security_control.frame_counter_size & 1) << 6);
     if(!finfo->aux_hdr.security_control.frame_counter_suppression)
     {
       tx_frame_buffer[pos++] = (finfo->aux_hdr.frame_counter >> 0) & 0xff;
       tx_frame_buffer[pos++] = (finfo->aux_hdr.frame_counter >> 8) & 0xff;
       tx_frame_buffer[pos++] = (finfo->aux_hdr.frame_counter >> 16) & 0xff;
       tx_frame_buffer[pos++] = (finfo->aux_hdr.frame_counter >> 24) & 0xff;
       if(finfo->aux_hdr.security_control.frame_counter_size)
         tx_frame_buffer[pos++] = (finfo->aux_hdr.frame_counter_ext >> 0) & 0xff;
     }
     if(finfo->aux_hdr.security_control.key_id_mode)
       key_id_size = (finfo->aux_hdr.security_control.key_id_mode-1)*4 + 1;
     else
       key_id_size = 0;

     for(c=0;c<key_id_size;c++)
       tx_frame_buffer[pos++] = finfo->aux_hdr.key[c];
  }
#else
  if(flen->aux_sec_len) {
     pos += flen->aux_sec_len;
  }
#endif
  
 if (flen->hdrie_len >0){
      uint8_t remLen = flen->hdrie_len;
      uint8_t lastIeLen = 0;
      
      while (remLen >0)
      {
        lastIeLen = ie802154e_add_hdrie((ie802154e_hie_t*)finfo->hdrie+lastIeLen, &tx_frame_buffer[pos],BM_BUFFER_SIZE-pos);
        pos += lastIeLen;
        remLen -=lastIeLen;
      }
 }
  return pos;
}

/***************************************************************************//**
 * @fn          hdr_parse
 *
 * @brief      Parses the header.
 *
 * @param[in]  finfo - Pointer to frame802154e_t struct, which specifies the
 *                     frame to send
 *             buf_len - length of the buffer
 *             buf - Pointer to the buffer storing the frame
 *
 * @return     parsed length
 ******************************************************************************/
static inline uint8_t
hdr_parse(uint8_t* buf, uint8_t buf_len, frame802154e_t* finfo)
{
  uint8_t *rx_frame_buf;
  frame802154e_fcf_t fcf;
  uint8_t c;
  extern BM_BUF_t* usmBufBeaconTx;

  rx_frame_buf = buf;

  /* decode the FCF */
  fcf.frame_type = rx_frame_buf[0] & 7;
  fcf.security_enabled = (rx_frame_buf[0] >> 3) & 1;
  fcf.frame_pending = (rx_frame_buf[0] >> 4) & 1;
  fcf.ack_required = (rx_frame_buf[0] >> 5) & 1;
  fcf.panid_compression = (rx_frame_buf[0] >> 6) & 1;

  fcf.seq_num_suppress = (rx_frame_buf[1] >> 0) & 1;
  fcf.ie_list_present = (rx_frame_buf[1] >> 1) & 1;
  fcf.dest_addr_mode = (rx_frame_buf[1] >> 2) & 3;
  fcf.frame_version = (rx_frame_buf[1] >> 4) & 3;
  fcf.src_addr_mode = (rx_frame_buf[1] >> 6) & 3;


  if(fcf.frame_version != MAC_FCF_FRAME_VERSION_B10) {
    if(buf==BM_getBufPtr(usmBufBeaconTx)){
      for(;;);
    }
    return 0;
  }

  /* TODO aux security header, not yet implemented */
#ifndef FEATURE_MAC_SECURITY  //original : #if 1
  if(fcf.security_enabled) {
    return 0;
  }
#endif

  rx_frame_buf += MAC_FCF_FIELD_LEN;   /* Skip first two bytes */

  /* copy fcf and seqNum */
  memcpy(&finfo->fcf, &fcf, sizeof(frame802154e_fcf_t));

  if(!(fcf.seq_num_suppress)) {
    finfo->seq = rx_frame_buf[0];
    rx_frame_buf++;
  }
  
  if ((fcf.panid_compression == 0) 
      || (fcf.dest_addr_mode != ULPSMACADDR_MODE_LONG) 
        || (fcf.src_addr_mode != ULPSMACADDR_MODE_LONG))
  {
    /* Destination PAN */
    finfo->dest_pid = rx_frame_buf[0] + (rx_frame_buf[1] << 8);
    rx_frame_buf += 2;
  }
  
  /* Destination address, if any */
  if(fcf.dest_addr_mode) {
    if(fcf.dest_addr_mode == MAC_ADDR_MODE_SHORT) {
      for(c = 2; c < 8; c++) {
        finfo->dest_addr[c] = 0;
      }
      finfo->dest_addr[0] = rx_frame_buf[0];
      finfo->dest_addr[1] = rx_frame_buf[1];
      rx_frame_buf += 2;
    } else if(fcf.dest_addr_mode == MAC_ADDR_MODE_EXT) {
      for(c = 0; c < 8; c++) {
        finfo->dest_addr[c] = rx_frame_buf[c];
      }
      rx_frame_buf += 8;
    } else if(fcf.dest_addr_mode == MAC_ADDR_MODE_SHORTEST) {
      for(c = 1; c < 8; c++) {
        finfo->dest_addr[c] = 0;
      }
      finfo->dest_addr[0] = rx_frame_buf[0];
      rx_frame_buf += 1;
    }
  } 
  else 
  {
    for(c = 0; c < 8; c++) {
      finfo->dest_addr[c] = 0;
    }
  }
  
  /* Source PAN */
 if ((fcf.panid_compression == 0) 
      && ((fcf.dest_addr_mode != ULPSMACADDR_MODE_LONG) 
        && (fcf.src_addr_mode != ULPSMACADDR_MODE_LONG)))
  {
    finfo->src_pid = rx_frame_buf[0] + (rx_frame_buf[1] << 8);
    rx_frame_buf += 2;
  } 
  else 
  {
    finfo->src_pid = finfo->dest_pid;
  }

  /* Source address, if any */
  if(fcf.src_addr_mode) {
    if(fcf.src_addr_mode == MAC_ADDR_MODE_SHORT) {
      for(c = 2; c < 8; c++) {
        finfo->src_addr[c] = 0;
      }

      finfo->src_addr[0] = rx_frame_buf[0];
      finfo->src_addr[1] = rx_frame_buf[1];
      rx_frame_buf += 2;
    } else if(fcf.src_addr_mode == MAC_ADDR_MODE_EXT) {
      for(c = 0; c < 8; c++) {
        finfo->src_addr[c] = rx_frame_buf[c];
      }
      rx_frame_buf += 8;
    } else if (fcf.src_addr_mode == MAC_ADDR_MODE_SHORTEST) {
      for(c = 1; c < 8; c++) {
        finfo->src_addr[c] = 0;
      }

      finfo->src_addr[0] = rx_frame_buf[0];
      rx_frame_buf += 1;
    }
  } else {
    for(c = 0; c < 8; c++) {
      finfo->src_addr[c] = 0;
    }

    finfo->src_pid = 0;
  }

#ifdef FEATURE_MAC_SECURITY  //original : noexist
    uint8_t key_id_mode;
    uint8_t frame_counter_size;
    uint8_t key_id_size;

  if(fcf.security_enabled) {
    finfo->aux_hdr_offset = rx_frame_buf - buf;
  /* copy aux security header */
    finfo->aux_hdr.security_control.security_level = (rx_frame_buf[0] >> 0) & 7;
    finfo->aux_hdr.security_control.key_id_mode = (rx_frame_buf[0] >> 3) & 3;
    finfo->aux_hdr.security_control.frame_counter_suppression = (rx_frame_buf[0] >> 5) & 1;
    finfo->aux_hdr.security_control.frame_counter_size = (rx_frame_buf[0] >> 6) & 1;
    finfo->aux_hdr.security_control.reserved = 0;
    if(!finfo->aux_hdr.security_control.frame_counter_suppression)
    {
      finfo->aux_hdr.frame_counter = (rx_frame_buf[4] << 24) | (rx_frame_buf[3] << 16) |
                                     (rx_frame_buf[2] << 8) | (rx_frame_buf[1] << 0);
      if(finfo->aux_hdr.security_control.frame_counter_size)
        finfo->aux_hdr.frame_counter_ext = rx_frame_buf[5];
    }

    if(finfo->aux_hdr.security_control.frame_counter_suppression)
      frame_counter_size = 0;
    else if(finfo->aux_hdr.security_control.frame_counter_size)
      frame_counter_size = 5;
    else
      frame_counter_size = 4;

    key_id_mode = (finfo->aux_hdr.security_control.key_id_mode) & 0x03;  //JIRA 44, done
    key_id_size = (key_id_mode == 0 ? 0 : 4*key_id_mode -3);

    if(key_id_size)
    {
        memcpy(&finfo->aux_hdr.key, &rx_frame_buf[frame_counter_size+1], key_id_size);
    }
    rx_frame_buf += (1+frame_counter_size+key_id_size);
  }
#endif
  if(fcf.ie_list_present)
  {
    finfo->hdrie = rx_frame_buf;
    finfo->hdrie_len = ie802154e_hdrielist_len(rx_frame_buf, buf_len-(rx_frame_buf-buf));
    finfo->payloadie = rx_frame_buf + finfo->hdrie_len;
    finfo->payload = finfo->payloadie;
    finfo->payloadie_len = ie802154e_payloadielist_len(rx_frame_buf+finfo->hdrie_len, buf_len-finfo->hdrie_len-(rx_frame_buf-buf));
  }
  else
  {
    finfo->hdrie = NULL;
    finfo->hdrie_len = 0;
    finfo->payloadie = NULL;
    finfo->payload = rx_frame_buf;
    finfo->payloadie_len = 0;
  }

  /* header length */
  c = rx_frame_buf - buf;
  c += finfo->hdrie_len;

  /* In 802.15.4e payload_len = hdr IE len + payload IE len + payload len */
  finfo->payload_len = buf_len - c;
  /* In 802.15.4e payload_ptr = hdr IE ptr (if present) or payload IE ptr (if present) or payload  */
  finfo->payload = rx_frame_buf;

  /* return header length if successful */
  if(c > buf_len&&buf==BM_getBufPtr(usmBufBeaconTx)){
    for(;;);
  }
  return c > buf_len ? 0 : c;
}

/***************************************************************************//**
 * @fn          frame802154e_hdrlen
 *
 * @brief      Calcualtes the header length.
 *
 * @param[in]  p - Pointer to frame802154e_t struct, which specifies the
 *                 frame to send
 * @return     The length of the frame header
 ******************************************************************************/
uint8_t
frame802154e_hdrlen(frame802154e_t *p)
{
  frame802154e_flen_t flen;

  field_len(p, &flen);

  return hdr_len(&flen);
}

uint8_t
frame802154e_hdrlen2(frame802154e_t *p, frame802154e_flen_t* flen)
{
  field_len(p, flen);

  return hdr_len(flen);
}

uint8_t
frame802154e_check_fcf(frame802154e_fcf_t* fcf)
{
  // not supported features
  if(((fcf->src_addr_mode == 0x0) && (fcf->dest_addr_mode == 0x0)) ||
      (fcf->src_addr_mode == MAC_ADDR_MODE_SHORTEST) ||
      (fcf->dest_addr_mode == MAC_ADDR_MODE_SHORTEST)  ) {
    return 0;
  }

  return 1;
}

/***************************************************************************//**
 * @fn          frame802154e_create
 *
 * @brief      Creates the header.
 *
 * @param[in]  p - Pointer to frame802154e_t struct, which specifies the
 *                 frame to send
 *             buf - Pointer to the buffer to store the frame
 *             buf_len - Legnth of the buffer
 * @return     The length of the frame header or 0 if there was
 *             insufficient space in the buffer for the frame headers
 ******************************************************************************/
uint8_t
frame802154e_create(frame802154e_t *p, uint8_t *buf, uint8_t buf_len)
{
  frame802154e_flen_t flen;

  field_len(p, &flen);

  /* check if there is enough room for header */
  if(hdr_len(&flen)> buf_len) {
    return 0;
  }

  return hdr_create(p, &flen, buf);
}

/*----------------------------------------------------------------------------*/
uint8_t
frame802154e_create2(frame802154e_t *p, frame802154e_flen_t* flen, uint8_t *buf, uint8_t buf_len)
{
  /* check if there is enough room for header */
  //if(hdr_len(flen)> buf_len) {
    //return 0;
  //}

  return hdr_create(p, flen, buf);
}

/***************************************************************************//**
 * @fn          frame802154e_parse_header
 *
 * @brief      Parses an input frame.  Scans the input frame to find each
 *             section, and stores the information of each section in a
 *             frame802154_t structure
 *
 * @param[in]  buf - The input buf from the radio chip
 *             buf_len - The size of the input buf
 *             pf - he frame802154e_t struct to store the parsed frame information
 * @return     parsed length
 ******************************************************************************/
uint8_t
frame802154e_parse_header(uint8_t* buf, uint8_t buf_len, frame802154e_t *pf)
{
  if(buf_len < 3) {
    return 0;
  }

  return hdr_parse(buf, buf_len, pf);
}


