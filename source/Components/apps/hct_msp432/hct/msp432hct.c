/*
* Copyright (c) 2017, Texas Instruments Incorporated
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
*  ====================== coap_host_config.c =============================================
*  This file is for CC2650 to read configuration from MSP432. This is just for 
*  I3 mote at this moment.
*/

#include "Board.h"
#include "uip-conf.h"
#include "hal_api.h"

#include "hct_msp432/nwp/nwp_cc2650.h"
#include "hct_msp432/serialbus/BusSPI.h"

#include "msp432hct.h"

#define SPI_BUFFER_SIZE         128
#define MAX_TLVS                10

extern PIN_Handle pinHandle;
extern PIN_State pinState;

static uint8_t spiTransmitBuffer[SPI_BUFFER_SIZE];
static uint8_t spiReceiveBuffer[SPI_BUFFER_SIZE];
static HCT_TLV_t tlvs[MAX_TLVS];

static uint8_t hctSequenceNumber = 0;

uint16_t numSpiError[3]={0};


static void wakeUpMSP432(void) 
{
  /* Wake-up MSP432 to get configuration */
  PIN_setOutputValue(pinHandle, Board_SSM_IRQ, 0);
  PIN_setOutputValue(pinHandle, Board_SSM_IRQ, 1);
}

bool MSP432HCT_getConfig(uint32_t* obsPer)
{
  HCT_Frame_t * header;
  uint8_t * p_payload;
  uint16_t payloadLength;
  
  uint16_t pktLen;
  uint8_t org, rpy;
  
  bool status = false;
  bool ifconfigred = false;
  
  uint8_t sequenceNumber;
   
  /* Initialize SPI */
  BusSPI_open_slave(Board_SPI);
  
//  do
//  {
    Task_sleep(CLOCK_SECOND*3);
    
    /* Prepare ConfigSet packet */
    pktLen = nwpConfigSet(spiTransmitBuffer, hctSequenceNumber);
    
    /* Check error case */
    if (pktLen > HCT_FIX_PKT_SIZE){
      while(1)
        ;
    }
    
    /* Wake-up MSP432 */
    wakeUpMSP432();
    
    /* Perform SPI transaction as slave to send command */
    status = BusSPI_writeRead(spiTransmitBuffer, HCT_FIX_PKT_SIZE ,
                                spiReceiveBuffer,  HCT_FIX_PKT_SIZE);
    if (status)
    {
      /* Perform SPI transaction as slave to get CoAP observe period */
      /* Here we are expecting a CoAP observe period configuration */
      status = BusSPI_writeRead(spiTransmitBuffer, HCT_FIX_PKT_SIZE,
                                  spiReceiveBuffer,  HCT_FIX_PKT_SIZE);
      if (status)
      {
        /* Parse received header and check CRC */
        status = HCTRetrieveHeader(spiReceiveBuffer, &header, &p_payload, &payloadLength);
        if (status)
        {
          /* Check header */
          HCTRetrieveFlags(header->flags, &org, &rpy, &sequenceNumber);
          if (org == HCT_ORG_FUSIONPROC && sequenceNumber == hctSequenceNumber)
          {
            /* Parse payload to get CoAP observe period */
            uint8_t tlvCount = 0;
            uint8_t length;
            while (payloadLength > 0)
            {
              length = HCTParseTLV(p_payload, &tlvs[tlvCount]);
              p_payload += length;
              payloadLength -= length;
              tlvCount++;
            }
            
            /* If successful */
            if (tlvs[0].type == HCT_TLV_TYPE_COAP_OBSERVE_PERIOD)
            {
              int8_t i;
              uint32_t tmpVal=0;
              /* This value is too large*/
              if (tlvs[0].length > 4)
                  return false;
              
              for(i=tlvs[0].length-1;i>=0;i--)
              {
                //fengmo fix *(obsPer+i) =(*(tlvs[0].value+i));
                tmpVal = (tmpVal<<8)+(*(tlvs[0].value+i));
              }
              
              *obsPer = tmpVal;
              
              ifconfigred = true;
            }
          }
        }
        else
        {
            numSpiError[1]++;
        }
      }
    }
    else
    {
      numSpiError[0]++;
    }
    ifconfigred = ifconfigred;
//  } while(ifconfigred == false);
  
  return status;
}

bool MSP432HCT_getSensorData(unsigned char * pMsg, unsigned char *pSize)
{
  HCT_Frame_t * header;
  uint8_t * p_payload;
  uint16_t payloadLength;
  
  uint16_t pktLen;
  uint8_t org, rpy;
  uint8_t sequenceNumber;
  
  bool status;
  
  hctSequenceNumber++;
  if (hctSequenceNumber >=HCT_SEQ_MASK)
    hctSequenceNumber = 0;
  
  pktLen = nwpDataSet(spiTransmitBuffer, hctSequenceNumber);
  
  /* Check error case */
  if (pktLen > HCT_FIX_PKT_SIZE){
    while(1)
      ;
  }
  
  /* Wake-up MSP432 */
  wakeUpMSP432();
  
  /* Perform SPI transaction as slave to send command */
  status = BusSPI_writeRead(spiTransmitBuffer, HCT_FIX_PKT_SIZE,
                              spiReceiveBuffer, HCT_FIX_PKT_SIZE);
  if (status)
  {
    /* Perform SPI transaction as slave to get DATA */
    /* Here we are expecting a TLV set */
    status = BusSPI_writeRead(spiTransmitBuffer, HCT_FIX_PKT_SIZE,
                                spiReceiveBuffer, HCT_FIX_PKT_SIZE);
    
    if (status)
    {
      status = HCTRetrieveHeader(spiReceiveBuffer, &header,
                                 &p_payload, &payloadLength);
      
      /* Parse received header and check CRC */
      if (status)
      {
        /* Check header */
        HCTRetrieveFlags(header->flags, &org, &rpy, &sequenceNumber);
        if (org == HCT_ORG_FUSIONPROC && sequenceNumber == hctSequenceNumber)
        {
          memcpy(pMsg,p_payload,payloadLength);
          (*pSize)+=payloadLength;
        }
      }
      else
      {
       numSpiError[1]++;
      }
    }
    else
    {
       numSpiError[2]++;
    }
  }
  else
  {
    numSpiError[0]++;
  }
  
  return status;
}
