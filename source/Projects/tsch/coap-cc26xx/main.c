/**
@file  main.c
@brief main entry of the Dummy stack sample application

<!--
Copyright 2013 Texas Instruments Incorporated. All rights reserved.

IMPORTANT: Your use of this Software is limited to those specific rights
granted under the terms of a software license agreement between the user
who downloaded the software, his/her employer (which must be your employer)
and Texas Instruments Incorporated (the "License").  You may not use this
Software unless you agree to abide by the terms of the License. The License
limits your use, and you acknowledge, that the Software may not be modified,
copied or distributed unless embedded on a Texas Instruments microcontroller
or used solely and exclusively in conjunction with a Texas Instruments radio
frequency transceiver, which is integrated into your product.  Other than for
the foregoing purpose, you may not use, reproduce, copy, prepare derivative
works of, modify, distribute, perform, display or sell this Software and/or
its documentation for any purpose.

YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
PROVIDED ``AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

Should you have any questions regarding your right to use this Software,
contact Texas Instruments Incorporated at www.TI.com.
-->
*/
/*******************************************************************************
* INCLUDES
*/
#include <ti/sysbios/BIOS.h>
#include "Board.h"
#include "led.h"
#include "mac_config.h"
#include "ehest/ehest.h"
#include "bsp_aes.h"
#include "nv_params.h"


PIN_Handle pinHandle;
PIN_State pinState;

/*******************************************************************************
* CONSTANTS
*/


/*******************************************************************************
* MACROS
*/


/*******************************************************************************
* @fn          exceptionHandler
*
* @brief       Default exception handler function register in .cfg M3Hwi.excHandlerFunc
*
* input parameters
*
* @param       None.
*
* output parameters
*
* @param       None.
*
* @return      None.
*/
void exceptionHandler()
{
    while(1){}
}





/*******************************************************************************
* @fn          main
*
* @brief       main function
*
* input parameters
*
* @param       None
*
* output parameters
*
* @param       None.
*
* @return      None.
*/

void main()
{  
    PIN_init(BoardGpioInitTable);
    LED_init();

#ifdef FEATURE_MAC_SECURITY
    bspAesInit(0);
#endif

    coap_config_init();
    rplConfig_init();
    set_defaultTschMacConfig();
    
#if I3_MOTE_SPLIT_ARCH
    pinHandle = PIN_open(&pinState, BoardGpioInitTable);
    SPI_init();    
#endif

#if I3_MOTE || I3_MOTCoAP - sensortagE_SPLIT_ARCH
    ehest_enable();
#endif
//    NVM_init(); //fengmo NVM
    BIOS_start();     /* enable interrupts and start SYS/BIOS */

}