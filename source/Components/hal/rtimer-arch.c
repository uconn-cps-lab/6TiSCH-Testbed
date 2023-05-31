/*
* Copyright (c) 2011, Swedish Institute of Computer Science.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. Neither the name of the Institute nor the names of its contributors
*    may be used to endorse or promote products derived from this software
*    without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*
* This file is part of the Contiki operating system.
*/

/**
* \file
*         MSP430-specific rtimer code for MSP430X
* \author
*         Adam Dunkels <adam@sics.se>
*/
/******************************************************************************
 *
 * Copyright (c) 2014 Texas Instruments Inc.  All rights reserved.
 *
 * DESCRIPTION: Port the rtimer platform implementation to CC13XX/CC26XX.
 *
 * HISTORY:
 *
 *
 ******************************************************************************/

#include "rtimer.h"

#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/family/arm/m3/Hwi.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>

#include <aon_rtc.h>
#include <aon_event.h>
#include <prcm.h>

#include "ulpsmac.h"

/*---------------------------------------------------------------------------*/
void rtimer_isr(void)
{
    rtimer_run_next();
}
/*---------------------------------------------------------------------------*/
void
rtimer_arch_init(void)
{
    AONRTCDisable();
    AONRTCReset();

    HWREG(AON_RTC_BASE + AON_RTC_O_SYNC) = 1;
    /* read sync register to complete reset */
    HWREG(AON_RTC_BASE + AON_RTC_O_SYNC);
    AONRTCDelayConfig( AON_RTC_CONFIG_DELAY_NODELAY );
    AONRTCCombinedEventConfig( AON_RTC_CH0 | AON_RTC_CH1 );

    // Set the combined event as wake up source, and setup wakeup event
    // Note: This is required when using DeepSleep with uLDO!
    AONEventMcuWakeUpSet( AON_EVENT_MCU_WU1, AON_EVENT_RTC_CH1);
    AONRTCEnable();

    // clear the RTC channel
    AONRTCChannelDisable( AON_RTC_CH1 );

    // clear the RTC event
    HWREG(AON_RTC_BASE + AON_RTC_O_EVFLAGS) = AON_RTC_EVFLAGS_CH1;
}
/*---------------------------------------------------------------------------*/
rtimer_clock_t
rtimer_arch_now(void)
{
    rtimer_clock_t t1, t2;
    do {
        t1 = AONRTCCurrentCompareValueGet();
        t2 = AONRTCCurrentCompareValueGet();
    } while(t1 != t2);
    return t1;
}
/*---------------------------------------------------------------------------*/
void rtimer_arch_schedule(rtimer_clock_t t)
{
    uint32_t key;

    key = Hwi_disable();

    HWREG(AON_RTC_BASE + AON_RTC_O_EVFLAGS) = AON_RTC_EVFLAGS_CH1;
    AONRTCChannelDisable( AON_RTC_CH1 );

    AONRTCCompareValueSet(AON_RTC_CH1, (rtimer_clock_t)(t));
    AONEventMcuWakeUpSet( AON_EVENT_MCU_WU1, AON_EVENT_RTC_CH1 );
    AONRTCChannelEnable( AON_RTC_CH1);

    //Combined event with RTC CH0. Not clear why it is needed.
    AONRTCCombinedEventConfig(AON_RTC_CH1|AON_RTC_CH0);
    AONRTCDelayConfig( AON_RTC_CONFIG_DELAY_NODELAY );
    AONRTCEnable();
    
    Hwi_restore(key);

}
/*---------------------------------------------------------------------------*/
