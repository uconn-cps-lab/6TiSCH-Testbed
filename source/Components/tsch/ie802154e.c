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
 *  ====================== ie802154e.c =============================================
 *  
 */
#include <string.h>

#include "pib-802154e.h"
#include "ie802154e.h"
#include "tsch_api.h"

/***************************************************************************//**
 * @fn          ie802154e_add_payloadie
 *
 * @brief      Creates payload IE and add it to the buffer
 *
 * @param[in]  ie_info - information for the IE to create
 *             buf - Pointer to buffer to store the frame
 *             buf_len - Size of the buffer
 *
 * @return     Length of added IE or 0 if not successful
 ******************************************************************************/
uint8_t
ie802154e_add_payloadie(ie802154e_pie_t* ie_info, uint8_t* buf, uint8_t buf_len)
{
  uint8_t* cur_buf_ptr;
  uint16_t content_len;
  uint16_t  remain_buf_len;
  uint16_t  size;
  uint8_t group_id;
  uint8_t elem_id;
  uint8_t sub_type;
  uint16_t descriptor;

  if(buf_len < MIN_PAYLOAD_IE_LEN) {
    return 0;
  }

  if(!buf || !ie_info)
    return 0;

  cur_buf_ptr = buf;
  remain_buf_len = (int16_t)buf_len;
  content_len = 0;

  group_id = ie_info->ie_group_id;
  size = ie_info->ie_content_length;

  if((group_id != IE_GROUP_ID_MLME) || ie_info->outer_descriptor)
  {
    descriptor = IE_PACKING(IE_TYPE_PAYLOAD,IE_TYPE_S,IE_TYPE_P)
                |IE_PACKING(group_id,IE_GROUP_ID_S,IE_GROUP_ID_P)
                |IE_PACKING(size,IE_LEN_P_S,IE_LEN_P_P);

    cur_buf_ptr[0] = GET_BYTE(descriptor,0);
    cur_buf_ptr[1] = GET_BYTE(descriptor,1);

    cur_buf_ptr += MIN_PAYLOAD_IE_LEN;
    remain_buf_len -= MIN_PAYLOAD_IE_LEN;
    content_len += MIN_PAYLOAD_IE_LEN;
  }

  if(remain_buf_len < size || size > MAX_PAYLOAD_IE_LEN)
  {
    return 0;
  }

  if(group_id == IE_GROUP_ID_ESDU ||
    (group_id >= IE_GROUP_ID_UNMG_L && group_id <= IE_GROUP_ID_UNMG_H))
  {
      if(size)
      {
        uint8_t* arg_ptr;
        arg_ptr = (uint8_t *) ie_info->ie_content_ptr;
        if(!arg_ptr)
          return 0;

        memcpy(cur_buf_ptr, arg_ptr, size);
        content_len += size;
      }
  }
  else if(group_id == IE_GROUP_ID_TERM)
  {
    // nothing to do
  }
  else if(group_id == IE_GROUP_ID_MLME)
  {
    if(ie_info->outer_descriptor && size == 0)
      return content_len;

    sub_type = ie_info->ie_sub_type;
    elem_id = ie_info->ie_elem_id;
    size = ie_info->ie_sub_content_length;

    if(remain_buf_len < size || size > MAX_PAYLOAD_IE_LEN) {
      return 0;
    }

    if(sub_type == IE_SUBTYPE_SHORT)
    {
      descriptor = IE_PACKING(IE_SUBTYPE_SHORT,IE_SUBTYPE_S,IE_SUBTYPE_P)
                  |IE_PACKING(elem_id,IE_ID_SHORT_S,IE_ID_SHORT_P)
                  |IE_PACKING(size,IE_SUBLEN_SHORT_S,IE_SUBLEN_SHORT_P);

      cur_buf_ptr[0] = GET_BYTE(descriptor,0);
      cur_buf_ptr[1] = GET_BYTE(descriptor,1);

      cur_buf_ptr += MIN_PAYLOAD_IE_LEN; // sub IE descriptor
      content_len += MIN_PAYLOAD_IE_LEN;
      remain_buf_len -= MIN_PAYLOAD_IE_LEN;

      //Switch case here
      switch(elem_id)
      {
        case IE_ID_TSCH_SYNCH:
        {
          ie_tsch_synch_t* arg_ptr;

          if(size != IE_TSCH_SYNCH_LEN)
            return 0;

          arg_ptr = (ie_tsch_synch_t *) ie_info->ie_content_ptr;

          ie802154e_set_asn(&cur_buf_ptr[0], arg_ptr->macASN);
          cur_buf_ptr[5] = arg_ptr->macJoinPriority;

          content_len += size;
          break;
        }
        case IE_ID_TSCH_SLOTFRAMELINK:
        {
          ie_tsch_slotframelink_t* sflink_arg_ptr;
          ie_slotframe_info_field_t* sf_arg_ptr;
          ie_link_info_field_t* link_ptr;
          uint8_t sf_iter, link_iter;
          uint8_t noslotframes;
          uint8_t nolinks;

          if(size < IE_SLOTFRAME_LINK_INFO_LEN)
            return 0;

          sflink_arg_ptr = (ie_tsch_slotframelink_t *) ie_info->ie_content_ptr;
          if(!sflink_arg_ptr)
            return 0;

          noslotframes = sflink_arg_ptr->numSlotframes;
          cur_buf_ptr[0] = noslotframes;
          cur_buf_ptr += IE_SLOTFRAME_LINK_INFO_LEN;
          content_len += IE_SLOTFRAME_LINK_INFO_LEN;
          remain_buf_len -= IE_SLOTFRAME_LINK_INFO_LEN;

          if(noslotframes)
          {
            sf_arg_ptr = sflink_arg_ptr->slotframe_info_list;
            for(sf_iter=0; sf_iter<noslotframes; sf_iter++)
            {
               if(remain_buf_len < IE_SLOTFRAME_INFO_LEN) {
                 return 0;
               }

               cur_buf_ptr[0] = sf_arg_ptr[sf_iter].macSlotframeHandle;
               cur_buf_ptr[1] = GET_BYTE(sf_arg_ptr[sf_iter].macSlotframeSize,0);
               cur_buf_ptr[2] = GET_BYTE(sf_arg_ptr[sf_iter].macSlotframeSize,1);
               cur_buf_ptr[3] = sf_arg_ptr[sf_iter].numLinks;

               cur_buf_ptr += IE_SLOTFRAME_INFO_LEN;
               content_len += IE_SLOTFRAME_INFO_LEN;
               remain_buf_len -= IE_SLOTFRAME_INFO_LEN;

               nolinks = sf_arg_ptr[sf_iter].numLinks;

               if(nolinks)
               {
                 link_ptr = sf_arg_ptr[sf_iter].link_info_list;
                 for(link_iter=0; link_iter<nolinks; link_iter++) {
                   if(remain_buf_len < IE_LINK_INFO_LEN) {
                     return 0;
                   }
                   cur_buf_ptr[0] = GET_BYTE(link_ptr[link_iter].macTimeslot,0);
                   cur_buf_ptr[1] = GET_BYTE(link_ptr[link_iter].macTimeslot,1);
                   cur_buf_ptr[2] = GET_BYTE(link_ptr[link_iter].macChannelOffset,0);
                   cur_buf_ptr[3] = GET_BYTE(link_ptr[link_iter].macChannelOffset,1);
                   cur_buf_ptr[4] = link_ptr[link_iter].macLinkOption;

                   cur_buf_ptr += IE_LINK_INFO_LEN;
                   content_len += IE_LINK_INFO_LEN;
                   remain_buf_len -= IE_LINK_INFO_LEN;
                 } // end of link_iter
               } // end of nolinks
            } // end of sf_iter
          } // end of noslotframes

          break;
        } // end IE_ID_TSCH_SLOTFRAMELINK
        case IE_ID_TSCH_TIMESLOT:
        {
          ie_timeslot_template_t* ie_tt;

          if(size == IE_TIMESLOT_TEMPLATE_LEN_FULL+3)   // unless packed structure
            size = IE_TIMESLOT_TEMPLATE_LEN_FULL;

          if(size != IE_TIMESLOT_TEMPLATE_LEN_FULL && size != IE_TIMESLOT_TEMPLATE_LEN_ID_ONLY)
            return 0;

          ie_tt = (ie_timeslot_template_t *) ie_info->ie_content_ptr;
          cur_buf_ptr[0] = ie_tt->timeslot_id;

          if(size == IE_TIMESLOT_TEMPLATE_LEN_FULL) {
            ie802154e_set16(&cur_buf_ptr[1], ie_tt->macTsCcaOffset);
            ie802154e_set16(&cur_buf_ptr[3], ie_tt->TsCCA);
            ie802154e_set16(&cur_buf_ptr[5], ie_tt->TsTxOffset);
            ie802154e_set16(&cur_buf_ptr[7], ie_tt->TsRxOffset);
            ie802154e_set16(&cur_buf_ptr[9], ie_tt->TsRxAckDelay);
            ie802154e_set16(&cur_buf_ptr[11], ie_tt->TsTxAckDelay);
            ie802154e_set16(&cur_buf_ptr[13], ie_tt->TsRxWait);
            ie802154e_set16(&cur_buf_ptr[15], ie_tt->TsAckWait);
            ie802154e_set16(&cur_buf_ptr[17], ie_tt->TsRxTx);
            ie802154e_set16(&cur_buf_ptr[19], ie_tt->TsMaxAck);
            ie802154e_set16(&cur_buf_ptr[21], ie_tt->TsMaxTx);
            ie802154e_set16(&cur_buf_ptr[23], ie_tt->TsTimeslotLength);
          }

          content_len += size;
          break;
        }
        case IE_ID_EB_FILTER:
        {
          ie_eb_filter_t* ie_eb_filter;

          if(size < IE_EB_FILTER_LEN_MIN)
            return 0;

          ie_eb_filter = (ie_eb_filter_t *) ie_info->ie_content_ptr;

          *cur_buf_ptr++ = IE_PACKING(ie_eb_filter->permit_join_on,1,0) |
                           IE_PACKING(ie_eb_filter->inc_lq_filter,1,1) |
                           IE_PACKING(ie_eb_filter->inc_perc_filter,1,2) |
                           IE_PACKING(ie_eb_filter->num_entries_pib_id,2,3);
          if(ie_eb_filter->inc_lq_filter)
            *cur_buf_ptr++ = ie_eb_filter->lq;
          if(ie_eb_filter->inc_perc_filter)
            *cur_buf_ptr++ = ie_eb_filter->perc_filter;
          if(ie_eb_filter->num_entries_pib_id)
            memcpy(cur_buf_ptr, ie_eb_filter->pib_ids, ie_eb_filter->num_entries_pib_id);

          content_len += size;
          break;
        }
     case IE_ID_TSCH_PERIODIC_SCHED:
      {
          ie_tsch_periodic_scheduler_t* pPerSched;

          if (size < sizeof(ie_tsch_periodic_scheduler_t))
              return 0;

          pPerSched = (ie_tsch_periodic_scheduler_t *) ie_info->ie_content_ptr;

          cur_buf_ptr[0] = pPerSched-> period;
          cur_buf_ptr[1] = pPerSched->numFragPackets;
          cur_buf_ptr[2] = pPerSched->numHops;

          content_len += size;
          break;
      }
      case IE_ID_TSCH_LINK_PERIOD:
      {
          ie_tsch_link_period_t* pLinkPer;
          uint8_t i;

          pLinkPer = (ie_tsch_link_period_t *) ie_info->ie_content_ptr;

          for (i=0;i<size/2;i++)
          {
              cur_buf_ptr[i*2] = pLinkPer->period[i];
              cur_buf_ptr[i*2+1] = pLinkPer->periodOffset[i];
          }

          content_len += size;
          break;
      }
      case IE_ID_TSCH_COMPRESS_LINK:
      {
          ie_tsch_link_comp_t* pLinkPer;

          pLinkPer = (ie_tsch_link_comp_t *) ie_info->ie_content_ptr;
          cur_buf_ptr[0] = pLinkPer->linkType;
          cur_buf_ptr[1] = pLinkPer->startTimeslot;
          cur_buf_ptr[2] = pLinkPer->numContTimeslot;

          content_len += size;
          break;
      }
      case IE_ID_TSCH_SUGGEST_PARENT:
      {
          uint16_t* parent;

          parent = (uint16_t *) ie_info->ie_content_ptr;
          cur_buf_ptr[0] = (*parent)>>8;
          cur_buf_ptr[1] = (*parent)&0xff;

          content_len += size;
          break;
      }
        default:
        {
          return 0;
        }
      } // end switch
    }
    else //IE_SUBTYPE_LONG
    {
      descriptor = IE_PACKING(IE_SUBTYPE_LONG,IE_SUBTYPE_S,IE_SUBTYPE_P)
                  |IE_PACKING(ie_info->ie_elem_id,IE_ID_LONG_S,IE_ID_LONG_P)
                  |IE_PACKING(ie_info->ie_sub_content_length,IE_SUBLEN_LONG_S,IE_SUBLEN_LONG_P);

      cur_buf_ptr[0] = GET_BYTE(descriptor,0);
      cur_buf_ptr[1] = GET_BYTE(descriptor,1);

      cur_buf_ptr += MIN_PAYLOAD_IE_LEN; // sub IE descriptor
      content_len += MIN_PAYLOAD_IE_LEN;
      remain_buf_len -= MIN_PAYLOAD_IE_LEN;

      //Switch case here
      switch(elem_id)
      {
        case IE_ID_CHANNELHOPPING:
        {
          ie_hopping_t* ie_hop;

          if(size < IE_HOPPING_TIMING_LEN)
            return 0;

          ie_hop = (ie_hopping_t *) ie_info->ie_content_ptr;
          cur_buf_ptr[0] = ie_hop->macHoppingSequenceID;

          content_len += size;
          break;
        }
        default:
        {
          return 0;
        }
      }
    } //end of sub_type long
  } // end of group_id = IE_GROUP_ID_MLME
  else // reserved group_id
    return 0;

  return (content_len);
}

/***************************************************************************//**
 * @fn         ie802154e_add_payloadie_term
 *
 * @brief      Creates termination IE and add it to the buffer
 *
 * @param[in]  buf - Pointer to buffer to store the frame
 *             buf_len - Size of the buffer
 *
 * @return     Length of added IE or 0 if not successful
 ******************************************************************************/
uint8_t
ie802154e_add_payloadie_term(uint8_t* buf, uint8_t buf_len)
{
  ie802154e_pie_t pie;

  pie.ie_type = IE_TYPE_PAYLOAD;
  pie.ie_group_id = IE_GROUP_ID_TERM;
  pie.ie_content_length = 0;
  pie.ie_sub_type = 0;
  pie.ie_elem_id = 0;
  pie.ie_sub_content_length = 0;
  pie.ie_content_ptr = NULL;
  pie.outer_descriptor = 1;

  return ie802154e_add_payloadie(&pie, buf, buf_len);
}

/***************************************************************************//**
 * @fn         ie802154e_parse_tsch_synch
 *
 * @brief      Parses the TSCH synchronization IE
 *
 * @param[in]  buf - Pointer to buffer storing IE content
 *             arg - Structure to store IE information
 *
 * @return     None
 ******************************************************************************/
void
ie802154e_parse_tsch_synch(uint8_t* buf, ie_tsch_synch_t* arg)
{
  arg->macASN = ie802154e_get_asn(&buf[0]);
  arg->macJoinPriority = buf[5];
}

/***************************************************************************//**
 * @fn         ie802154e_parse_timeslot_template
 *
 * @brief      Parses the TSCH timeslot IE
 *
 * @param[in]  pie - Pointer to the IE
 *             ie_tt - Structure to store IE information
 *
 * @return     None
 ******************************************************************************/
void
ie802154e_parse_timeslot_template(ie802154e_pie_t* pie, ie_timeslot_template_t* ie_tt)
{
  uint8_t* buf;

  buf = pie->ie_content_ptr;
  if(pie->ie_sub_content_length == IE_TIMESLOT_TEMPLATE_LEN_FULL) {
    ie_tt->timeslot_id = buf[0];

    ie_tt->macTsCcaOffset = ie802154e_get16(&buf[1]);
    ie_tt->TsCCA = ie802154e_get16(&buf[3]);
    ie_tt->TsTxOffset = ie802154e_get16(&buf[5]);
    ie_tt->TsRxOffset = ie802154e_get16(&buf[7]);
    ie_tt->TsRxAckDelay = ie802154e_get16(&buf[9]);
    ie_tt->TsTxAckDelay = ie802154e_get16(&buf[11]);
    ie_tt->TsRxWait = ie802154e_get16(&buf[13]);
    ie_tt->TsAckWait = ie802154e_get16(&buf[15]);
    ie_tt->TsRxTx = ie802154e_get16(&buf[17]);
    ie_tt->TsMaxAck = ie802154e_get16(&buf[19]);
    ie_tt->TsMaxTx = ie802154e_get16(&buf[21]);
    ie_tt->TsTimeslotLength = ie802154e_get16(&buf[23]);

  } else {
    ie_tt->timeslot_id = buf[0];
  }
}

/***************************************************************************//**
 * @fn         ie802154e_parse_hopping_timing
 *
 * @brief      Parses the Channel hopping IE
 *
 * @param[in]  pie - Pointer to the IE
 *             ie_hop - Structure to store IE information
 *
 * @return     None
 ******************************************************************************/
void
ie802154e_parse_hopping_timing(ie802154e_pie_t* pie, ie_hopping_t* ie_hop)
{
  uint8_t* buf;

  buf = pie->ie_content_ptr;
  ie_hop->macHoppingSequenceID = buf[0];
}

/***************************************************************************//**
 * @fn         ie802154e_parse_eb_filter
 *
 * @brief      Parses the EB Filter IE
 *
 * @param[in]  buf - Pointer to the IE content
 *             ie_eb_filter - Structure to store IE information
 *
 * @return     None
 ******************************************************************************/
void
ie802154e_parse_eb_filter(uint8_t* buf, ie_eb_filter_t* ie_eb_filter)
{
  ie_eb_filter->permit_join_on = buf[0] & 0x01;
}

/***************************************************************************//**
 * @fn         ie802154e_parse_tsch_slotframelink
 *
 * @brief      Parses the TSCH slotframe and link IE
 *
 * @param[in]  buf - Pointer to the IE content
 *             arg - Structure to store IE information
 *
 * @return     None
 ******************************************************************************/
void
ie802154e_parse_tsch_slotframelink(uint8_t* buf, ie_tsch_slotframelink_t* arg)
{
  ie_slotframe_info_field_t* sf_ie;
  ie_link_info_field_t* link_ie;
  uint8_t sf_iter, link_iter;

  arg->numSlotframes = buf[0];
  buf += IE_SLOTFRAME_LINK_INFO_LEN;

  sf_ie = arg->slotframe_info_list; /** */
  for(sf_iter=0; sf_iter<arg->numSlotframes; sf_iter++) {

    sf_ie[sf_iter].macSlotframeHandle = buf[0];
    sf_ie[sf_iter].macSlotframeSize = ie802154e_get16(&buf[1]);
    sf_ie[sf_iter].numLinks = buf[3];

    buf += IE_SLOTFRAME_INFO_LEN;

    link_ie = sf_ie[sf_iter].link_info_list;
    for(link_iter=0; link_iter<sf_ie[sf_iter].numLinks; link_iter++) {
      link_ie[link_iter].macTimeslot = ie802154e_get16(&buf[0]);
      link_ie[link_iter].macChannelOffset = ie802154e_get16(&buf[2]);
      link_ie[link_iter].macLinkOption = buf[4];

      buf += IE_LINK_INFO_LEN;


      if (link_ie[link_iter].macLinkOption == 0)
      {
          if (MlmeCb)
          {
              tschMlmeRestart_t restart;
              restart.hdr.event =  MAC_MLME_RESTART;
              MlmeCb((tschCbackEvent_t *)(&restart),NULL);
          }

      }
    }
  }
}

/***************************************************************************//**
 * @fn         ie802154e_parse_tsch_periodic_information
 *
 * @brief      Parses TSCH periodic scheduler information
 *
 * @param[in]  buf - Pointer to the IE content
 *             arg - Structure to store IE information
 *
 * @return     None
 ******************************************************************************/
void
ie802154e_parse_tsch_periodic_information(uint8_t* buf, ie_tsch_periodic_scheduler_t* arg)
{
  arg->period = buf[0];
  arg->numFragPackets = buf[1];
}

/***************************************************************************//**
 * @fn         ie802154e_parse_link_period
 *
 * @brief      Parses link period information
 *
 * @param[in]  pIE - Pointer to the IE
 *             sublen - IE length
 *             arg - Structure to store IE information
 *
 * @return     None
 ******************************************************************************/
void
ie802154e_parse_link_period(ie802154e_pie_t* pIE, uint16_t len, ie_tsch_link_period_t* arg)
{
    uint16_t i;
    uint8_t *buf;

    buf = pIE->ie_content_ptr;

    len -= MIN_PAYLOAD_IE_LEN;

    for (i=0;i<(len/2);i++)
    {
        arg->period[i]=*buf;
        arg->periodOffset[i]=*(buf+1);
        buf+=2;

      if ((arg->period[i] == 0) || (arg->periodOffset[i] > arg->period[i]))
      {
          if (MlmeCb)
          {
              tschMlmeRestart_t restart;
              restart.hdr.event =  MAC_MLME_RESTART;
              MlmeCb((tschCbackEvent_t *)(&restart),NULL);
          }
      }
    }
}

/***************************************************************************//**
 * @fn         ie802154e_parse_link_compression
 *
 * @brief      Parses link compression IE information
 *
 * @param[in]  pIE - Pointer to the IE
 *             arg - Structure to store IE information
 *
 * @return     None
 ******************************************************************************/
void
ie802154e_parse_link_compression(ie802154e_pie_t* pIE, ie_tsch_link_comp_t* arg)
{
    uint8_t *buf;

    buf = pIE->ie_content_ptr;
    arg->linkType=*buf;
    arg->startTimeslot=*(buf+1);
    arg->numContTimeslot=*(buf+2);
}

/*---------------------------------------------------------------------------*/
static uint8_t
getPayloadIe(uint8_t* buf, ie802154e_pie_t* pie)
{
  uint16_t ie_header;

  ie_header = MAKE_UINT16(buf[0],buf[1]);
  pie->ie_type = IE_UNPACKING(ie_header,IE_TYPE_S,IE_TYPE_P);
  pie->ie_group_id = IE_UNPACKING(ie_header,IE_GROUP_ID_S,IE_GROUP_ID_P);
  pie->ie_content_length = IE_UNPACKING(ie_header,IE_LEN_P_S,IE_LEN_P_P);

  if(pie->ie_type != IE_TYPE_PAYLOAD ||
     pie->ie_content_length > MAX_PAYLOAD_IE_LEN ||
    (pie->ie_group_id != IE_GROUP_ID_ESDU && pie->ie_group_id != IE_GROUP_ID_MLME &&
     pie->ie_group_id != IE_GROUP_ID_TERM &&
    (pie->ie_group_id > IE_GROUP_ID_UNMG_H || pie->ie_group_id < IE_GROUP_ID_UNMG_L)))
    return 0;

  if(!pie->ie_content_length)
    pie->ie_content_ptr = NULL;
  else
    pie->ie_content_ptr = buf + MIN_PAYLOAD_IE_LEN;

  return (MIN_PAYLOAD_IE_LEN + pie->ie_content_length);
}

/*---------------------------------------------------------------------------*/
static uint8_t
getPayloadSubIe(uint8_t* buf, ie802154e_pie_t* pie)
{
  uint16_t ie_header;

  ie_header = MAKE_UINT16(buf[0],buf[1]);
  pie->ie_sub_type = IE_UNPACKING(ie_header,IE_SUBTYPE_S,IE_SUBTYPE_P);
  if(pie->ie_sub_type == IE_SUBTYPE_SHORT)
  {
    pie->ie_elem_id = IE_UNPACKING(ie_header,IE_ID_SHORT_S,IE_ID_SHORT_P);
    pie->ie_sub_content_length = IE_UNPACKING(ie_header,IE_SUBLEN_SHORT_S,IE_SUBLEN_SHORT_P);

    if(pie->ie_sub_content_length > MAX_PAYLOAD_IE_LEN ||
       (pie->ie_elem_id != IE_ID_TSCH_SYNCH && pie->ie_elem_id != IE_ID_TSCH_SLOTFRAMELINK &&
        pie->ie_elem_id != IE_ID_TSCH_TIMESLOT && pie->ie_elem_id != IE_ID_EB_FILTER &&
       (pie->ie_elem_id > IE_ID_SHORT_UNMG_H || pie->ie_elem_id < IE_ID_SHORT_UNMG_L)))
      return 0;
  }
  else
  {
    pie->ie_elem_id = IE_UNPACKING(ie_header,IE_ID_LONG_S,IE_ID_LONG_P);
    pie->ie_sub_content_length = IE_UNPACKING(ie_header,IE_SUBLEN_LONG_S,IE_SUBLEN_LONG_P);

    if(pie->ie_sub_content_length > MAX_PAYLOAD_IE_LEN ||
       (pie->ie_elem_id != IE_ID_CHANNELHOPPING &&
       //(pie->ie_elem_id > IE_ID_LONG_UNMG_H || pie->ie_elem_id < IE_ID_LONG_UNMG_L)))
       (pie->ie_elem_id > IE_ID_LONG_UNMG_H )))
      return 0;
  }

  if(!pie->ie_sub_content_length)
    pie->ie_content_ptr = NULL;
  else
    pie->ie_content_ptr = buf + MIN_PAYLOAD_IE_LEN;;

  return (MIN_PAYLOAD_IE_LEN + pie->ie_sub_content_length);
}


/***************************************************************************//**
 * @fn         ie802154e_parse_payloadie
 *
 * @brief      Parses the payload IE
 *
 * @param[in]  buf - Pointer to the IE buffer
 *             buf_len - Length of the IE buffer
 *             find_ie - Structure to store IE information
 *
 * @return     Length of the IE or 0 if unsuccessful
 ******************************************************************************/
uint8_t
ie802154e_parse_payloadie(uint8_t* buf, uint8_t buf_len, ie802154e_pie_t* find_ie)
{
  uint8_t *pbuf = buf;
  uint8_t group_id;
  uint16_t elemlen;

  // check if valid payload ie
  if(buf_len < MIN_PAYLOAD_IE_LEN)
    return 0;

  elemlen = getPayloadIe(pbuf, find_ie);

  // check if payload ie
  if(elemlen < MIN_PAYLOAD_IE_LEN || elemlen > MAX_PAYLOAD_IE_LEN)
    return 0;

  group_id = find_ie->ie_group_id;

  if(group_id == IE_GROUP_ID_MLME ||
     group_id == IE_GROUP_ID_ESDU ||
     group_id == IE_GROUP_ID_TERM ||
    (group_id >= IE_GROUP_ID_UNMG_L && group_id <= IE_GROUP_ID_UNMG_H))
  {
    return elemlen;
  }

return 0;
}

/***************************************************************************//**
 * @fn         ie802154e_parse_payloadsubie
 *
 * @brief      Parses the payload sub IE
 *
 * @param[in]  buf - Pointer to the IE buffer
 *             buf_len - Length of the IE buffer
 *             find_ie - Structure to store IE information
 *
 * @return     Length of the sub IE or 0 if unsuccessful
 ******************************************************************************/
uint8_t
ie802154e_parse_payloadsubie(uint8_t* buf, uint8_t buf_len, ie802154e_pie_t* find_ie)
{
  uint8_t *pbuf = buf;
  uint8_t elem_id;
  uint16_t sublen;

  // check if payload subie
  if(buf_len < MIN_PAYLOAD_IE_LEN)
    return 0;

  sublen = getPayloadSubIe(pbuf, find_ie);

  // check if valid payload subie
  if(sublen < MIN_PAYLOAD_IE_LEN || sublen > MAX_PAYLOAD_IE_LEN)
    return 0;

  elem_id = find_ie->ie_elem_id;

  if(elem_id == IE_ID_TSCH_SYNCH || elem_id == IE_ID_TSCH_SLOTFRAMELINK ||
     elem_id == IE_ID_TSCH_TIMESLOT || elem_id == IE_ID_EB_FILTER ||
     elem_id == IE_ID_CHANNELHOPPING ||
    (elem_id >= IE_ID_SHORT_UNMG_L && elem_id <= IE_ID_SHORT_UNMG_H) ||
    //(elem_id >= IE_ID_LONG_UNMG_L && elem_id <= IE_ID_LONG_UNMG_H))
    ( elem_id <= IE_ID_LONG_UNMG_H))
  {
    return sublen;
  }

return 0;
}


/***************************************************************************//**
 * @fn         ie802154e_find_in_payloadielist
 *
 * @brief      Searchs the specific IE
 *
 * @param[in]  buf - Pointer to the IE buffer
 *             buf_len - Length of the IE buffer
 *             find_ie - Structure storing IE information
 *
 * @return     1 if found, 0 otherwise
 ******************************************************************************/
uint8_t
ie802154e_find_in_payloadielist(uint8_t* buf, uint8_t buf_len, ie802154e_pie_t* find_ie)
{
  ie802154e_pie_t pie;
  uint16_t remainlen = buf_len;
  uint8_t *pbuf = buf;
  uint8_t group_id;
  uint8_t elem_id;
  uint16_t elemlen;
  uint16_t sublen;

  memset(&pie, 0, sizeof(pie));

  elem_id = find_ie->ie_elem_id;

  if(elem_id == IE_ID_TSCH_SYNCH || elem_id == IE_ID_TSCH_SLOTFRAMELINK ||
     elem_id == IE_ID_TSCH_TIMESLOT || elem_id == IE_ID_EB_FILTER ||
     elem_id == IE_ID_CHANNELHOPPING ||
    (elem_id >= IE_ID_SHORT_UNMG_L && elem_id <= IE_ID_SHORT_UNMG_H) ||
    //(elem_id >= IE_ID_LONG_UNMG_L && elem_id <= IE_ID_LONG_UNMG_H))
    ( elem_id <= IE_ID_LONG_UNMG_H))
    find_ie->ie_group_id = IE_GROUP_ID_MLME;

  group_id = find_ie->ie_group_id;

  while((elemlen = ie802154e_parse_payloadie(pbuf, remainlen, find_ie)))
  {
    if(find_ie->ie_group_id == group_id)
    {
      if(group_id != IE_GROUP_ID_MLME)
        return 1;
      else
      {
        pbuf += MIN_PAYLOAD_IE_LEN;
        remainlen -= MIN_PAYLOAD_IE_LEN;
        break;
      }
    }

    // check if not looking for termination IE
    if(find_ie->ie_group_id == IE_GROUP_ID_TERM)
      return 0;

    pbuf += elemlen;
    remainlen -= elemlen;
  }

  // check if correct group id
  if(find_ie->ie_group_id != group_id)
    return 0;

  while((sublen = ie802154e_parse_payloadsubie(pbuf, remainlen, find_ie)))
  {
    if(find_ie->ie_elem_id == elem_id)
      return 1;

    pbuf += sublen;
    remainlen -= sublen;
  }

  return 0;
}

/***************************************************************************//**
 * @fn         ie802154e_payloadielist_len
 *
 * @brief      Calculates the length of payload IEs
 *
 * @param[in]  buf - Pointer to the IE buffer
 *             buf_len - Length of the IE buffer
 *
 * @return     Length of payload IEs
 ******************************************************************************/
uint8_t
ie802154e_payloadielist_len(uint8_t* buf, uint8_t buf_len)
{
  ie802154e_pie_t pie;
  uint16_t remainlen = buf_len;
  uint8_t *pbuf = buf;
  uint8_t group_id;
  uint16_t elemlen;
  uint16_t pielen = 0;

  memset(&pie, 0, sizeof(pie));

  while(remainlen >= MIN_PAYLOAD_IE_LEN)
  {
    if((elemlen = getPayloadIe(pbuf, &pie)) < MIN_PAYLOAD_IE_LEN)
       break;

    pbuf += elemlen;
    remainlen -= elemlen;

    group_id = pie.ie_group_id;

    if(group_id == IE_GROUP_ID_MLME)
    {
      pielen += elemlen;
    }
    else if(group_id == IE_GROUP_ID_ESDU ||
    (group_id >= IE_GROUP_ID_UNMG_L && group_id <= IE_GROUP_ID_UNMG_H))
    {
      pielen += elemlen;
    }
    else if(group_id == IE_GROUP_ID_TERM)
    {
      pielen += elemlen;
      remainlen = 0;
    }
    else
      remainlen = 0;
  }

  return pielen;
}


/***************************************************************************//**
 * @fn         ie802154e_add_hdrie
 *
 * @brief      Creates header IE and adds it to the buffer
 *
 * @param[in]  ie_info - Pointer to the information of the IE
 *             buf - Pointer to the buffer including IE
 *             buf_len - Length of the IE buffer
 *
 * @return     Added length of IEs
 ******************************************************************************/
uint8_t
ie802154e_add_hdrie(ie802154e_hie_t* ie_info, uint8_t* buf, uint8_t buf_len)
{
  uint8_t* cur_buf_ptr;
  uint8_t content_len;
  int16_t  remain_buf_len;
  uint8_t  size;
  uint16_t descriptor;

  if(buf_len < MIN_HEADER_IE_LEN) {
    return 0;
  }

  if(!buf || !ie_info)
    return 0;

  descriptor = IE_PACKING(IE_TYPE_HEADER,IE_TYPE_S,IE_TYPE_P)
              |IE_PACKING(ie_info->ie_elem_id,IE_ID_H_S,IE_ID_H_P)
              |IE_PACKING(ie_info->ie_content_length,IE_LEN_H_S,IE_LEN_H_P);

  buf[0] = GET_BYTE(descriptor,0);
  buf[1] = GET_BYTE(descriptor,1);

  // hie header will be filled after calc len, skip to content
  cur_buf_ptr = buf + MIN_HEADER_IE_LEN;
  remain_buf_len = (int16_t) buf_len - MIN_HEADER_IE_LEN;

  content_len = 0;
  switch(ie_info->ie_elem_id)
  {
    case IE_ID_ACKNACK_TIME_CORRECTION:
    {
      ie_acknack_time_correction_t* arg_ptr;
      int16_t time_sync_ack;

      size = IE_ACKNACK_TIME_CORRECTION_LEN;
      if(remain_buf_len < size) {
        return 0;
      }

      arg_ptr = (ie_acknack_time_correction_t *) ie_info->ie_content_ptr;
      time_sync_ack = arg_ptr->time_synch;

      //1. time_sync should be range [-2048, +2047] = [0xF800, 0x07FF]
      if(time_sync_ack > 2047) time_sync_ack = 2047;
      else if(time_sync_ack < -2048) time_sync_ack = -2048;

      time_sync_ack &= 0x0FFF;

      //2. 15th bit indicates ACK or NACK
      if(arg_ptr->acknack) time_sync_ack |= 0x8000;

      ie802154e_set16(&cur_buf_ptr[0], time_sync_ack);
      ie802154e_set32(&cur_buf_ptr[2], arg_ptr->rxTotal);       //zelin
      cur_buf_ptr[6] = arg_ptr->rxRssi;
      
      content_len = size;
      break;
    } // end IE_ID_ACKNACK_TIME_CORRECTION
    case IE_ID_LIST_TERM1:
    case IE_ID_LIST_TERM2:
    {
      content_len = 0;
      break;
    } // end IE_ID_LIST_TERM1/2
    default:
    {
      return 0;
      //break;
    } // end default

  } // end switch

  return (MIN_HEADER_IE_LEN + content_len);

}

/***************************************************************************//**
 * @fn         ie802154e_add_hdrie_term
 *
 * @brief      Creates header termination IE and adds it to the buffer
 *
 * @param[in]  buf - Pointer to the buffer including IE
 *             buf_len - Length of the IE buffer
 *
 * @return     Added length of IEs
 ******************************************************************************/
uint8_t
ie802154e_add_hdrie_term(uint8_t* buf, uint8_t buf_len)
{
  ie802154e_hie_t hie;

  hie.ie_type = IE_TYPE_HEADER;
  hie.ie_elem_id = IE_ID_LIST_TERM;
  hie.ie_content_length = 0;
  hie.ie_content_ptr = NULL;

  return ie802154e_add_hdrie(&hie, buf, buf_len);
}

/*---------------------------------------------------------------------------*/
static uint8_t
getHeaderIe(uint8_t* buf, ie802154e_hie_t* hie)
{
  uint16_t ie_header;

  ie_header = MAKE_UINT16(buf[0],buf[1]);
  hie->ie_type = IE_UNPACKING(ie_header,IE_TYPE_S,IE_TYPE_P);
  hie->ie_elem_id = IE_UNPACKING(ie_header,IE_ID_H_S,IE_ID_H_P);
  hie->ie_content_length = IE_UNPACKING(ie_header,IE_LEN_H_S,IE_LEN_H_P);

  if(hie->ie_type != IE_TYPE_HEADER)
    return 0;

  if(hie->ie_elem_id != IE_ID_ACKNACK_TIME_CORRECTION &&
     hie->ie_elem_id != IE_ID_LIST_TERM1 &&
     hie->ie_elem_id != IE_ID_LIST_TERM2 &&
    //(hie->ie_elem_id > IE_ID_HDR_UNMG_H || hie->ie_elem_id < IE_ID_HDR_UNMG_L))
    (hie->ie_elem_id > IE_ID_HDR_UNMG_H ))
    return 0;

  if(hie->ie_content_length > MAX_HEADER_IE_LEN)
    return 0;

  if(!hie->ie_content_length)
    hie->ie_content_ptr = NULL;
  else
    hie->ie_content_ptr = buf + MIN_HEADER_IE_LEN;

  return (MIN_HEADER_IE_LEN + hie->ie_content_length);
}

/***************************************************************************//**
 * @fn         parse_hdrie
 *
 * @brief      Parses header IEs
 *
 * @param[in]  buf - Pointer to the buffer including IE
 *             buf_len - Length of the IE buffer
 *             hie - Structure to store header IE
 *
 * @return     Parsed length of IEs
 ******************************************************************************/
static uint8_t
parse_hdrie(uint8_t* buf, uint8_t buf_len, ie802154e_hie_t* hie)
{
  uint8_t* cur_buf_ptr;
  int16_t  remain_buf_len;
  uint16_t ie_header;

  remain_buf_len = buf_len;
  cur_buf_ptr = buf;

  ie_header = MAKE_UINT16(cur_buf_ptr[0],cur_buf_ptr[1]);
  hie->ie_type = IE_UNPACKING(ie_header,IE_TYPE_S,IE_TYPE_P);
  hie->ie_elem_id = IE_UNPACKING(ie_header,IE_ID_H_S,IE_ID_H_P);
  hie->ie_content_length = IE_UNPACKING(ie_header,IE_LEN_H_S,IE_LEN_H_P);

  if(hie->ie_type != IE_TYPE_HEADER) {
    // TODO: SCHOI: Need to return different value
    return 0;
  }

  cur_buf_ptr += MIN_HEADER_IE_LEN;
  remain_buf_len -= MIN_HEADER_IE_LEN;

  if(remain_buf_len < hie->ie_content_length) {
    return 0;
  }

  hie->ie_content_ptr = cur_buf_ptr;

  return (MIN_HEADER_IE_LEN + hie->ie_content_length);
}

/***************************************************************************//**
 * @fn         ie802154e_parse_headerie
 *
 * @brief      Parses header IEs
 *
 * @param[in]  buf - Pointer to the buffer including IE
 *             buf_len - Length of the IE buffer
 *             hie - Structure to store header IE
 *
 * @return     Parsed length of IEs
 ******************************************************************************/
uint8_t
ie802154e_parse_headerie(uint8_t* buf, uint8_t buf_len, ie802154e_hie_t* hie)
{
  // check if valid header ie
  if(buf_len < MIN_HEADER_IE_LEN)
    return 0;

  return getHeaderIe(buf, hie);
}

/***************************************************************************//**
 * @fn         ie802154e_hdrielist_len
 *
 * @brief      Calculates header IE length
 *
 * @param[in]  buf - Pointer to the buffer including IE
 *             buf_len - Length of the IE buffer
 *
 * @return     Length of IEs
 ******************************************************************************/
uint8_t
ie802154e_hdrielist_len(uint8_t* buf, uint8_t buf_len)
{
  ie802154e_hie_t hie;
  uint16_t remainlen = buf_len;
  uint8_t *pbuf = buf;
  uint16_t elemlen;
  uint16_t hielen = 0;

  memset(&hie, 0, sizeof(hie));

  while(remainlen >= MIN_HEADER_IE_LEN)
  {
    if((elemlen = getHeaderIe(pbuf, &hie)) < MIN_HEADER_IE_LEN)
      break;

    pbuf += elemlen;
    remainlen -= elemlen;

    hielen += elemlen;

    if(hie.ie_elem_id == IE_ID_LIST_TERM1 || hie.ie_elem_id == IE_ID_LIST_TERM2)
      break;
  }

  return hielen;
}

/***************************************************************************//**
 * @fn         ie802154e_parse_acknack_time_correction
 *
 * @brief      Parses the ACK/NACK time correction IE
 *
 * @param[in]  buf - Pointer to the IE content
 *             arg - Structure to store IE information
 *
 * @return     None
 ******************************************************************************/
void
ie802154e_parse_acknack_time_correction(uint8_t* buf, ie_acknack_time_correction_t* arg)
{
  uint16_t time_sync_ack;

  time_sync_ack = ie802154e_get16(&buf[0]);
  arg->time_synch = time_sync_ack & 0x0fff;
  if(arg->time_synch > 2047)
    arg->time_synch -= 4096;

  // 2. 0x8000 is NACK
  arg->acknack = (time_sync_ack & 0x8000) >> 15;
  
  arg->rxTotal = ie802154e_get32(&buf[2]);      //zelin
  arg->rxRssi = buf[6];
}

/***************************************************************************//**
 * @fn         ie802154e_find_in_hdrielist
 *
 * @brief      Searchs the specific IE
 *
 * @param[in]  buf - Pointer to the IE buffer
 *             buf_len - Length of the IE buffer
 *             find_ie - Structure storing IE information
 *
 * @return     1 if found, 0 otherwise
 ******************************************************************************/
uint8_t
ie802154e_find_in_hdrielist(uint8_t* buf, uint8_t buf_len, ie802154e_hie_t* find_ie)
{
  ie802154e_hie_t hie;
  uint8_t  tot_parsed_len;
  uint8_t* cur_buf_ptr;
  int16_t  remain_buf_len;
  uint8_t  found;

  tot_parsed_len = 0;
  found = 0;
  while (1) {
    uint8_t  parsed_len;

    cur_buf_ptr = buf + tot_parsed_len;
    remain_buf_len = (int16_t) buf_len - (int16_t) tot_parsed_len;

    if(remain_buf_len < MIN_HEADER_IE_LEN) {
      break;
    }

    parsed_len = parse_hdrie(cur_buf_ptr, remain_buf_len, &hie);
    if(parsed_len == 0) {
      break;
    }

    if(hie.ie_elem_id == find_ie->ie_elem_id) {
      found = 1;
      find_ie->ie_content_length = parsed_len - MIN_HEADER_IE_LEN;
      find_ie->ie_content_ptr = hie.ie_content_ptr;
      break;
    }

    if(hie.ie_elem_id == IE_ID_LIST_TERM) {
      break;
    }

    tot_parsed_len += parsed_len;

  } // ends while loop

  return found;
}

