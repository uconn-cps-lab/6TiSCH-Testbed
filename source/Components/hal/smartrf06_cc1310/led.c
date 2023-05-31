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
 *  ====================== led.c =============================================
 *  LED driver implementation
 */
 
#include "led.h"
#include "Board.h"

PIN_State LED_pinState;
PIN_Handle LED_pinHandle;

/* led pin configuration table */
PIN_Config LED_pinTable[] = {
    Board_LED1       | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW   | PIN_PUSHPULL | PIN_DRVSTR_MAX,     /* LED initially off             */
    Board_LED2       | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW   | PIN_PUSHPULL | PIN_DRVSTR_MAX,     /* LED initially off             */
    Board_LED3       | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW   | PIN_PUSHPULL | PIN_DRVSTR_MAX,     /* LED initially off             */
    Board_LED4       | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW   | PIN_PUSHPULL | PIN_DRVSTR_MAX,     /* LED initially off             */
    PIN_TERMINATE                                                                   		/* Terminate list     */
};


uint8_t LED_init(void)
{
  LED_pinHandle = PIN_open(&LED_pinState, LED_pinTable);
  if (!LED_pinHandle)
  return -1;

  return 0;
}

void LED_set(uint8_t ledId, uint8_t value)
{
    PIN_Id pinId;

    switch(ledId)
    {
        case 1:
            pinId = Board_LED1;
            PIN_setOutputValue(LED_pinHandle, pinId, (uint_t)value);
        break;
        case 2:
            pinId = Board_LED2;
            PIN_setOutputValue(LED_pinHandle, pinId, (uint_t)value);
        break;
        case 3:
            pinId = Board_LED3;
            PIN_setOutputValue(LED_pinHandle, pinId, (uint_t)value);
        break;
        case 4:
            pinId = Board_LED4;
            PIN_setOutputValue(LED_pinHandle, pinId, (uint_t)value);
        break;
        default:
        break;
    }

}

uint8_t LED_get(uint8_t ledId)
{
    uint8_t val;
    PIN_Id pinId;

    switch(ledId)
    {
        case 1:
        pinId = Board_LED1;
        break;
        case 2:
        pinId = Board_LED2;
        break;
        case 3:
        pinId = Board_LED3;
        break;
        case 4:
        pinId = Board_LED4;
        break;
        default:
        break;
    }
    val = PIN_getOutputValue(pinId);
    return val;
}