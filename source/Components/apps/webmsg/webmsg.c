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
*  ====================== web_msg.h =============================================
*  This file defines the web message data format
*/

/*******************************************************************************
* INCLUDES
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "webmsg/webmsg.h"
#include "SensorBatMon.h"


#if SENSORTAG || I3_MOTE
#include "SensorI2C.h"
#include "SensorTmp007.h"
#include "SensorHDC1000.h"
#include "SensorHDC2010.h"
#include "SensorOpt3001.h"
#include "SensorBmp280.h"
#include "SensorMpu9250.h"
#include "SensorLIS2HH12.h"
#include "SensorLIS2DW12.h"
#endif

#include "tsch-acm.h"
#include "tsch-txm.h"
#include "tsch-db.h"
#include "pwrest/pwrest.h"
#include "hct_msp432/hct/hct.h"

#if I3_MOTE
#include "ehest/ehest.h"
#include "hct_msp432/hct/msp432hct.h"
#endif
/*******************************************************************************
* GLOBAL VARIABLES
*/
#ifndef HARP_MINIMAL
SensorMsg_t sensorMsg;

extern uint16_t led_status;
extern uint64_t packet_sent_asn_sensors;
extern TSCH_DB_handle_t db_hnd;

/***************************************************************************//**
* @brief      Insert sensor values to message string
*
* @param[in]  pMsg - pointer to sensor message
* @param[in]  pSize - pointer to sensor message size
* @param[in]  nBytes - sensor data size
* @param[in]  value - pointer to sensor data
*
* @return     none
******************************************************************************/
static void webmsg_insertTlvPayload(unsigned char *pMsg, unsigned char *pSize, uint8_t nBytes, void* value)
{
  memcpy(pMsg+(*pSize), value, nBytes);
  (*pSize)+=nBytes;
}

static void webmsg_insertTlvHeader(unsigned char *pMsg, unsigned char *pSize, uint8_t type, uint8_t length)
{
   memcpy(pMsg+(*pSize),&type, 1);
   (*pSize)+=1;
   memcpy(pMsg+(*pSize),&length, 1);
   (*pSize)+=1;
}

#if !I3_MOTE_SPLIT_ARCH
/***************************************************************************//**
* @brief  Read sensor data from driver    
*
* @param[in]  none
*
* @return     none
******************************************************************************/
static void webmsg_readSensor(unsigned char * pMsg, unsigned char *pSize)
{
  bool success;
  SENSOR_BatData_t cc26xxBat;
  
  success = SensorBatmon_oneShotRead(&cc26xxBat);
  if (success)
  {
    sensorMsg.bat  = cc26xxBat.bat;
  }
  
#if SENSORTAG 
  SENSOR_HumData_t humData;
  SENSOR_LightData_t lightData;
  SENSOR_PressData_t pressData;
  SENSOR_MotionData_t motionData;
  
  success = SensorHdc1000_oneShotRead(&humData);
  if (success)
  {
    sensorMsg.rhum  = humData.hum;
  }
 
  success = SensorOpt3001_oneShotRead(&lightData);
  if (success)
  {
    sensorMsg.lux  = lightData.lux;
  }

  success = SensorBmp280_oneShotRead(&pressData);
  if (success)
  {
    sensorMsg.temp  = pressData.temp;
    sensorMsg.press  = pressData.press;
  }
  
  success = SensorMpu9250_oneShotRead(&motionData);
  if (success)
  {
    sensorMsg.accelx = motionData.accelx;
    sensorMsg.accely = motionData.accely;
    sensorMsg.accelz = motionData.accelz;
  }
#elif I3_MOTE
  SENSOR_HumData_t humData;
  SENSOR_LightData_t lightData;
  SENSOR_PressData_t pressData;
  SENSOR_MotionData_t motionData;
  
  success = SensorOpt3001_oneShotRead(&lightData);
  if (success)
  {
    sensorMsg.lux  = lightData.lux;
  }

  success = SensorBmp280_oneShotRead(&pressData);
  if (success)
  {
    sensorMsg.temp  = pressData.temp;
    sensorMsg.press  = pressData.press;
  }
  
#if CC13XX_DEVICES
  success = SensorHdc2010_oneShotRead(&humData);
  if (success)
  {
    sensorMsg.rhum  = humData.hum;
  }
  
  success = SensorLis2DW12_oneShotRead(&motionData);
   if (success)
  {
    sensorMsg.accelx = motionData.accelx;
    sensorMsg.accely = motionData.accely;
    sensorMsg.accelz = motionData.accelz;
  }
  
#elif CC26XX_DEVICES
  success = SensorHdc1000_oneShotRead(&humData);
  if (success)
  {
    sensorMsg.rhum  = humData.hum;
  }
  
  success = SensorLis2hh12_oneShotRead(&motionData);
   if (success)
  {
    sensorMsg.accelx = motionData.accelx;
    sensorMsg.accely = motionData.accely;
    sensorMsg.accelz = motionData.accelz;
  }
#endif
#endif
  
  webmsg_insertTlvHeader(pMsg, pSize, HCT_TLV_TYPE_TEMP, 2);
  webmsg_insertTlvPayload(pMsg, pSize, 2, &sensorMsg.temp);
  
  webmsg_insertTlvHeader(pMsg, pSize, HCT_TLV_TYPE_HUM, 2);
  webmsg_insertTlvPayload(pMsg, pSize, 2, &sensorMsg.rhum);
  
  webmsg_insertTlvHeader(pMsg, pSize, HCT_TLV_TYPE_LIGHT, 4);
  webmsg_insertTlvPayload(pMsg, pSize, 4, &sensorMsg.lux);
  
  webmsg_insertTlvHeader(pMsg, pSize, HCT_TLV_TYPE_PRE, 2);
  webmsg_insertTlvPayload(pMsg, pSize, 2, &sensorMsg.press);
  
  webmsg_insertTlvHeader(pMsg, pSize, HCT_TLV_TYPE_MOT, 6);
  webmsg_insertTlvPayload(pMsg, pSize, 2, &sensorMsg.accelx);
  webmsg_insertTlvPayload(pMsg, pSize, 2, &sensorMsg.accely);
  webmsg_insertTlvPayload(pMsg, pSize, 2, &sensorMsg.accelz);
}
#endif //I3_MOTE_SPLIT_ARCH

/***************************************************************************//**
* @brief      Update sensor message content
*
* @param[in]  pMsg - pointer to sensor message
* @param[in]  pSize - pointer to sensor message size
*
* @return     none
******************************************************************************/
uint16_t packet_created_asn;
void webmsg_update(unsigned char *pMsg, unsigned char *pSize, uint8_t seq)
{
  static bool firstVisit = true;
  PWREST_PwrMeasurement_t pwrMeasure;
  uint8_t pwrEff;
  
  //removed for storing sequence number
  //memset(&sensorMsg,0,sizeof(sensorMsg));
  *pSize=0;
  
#if I3_MOTE_SPLIT_ARCH
  {
     extern bool MSP432HCT_getSensorData(unsigned char * pMsg, unsigned char *pSize);
     MSP432HCT_getSensorData(pMsg,pSize);
     sensorMsg.others = 650*10/8;
     sensorMsg.bat = 300;
  }
#else
  webmsg_readSensor(pMsg,pSize);
  sensorMsg.others = 500;
#endif
  
  sensorMsg.freq_channel = link_hopping_info[2].channel;
  sensorMsg.led = led_status;  
  
  if (firstVisit == true)
  {
    firstVisit = false;
    PWREST_init();
    PWREST_on(PWREST_TYPE_TOTAL);
  }
  else
  {
    PWREST_off(PWREST_TYPE_TOTAL);
    PWREST_calAvgCur(&pwrMeasure);
    PWREST_on(PWREST_TYPE_TOTAL);
    
    /*Measurement is in nA. In the webapp backend, it will divide the received
    number by 100*/
#if I3_MOTE
#if CC26XX_DEVICES
    /* I3 2.4G has  80% power efficiency*/
    pwrEff = 8;
#else
    /* I3 SG has  90% power efficiency*/
    pwrEff = 9;
#endif
    ehest_getDutyCycle(&sensorMsg.insEhDutyCycle,&sensorMsg.avgEhDutyCycle);
#else
    /* default 100% power power efficiency*/
    pwrEff = 10;
#endif
    
    sensorMsg.rf_rx= (uint16_t)(pwrMeasure.avgActiveCur[PWREST_TYPE_RX]/pwrEff);
    sensorMsg.rf_tx= (uint16_t)(pwrMeasure.avgActiveCur[PWREST_TYPE_TX]/pwrEff);
    sensorMsg.cc2650_active= (uint16_t) (pwrMeasure.avgActiveCur[PWREST_TYPE_CPU]/pwrEff);
    sensorMsg.cc2650_sleep= (uint16_t) (pwrMeasure.avgSleepCur[PWREST_TYPE_CPU]/pwrEff);
    
#if (I3_MOTE || SENSORTAG)
    uint8_t i;
    
    for(i= PWREST_TYPE_SENSORS_TMP; i< PWREST_TYPE_TOTAL; i++)
    {
      sensorMsg.gpsen_active += pwrMeasure.avgActiveCur[i];
      sensorMsg.gpsen_sleep += pwrMeasure.avgSleepCur[i];
    }
    
    sensorMsg.gpsen_active /= 10;
    sensorMsg.gpsen_sleep  /= 10;
    sensorMsg.msp432_active = 0;
    sensorMsg.msp432_sleep = 0;
    
    webmsg_insertTlvHeader(pMsg, pSize, HCT_TLV_TYPE_SENSOR_POWER, 4);
    webmsg_insertTlvPayload(pMsg, pSize, 2, &sensorMsg.gpsen_active);
    webmsg_insertTlvPayload(pMsg, pSize, 2, &sensorMsg.gpsen_sleep);
//    
//    webmsg_insertTlvHeader(pMsg, pSize, HCT_TLV_TYPE_MSP432_POWER, 4);
//    webmsg_insertTlvPayload(pMsg, pSize, 2, &sensorMsg.msp432_active);
//    webmsg_insertTlvPayload(pMsg, pSize, 2, &sensorMsg.msp432_sleep);
#endif
  }
//  
//  webmsg_insertTlvHeader(pMsg, pSize, HCT_TLV_TYPE_LED, 1);
//  webmsg_insertTlvPayload(pMsg, pSize, 1, &sensorMsg.led);
//  
//  webmsg_insertTlvHeader(pMsg, pSize, HCT_TLV_TYPE_CHANNEL, 1);
//  webmsg_insertTlvPayload(pMsg, pSize, 1, &sensorMsg.freq_channel);
//  
  webmsg_insertTlvHeader(pMsg, pSize, HCT_TLV_TYPE_BAT, 2);
  webmsg_insertTlvPayload(pMsg, pSize, 2, &sensorMsg.bat);
// 
//  webmsg_insertTlvHeader(pMsg, pSize, HCT_TLV_TYPE_EH, 2);
//  webmsg_insertTlvPayload(pMsg, pSize, 1, &sensorMsg.avgEhDutyCycle);
//  webmsg_insertTlvPayload(pMsg, pSize, 1, &sensorMsg.insEhDutyCycle);
  
//  webmsg_insertTlvHeader(pMsg, pSize, HCT_TLV_TYPE_CC2650_POWER, 4);
//  webmsg_insertTlvPayload(pMsg, pSize, 2, &sensorMsg.rf_tx);
//  webmsg_insertTlvPayload(pMsg, pSize, 2, &sensorMsg.rf_rx);
//
//  webmsg_insertTlvHeader(pMsg, pSize, HCT_TLV_TYPE_OTHER_POWER, 2);
//  webmsg_insertTlvPayload(pMsg, pSize, 2, &sensorMsg.others);
  
//  webmsg_insertTlvHeader(pMsg, pSize, HCT_TLV_TYPE_SEQUENCE, 1);
//  webmsg_insertTlvPayload(pMsg, pSize, 1, &seq);
//  
  uint16_t asn = (uint16_t)TSCH_ACM_get_ASN();
  packet_created_asn = asn;
  webmsg_insertTlvHeader(pMsg, pSize, HCT_TLV_TYPE_ASN_STAMP, 2);
  webmsg_insertTlvPayload(pMsg, pSize, 2, &asn);
//  
//  // Jiachen: slot offset
//  uint16_t sf;
//  uint16_t slot_offset;
//  sf = db_hnd.slotframes[0].vals.slotframe_size;
//  slot_offset = packet_sent_asn_sensors%sf;
//  webmsg_insertTlvHeader(pMsg, pSize, HCT_TLV_TYPE_SLOT_OFFSET, 2);
//  webmsg_insertTlvPayload(pMsg, pSize, 2, &slot_offset);
//  
  // Jiachen: include the ASN stamp when the last packet was sent.
  webmsg_insertTlvHeader(pMsg, pSize, HCT_TLV_TYPE_LAST_SENT_ASN, 2);
  webmsg_insertTlvPayload(pMsg, pSize,  2, &packet_sent_asn_sensors);
//  
  // Jiachen: add a tag in the end of the payload
  uint8_t tag = TAG_COAP_SENSORS; 
  webmsg_insertTlvHeader(pMsg, pSize, HCT_TLV_TYPE_COAP_DATA_TYPE, 1);
  webmsg_insertTlvPayload(pMsg, pSize, 1, &tag);
}
#endif