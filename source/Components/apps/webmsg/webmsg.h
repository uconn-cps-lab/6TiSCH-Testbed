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
 *  ====================== webmsg.h =============================================
 */
#ifndef __WEB_MSG_H__
#define __WEB_MSG_H__

#ifdef __cplusplus
extern "C"
{
#endif
  
#include <stdint.h>

typedef struct 
{
  int16_t temp;
  uint16_t rhum;
  uint32_t lux;
  uint16_t press;
  int16_t accelx;
  int16_t accely;
  int16_t accelz;
  uint8_t led;
  uint8_t freq_channel;
  uint16_t bat;
  uint8_t insEhDutyCycle;
  uint8_t avgEhDutyCycle;
  uint16_t cc2650_active;
  uint16_t cc2650_sleep;
  uint16_t rf_tx;
  uint16_t rf_rx;
  uint16_t gpsen_active;
  uint16_t gpsen_sleep;
  uint16_t msp432_active;
  uint16_t msp432_sleep;
  uint16_t others;
} SensorMsg_t;
        
void webmsg_update(unsigned char *pMsg, unsigned char *pSize, uint8_t seq);

#ifdef __cplusplus  
}
#endif  

#endif // __WEB_MSG_H__


