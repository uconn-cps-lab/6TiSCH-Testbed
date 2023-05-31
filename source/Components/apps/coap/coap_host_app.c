/*
 * Copyright (c) 2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *  ====================== coap_host_app.c =============================================
 *  This file is coap server application file to run in the sensor device node.
 */

/*******************************************************************************
*                                            INCLUDES
*******************************************************************************/
#include "stdio.h"
#include <string.h>
#include "config.h"

#include "Board.h"
#include "coap.h"
#include "net/tcpip.h"

#include "stdbool.h"

#include "led.h"

#include "webmsg/webmsg.h"
#include "uip_rpl_process.h"
#include "hct_msp432/hct/hct.h" //Jiachen

#include "nhl_api.h"
#include "nhl_setup.h"
#include "nhl_mgmt.h"
#include "mac_config.h"

#include "coap_host_app.h"

#include "harp.h" // jiachen

#ifdef WITH_JSON
#include "json/jsontree.h"
json_output json_output_context;
#endif

#include "SensorBatMon.h"

#ifndef HARP_MINIMAL

#if SENSORTAG || I3_MOTE
#include "SensorI2C.h"
#include "SensorTmp007.h"
#include "SensorHDC1000.h"
#include "SensorOpt3001.h"
#include "SensorBmp280.h"
#include "SensorMpu9250.h"
#include "SensorLIS2HH12.h"
#include "SensorLIS2DW12.h"
#endif
#endif
#include "pwrest/pwrest.h"

#if I3_MOTE_SPLIT_ARCH
#include "hct_msp432/hct/msp432hct.h"
#endif

#if FEATURE_DTLS
#include "../../dtls/dtls_client.h"
#endif

#include "nv_params.h"

#include "mac_reset.h"
#include "coap_common_app.h"
#if DMM_ENABLE
#include "dmm_scheduler.h"
#endif //DMM_ENABLE

#define DTLS_MAC_HANDSHAKE_TIMEOUT (30 * CLOCK_SECOND)

static uip_ipaddr_t server_ipaddr;

#ifndef HARP_MINIMAL
coap_resource_t *sensor_resource;
int COAP_SENSOR_RESOURCE_DIRTY_FREQ = 50; // every 50*10 slots
coap_resource_t *diagnosis_resource;
#endif

/* -----------------------------------------------------------------------------
 *                                         Local Variables
 * -----------------------------------------------------------------------------
 */
#ifndef HARP_MINIMAL
static uint8_t msg_ptr[150], msg_size;
#endif
#if NVM_ENABLED
static coap_resource_t *nvm_params_resource;
#endif
uint8_t globalRestart = 0;

static coap_resource_t *schedule_resource;
static coap_resource_t *ping_resource;

uint16_t diag_cnt;
/* -----------------------------------------------------------------------------
 *                                         Global Variables
 * -----------------------------------------------------------------------------
 */
coap_context_t *coap_context;
uint16_t coapObserveDis = 0;

uint16_t led_status = 0;
uint32_t coap_ObservePeriod = 11000;

// Jiachen
extern uint64_t packet_sent_asn_ping;
extern uint16_t received_asn;

uint16_t lostCoapPacketCount = 0;
#if DMM_ENABLE && BLE_LOCALIZATION_BEFORE_TISCH
uint8_t nwPerfTpe = 2;
#else
uint8_t nwPerfTpe = 0;
#endif //DMM_ENABLE && BLE_LOCALIZATION_BEFORE_TISCH
extern void TSCH_scan_timer_handler();
extern void TSCH_start_scan_handler();

#if NVM_ENABLED
extern void hnd_get_nvm_params(coap_context_t *ctx, struct coap_resource_t *resource,
                               coap_address_t *peer, coap_pdu_t *request, str *token,
                               coap_pdu_t *response); //fengmo NVM
extern void hnd_put_nvm_params(coap_context_t *ctx, struct coap_resource_t *resource,
                               coap_address_t *peer, coap_pdu_t *request, str *token,
                               coap_pdu_t *response);
#endif
/*******************************************************************************
 * @fn          coap_event_handler
 *
 * @brief       CoAP event handler
 *
 * @param       *ctx - pointer to CoAP context
 *
 * @return      none
 *******************************************************************************
 */
#if (FEATURE_DTLS)
/*******************************************************************************
* @fn
*
* @brief       Post the DTLS connect to the COAP mail box
*
* @param
*
* @return      none
********************************************************************************
*/
static void coap_connect_dtls_mac_sec(uip_ipaddr_t *destipaddr_p)
{
  TCPIP_cmdmsg_t msg;
  msg.event_type = COAP_DTLS_MAC_SEC_CONNECT;
  msg.data = destipaddr_p;
  Mailbox_post(coap_mailbox, &msg, BIOS_NO_WAIT);
}

/***************************************************************************/ /**
 * @fn          macSecurityStartKeyExchangeWithParent
 *
 * @brief      start the key exchange with the parent
 *
 * @param[in]  *destipaddr_p, IPv6 adress of the peer
 *
 * @param[Out] N/A
 * @return
 ******************************************************************************/
void macSecurityStartKeyExchangeWithParent(uip_ipaddr_t *destipaddr_p)
{
  clock_time_t now = clock_time();
  coap_connect_dtls_mac_sec(destipaddr_p);
  macSecurityPib.macDeviceTable[0].keyUsage = MAC_KEY_USAGE_NEGOTIATING;
  macSecurityPib.macDeviceTable[0].handshakeStartTime = now;
  macSecurityPib.macDeviceTable[0].connectAttemptCount++;
}

/***************************************************************************/ /**
 * @fn          macSecurityKeyExchangeWithParent
 *
 * @brief      check to see if key exchange with the parent should be started
 *
 * @param[in]  None
 *
 * @param[Out] N/A
 * @return     1, if needs to start, 0 otherwise
 ******************************************************************************/
uint8_t macSecurityKeyExchangeWithParent()
{
  uint8_t retVal = 0;
  clock_time_t now = clock_time();

  if ((macSecurityPib.macDeviceTable[0].shortAddress != 0) &&
      ((macSecurityPib.macDeviceTable[0].keyUsage == 0) ||
       (((macSecurityPib.macDeviceTable[0].keyUsage & MAC_KEY_USAGE_TX) == 0) &&
        (clock_delta(now, macSecurityPib.macDeviceTable[0].handshakeStartTime) > DTLS_MAC_HANDSHAKE_TIMEOUT))))
  {
    if (macSecurityPib.macDeviceTable[0].connectAttemptCount < MAC_SEC_MAX_DTLS_CONNECT_ATTEMPT)
    {
      retVal = 1;
    }
  }

  return (retVal);
}

/*******************************************************************************
 * @fn          coap_mac_sec_event_handler
 *
 * @brief       CoAP coap_mac_sec_event_handler handler
 *
 * @param
 *
 * @return      none
 *******************************************************************************
 */
void coap_mac_sec_event_handler(void *ctx)
{
//This function should never be called.  Something is wrong!
//For MAC sec, DTLS performs handshake only.   There should be no DTLS payload data for MAC sec
#if 1
  volatile int errloop = 0;
  while (1)
  {
    errloop++;
  }
#endif
}
#endif //FEATURE_DTLS

void coap_event_handler(coap_context_t *ctx)
{
  // Read from CoAP ctx - receive queue and parse data into new PDU
  coap_read(ctx);
  // Read newly parsed PDU and handle request/response - includes sending message
  coap_dispatch(ctx);
}

/*******************************************************************************
* @fn          json_putchar
*
* @brief
*
* @param
*
* @return      none
********************************************************************************
*/
#ifdef WITH_JSON
static int json_putchar(int c)
{
  if (json_output_context.outbuf_pos < 128)
  {
    json_output_context.outbuf[json_output_context.outbuf_pos++] = c;
    return c;
  }
  return 0;
}
#endif

// not recommand for HARP
//unsigned char* resp_data;
//static void client_response_handler(struct coap_context_t  *ctx,
//					const coap_address_t *remote,
//					coap_pdu_t *sent,
//					coap_pdu_t *received,
//					const coap_tid_t id)
//{
//
//	size_t         data_len;
//	if (COAP_RESPONSE_CLASS(received->hdr->code) == 2)
//	{
//		if (coap_get_data(received, &data_len, &resp_data))
//		{
////			printf("Received: %s\n", data);
//		}
//	}
//}

/*******************************************************************************
* @fn          hnd_get_sensor
*
* @brief       Sensor get handler function
*
* @param
*
* @return      none
********************************************************************************
*/
#ifndef HARP_MINIMAL
void hnd_get_sensors(coap_context_t *ctx, struct coap_resource_t *resource,
                     coap_address_t *peer, coap_pdu_t *request, str *token,
                     coap_pdu_t *response)
{

  coap_opt_iterator_t opt_iter;
  coap_subscription_t *subscription;

  if (request != NULL && coap_check_option(request, COAP_OPTION_OBSERVE, &opt_iter))
  {
    subscription = coap_add_observer(resource, peer, token);
    if (subscription)
    {
      subscription->non = request->hdr->type == COAP_MESSAGE_NON;
      coap_add_option(response, COAP_OPTION_OBSERVE, 0, NULL);
    }
  }
  else if (request != NULL)
  {
    coap_delete_observer(resource, peer, token);
  }
  if (request == NULL)
  {
    coap_add_option(response, COAP_OPTION_OBSERVE,
                    coap_encode_var_bytes(msg_ptr, ctx->observe), msg_ptr);
  }

  webmsg_update(msg_ptr, &msg_size, (uint8_t)(resource->seq));

  response->hdr->code = coap_add_data(response, msg_size, msg_ptr) ? COAP_RESPONSE_CODE(205) : COAP_RESPONSE_CODE(413);
}
#endif
/*******************************************************************************
* @fn          hnd_put_led
*
* @brief       This function turn LED on or off based on put message
*
* @param
*
* @return      none
********************************************************************************
*/
//extern TSCH_DB_handle_t db_hnd;
void hnd_put_led(coap_context_t *ctx, struct coap_resource_t *resource,
                 coap_address_t *peer, coap_pdu_t *request, str *token,
                 coap_pdu_t *response)
{
  size_t size;
  unsigned char *data;

  coap_get_data(request, &size, &data);
  //    db_hnd.slotframes[0].vals.slotframe_size = 200;
  //     if(size == 0)
  //    {
  //        response->hdr->code = COAP_RESPONSE_CODE(400);
  //        return;
  //    }
  //    else
  //    {
  //        data[size] = '\0';
  //        uint8_t onoff=(atoi((const char *)data) == 1);
  //        if(onoff==led_status)
  //        {
  //            LED_set(2, onoff);
  //            response->hdr->code = COAP_RESPONSE_CODE(203);
  //        }
  //        else
  //        {
  //            LED_set(2, onoff);
  //            response->hdr->code = COAP_RESPONSE_CODE(204);
  //        }
  //        led_status=onoff;
  //
  //    }


  COAP_SENSOR_RESOURCE_DIRTY_FREQ = atoi((const char *)data);
    char msg[64];
  sprintf(msg, "set freq to every %d 10-slots\n", COAP_SENSOR_RESOURCE_DIRTY_FREQ);
  uint8_t msgSize = 64;
  
//  HARP_mailbox_msg_t mmsg;
//  mmsg.type = HARP_MAILBOX_DUMMY;
//  Mailbox_post(harp_mailbox, &mmsg, BIOS_NO_WAIT);

  response->hdr->code = coap_add_data(response, msgSize, (const unsigned char *)msg) ? COAP_RESPONSE_CODE(205) : COAP_RESPONSE_CODE(413);
}

/*******************************************************************************
* @fn          hnd_get_diagnosis
*
* @brief       network diagnosis information get handler function
*
* @param
*
* @return      none
********************************************************************************
*/
#ifndef HARP_MINIMAL
void hnd_get_diagnosis(coap_context_t *ctx, struct coap_resource_t *resource,
                       coap_address_t *peer, coap_pdu_t *request, str *token,
                       coap_pdu_t *response)
{
  diag_cnt++;
  char diagNosisMsg[100], msgSize;
  uint8_t len;
  coap_opt_iterator_t opt_iter;
  coap_subscription_t *subscription;

  if (request != NULL && coap_check_option(request, COAP_OPTION_OBSERVE, &opt_iter))
  {
    subscription = coap_add_observer(resource, peer, token);
    if (subscription)
    {
      subscription->non = request->hdr->type == COAP_MESSAGE_NON;
      coap_add_option(response, COAP_OPTION_OBSERVE, 0, NULL);
    }
  }
  else if (request != NULL)
  {
    coap_delete_observer(resource, peer, token);
  }
  if (request == NULL)
  {
    coap_add_option(response, COAP_OPTION_OBSERVE,
                    coap_encode_var_bytes(msg_ptr, ctx->observe), msg_ptr);
  }
  msgSize = 0;
  memcpy(diagNosisMsg, &nwPerfTpe, 1);
  msgSize += 1;

  if (nwPerfTpe == 0)
  {
    uint8_t i;
    int8_t rssi;
    uint8_t channel;
    uint8_t count = 0;

    uint32_t bitMap0_31 = 0;
    uint32_t bitMap32_63 = 0;

    for (i = 0; i < 64; i++)
    {
      if (ULPSMAC_Dbg.numTxTotalPerCh[i] > 0)
      {
        count++;
        if (i <= 31)
        {
          bitMap0_31 |= (1 << i);
        }
        else
        {
          bitMap32_63 |= (1 << (i - 32));
        }
      }
    }

    /* Due to message size limitation, do not send channel information if over 
    the 128 MAC frame length*/
    memcpy(diagNosisMsg + msgSize, &len, 1);
    msgSize += 1;
    if (count <= 16)
    {
      memcpy(diagNosisMsg + msgSize, &bitMap0_31, 4);
      msgSize += 4;
      memcpy(diagNosisMsg + msgSize, &bitMap32_63, 4);
      msgSize += 4;
      for (i = 0; i < 64; i++)
      {
        if (ULPSMAC_Dbg.numTxTotalPerCh[i] > 0)
        {
          rssi = (int8_t)ULPSMAC_Dbg.avgRssi[i];
          memcpy(diagNosisMsg + msgSize, &rssi, 1);
          msgSize += 1;
          rssi = (int8_t)ULPSMAC_Dbg.avgRxRssi[i];
          memcpy(diagNosisMsg + msgSize, &rssi, 1);
          msgSize += 1;
          memcpy(diagNosisMsg + msgSize, &ULPSMAC_Dbg.numTxNoAckPerCh[i], 1);
          msgSize += 1;
          memcpy(diagNosisMsg + msgSize, &ULPSMAC_Dbg.numTxTotalPerCh[i], 1);
          msgSize += 1;
        }
      }
    }

    memcpy(diagNosisMsg + msgSize, &ULPSMAC_Dbg.numTxFail, 4);
    msgSize += 4;
    memcpy(diagNosisMsg + msgSize, &ULPSMAC_Dbg.numTxNoAck, 4);
    msgSize += 4;
    memcpy(diagNosisMsg + msgSize, &ULPSMAC_Dbg.numTxTotal, 4);
    msgSize += 4;
    memcpy(diagNosisMsg + msgSize, &ULPSMAC_Dbg.numRxTotal, 4);
    msgSize += 4;
    memcpy(diagNosisMsg + msgSize, &ULPSMAC_Dbg.numTxLengthTotal, 4);
    msgSize += 4;

    memset(&ULPSMAC_Dbg.avgRssi, 0, sizeof(ULPSMAC_Dbg.avgRssi));
    memset(&ULPSMAC_Dbg.rssiCount, 0, sizeof(ULPSMAC_Dbg.rssiCount));
    memset(&ULPSMAC_Dbg.avgRxRssi, 0, sizeof(ULPSMAC_Dbg.avgRxRssi));
    memset(&ULPSMAC_Dbg.rxRssiCount, 0, sizeof(ULPSMAC_Dbg.rxRssiCount));
    memset(&ULPSMAC_Dbg.numTxNoAckPerCh, 0, sizeof(ULPSMAC_Dbg.numTxNoAckPerCh));
    memset(&ULPSMAC_Dbg.numTxTotalPerCh, 0, sizeof(ULPSMAC_Dbg.numTxTotalPerCh));
  }
  else if (nwPerfTpe == 1)
  {
    /* Topology Information*/
    memcpy(diagNosisMsg + msgSize, &ULPSMAC_Dbg.parent, 2);
    msgSize += 2;
    memcpy(diagNosisMsg + msgSize, &ULPSMAC_Dbg.numParentChange, 2);
    msgSize += 2;
    memcpy(diagNosisMsg + msgSize, &ULPSMAC_Dbg.numSyncLost, 2);
    msgSize += 2;
    memcpy(diagNosisMsg + msgSize, &ULPSMAC_Dbg.avgSyncAdjust, 2);
    msgSize += 2;
    memcpy(diagNosisMsg + msgSize, &ULPSMAC_Dbg.maxSyncAdjust, 2);
    msgSize += 2;

    /*Internal Packet Drop Counting*/
    memcpy(diagNosisMsg + msgSize, &ULPSMAC_Dbg.numOutOfBuffer, 2);
    msgSize += 2;
    memcpy(diagNosisMsg + msgSize, &uipRxPacketLossCount, 2);
    msgSize += 2;
    memcpy(diagNosisMsg + msgSize, &lowpanTxPacketLossCount, 2);
    msgSize += 2;
    memcpy(diagNosisMsg + msgSize, &lowpanRxPacketLossCount, 2);
    msgSize += 2;
    memcpy(diagNosisMsg + msgSize, &lostCoapPacketCount, 2);
    msgSize += 2;
    memcpy(diagNosisMsg + msgSize, &coapObserveDis, 2);
    msgSize += 2;

    ULPSMAC_Dbg.avgSyncAdjust = 0;
    ULPSMAC_Dbg.maxSyncAdjust = 0;
    ULPSMAC_Dbg.numSyncAdjust = 0;
#if DMM_ENABLE && BLE_LOCALIZATION_BEFORE_TISCH
    //BLE beacon reports
    if (beaconMeas.reportToGwCount >= 3 || beaconMeas.length == 0)
    {
      //Skip the BLE beacon report
      nwPerfTpe++;
    }
#else
    nwPerfTpe++;
#endif //DMM_ENABLE
  }
#if DMM_ENABLE && BLE_LOCALIZATION_BEFORE_TISCH
  else if (nwPerfTpe == 2)
  {
    //BLE beacon reports
    uint8_t i;
    for (i = 0; i < beaconMeas.length; i++)
    {
      if (beaconMeas.rssiCount[i] < 24) //Need at least 24 beacon measurements
      {
        continue;
      }
      int8_t rssi = beaconMeas.rssi[i] / beaconMeas.rssiCount[i] - 128;
      memcpy(diagNosisMsg + msgSize, &(beaconMeas.eui[i]), 4);
      msgSize += 4;
      memcpy(diagNosisMsg + msgSize, &rssi, 1);
      msgSize += 1;
    }
    beaconMeas.reportToGwCount += 1;
  }
#endif //DMM_ENABLE
  nwPerfTpe = (nwPerfTpe + 1) % 3;

  // Jiachen
  uint8_t type = HCT_TLV_TYPE_COAP_DATA_TYPE;
  memcpy(diagNosisMsg + msgSize, &type, 1);
  msgSize += 1;
  uint8_t length = 1;
  memcpy(diagNosisMsg + msgSize, &length, 1);
  msgSize += 1;
  uint8_t tag = TAG_COAP_DIAGNOSIS;
  memcpy(diagNosisMsg + msgSize, &tag, 1);
  msgSize += 1;

  response->hdr->code = coap_add_data(response, msgSize, (const unsigned char *)diagNosisMsg) ? COAP_RESPONSE_CODE(205) : COAP_RESPONSE_CODE(413);
}

/*******************************************************************************
* @fn          hnd_put_diagnosis
*
* @brief       
*
* @param
*
* @return      none
********************************************************************************
*/
void hnd_put_diagnosis(coap_context_t *ctx, struct coap_resource_t *resource,
                       coap_address_t *peer, coap_pdu_t *request, str *token,
                       coap_pdu_t *response)
{
  size_t size;
  unsigned char *data;

  coap_get_data(request, &size, &data);

  if (size == 0)
  {
    response->hdr->code = COAP_RESPONSE_CODE(400);
    return;
  }
  else
  {
    data[size] = '\0';
    uint16_t parent = atoi((const char *)data);
    TSCH_MACConfig.restrict_to_node = parent;
    TSCH_prepareDisConnect();
    TSCH_restartNode();
  }
}
#endif
/*******************************************************************************
* @fn          hnd_get_dummy
*
* @brief       get dummy message: return last_sent_asn to measure e2e latency
*
* @param
*
* @return      none
********************************************************************************
*/
void hnd_get_dummy(coap_context_t *ctx, struct coap_resource_t *resource,
                   coap_address_t *peer, coap_pdu_t *request, str *token,
                   coap_pdu_t *response)
{
  char dummyMsg[32];
  uint8_t type, length;
  uint8_t msgSize = 0;

  type = HCT_TLV_TYPE_COAP_DATA_TYPE;
  memcpy(dummyMsg + msgSize, &type, 1);
  msgSize += 1;
  length = 1;
  memcpy(dummyMsg + msgSize, &length, 1);
  msgSize += 1;
  uint8_t tag = TAG_COAP_PING;
  memcpy(dummyMsg + msgSize, &tag, 1);
  msgSize += 1;

  response->hdr->code = coap_add_data(response, msgSize, (const unsigned char *)dummyMsg) ? COAP_RESPONSE_CODE(205) : COAP_RESPONSE_CODE(413);
}

/*******************************************************************************
* @fn          hnd_put_dummy
*
* @brief       put dummy message: return measured rtt(e2e latency)
*
* @param
*
* @return      none
********************************************************************************
*/
uint16_t rtt;
void hnd_put_dummy(coap_context_t *ctx, struct coap_resource_t *resource,
                   coap_address_t *peer, coap_pdu_t *request, str *token,
                   coap_pdu_t *response)
{
  char dummyMsg[32];
  uint8_t type, length;
  uint8_t msgSize = 0;

  // Jiachen
  rtt = received_asn - (uint16_t)packet_sent_asn_ping;
  type = HCT_TLV_TYPE_RTT;
  memcpy(dummyMsg + msgSize, &type, 2);
  msgSize += 1;
  length = 2;
  memcpy(dummyMsg + msgSize, &length, 1);
  msgSize += 1;
  memcpy(dummyMsg + msgSize, &rtt, 2);
  msgSize += 2;

  response->hdr->code = coap_add_data(response, msgSize, (const unsigned char *)dummyMsg) ? COAP_RESPONSE_CODE(205) : COAP_RESPONSE_CODE(413);
}

/*******************************************************************************
* @fn          init_resources
*
* @brief       This function initializes CoAP resources
*
* @param       ctx - CoAP context
*
* @return      none
********************************************************************************
*/
void init_resources(coap_context_t *ctx)
{

  debug("init resources\n");

#ifndef HARP_MINIMAL
  //  // listen response of sent requests
  //  coap_register_response_handler(ctx, client_response_handler);
  /* This is just one single resource to encapsulate all sensor readings */
  sensor_resource = coap_resource_init("sensors", 7, 0);
  coap_register_handler(sensor_resource, COAP_REQUEST_GET, hnd_get_sensors);
  sensor_resource->observable = 1;
  /* Add attributes as needed - ct=50 for JSON, ct=0 for plain text */
#ifdef WITH_JSON
  coap_add_attr(sensor_resource, (unsigned char *)"ct", 2, (unsigned char *)"50", 2, 0);
#else
  coap_add_attr(sensor_resource, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
#endif /* WITH_JSON */
  coap_add_resource(ctx, sensor_resource);

#endif

  /* Resource for HARP */
  coap_resource_t *harp_init_resource;
  harp_init_resource = coap_resource_init("harp_init", 9, 0);
  coap_register_handler(harp_init_resource, COAP_REQUEST_POST, hnd_post_harp_init);
  coap_add_attr(harp_init_resource, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
  coap_add_resource(ctx, harp_init_resource);

  coap_resource_t *harp_iface_resource;
  harp_iface_resource = coap_resource_init("harp_iface", 10, 0);
  coap_register_handler(harp_iface_resource, COAP_REQUEST_POST, hnd_post_harp_iface);
  coap_register_handler(harp_iface_resource, COAP_REQUEST_GET, hnd_get_harp_iface);
  coap_register_handler(harp_iface_resource, COAP_REQUEST_PUT, hnd_put_harp_iface);
  coap_add_attr(harp_iface_resource, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
  coap_add_resource(ctx, harp_iface_resource);

  coap_resource_t *harp_sp_resource;
  harp_sp_resource = coap_resource_init("harp_sp", 7, 0);
  coap_register_handler(harp_sp_resource, COAP_REQUEST_POST, hnd_post_harp_sp);
  coap_register_handler(harp_sp_resource, COAP_REQUEST_GET, hnd_get_harp_sp);
  coap_register_handler(harp_sp_resource, COAP_REQUEST_PUT, hnd_put_harp_sp);
  coap_add_attr(harp_sp_resource, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
  coap_add_resource(ctx, harp_sp_resource);

  /* Resource to turn on/off all LEDS */
  coap_resource_t *led_resource;
  led_resource = coap_resource_init("led", 3, 0);
  coap_register_handler(led_resource, COAP_REQUEST_PUT, hnd_put_led);
  coap_add_attr(led_resource, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
  coap_add_resource(ctx, led_resource);

  /* Resource for netowrk diagnosis information*/
#ifndef HARP_MINIMAL
  diagnosis_resource = coap_resource_init("diagnosis", 9, 0);
  diagnosis_resource->observable = 1;
  coap_register_handler(diagnosis_resource, COAP_REQUEST_GET, hnd_get_diagnosis);

  coap_register_handler(diagnosis_resource, COAP_REQUEST_PUT, hnd_put_diagnosis);
  coap_add_attr(diagnosis_resource, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
  coap_add_resource(ctx, diagnosis_resource);
#endif

#if NVM_ENABLED
  /* Resource for NVM parameters*/ //fengmo NVM
  nvm_params_resource = coap_resource_init("nvmparams", 9, 0);
  coap_register_handler(nvm_params_resource, COAP_REQUEST_GET, hnd_get_nvm_params);
  coap_register_handler(nvm_params_resource, COAP_REQUEST_PUT, hnd_put_nvm_params);
  //coap_add_attr(nvm_params_resource, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
  coap_add_resource(ctx, nvm_params_resource);
#endif

  /* Resource for scheduler interface */
  schedule_resource = coap_resource_init("schedule", 8, 0);
  coap_register_handler(schedule_resource, COAP_REQUEST_DELETE, hnd_delete_schedule);
  coap_register_handler(schedule_resource, COAP_REQUEST_PUT, hnd_put_schedule);
  coap_register_handler(schedule_resource, COAP_REQUEST_GET, hnd_get_schedule);
  coap_add_attr(schedule_resource, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
  coap_add_resource(ctx, schedule_resource);

  /* Resource for PING */
  ping_resource = coap_resource_init("ping", 4, 0);
  coap_register_handler(ping_resource, COAP_REQUEST_GET, hnd_get_dummy);
  coap_register_handler(ping_resource, COAP_REQUEST_PUT, hnd_put_dummy); // Jiachen
  coap_add_resource(ctx, ping_resource);
}

/*******************************************************************************
* @fn          get_context
*
* @brief       This function gets context based IP address and UDP port
*
* @param       ipaddr - IP address
*              port - udp port
*
* @return      none
********************************************************************************
*/
coap_context_t *get_context(uip_ipaddr_t *ipaddr_p, unsigned short port)
{
  coap_context_t *ctx = NULL;

  coap_address_t addr;
  addr.addr = *ipaddr_p;
  addr.port = port;

  ctx = coap_new_context(&addr);

  return ctx;
}

/*******************************************************************************
* @fn          udp_receive_callback
*
* @brief       UDP Connection Callback Function
*
* @param       *c - udp connection socket pointer
*              *source_addr - source IP address
*              source_port - UDP source port
*              *dest_addr - destination IP address
*              dest_port - UDP desintation port
*              *data - pointer to data message
*              dataLen - data message length
*
* @return      none
********************************************************************************
*/
void udp_receive_callback(struct udp_simple_socket *c, void *ptr,
                          const uip_ipaddr_t *source_addr,
                          uint16_t source_port,
                          const uip_ipaddr_t *dest_addr,
                          uint16_t dest_port, const char *data,
                          uint16_t datalen)
{
  TCPIP_cmdmsg_t msg;

  if (appRxLenFifo[appRxFifo_WriteIdx] != 0)
  {
    lostCoapPacketCount++;
  }
  else
  {
    //fengmo debug node packet loss begin
    if (datalen <= UIP_BUFSIZE - UIP_IPUDPH_LEN)
    {
      memcpy(appRxFifo[appRxFifo_WriteIdx].c, uip_buf, datalen + UIP_IPUDPH_LEN); //fengmo debug node packet loss, Jira 44, done
    }
    else
    {
      COAP_fatalErrorHandler();
    }

    appRxLenFifo[appRxFifo_WriteIdx] = datalen;
    appRxFifoIndex_inc(&appRxFifo_WriteIdx);

    msg.event_type = COAP_DATA;

    if (Mailbox_post(coap_mailbox, &msg, BIOS_NO_WAIT) != TRUE)
    {
      TCPIP_Dbg.tcpip_post_app_err++;
    }
  }
}
#if FEATURE_DTLS
/*******************************************************************************
* @fn          coap_dtls_timer_handler
*
* @brief       Post the DTLS timer expiration event to the COAP mail box
*
* @param       event:  Should have a value of EVENT_TIMER
*              data: Should point to the timer data structure
*
* @return      none
********************************************************************************
*/
void coap_dtls_timer_handler(process_event_t event, process_data_t data)
{
  TCPIP_cmdmsg_t msg;

  if (event == EVENT_TIMER)
  {
    msg.event_type = COAP_DTLS_TIMER;
    Mailbox_post(coap_mailbox, &msg, BIOS_NO_WAIT);
  }
}

/*******************************************************************************
* @fn          coap_mac_sec_timer_handler
*
* @brief       Post the MAC sec timer expiration event to the COAP mail box
*
* @param       event:  Should have a value of EVENT_TIMER
*              data: Should point to the timer data structure
*
* @return      none
********************************************************************************
*/
void coap_mac_sec_timer_handler(process_event_t event, process_data_t data)
{
  TCPIP_cmdmsg_t msg;

  if (event == EVENT_TIMER)
  {
    msg.event_type = COAP_MAC_SEC_TIMER;
    Mailbox_post(coap_mailbox, &msg, BIOS_NO_WAIT);
  }
}

process_obj_t coap_dtls_timer_handler_obj = {.process_post = coap_dtls_timer_handler};
process_obj_t coap_mac_sec_timer_handler_obj = {.process_post = coap_mac_sec_timer_handler};
#endif
/*******************************************************************************
* @fn          coap_socket_open
*
* @brief       Can be triggered when node desync and resync
*
* @param
*
* @return      none
********************************************************************************
*/
void coap_socket_open(void)
{
  udp_simple_socket_open(&coap_context->udp_socket, NULL, 0, coapConfig.coap_port, NULL, udp_receive_callback);
#if FEATURE_DTLS
  udp_simple_socket_open(&coap_context->udp_dtls_socket, NULL, 0, coapConfig.coap_dtls_port, NULL, udp_receive_callback);
  dtls_set_udp_socket(dtls_context[0], &coap_context->udp_dtls_socket);
  udp_simple_socket_open(&coap_context->udp_mac_sec_socket, NULL, 0, coapConfig.coap_mac_sec_port, NULL, udp_receive_callback);
  dtls_set_udp_socket(dtls_context[1], &coap_context->udp_mac_sec_socket);
#endif
}

/*******************************************************************************
* @fn
*
* @brief
*
* @param
*
* @return      none
********************************************************************************/
void coap_msg_post_scan_timer() //JIRA50
{
  TCPIP_cmdmsg_t msg;
  msg.event_type = COAP_RESTART_SCAN;

  Mailbox_post(coap_mailbox, &msg, BIOS_NO_WAIT);
}

/*******************************************************************************
* @fn
*
* @brief
*
* @param
*
* @return      none
********************************************************************************/
void coap_msg_post_start_scan() //JIRA53
{
  TCPIP_cmdmsg_t msg;
  msg.event_type = COAP_START_SCAN;

  Mailbox_post(coap_mailbox, &msg, BIOS_NO_WAIT);
}

/*******************************************************************************
* @fn          coap_task
*
* @brief       CoAP task
*
* @param       arg0 - meaningless
*              arg1 - meaningless
*
* @return      none
********************************************************************************
*/
void coap_task(UArg a0, UArg a1)
{
#if DMM_ENABLE
  DMM_waitBleOpDone();
#endif //DMM_ENABLE

  { //Use the init block to avoid wasting the task stack space for the one time only init variables.  Save some stack space of the task
#ifndef HARP_MINIMAL
    bool status;

    status = SensorBatmon_init();

#if SENSORTAG
    status = SensorI2C_open();
    status = SensorTmp007_init();
    status = SensorOpt3001_init();
    status = SensorHdc1000_init();
    status = SensorBmp280_init();
    status = SensorMpu9250_init();
#elif I3_MOTE
    status = SensorI2C_open();
    status = SensorOpt3001_init();
    status = SensorBmp280_init();

#if CC13XX_DEVICES
    status = SensorLIS2DW12_init();
#elif CC26XX_DEVICES
    status = SensorTmp007_init();
    status = SensorHdc1000_init();
#endif
#endif
    status = status;
#endif
    uip_setup_appConnCallback(coap_socket_open);

#if I3_MOTE_SPLIT_ARCH
    if (MSP432HCT_getConfig(&coap_ObservePeriod) == false)
    {
      HAL_LED_set(2, 1);
      COAP_fatalErrorHandler();
    }
#endif

    coap_context = get_context(&server_ipaddr, 0); //parameters not used at all

    init_resources(coap_context);

#if FEATURE_DTLS
    init_dtls(0, &coap_context->udp_dtls_socket, &coap_dtls_timer_handler_obj, (appDataHandler_t)coap_event_handler, coap_context);
    init_dtls(1, &coap_context->udp_mac_sec_socket, &coap_mac_sec_timer_handler_obj, (appDataHandler_t)coap_mac_sec_event_handler, coap_context);
#endif
    Task_sleep(CLOCK_SECOND / 4);
    {
      NHL_MlmeScanReq_t nhlMlmeScanReq;
      NHL_setupScanArg(&nhlMlmeScanReq);
      NHL_startScan(&nhlMlmeScanReq, NULL);
    }
  }

  while (1)
  {
    TCPIP_cmdmsg_t coap_msg;
    Mailbox_pend(coap_mailbox, &coap_msg, BIOS_WAIT_FOREVER);
#if DISABLE_TISCH_WHEN_BLE_CONNECTED && DMM_ENABLE && CONCURRENT_STACKS
    uint8_t ledv = 1;
    while (bleConnected)
    {
      LED_set(1, ledv);
      ledv ^= 1;
      if (globalRestart) //fengmo NVM
      {
        Task_sleep(5 * CLOCK_SECOND);
        Board_reset();
      }
      Task_sleep(CLOCK_SECOND / 4);
      ;
    }
#endif                 //DMM_ENABLE && !CONCURRENT_STACKS
    if (globalRestart) //fengmo NVM
    {
      Task_sleep(5 * CLOCK_SECOND);
      Board_reset();
    }

    PWREST_on(PWREST_TYPE_CPU);

    if (coap_msg.event_type == COAP_START_SCAN) //JIRA53
    {
      TSCH_start_scan_handler();
    }
    else if (coap_msg.event_type == COAP_RESTART_SCAN) //JIRA50
    {
      TSCH_scan_timer_handler();
    }
    else if (coap_msg.event_type == COAP_TIMER)
    {
#ifndef HARP_MINIMAL
      static uint8_t resourceCheckCount = 0;
      extern coap_context_t the_coap_context;
      if (etimer_expired(&the_coap_context.notify_timer))
      {

        if (resourceCheckCount % COAP_SENSOR_RESOURCE_DIRTY_FREQ == 0)
        {
          sensor_resource->dirty = 1;
        }

        if (resourceCheckCount % COAP_NW_PERF_RESOURCE_DIRTY_FREQ == 0)
        {
          diagnosis_resource->dirty = 1;
        }

        resourceCheckCount++;
        if (resourceCheckCount % COAP_SENSOR_RESOURCE_DIRTY_FREQ == 0 &&
            resourceCheckCount % COAP_NW_PERF_RESOURCE_DIRTY_FREQ == 0)
        {
          resourceCheckCount = 0;
        }
      }
      coap_retransmit_observe_process();
#endif
    }
    else
#if FEATURE_DTLS
        if (coap_msg.event_type == COAP_DTLS_TIMER)
    {
      dtls_handle_retransmit(dtls_context[0]);
    }
    else if (coap_msg.event_type == COAP_DTLS_MAC_SEC_CONNECT)
    {
      session_t session;
      memcpy(session.addr.u8, coap_msg.data, sizeof(session.addr.u8));
      session.port = coapConfig.coap_mac_sec_port;
#if DTLS_LEDS_ON
      LED_set(1, 0);
      LED_set(2, 1);
#endif //DTLS_LEDS_OFF
      dtls_connect(dtls_context[1], &session);
    }
    else if (coap_msg.event_type == COAP_DELETE_ALL_OBSERVERS)
    {
      coap_delete_all_observers();
    }
    else if (coap_msg.event_type == COAP_DATA)
    {
      appRxFifoIndex_inc(&appRxFifo_ReadIdx); //fengmo debug node packet loss
      dtls_handle_read(dtls_context[0]);
    }
#else
    {
      appRxFifoIndex_inc(&appRxFifo_ReadIdx); //fengmo debug node packet loss
      coap_event_handler(coap_context);
    }
#endif
    if (coap_msg.event_type == COAP_DATA) //fengmo debug node packet loss
    {
      appRxLenFifo[appRxFifo_ReadIdx] = 0;
    }
    PWREST_off(PWREST_TYPE_CPU);
  }
}
