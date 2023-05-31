/*******************************************************************************
* @fn          hnd_get_nvm_params
*
* @brief       NVM parameters get handler function
*
* @param
*
* @return      none
********************************************************************************
*/
#if NVM_ENABLED 
#include "stdio.h"
#include <string.h>
#include "coap.h"

#include "json/jsonparse.h"
#include "nv_params.h"

extern uint16_t panId;

#define NVM_TLV_TYPE_MPSK           0x01
#define NVM_TLV_TYPE_PNID           0x02
#define NVM_TLV_TYPE_BCH0           0x03
#define NVM_TLV_TYPE_BCHMO          0x04
#define NVM_TLV_TYPE_ASRQTO         0x05
#define NVM_TLV_TYPE_SLFS           0x06
#define NVM_TLV_TYPE_KALP           0x07
#define NVM_TLV_TYPE_SCANI          0x08
#define NVM_TLV_TYPE_NSSL           0x09
#define NVM_TLV_TYPE_FXCN           0x0A
#define NVM_TLV_TYPE_TXPW           0x0B
#define NVM_TLV_TYPE_FXPA           0x0C
#define NVM_TLV_TYPE_RPLDD          0x0D
#define NVM_TLV_TYPE_COARCT         0x0E
#define NVM_TLV_TYPE_COAPPT         0x0F
#define NVM_TLV_TYPE_CODTPT         0x10
#define NVM_TLV_TYPE_COAOMX         0x11
#define NVM_TLV_TYPE_CODFTO         0x12
#define NVM_TLV_TYPE_PHYM           0x13
#define NVM_TLV_TYPE_DBGL           0x14
#define NVM_TLV_TYPE_COMMIT         0x15
#define NVM_TLV_TYPE_EOL            0x16

static int add_uchar_tlv(char *msg, int idx, uint8_t tlv_type, uint8_t tlv_val)
{
   msg[idx++] = tlv_type;
   msg[idx++] = 1;
   msg[idx++] = tlv_val;
   return(idx);
}

static int add_ushort_tlv(char *msg, int idx, uint8_t tlv_type, uint16_t tlv_val)
{
   msg[idx++] = tlv_type;
   msg[idx++] = 2;
   msg[idx++] = (tlv_val >> 8) & 0xFF;
   msg[idx++] = tlv_val & 0xFF;
   return(idx);
}

static void get_uchar_tlv(uint8_t *tlvdata, uint16_t idx, uint8_t lenact, int *error_p, uint8_t *outVal_p)
{
   if (lenact == 1)
   {
      *outVal_p = tlvdata[idx];
   }
   else
   {
      *error_p = 1;
   }
}

static void get_ushort_tlv(uint8_t *tlvdata, uint16_t idx, uint8_t lenact, int *error_p, uint16_t *outVal_p)
{
   if (lenact == 2)
   {
      *outVal_p = (tlvdata[idx] * 256) + tlvdata[idx+1];
   }
   else
   {
      *error_p = 1;
   }
}

void
hnd_get_nvm_params(coap_context_t  *ctx, struct coap_resource_t *resource,
                coap_address_t *peer, coap_pdu_t *request, str *token,
                coap_pdu_t *response)   //fengmo NVM
{
  char nvmParamsMsg[44];
  uint16_t msgSize = 0;
  size_t size;
  unsigned char *data;
  int chunkIdx = 0;

  coap_get_data(request, &size, &data);

  if (size > 0)
  {
     data[size] = 0;
     sscanf((char *)data, "%d", &chunkIdx);
#ifdef LINUX_GATEWAY
     //printf("###########hnd_get_nvm_params: %d\n", chunkIdx);
#endif
  }

  switch (chunkIdx)
  {
  case 0:  //43 bytes
     nvmParamsMsg[msgSize++] = NVM_TLV_TYPE_MPSK;
     nvmParamsMsg[msgSize++] = 16;
     memcpy(nvmParamsMsg+msgSize, nvParams.mac_psk, 16);
     msgSize += 16;

     msgSize = add_ushort_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_PNID, nvParams.panid);
     msgSize = add_uchar_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_BCH0, nvParams.bcn_chan_0);
     msgSize = add_uchar_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_BCHMO, nvParams.bcn_ch_mode);
     msgSize = add_ushort_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_ASRQTO, nvParams.assoc_req_timeout_sec);
     msgSize = add_ushort_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_SLFS, nvParams.slotframe_size);
     msgSize = add_ushort_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_KALP, nvParams.keepAlive_period);
     msgSize = add_uchar_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_SCANI, nvParams.scan_interval);
     break;
  case 1:  //40 bytes
     msgSize = add_uchar_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_NSSL, nvParams.num_shared_slot);
     msgSize = add_uchar_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_FXCN, nvParams.fixed_channel_num);
     msgSize = add_ushort_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_TXPW, nvParams.tx_power);
     msgSize = add_uchar_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_FXPA, nvParams.fixed_parent);
     msgSize = add_uchar_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_RPLDD, nvParams.rpl_dio_doublings);
     msgSize = add_ushort_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_COARCT, nvParams.coap_resource_check_time);
     msgSize = add_ushort_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_COAPPT, nvParams.coap_port);
     msgSize = add_ushort_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_CODTPT, nvParams.coap_dtls_port);
     msgSize = add_uchar_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_COAOMX, nvParams.coap_obs_max_non);
     msgSize = add_uchar_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_CODFTO, nvParams.coap_default_response_timeout);
     msgSize = add_uchar_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_PHYM, nvParams.phy_mode);
     msgSize = add_uchar_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_DBGL, nvParams.debug_level);
     break;
  default:  //3 bytes
     msgSize = add_uchar_tlv(nvmParamsMsg, msgSize, NVM_TLV_TYPE_EOL, 1);
     break;
  }

  response->hdr->code = coap_add_data(response, msgSize, (const unsigned char *)nvmParamsMsg) ?
    COAP_RESPONSE_CODE(205) : COAP_RESPONSE_CODE(413);
}
/*******************************************************************************
* @fn          hnd_put_nvm_params
*
* @brief
*
* @param
*
* @return      none
********************************************************************************
*/
void hnd_put_nvm_params(coap_context_t  *ctx, struct coap_resource_t *resource,
         coap_address_t *peer, coap_pdu_t *request, str *token,
         coap_pdu_t *response)
{
   int error = 0;
   size_t size;
   unsigned char *tlvdata;
   int idx = 0;
   uint8_t type;
   uint8_t len;
   uint8_t lenrem;
   uint8_t lenact;

   coap_get_data(request, &size, &tlvdata);

   while (idx < size)
   {
      type = tlvdata[idx++];
      len = tlvdata[idx++];
      lenrem = size - idx;
      lenact = (len <= lenrem) ? len : lenrem;
      switch (type)
      {
      case NVM_TLV_TYPE_MPSK:
         if (lenact == 16)
         {
            memcpy(nvParams.mac_psk, tlvdata+idx, lenact);
         }
         else
         {
            error = 1;
         }
         break;
      case NVM_TLV_TYPE_PNID:
         get_ushort_tlv(tlvdata, idx, lenact, &error, &nvParams.panid);
         break;
      case NVM_TLV_TYPE_BCH0:
         get_uchar_tlv(tlvdata, idx, lenact, &error, &nvParams.bcn_chan_0);
         break;
      case NVM_TLV_TYPE_BCHMO:
         get_uchar_tlv(tlvdata, idx, lenact, &error, &nvParams.bcn_ch_mode);
         break;
      case NVM_TLV_TYPE_ASRQTO:
         get_ushort_tlv(tlvdata, idx, lenact, &error, &nvParams.assoc_req_timeout_sec);
         break;
      case NVM_TLV_TYPE_SLFS:
         get_ushort_tlv(tlvdata, idx, lenact, &error, &nvParams.slotframe_size);
         break;
      case NVM_TLV_TYPE_KALP:
         get_ushort_tlv(tlvdata, idx, lenact, &error, &nvParams.keepAlive_period);
         break;
      case NVM_TLV_TYPE_SCANI:
         get_uchar_tlv(tlvdata, idx, lenact, &error, &nvParams.scan_interval);
         break;
      case NVM_TLV_TYPE_NSSL:
         get_uchar_tlv(tlvdata, idx, lenact, &error, &nvParams.num_shared_slot);
         break;
      case NVM_TLV_TYPE_FXCN:
         get_uchar_tlv(tlvdata, idx, lenact, &error, &nvParams.fixed_channel_num);
         break;
      case NVM_TLV_TYPE_TXPW:
         get_ushort_tlv(tlvdata, idx, lenact, &error, &nvParams.tx_power);
         break;
      case NVM_TLV_TYPE_FXPA:
         get_uchar_tlv(tlvdata, idx, lenact, &error, &nvParams.fixed_parent);
         break;
      case NVM_TLV_TYPE_RPLDD:
         get_uchar_tlv(tlvdata, idx, lenact, &error, &nvParams.rpl_dio_doublings);
         break;
      case NVM_TLV_TYPE_COARCT:
         get_ushort_tlv(tlvdata, idx, lenact, &error, &nvParams.coap_resource_check_time);
         break;
      case NVM_TLV_TYPE_COAPPT:
         get_ushort_tlv(tlvdata, idx, lenact, &error, &nvParams.coap_port);
         break;
      case NVM_TLV_TYPE_CODTPT:
         get_ushort_tlv(tlvdata, idx, lenact, &error, &nvParams.coap_dtls_port);
         break;
      case NVM_TLV_TYPE_COAOMX:
         get_uchar_tlv(tlvdata, idx, lenact, &error, &nvParams.coap_obs_max_non);
         break;
      case NVM_TLV_TYPE_CODFTO:
         get_uchar_tlv(tlvdata, idx, lenact, &error, &nvParams.coap_default_response_timeout);
         break;
      case NVM_TLV_TYPE_PHYM:
         get_uchar_tlv(tlvdata, idx, lenact, &error, &nvParams.phy_mode);
         break;
      case NVM_TLV_TYPE_DBGL:
         get_uchar_tlv(tlvdata, idx, lenact, &error, &nvParams.debug_level);
         break;
      case NVM_TLV_TYPE_COMMIT:
         NVM_update();
         coap_add_data(response, 9, "COMMIT_OK");
#ifdef LINUX_GATEWAY
         {
            extern uint8_t globalTermination;
            printf("Please restart the gateway after the root node NVM update\n");
            globalTermination  = 1;
         }
#else
         {
            extern uint8_t globalRestart;
            globalRestart = 1;
         }
#endif
         break;
      default:
         break;
      }

      idx = idx + len;
   }

   response->hdr->code = (error) ? COAP_RESPONSE_CODE(413) : COAP_RESPONSE_CODE(204);
}
#endif