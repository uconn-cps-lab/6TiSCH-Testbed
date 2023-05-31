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
 *  ====================== hal_api.h =============================================
 *  Public interface file for cc13xx/cc26xx hardware abstraction layer
 */

//*****************************************************************************
#ifndef HAL__API
#define HAL__API

#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
 * Includes
 */
#include <ti/drivers/rf/RF.h>
#include "rtimer.h"
  
/*******************************************************************************
 * TYPEDEFS
 */
/*! Frequency band*/
typedef enum
{
  HAL_FreqBand_868MHZ,
  HAL_FreqBand_915MHZ,
  HAL_FreqBand_2400MHZ,
} HAL_FreqBand_t;

/*! Physical modulation Type */
typedef enum
{
  HAL_PhyMode_IEEE250kbpsDSSS,
  HAL_PhyMode_PRO50kbpsFSK,
} HAL_PhyMode_t;

/*! HAL status type */
typedef enum
{
  HAL_SUCCESS,
  HAL_ERR,
  HAL_INVALID_PARAMETER,
} HAL_Status_t;


typedef void (*HAL_Callback_t)(RF_EventMask e, uint8_t* ppBuf);

#if PHY_IEEE_MODE
/* Supported Channel Bit Map*/
#define HAL_RADIO_CHANNEL_SUPPORTED_0_31       (0x07FFF000UL)
#define HAL_RADIO_CHANNEL_SUPPORTED_32_63      (0)
#define HAL_RADIO_CHANNEL_SUPPORTED_64_95      (0)
#define HAL_RADIO_CHANNEL_SUPPORTED_96_127     (0)
#define HAL_RADIO_CHANNEL_SUPPORTED_128_159    (0)

#define HAL_RADIO_MIN_CHANNEL          (12)
#define HAL_RADIO_MAX_CHANNEL          (26)

/* Radio length to packet duration conversion*/
#define HAL_RADIO_DURATION_TICKS(bytes)   ULPSMAC_US_TO_TICKS(256+(bytes)*32)
/* Radio Command Delays*/
#define HAL_RADIO_RAMPUP_DELAY_TICKS      ULPSMAC_US_TO_TICKS(1260)
#define HAL_RADIO_TXRX_CMD_DELAY_TICKS    ULPSMAC_US_TO_TICKS(180)
#define HAL_RADIO_RX_CB_DELAY_TICKS       ULPSMAC_US_TO_TICKS(0)

#elif PHY_PROP_50KBPS

#define SUBGHZ_915 1

#if SUBGHZ_868
#define HAL_RADIO_CHANNEL_SUPPORTED_0_31       (0xFFFFFFFFUL)
#define HAL_RADIO_CHANNEL_SUPPORTED_32_63      (0x00000007UL)
#define HAL_RADIO_CHANNEL_SUPPORTED_64_95      (0)
#define HAL_RADIO_CHANNEL_SUPPORTED_96_127     (0)
#define HAL_RADIO_CHANNEL_SUPPORTED_128_159    (0)

#define HAL_RADIO_MAX_CHANNEL           (34)

#elif SUBGHZ_915
#define HAL_RADIO_CHANNEL_SUPPORTED_0_31       (0xFFFFFFFFUL)
#define HAL_RADIO_CHANNEL_SUPPORTED_32_63      (0xFFFFFFFFUL)
#define HAL_RADIO_CHANNEL_SUPPORTED_64_95      (0)
#define HAL_RADIO_CHANNEL_SUPPORTED_96_127     (0)
#define HAL_RADIO_CHANNEL_SUPPORTED_128_159    (0)
#define HAL_RADIO_MAX_CHANNEL           (63)
//#define HAL_RADIO_CHANNEL_SUPPORTED_64_95      (0xFFFFFFFFUL)
//#define HAL_RADIO_CHANNEL_SUPPORTED_96_127     (0xFFFFFFFFUL)
//#define HAL_RADIO_CHANNEL_SUPPORTED_128_159    (0x00000003UL)
//#define HAL_RADIO_MAX_CHANNEL           (129)

#endif

#define HAL_RADIO_MIN_CHANNEL           (0)

#define HAL_RADIO_DURATION_TICKS(bytes)   ULPSMAC_US_TO_TICKS(1760+(bytes)*160)
//#define HAL_RADIO_TX_PACKET_DELAY_TICKS   ULPSMAC_US_TO_TICKS(780)
//#define HAL_RADIO_TX_ACK_DELAY_TICKS      ULPSMAC_US_TO_TICKS(220)
//#define HAL_RADIO_RX_PACKET_DELAY_TICKS   ULPSMAC_US_TO_TICKS(1440)
//#define HAL_RADIO_RX_ACK_DELAY_TICKS      ULPSMAC_US_TO_TICKS(220)
//#define HAL_RADIO_SET_CHANNEL_DELAY_TICKS ULPSMAC_US_TO_TICKS(354)
#define HAL_RADIO_RAMPUP_DELAY_TICKS      ULPSMAC_US_TO_TICKS(1260)
#define HAL_RADIO_TXRX_CMD_DELAY_TICKS    ULPSMAC_US_TO_TICKS(180)
#define HAL_RADIO_RX_CB_DELAY_TICKS       ULPSMAC_US_TO_TICKS(0)
#endif

#define HAL_RADIO_NUM_BITMAP     (5)
#define HAL_RADIO_CHANNEL_BITMAP_LEN          (32)
#define HAL_RADIO_NUM_CHANNELS  (HAL_RADIO_MAX_CHANNEL - HAL_RADIO_MIN_CHANNEL + 1)

/*******************************************************************************
 * GLOBAL VARIABLES
 */
extern HAL_FreqBand_t HAL_radioFreqBand;
extern rtimer_clock_t HAL_txEndTime;

/*******************************************************************************
 * FUNCTIONS
 */
HAL_Status_t HAL_RADIO_init(HAL_FreqBand_t freqBand, HAL_PhyMode_t phyMode);
HAL_Status_t HAL_RADIO_transmit(uint8_t* pBuf, uint16_t bufLen,
                                uint32_t startTime);
HAL_Status_t HAL_RADIO_receiveOn(HAL_Callback_t pCb, uint32_t startTime);
HAL_Status_t HAL_RADIO_receiveOff(void);
HAL_Status_t HAL_RADIO_setChannel(uint16_t channel);
uint16_t HAL_RADIO_getChannel(void);
HAL_Status_t HAL_RADIO_setTxPower(int8_t power);
int8_t HAL_RADIO_getTxPower(void);
void HAL_LED_set(uint8_t ledID, uint8_t val);
uint8_t HAL_LED_get(uint8_t ledID);
void HAL_RADIO_allowStandby(void);

#ifdef __cplusplus
}
#endif

#endif //HAL_API


