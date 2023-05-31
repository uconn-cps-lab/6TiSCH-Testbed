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
* include this software, other than combinations with devices manufactured by or for TI (â€œTI Devicesâ€�). 
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
* THIS SOFTWARE IS PROVIDED BY TI AND TIâ€™S LICENSORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
* BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL TI AND TIâ€™S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
/*
 *  ====================== tsch-db.c =============================================
 *  
 */
#include "mac_config.h"
#include "tsch_os_fun.h"

#ifdef __IAR_SYSTEMS_ICC__
#include <string.h>
#endif

#include "lib/memb.h"
#include "hal_api.h"

#include "tsch-db.h"
#include "tsch-txm.h"

#include "nhldb-tsch-cmm.h"
#include "nhl_mlme_cb.h"
#include "nhl_device_table.h"

/*---------------------------------------------------------------------------*/
TSCH_DB_handle_t db_hnd =
{ {0,0,0,0,0},   // mac_asn[5]
  1,             // join_priority
  0xff,          // mac_disconnect_time
  0,             // num_of_slotframes
  0,             // num_of_tk_entries
  LOWMAC_FALSE,  // is_pan_coord
  0,             // tsch_locked
  LOWMAC_FALSE,  // tsch_started
  LOWMAC_FALSE,  // tsch_sync_inited
  {{LOWMAC_FALSE,}},  // channel hopping
#if PHY_PROP_50KBPS
//  {{0, 0, 1800, 128, 2120, 1120, 800, 1000, 3000, 2800, 192, 12000, 21280, 37000UL}},
  {{0, 0, 1800, 128, 2120, 1120, 800, 1000, 3500, 2800, 192, 12000, 21280, 37000UL}},
#elif PHY_IEEE_MODE
  //{{0, 0, 1800, 128, 2120, 1120, 800, 1000, 2200, 400, 192, 1100, 4256, 8700UL}},
  //{{0, 0, 1800, 128, 1420, 420, 800, 1000, 2200, 400, 192, 1100, 4256, 7200UL}},
  {{0, 0, 1800, 128, 2120, 1120, 800, 1000, 2200, 400, 192, 2400, 4256, 10000UL}},
#endif
  {{LOWMAC_FALSE,}},             // slotframe
  {{LOWMAC_FALSE,}},             // keepalive
  {{LOWMAC_FALSE,}},             // adv_entries
};

/*---------------------------------------------------------------------------*/

MEMB(link_memb, TSCH_DB_link_t, TSCH_MAX_NUM_LINKS);

/*---------------------------------------------------------------------------*/
static inline uint8_t
msb_in_asn_arr(uint16_t* asn_arr)
{
  int8_t   shift;

  if(asn_arr[2] != 0) {
    for(shift=7; shift>=0; shift--) {
      if((asn_arr[2] >> shift) & 0x1) {
        return (shift + 32);
      }
    }
  } else if(asn_arr[1] != 0) {
    for(shift=15; shift>=0; shift--) {
      if((asn_arr[1] >> shift) & 0x1) {
        return (shift + 16);
      }
    }
  } else {
    for(shift=15; shift>=0; shift--) {
      if((asn_arr[0] >> shift) & 0x1) {
        return (shift);
      }
    }
  }

  return 0;
}

/*---------------------------------------------------------------------------*/
static inline uint8_t
get_bit_in_asn_arr(uint16_t* asn_arr, uint8_t pos)
{
  if(pos & 0x20) {
    return ((asn_arr[2] >> (pos - 32)) & 0x1);
  } else if (pos & 0x10) {
    return ((asn_arr[1] >> (pos - 16)) & 0x1);
  }
  return ((asn_arr[0] >> pos) & 0x1);
}

/*---------------------------------------------------------------------------*/
static inline void
set_bit_in_quo_arr(uint16_t* quo_arr, uint8_t pos)
{
  if(pos & 0x20) {
    quo_arr[2] |= (0x1 << (pos - 32));
  } else if (pos & 0x10) {
    quo_arr[1] |= (0x1 << (pos - 16));
  }

  quo_arr[0] |= (0x1 << pos);
}

/*---------------------------------------------------------------------------*/
// asn = quotient / divisor + remainder
uint8_t
TSCH_DB_asn_div(uint64_t asn, uint16_t divisor, uint8_t divisor_msb, uint64_t* quo_out, uint16_t* rem_out)
{
  uint8_t  dividend_msb;
  int8_t   div_shift;

  uint16_t asn_arr[3];
  uint16_t quo_arr[3];

  uint16_t remainder;

  if(divisor > asn) {
    *quo_out = 0;
    *rem_out = asn;
    return LOWMAC_TRUE;
  }

  if(divisor == asn) {
    *quo_out = 1;
    *rem_out = 0;
    return LOWMAC_TRUE;
  }

  asn_arr[0] = asn & 0xffff;
  asn_arr[1] = (asn >> 16) & 0xffff;
  asn_arr[2] = (asn >> 32) & 0x00ff;

  dividend_msb = msb_in_asn_arr(asn_arr);
  quo_arr[0] = quo_arr[1] = quo_arr[2] = 0;
  remainder = (uint16_t) (asn >> (dividend_msb - divisor_msb));
  for(div_shift=(dividend_msb - divisor_msb); div_shift >= 0; div_shift--) {

    if(remainder >= divisor) {
      remainder -= divisor;
      set_bit_in_quo_arr(quo_arr, div_shift);
    }

    if(div_shift > 0) {
      remainder = (remainder << 1) & 0xfffe;
      //remainder |= ((asn >> (div_shift - 1)) & 0x1);
      remainder |= get_bit_in_asn_arr(asn_arr, (div_shift - 1));
    }
  }

  *quo_out = (quo_arr[2] & 0x00ff);
  *quo_out = (*quo_out << 16);
  *quo_out += (quo_arr[1] & 0xffff);
  *quo_out = (*quo_out << 16);
  *quo_out += (quo_arr[0] & 0xffff);

  *rem_out = remainder;

  return LOWMAC_TRUE;
}

/*---------------------------------------------------------------------------*/
// asn = quotient / divisor + remainder
uint8_t
TSCH_DB_asn_mod(uint64_t asn, uint16_t divisor, uint8_t divisor_msb, uint16_t* rem_out)
{
  uint8_t  dividend_msb;
  int8_t   div_shift;

  uint16_t asn_arr[3];
  uint16_t remainder;

  if(divisor > asn) {
    *rem_out = asn;
    return LOWMAC_TRUE;
  }

  if(divisor == asn) {
    *rem_out = 0;
    return LOWMAC_TRUE;
  }

  asn_arr[0] = asn & 0xffff;
  asn_arr[1] = (asn >> 16) & 0xffff;
  asn_arr[2] = (asn >> 32) & 0x00ff;

  dividend_msb = msb_in_asn_arr(asn_arr);

  remainder = (uint16_t) (asn >> (dividend_msb - divisor_msb));
  for(div_shift=(dividend_msb - divisor_msb); div_shift >= 0; div_shift--) {

    if(remainder >= divisor) {
      remainder -= divisor;
    }

    if(div_shift > 0) {
      remainder = (remainder << 1) & 0xfffe;
      //remainder |= ((asn >> (div_shift - 1)) & 0x1);
      remainder |= get_bit_in_asn_arr(asn_arr, (div_shift - 1));
    }
  }

  *rem_out = remainder;

  return LOWMAC_TRUE;
}

/**************************************************************************************************//**
 *
 * @brief       Get the 1st macHoppingSequence Length output of 9 bit LFSR
 *
 * @param[out]  pShuffle - pointer to the shuffle 
 * @param[in]   shuffleLength - shuffle length 
 *
 * @return
 *
 ***************************************************************************************************/
void getShuffle(uint16_t* pShuffle, uint8_t shuffuleLength)
{
  // x^9 + x^5 + 1
  // bit  b8 b7 b6 b5 b4 b3 b2 b1 b0
  // x^n  x1 x2 x3 x4 x5 x6 x7 x8 x9
  
  unsigned char bit_low = 0xff;  // bit 0 ~ 7 = x9 ~ x2
  unsigned short bit8 = 0x0;     // bit 8 = x1
  uint8_t count=0;
  
  while(count < shuffuleLength)
  {
    unsigned char new_bit4;       // bit 4 = x5
    unsigned char new_bit8;       // bit 8 = x1
    
    // x5 = x9 XOR x4, therefore bit4 = bit0 XOR bit5
    new_bit4 = (bit_low & 0x1) ^ ((bit_low >> 5) & 0x1);
    // x1 = x9, therefore bit8 = bit0
    new_bit8 = (bit_low & 0x1);
    
    bit_low = ((bit_low >> 1) & 0x7f) + ((bit8 << 7) & 0x80);
    bit_low = (bit_low & 0xef) + ((new_bit4 << 4) & 0x10);
    bit8 = new_bit8;
    
    *pShuffle = ((bit8 << 8) & 0x100) + bit_low;
    count++;
    pShuffle++;
  } 
}

/**************************************************************************************************//**
 *
 * @brief       This is used to set up the default channel hopping sequence in DB
 *
 * @param[in]
 *
 *
 * @return
 *
 ***************************************************************************************************/
static void set_default_channel_hopping(void)
{
  TSCH_DB_channel_hopping_t* chop_db;
  TSCH_PIB_channel_hopping_t* chop_pib;
  const uint8_t default_chop_id = 0;
  uint8_t  i, j;
  uint32_t ch_mask;
  uint32_t supported_channels[HAL_RADIO_NUM_BITMAP];
  uint32_t cur_channels;

//  uint8_t swapIndex;
#if PHY_IEEE_MODE
  uint16_t shuffle[16];
  getShuffle(shuffle,16);
#elif PHY_PROP_50KBPS
#if SUBGHZ_915
  uint16_t shuffle[64]={0};
  getShuffle(shuffle,64);
#elif SUBGHZ_868
  uint16_t shuffle[35]={0};
   getShuffle(shuffle,35);
#endif
#endif

  chop_db = &db_hnd.channel_hopping[default_chop_id];
  chop_db->in_use = LOWMAC_TRUE;

  chop_pib = &chop_db->vals;
  chop_pib->hopping_sequence_id = default_chop_id;

  supported_channels[0]=HAL_RADIO_CHANNEL_SUPPORTED_0_31;
  supported_channels[1]=HAL_RADIO_CHANNEL_SUPPORTED_32_63;
  supported_channels[2]=HAL_RADIO_CHANNEL_SUPPORTED_64_95;
  supported_channels[3]=HAL_RADIO_CHANNEL_SUPPORTED_96_127;
  supported_channels[4]=HAL_RADIO_CHANNEL_SUPPORTED_128_159;

  chop_pib->number_of_channels = 0;

  j=0;
  for (i=0; i< HAL_RADIO_NUM_BITMAP; i++)
  {
      cur_channels = (uint32_t)supported_channels[i];
      for(j=0; j<HAL_RADIO_CHANNEL_BITMAP_LEN;j++)
      {
          ch_mask = (uint32_t) (0x01UL << j);
          if(cur_channels & ch_mask)
          {
              chop_pib->hopping_sequence_list[chop_pib->number_of_channels]
                  = j+i*HAL_RADIO_CHANNEL_BITMAP_LEN;
              chop_pib->number_of_channels++;
          }
      }
  }

  chop_pib->hopping_sequence_length = chop_pib->number_of_channels;
/*
  for (i=0;i<chop_pib->hopping_sequence_length;i++)
  {
      swapIndex = shuffle[i]%chop_pib->hopping_sequence_length;
      if ((chop_pib->hopping_sequence_list[i]) != (chop_pib->hopping_sequence_list[swapIndex]))
      {
          chop_pib->hopping_sequence_list[i]= (chop_pib->hopping_sequence_list[i])^(chop_pib->hopping_sequence_list[swapIndex]);
          chop_pib->hopping_sequence_list[swapIndex] = (chop_pib->hopping_sequence_list[i])^(chop_pib->hopping_sequence_list[swapIndex]);
          chop_pib->hopping_sequence_list[i]= (chop_pib->hopping_sequence_list[i])^(chop_pib->hopping_sequence_list[swapIndex]);
      }
  }*/
  //Tao: disabled shuffling so we have known mapping
}

/**************************************************************************************************//**
 *
 * @brief       This is used to find the slotframe data structure based in slotframe ID
 *
 * @param[in]   sf_handle -- id of time slot frame
 *
 *
 * @return      pointer of TSCH Slotframe  data structure
 *
 ***************************************************************************************************/
static TSCH_DB_slotframe_t* find_slotframe(uint8_t sf_handle)
{
  uint8_t iter;
  TSCH_DB_slotframe_t* cur_sf;

  for(iter=0; iter<TSCH_DB_MAX_SLOTFRAMES; iter++) {
    cur_sf = &db_hnd.slotframes[iter];
    if( (cur_sf->in_use == LOWMAC_TRUE) &&
        (cur_sf->vals.slotframe_handle == sf_handle)) {
      return cur_sf;
    }
  }

  return NULL;
}

/**************************************************************************************************//**
 *
 * @brief       This is used to create new keepalive entry in DB
 *
 * @param[in]   in_keepalive -- pointer of TSCH keepalive PIB data structure
 *
 *
 * @return      pointer of added keepalive DB data structure
 *
 ***************************************************************************************************/
static TSCH_DB_ti_keepalive_t* new_keepalive_entry(TSCH_PIB_ti_keepalive_t* in_keepalive)
{
    TSCH_DB_ti_keepalive_t* ka_entry;
    uint32_t ka_tot_ms;
    uint16_t ka_sec;
    uint16_t ka_ms;

    ka_entry = NULL;

    if(db_hnd.keepalive.in_use == LOWMAC_TRUE)
    {
        return NULL;
    }
    else
    {
        ka_entry = &db_hnd.keepalive;
    }

    ka_entry->usm_buf = NULL;

    ka_tot_ms = in_keepalive->period;
    ka_sec = ka_tot_ms / 1000UL;
    ka_ms = ka_tot_ms % 1000UL;

    if(ka_sec != 0)
    {
        ka_entry->ka_ct = (CLOCK_SECOND * ka_sec);
    }
    else
    {
        ka_entry->ka_ct = (CLOCK_SECOND * ka_ms) / 1000UL;
    }

    ka_entry->vals.dst_addr_short = in_keepalive->dst_addr_short;
    ka_entry->vals.period = in_keepalive->period;

    ka_entry->in_use = LOWMAC_TRUE;
    return ka_entry;
}

/**************************************************************************************************//**
 *
 * @brief       This is used to add the slotframe data structure to DB
 *
 *
 * @param[in]   sf_pib    -- pointer of TSCH slotframe PIB data structure
 *
 *
 * @return      pointer of added slotframe DB data structure
 *
 ***************************************************************************************************/
static inline TSCH_DB_slotframe_t* new_slotframe_entry(TSCH_PIB_slotframe_t* sf_pib)
{
  uint8_t iter;
  TSCH_DB_slotframe_t* cur_sf;

  for(iter=0; iter<TSCH_DB_MAX_SLOTFRAMES; iter++) {
    cur_sf = &db_hnd.slotframes[iter];
    if(cur_sf->in_use != LOWMAC_TRUE) {
      int8_t shift;

      cur_sf->vals.slotframe_handle = sf_pib->slotframe_handle;
      cur_sf->vals.slotframe_size = sf_pib->slotframe_size;

      /* initialize the link list */
      cur_sf->link_list_list = NULL;
      cur_sf->link_list = (list_t) &(cur_sf->link_list_list);
      list_init(cur_sf->link_list);

      cur_sf->size_msb = 0;
      for(shift = 15; shift >= 0; shift--) {
        if((cur_sf->vals.slotframe_size >> shift) & 0x1) {
          cur_sf->size_msb = shift;
          break;
        }
      }

      cur_sf->in_use = LOWMAC_TRUE;
      db_hnd.num_of_slotframes++;

      /* BZ 15201 */
      /* KWANG : avoid wrap around */
      if(db_hnd.num_of_slotframes == 0)
        db_hnd.num_of_slotframes = 1;

      return cur_sf;
    }
  }
  return NULL;
}

/**************************************************************************************************//**
 *
 * @brief       This is used to delete the slotframe data structure
 *
 *
 * @param[in]   sf_db -- pointer of slotframe data structure *
 *
 * @return
 *
 ***************************************************************************************************/
void del_slotframe_entry(TSCH_DB_slotframe_t* sf_db)
{
  sf_db->in_use = LOWMAC_FALSE;

  /* BZ 15201 */
  /* KWANG : avoid wrap around */
  if(db_hnd.num_of_slotframes == 0)
    db_hnd.num_of_slotframes = 1;

  db_hnd.num_of_slotframes--;
}

/**************************************************************************************************//**
 *
 * @brief       This is used to modify the slotframe data structure
 *
 *
 * @param[in]   slotframe -- pointer of slotframe data structure
 *              sf_pib    -- pointer of TSCH slotframe PIB data structure
 *
 *
 * @return
 *
 ***************************************************************************************************/
static LOWMAC_status_e modify_slotframe_entry(TSCH_DB_slotframe_t* sf_db, TSCH_PIB_slotframe_t* sf_pib)
{
  if(sf_db->vals.slotframe_size != sf_pib->slotframe_size) {
    uint8_t size_msb;
    int8_t  shift;

    size_msb = 0;
    for(shift = 15; shift >= 0; shift--) {
      if((sf_db->vals.slotframe_size >> shift) & 0x1) {
        size_msb = shift;
        break;
      }
    }

    sf_db->vals.slotframe_size = sf_pib->slotframe_size;
    sf_db->size_msb = size_msb;
  }
  return LOWMAC_STAT_SUCCESS;
}


/**************************************************************************************************//**
 *
 * @brief       This is used to push the TSCH DB link to data base
 *
 *
 * @param[in]   slotframe -- pointer of slotframe data structure *
 *              new_link    -- pointer of TSCH DB link data structure
 *
 *
 * @return
 *
 ***************************************************************************************************/
static void push_link(TSCH_DB_slotframe_t* slotframe, TSCH_DB_link_t* new_link)
{
  /* sorted linked list based on timeslot id */
  TSCH_DB_link_t* prev_link;
  TSCH_DB_link_t* next_link;

  prev_link = list_head(slotframe->link_list);
  if((!prev_link) || (prev_link->vals.timeslot > new_link->vals.timeslot)) {
    list_push(slotframe->link_list, new_link);
    return;
  }


  next_link = list_item_next(prev_link);
  while(prev_link) {

    if((!next_link) || (next_link->vals.timeslot > new_link->vals.timeslot)) {
      list_insert(slotframe->link_list, prev_link, new_link);
      return;
    }

    prev_link = next_link;
    next_link = list_item_next(prev_link);
  }
}

/**************************************************************************************************//**
 *
 * @brief       This is used to add the link entry in data base
 *
 *
 * @param[in]   db_sf -- pointer of slotframe data structure *
 *              link_pib    -- pointer of TSCH PIB link data structure
 *
 *
 * @return      pointer to the added TSCH link data structure in DB
 *
 ***************************************************************************************************/
static inline TSCH_DB_link_t* new_link_entry(TSCH_DB_slotframe_t* sfDB, TSCH_PIB_link_t* linkPIB)
{
  TSCH_DB_link_t* newLink;
  TSCH_DB_link_t* existingLink;
  uint8_t foundSimilarLink;

  newLink = memb_alloc(&link_memb);
  if(!newLink)
  {
    return NULL;
  }

  foundSimilarLink=0;

  existingLink = list_head(sfDB->link_list);

  while(existingLink)
  {
    if ((existingLink->vals.peer_addr == linkPIB->peer_addr) &&
        (existingLink->vals.link_option == linkPIB->link_option) &&
         (existingLink->vals.link_type == linkPIB->link_type))
    {
        foundSimilarLink=1;
        break;
    }
    /* link->vals.timeslot > timeslot_id */
    existingLink = list_item_next(existingLink);
  }

  /* init link list */
  newLink->next = NULL;

  /* init queued pkt list */
  if (foundSimilarLink)
  {
      newLink->qdpkt_list = existingLink->qdpkt_list;
  }
  else
  {
      newLink->qdpkt_list_list = NULL;
      newLink->qdpkt_list = (list_t) &(newLink->qdpkt_list_list);
  }

  newLink->vals.timeslot = linkPIB->timeslot;
  newLink->vals.link_option = linkPIB->link_option;

  newLink->slotframe = sfDB;

  push_link(sfDB, newLink);

  sfDB->number_of_links++;

  return newLink;
}

/**************************************************************************************************//**
 *
 * @brief       This is used to delete the link entry in data base
 *
 *
 * @param[in]   db_sf -- pointer of slotframe data structure
 *              link_db -- pointer of TSCH Link data structure *
 *
 *
 * @return
 *
 ***************************************************************************************************/
void del_link_entry(TSCH_DB_slotframe_t* sf_db, TSCH_DB_link_t* link_db)
{
  list_remove(sf_db->link_list, link_db);
  sf_db->number_of_links--;
  memb_free(&link_memb, link_db);
}

/**************************************************************************************************//**
 *
 * @brief       This is used to modify the link entry in data base
 *
 *
 * @param[in]   db_sf -- pointer of slotframe data structure
 *              link_db -- pointer of TSCH Link data structure
 *              link_pib    -- pointer of TSCH PIB link data structure
 *
 *
 * @return      status of operation
 *
 ***************************************************************************************************/
static LOWMAC_status_e modify_link_entry(TSCH_DB_slotframe_t* sf_db, TSCH_DB_link_t* link_db, TSCH_PIB_link_t* link_pib)
{

  if((link_db->vals.peer_addr != link_pib->peer_addr) ||
     (link_db->vals.link_type != link_pib->link_type) ){
    // currently we do not support modify of above properties
    // which causes delayed modification
    // Do delete and then add for such properties
    return LOWMAC_STAT_INVALID_PARAMETER;
  }

  if(link_db->vals.channel_offset != link_pib->channel_offset) {
    link_db->vals.channel_offset = link_pib->channel_offset;
  }

  if(link_db->vals.link_option != link_pib->link_option) {
    link_db->vals.link_option = link_pib->link_option;
  }

  if(link_db->vals.timeslot != link_pib->timeslot) {
    link_db->vals.timeslot = link_pib->timeslot;
    list_remove(sf_db->link_list, link_db);
    push_link(sf_db, link_db);
  }

  return LOWMAC_STAT_SUCCESS;
}

/**************************************************************************************************//**
 *
 * @brief       This is used to find the link based on link ID
 *
 *
 * @param[in]   db_sf -- pointer of slotframe data structure
 *              timeslot_id    -- timeslot ID
 *
 *
 * @return      pointer of found link data structure
 *
 ***************************************************************************************************/
static TSCH_DB_link_t* find_link_by_id(TSCH_DB_slotframe_t* db_sf, uint16_t link_id)
{
  TSCH_DB_link_t* link;

  link = list_head(db_sf->link_list);
  while(link) {

    if(link->vals.link_id == link_id) {
      return link;
    }

    link = list_item_next(link);
  }
  return NULL;

}


/**************************************************************************************************//**
 *
 * @brief       This is used to find the link based on current time slot
 *
 *
 * @param[in]   slotframe -- pointer of slotframe data structure
 *              timeslot_id    -- timeslot ID
 *
 *
 * @return      pointer of found link data structure
 *
 ***************************************************************************************************/
TSCH_DB_link_t*
TSCH_DB_find_match_link(TSCH_DB_slotframe_t* slotframe, uint64_t asn, uint16_t timeslot)
{
  TSCH_DB_link_t* link;
  uint8_t periodOffset;
  uint16_t slot_frame_size;

  /* Empty link return */
  if(!(link = list_head(slotframe->link_list)))
  {
     return NULL;
  }

  slot_frame_size = TSCH_DB_SLOTFRAME_SIZE_DIR(slotframe);

  while(link)
  {
    if (link->vals.timeslot == timeslot)
    {
      periodOffset = (asn/slot_frame_size)%link->vals.period;

      if (link->vals.periodOffset == periodOffset)
      {
         return link;
      }
    }

    /* link->vals.timeslot > timeslot_id */
    link = list_item_next(link);
  }
  return NULL;

}

/**************************************************************************************************//**
 *
 * @brief       This is used to find the next time slot in link database based on slotframe ID,
 *              and timeslot ID.
 *
 *
 * @param[in]   slotframe -- pointer of slotframe data structure
 *              timeslot_id    -- timeslot ID
 *              sf_size   -- size (number of time slot ) of slotframe
 *
 *
 * @return      next timeslot ID
 *
 ***************************************************************************************************/
uint64_t TSCH_DB_find_next_link(TSCH_DB_slotframe_t* slotframe, 
                                uint64_t curASN, uint8_t* nextLinkOption)
{
  TSCH_DB_link_t* link;
  uint64_t min_diff = 0xffffffffffffffff;
  uint64_t diff;
  uint64_t linkNextASN;
  uint64_t nextASN = 0;
  uint64_t periodStartASN;
  uint16_t slot_frame_size;
  uint16_t periodSize=0;

  /* Empty link return */
  if(!(link = TSCH_DB_SLOTFRAME_LINK_HEAD(slotframe))) {
    return nextASN;
  }

  slot_frame_size =TSCH_DB_SLOTFRAME_SIZE_DIR(slotframe);

  while(link)
  {
      periodSize = slot_frame_size*(link->vals.period);
      periodStartASN = curASN/periodSize*periodSize;
      linkNextASN = periodStartASN
        + slot_frame_size*(link->vals.periodOffset)
        +TSCH_DB_LINK_TIMESLOT(link);

      while (linkNextASN <= curASN)
      {
        linkNextASN += periodSize;
      }

      diff = linkNextASN - curASN;

      if ((diff != 0) && (diff < min_diff))
      {
        min_diff = diff;
        nextASN = linkNextASN;
        *nextLinkOption = TSCH_DB_LINK_OPTIONS(link);
      }
      link = list_item_next(link);
  }
  return nextASN;

}

/**************************************************************************************************//**
 *
 * @brief       (????)
 *
 *
 *
 * @param[in]   in_keepalive -- pointer of Keepalive data structure
 *
 * @return      keepalive entry in DB
 *
 ***************************************************************************************************/
static TSCH_DB_ti_keepalive_t* find_keepalive_entry(TSCH_PIB_ti_keepalive_t* in_keepalive)
{
    TSCH_DB_ti_keepalive_t* ka_entry;

    ka_entry = &db_hnd.keepalive;
    if((ka_entry->in_use == LOWMAC_TRUE) &&
       (ka_entry->vals.dst_addr_short == in_keepalive->dst_addr_short) )
    {
       return ka_entry;
    }
    return NULL;
}
/**************************************************************************************************//**
 *
 * @brief       (????)
 *
 *
 *
 * @param[in]   shortAddr: short address of the node
 *
 * @return      None
 *
 ***************************************************************************************************/
static void del_node(uint16_t shortAddr)
{
   uint16_t iter;
   TSCH_DB_slotframe_t *sf_db;
   TSCH_DB_link_t *link_db, *link_prev, *link_next;

   for(iter = 0; iter < TSCH_DB_MAX_SLOTFRAMES; iter++)
   {
      sf_db = &(db_hnd.slotframes[iter]);
      if (sf_db->in_use)
      {
         link_prev = NULL;
         link_db = list_head(sf_db->link_list);
         while(link_db)
         {
            link_next = list_item_next(link_db);
            if(link_db->vals.peer_addr == shortAddr)
            {
               TXM_freeQueuedTxPackets(link_db);

               if (link_prev == NULL)
               {
                  list_pop(sf_db->link_list);
               }
               else
               {
                  link_prev->next = link_next;
               }
               sf_db->number_of_links--;
               link_db->next = NULL;
               NHLDB_TSCH_delete_link_handle(link_db->vals.slotframe_handle, link_db->vals.link_id);
               memb_free(&link_memb, link_db);
            }
            else
            {
               link_prev = link_db;
            }
            link_db = link_next;
         }
      }
   }
#if IS_ROOT
   NHL_markDepartedTableEntry(shortAddr);  //The device has left, mark the table entry as available for recycling if necessary
#else
   NHL_deleteTableEntry(shortAddr, NULL);  //Delete the table entry
#endif
}

/**************************************************************************************************//**
 *
 * @brief       This is used to init the TSCH database
 *
 *
 * @param[in]
 *
 * @return      status of this operation
 *
 ***************************************************************************************************/
TSCH_DB_handle_t* TSCH_DB_init(void)
{
  uint16_t iter;

  uint32_t key = Task_disable();

  memb_init(&link_memb);

  // reset ASN
  TSCH_DB_set_mac_asn(&db_hnd, 0UL);

  for(iter=0; iter<TSCH_DB_MAX_SLOTFRAMES; iter++) {
    db_hnd.slotframes[iter].in_use = LOWMAC_FALSE;
  }

  db_hnd.keepalive.in_use = LOWMAC_FALSE;
  db_hnd.keepalive.usm_buf = NULL;
  
  for(iter=0; iter<TSCH_DB_MAX_ADV_ENTRIES; iter++) {
    db_hnd.adv_entries[iter].in_use = LOWMAC_FALSE;
  }

  set_default_channel_hopping();

  Task_restore(key);
  return &db_hnd;
}

/**************************************************************************************************//**
 *
 * @brief       This is used to set is_pan_coord attribute
 *
 *
 * @param[in]
 *
 *
 * @return      status of this operation
 *
 ***************************************************************************************************/
LOWMAC_status_e TSCH_DB_set_pan_coord(void)
{

  uint32_t key = Task_disable();
  db_hnd.is_pan_coord = LOWMAC_TRUE;
  Task_restore(key);
  return LOWMAC_STAT_SUCCESS;
}

/**************************************************************************************************//**
 *
 * @brief       This is used to get the TSCH DB content
 *
 *
 * @param[in]   attrib_id -- PIB ID of data base attribute
 *              in_val    -- pointer to attribute data
 *              in_size   -- pointer of size of attribute value (???)
 *
 *
 *
 * @return      status of this operation
 *
 ***************************************************************************************************/
LOWMAC_status_e TSCH_DB_get_db(uint16_t attrib_id, void* in_val, uint16_t* in_size)
{
  LOWMAC_status_e res = LOWMAC_STAT_SUCCESS;
  uint8_t* db_value = NULL;
  uint16_t db_size = 0;

  uint32_t key = Task_disable();

  switch(attrib_id) {
    case TSCH_DB_ATTRIB_ASN:
    {
      db_value = (uint8_t *) &db_hnd.mac_asn[0];
      db_size = TSCH_PIB_ASN_LEN;
      break;
    }
    case TSCH_DB_ATTRIB_JOIN_PRIO:
    {
      db_value = (uint8_t *) &db_hnd.join_priority;
      db_size = sizeof(db_hnd.join_priority);
      break;
    }

    case TSCH_DB_ATTRIB_DISCON_TIME:
    {
      db_value = (uint8_t *) &db_hnd.mac_disconnect_time;
      db_size = sizeof(db_hnd.mac_disconnect_time);
      break;
    }
    case TSCH_DB_ATTRIB_CHAN_HOP:
    {
      TSCH_PIB_channel_hopping_t* in_chhop;

      in_chhop = (TSCH_PIB_channel_hopping_t *) in_val;

      if(in_chhop->hopping_sequence_id >= TSCH_DB_MAX_CHANNEL_HOPPING) {
        res = LOWMAC_STAT_MAX_CHHOP_EXCEEDED;
        break;
      }

      if(db_hnd.channel_hopping[in_chhop->hopping_sequence_id].in_use == LOWMAC_FALSE) {
        res = LOWMAC_STAT_INVALID_PARAMETER;
        break;
      }

      db_value = (uint8_t *) &db_hnd.channel_hopping[in_chhop->hopping_sequence_id].vals;
      db_size = sizeof(TSCH_PIB_channel_hopping_t);
      break;
    }
    case TSCH_DB_ATTRIB_TIMESLOT:
    {
      db_value = (uint8_t *) &db_hnd.timeslot_template.vals;
      db_size = sizeof(TSCH_PIB_timeslot_template_t);
      break;
    }
    case TSCH_DB_ATTRIB_SLOTFRAME:
    {
      TSCH_PIB_slotframe_t* in_slotframe;
      TSCH_DB_slotframe_t* db_slotframe;

      in_slotframe = (TSCH_PIB_slotframe_t *) in_val;

      db_slotframe = find_slotframe(in_slotframe->slotframe_handle);
      if(!db_slotframe) {
        res = LOWMAC_STAT_INVALID_PARAMETER;
        break;
      }

      db_value = (uint8_t *)&db_slotframe->vals;
      db_size = sizeof(TSCH_PIB_slotframe_t);

      break;
    }
    case TSCH_DB_ATTRIB_LINK:
    {
      TSCH_DB_slotframe_t* db_slotframe;
      TSCH_PIB_link_t* in_link;
      TSCH_DB_link_t*  new_link;

      in_link = (TSCH_PIB_link_t *) in_val;

      db_slotframe = find_slotframe(in_link->slotframe_handle);
      if(!db_slotframe) {
        res = LOWMAC_STAT_INVALID_PARAMETER;
        break;
      }

      new_link = find_link_by_id(db_slotframe, in_link->link_id);
      if(!new_link) {
        res = LOWMAC_STAT_MAX_LINKS_EXCEEDED;
        break;
      }

      db_value = (uint8_t *) &new_link->vals;
      db_size = sizeof(TSCH_PIB_link_t);

      break;
    }
    default:
    {
      res = LOWMAC_STAT_DB_BAD_ATTRIB_ID;
      break;
    }
  }

  if(db_value) {
    memcpy(in_val, db_value, db_size);
    *in_size = db_size;
  }
  Task_restore(key);
  return res;
}

/**************************************************************************************************//**
 *
 * @brief       This is used to set the TSCH DB content
 *
 *
 * @param[in]   attrib_id -- PIB ID of data base attribute
 *              in_val    -- pointer to attribute data
 *              in_size   -- pointer of size of attribute value (???) *
 *
 *
 * @return      status of this operation
 *
 ***************************************************************************************************/

LOWMAC_status_e TSCH_DB_set_db(uint16_t attrib_id, void* in_val, uint16_t* in_size)
{
  LOWMAC_status_e res = LOWMAC_STAT_SUCCESS;
  uint8_t* db_value = NULL;
  uint16_t db_size = 0;

  uint32_t key = Task_disable();

  switch(attrib_id) {
    case TSCH_DB_ATTRIB_ASN:
    {
      if(db_hnd.tsch_sync_inited) {
        res = LOWMAC_STAT_DB_RDONLY;
      } else {
        // ASN is special case
        db_value = (uint8_t *) &db_hnd.mac_asn[0];
        db_size = TSCH_PIB_ASN_LEN;
        Task_restore(key);
        return res;
      }
      break;
    }
    case TSCH_DB_ATTRIB_JOIN_PRIO:
    {

      db_value = (uint8_t *) &db_hnd.join_priority;
      db_size = sizeof(db_hnd.join_priority);
      break;
    }

    case TSCH_DB_ATTRIB_DISCON_TIME:
    {
      db_value = (uint8_t *) &db_hnd.mac_disconnect_time;
      db_size = sizeof(db_hnd.mac_disconnect_time);
      break;
    }
    case TSCH_DB_ATTRIB_CHAN_HOP:
    {
      TSCH_PIB_channel_hopping_t* in_chhop;

      in_chhop = (TSCH_PIB_channel_hopping_t *) in_val;

      if(in_chhop->hopping_sequence_id >= TSCH_DB_MAX_CHANNEL_HOPPING) {
        res = LOWMAC_STAT_MAX_CHHOP_EXCEEDED;
        break;
      }

      if(db_hnd.channel_hopping[in_chhop->hopping_sequence_id].in_use == LOWMAC_TRUE) {
        res = LOWMAC_STAT_INVALID_PARAMETER;
        break;
      }

      db_hnd.channel_hopping[in_chhop->hopping_sequence_id].in_use = LOWMAC_TRUE;
      db_value = (uint8_t *) &db_hnd.channel_hopping[in_chhop->hopping_sequence_id].vals;
      db_size = sizeof(TSCH_PIB_channel_hopping_t);
      break;
    }
    case TSCH_DB_ATTRIB_TIMESLOT:
    {
      db_value = (uint8_t *) &db_hnd.timeslot_template.vals;
      db_size = sizeof(TSCH_PIB_timeslot_template_t);
      break;
    }

    case TSCH_DB_ATTRIB_SLOTFRAME:
    {
      TSCH_PIB_slotframe_t* in_slotframe;
      TSCH_DB_slotframe_t*  db_slotframe;

      in_slotframe = (TSCH_PIB_slotframe_t *) in_val;

      db_slotframe = find_slotframe(in_slotframe->slotframe_handle);
      if(db_slotframe) {
        // already exist, error in ADD
        res = LOWMAC_STAT_INVALID_PARAMETER;
        break;
      }

      db_slotframe = new_slotframe_entry(in_slotframe);
      if(!db_slotframe) {
        // no available slotframe entry
        res = LOWMAC_STAT_MAX_SLOTFRAMES_EXCEEDED;
        break;
      }

      db_value = (uint8_t *)&db_slotframe->vals;
      db_size = sizeof(TSCH_PIB_slotframe_t);

      break;
    }
    case TSCH_DB_ATTRIB_LINK:
    {
      TSCH_PIB_link_t* in_link;

      TSCH_DB_slotframe_t* db_slotframe;
      TSCH_DB_link_t* new_link;

      in_link = (TSCH_PIB_link_t *) in_val;

      db_slotframe = find_slotframe(in_link->slotframe_handle);
      if(!db_slotframe) {
        res = LOWMAC_STAT_INVALID_PARAMETER;
        break;
      }

      new_link = new_link_entry(db_slotframe, in_link);
      if(!new_link) {
        res = LOWMAC_STAT_MAX_LINKS_EXCEEDED;
        break;
      }

      /* TODO: Need to check duplicated link id */
      db_value = (uint8_t *) &new_link->vals;
      db_size = sizeof(TSCH_PIB_link_t);

      break;
    }
    case TSCH_DB_ATTRIB_TI_KEEPALIVE:
    {
      TSCH_PIB_ti_keepalive_t* in_keepalive;
      TSCH_DB_ti_keepalive_t*  db_keepalive;

      in_keepalive = (TSCH_PIB_ti_keepalive_t *) in_val;

      db_keepalive = find_keepalive_entry(in_keepalive);
      if(db_keepalive)
      {
        // Already exist
        res = LOWMAC_STAT_INVALID_PARAMETER;
        break;
      }

      db_keepalive = new_keepalive_entry(in_keepalive);
      if(!db_keepalive)
      {
        // Used up all the array
        res = LOWMAC_STAT_INVALID_PARAMETER;
        break;
      }

      SN_clockSetFunc(ka_ctimer,  TSCH_TXM_send_keepalive_pkt, db_keepalive);
      SN_clockSet(ka_ctimer, 0, db_keepalive->ka_ct);

      break;
    }
    default:
    {
      res = LOWMAC_STAT_DB_BAD_ATTRIB_ID;
      break;
    }
  }

  if(db_value) {
    memcpy(db_value, in_val, db_size);
    *in_size = db_size;
  }
  Task_restore(key);
  return res;
}

/**************************************************************************************************//**
 *
 * @brief       This is used to delete the TSCH DB content
 *
 *
 * @param[in]   attrib_id -- PIB ID of data base attribute
 *              in_val    -- pointer to attribute data
 *              in_size   -- pointer of size of attribute value (???)
 *              db_callback -- callback function after completion
 *
 *
 * @return      status of this operation
 *
 ***************************************************************************************************/
LOWMAC_status_e TSCH_DB_delete_db(uint16_t attrib_id, void* in_val, uint16_t* in_size)
{
  LOWMAC_status_e res = LOWMAC_STAT_SUCCESS;

  uint32_t key = Task_disable();

  switch(attrib_id) {
    case TSCH_DB_ATTRIB_SLOTFRAME:
    {
      TSCH_PIB_slotframe_t* in_slotframe;
      TSCH_DB_slotframe_t*  db_slotframe;
      TSCH_DB_link_t*       db_link;

      in_slotframe = (TSCH_PIB_slotframe_t *) in_val;

      db_slotframe = find_slotframe(in_slotframe->slotframe_handle);
      if(!db_slotframe) {
        res = LOWMAC_STAT_SLOTFRAME_NOT_FOUND;
        break;
      }
      
      TSCH_TXM_freePkts();
      
      db_link = list_head(db_slotframe->link_list);
      while(db_link) 
      {
        del_link_entry(db_slotframe, db_link);
        db_link = list_item_next(db_link);
      }
      
      del_slotframe_entry(db_slotframe);
      res = LOWMAC_STAT_SUCCESS;
     
      break;
    }
    case TSCH_DB_ATTRIB_LINK:
    {
      TSCH_PIB_link_t*     in_link;

      TSCH_DB_slotframe_t* db_slotframe;
      TSCH_DB_link_t*      db_link;

      in_link = (TSCH_PIB_link_t *) in_val;

      if (in_link->timeslot == 0xFFFE) //delete everything related to a node after it has departed
      {
         del_node(in_link->peer_addr);
         res = LOWMAC_STAT_SUCCESS;
         break;
      }

      db_slotframe = find_slotframe(in_link->slotframe_handle);
      if(!db_slotframe) {
        res = LOWMAC_STAT_INVALID_PARAMETER;
        break;
      }

      db_link = find_link_by_id(db_slotframe, in_link->link_id);
      if(!db_link) {
        /* duplicated timeslot, reject */
        res = LOWMAC_STAT_UNKNOWN_LINK;
        break;
      }

//      if (db_link->qdpkt_list == (list_t)&(db_link->qdpkt_list_list)) //Jira33
//      {
         TXM_freeQueuedTxPackets(db_link);
//      }
      NHLDB_TSCH_delete_link_handle(db_link->vals.slotframe_handle, db_link->vals.link_id);
      del_link_entry(db_slotframe, db_link);
      res = LOWMAC_STAT_SUCCESS;

      break;
    }
    default:
    {
      res = LOWMAC_STAT_DB_BAD_ATTRIB_ID;
      break;
    }
  }
  Task_restore(key);
  return res;
}

/**************************************************************************************************//**
 *
 * @brief       This is used to modify the TSCH DB content
 *
 *
 * @param[in]   attrib_id -- PIB ID of data base attribute
 *              in_val    -- pointer to attribute data
 *              in_size   -- pointer of size of attribute value (???)
 *              db_callback -- callback function after completion
 *
 *
 * @return      status of this operation
 *
 ***************************************************************************************************/
LOWMAC_status_e TSCH_DB_modify_db(uint16_t attrib_id, void* in_val, uint16_t* in_size, LOWMAC_dbcb_t db_callback)
{
  LOWMAC_status_e res = LOWMAC_STAT_SUCCESS;

  uint32_t key = Task_disable();

  switch(attrib_id) {
    case TSCH_DB_ATTRIB_SLOTFRAME:
    {
      TSCH_PIB_slotframe_t* in_slotframe;
      TSCH_DB_slotframe_t*  db_slotframe;

      in_slotframe = (TSCH_PIB_slotframe_t *) in_val;

      db_slotframe = find_slotframe(in_slotframe->slotframe_handle);
      if(!db_slotframe) {
        res = LOWMAC_STAT_SLOTFRAME_NOT_FOUND;
        break;
      }

      res = modify_slotframe_entry(db_slotframe, in_slotframe);
      break;
    }
    case TSCH_DB_ATTRIB_LINK:
    {
      TSCH_PIB_link_t*     in_link;

      TSCH_DB_slotframe_t* db_slotframe;
      TSCH_DB_link_t*      db_link;

      in_link = (TSCH_PIB_link_t *) in_val;

      db_slotframe = find_slotframe(in_link->slotframe_handle);
      if(!db_slotframe) {
        res = LOWMAC_STAT_INVALID_PARAMETER;
        break;
      }

      db_link = find_link_by_id(db_slotframe, in_link->link_id);
      if(!db_link) {
        /* duplicated timeslot, reject */
        res = LOWMAC_STAT_UNKNOWN_LINK;
        break;
      }

      res = modify_link_entry(db_slotframe, db_link, in_link);
      break;
    }
    default:
    {
      res = LOWMAC_STAT_DB_BAD_ATTRIB_ID;
      break;
    }
  }
  Task_restore(key);
  return res;
}


bool TSCH_DB_isKeepAliveParent(uint16_t ackSourAddr)
{
    TSCH_DB_ti_keepalive_t* ka_entry;

    ka_entry = &db_hnd.keepalive;
    if((ka_entry->in_use == LOWMAC_TRUE) &&
       (ka_entry->vals.dst_addr_short == ackSourAddr) )
    {
       return true;
    }
    else
    {
       return false;
    }
}

/**************************************************************************************************//**
 *
 * @brief       This is used to retrieve the links assigned to the current node
 *
 *
 * @param[in]   startLinkIdx -- start index of the links to be retrieved (the first link of the first slot frame has index of 0)
 *              buf    -- pointer to buffer to hold the retrieved link structures
 *              buf_size   -- size of the buffer, should not fill beyond this size if there are more links than can be held in the buffer
 *
 *
 * @return      number of bytes retrieved and stored in the buffer
 *
 * @note        The first two-bytes of the buffer contains an LE 16-bit integer indicating the total number of links having index
 *              greater than or equal to startLinkIdx.  The second two-bytes of the buffer contains an LE 16-bit integer indicating the
 *              actual number of retrieved links.  The two numbers differ if the buffer does not have enough space to hold all the
 *              retrievable links.
 *
 ***************************************************************************************************/
int TSCH_DB_getSchedule(uint16_t startLinkIdx, uint8_t *buf, int buf_size)
{

   TSCH_DB_slotframe_t* cur_sf;
   uint16_t totalLinkCount = 0;
   uint16_t retrivedLinkCount = 0;
   uint16_t linkIdx = 0;
   uint8_t  idx = 0;
   uint8_t  iter;
   TSCH_DB_link_t* link_p;

   if (buf_size >= 3)
   {
      idx = 3;

      for (iter = 0; iter < TSCH_DB_MAX_SLOTFRAMES; iter++)
      {
         cur_sf = &db_hnd.slotframes[iter];
         if(cur_sf->in_use == LOWMAC_TRUE)
         {
            link_p = list_head(cur_sf->link_list);
            while(link_p)
            {
               if (linkIdx >= startLinkIdx)
               {
                  uint8_t linkRecordSize =
                        sizeof(link_p->vals.slotframe_handle) +
                        sizeof(link_p->vals.link_option) +
                        sizeof(link_p->vals.link_type) +
                        sizeof(link_p->vals.period) +
                        sizeof(link_p->vals.periodOffset) +
                        sizeof(link_p->vals.link_id) +
                        sizeof(link_p->vals.timeslot) +
                        sizeof(link_p->vals.channel_offset) +
                        sizeof(link_p->vals.peer_addr);

                  totalLinkCount++;

                  if ((buf_size - idx) >= linkRecordSize)
                  {
                     memcpy(buf+idx, &(link_p->vals.slotframe_handle), sizeof(link_p->vals.slotframe_handle));
                     idx += sizeof(link_p->vals.slotframe_handle);
                     memcpy(buf+idx, &(link_p->vals.link_option), sizeof(link_p->vals.link_option));
                     idx += sizeof(link_p->vals.link_option);
                     memcpy(buf+idx, &(link_p->vals.link_type), sizeof(link_p->vals.link_type));
                     idx += sizeof(link_p->vals.link_type);
                     memcpy(buf+idx, &(link_p->vals.period), sizeof(link_p->vals.period));
                     idx += sizeof(link_p->vals.period);
                     memcpy(buf+idx, &(link_p->vals.periodOffset), sizeof(link_p->vals.periodOffset));
                     idx += sizeof(link_p->vals.periodOffset);
                     memcpy(buf+idx, &(link_p->vals.link_id), sizeof(link_p->vals.link_id));
                     idx += sizeof(link_p->vals.link_id);
                     memcpy(buf+idx, &(link_p->vals.timeslot), sizeof(link_p->vals.timeslot));
                     idx += sizeof(link_p->vals.timeslot);
                     memcpy(buf+idx, &(link_p->vals.channel_offset), sizeof(link_p->vals.channel_offset));
                     idx += sizeof(link_p->vals.channel_offset);
                     memcpy(buf+idx, &(link_p->vals.peer_addr), sizeof(link_p->vals.peer_addr));
                     idx += sizeof(link_p->vals.peer_addr);

                     retrivedLinkCount++;
                  }
               }
               link_p = list_item_next(link_p);
               linkIdx++;
            }
         }
      }
      iter = 0;
      buf[iter++] = totalLinkCount & 0xFF;
      buf[iter++] = retrivedLinkCount & 0xFF;
      buf[iter++] = startLinkIdx & 0xFF;
   }

   return(idx);
}
