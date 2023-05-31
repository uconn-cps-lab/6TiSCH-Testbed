/*******************************************************************************
Filename:       tsch-root.c
Revised:
Revision:

Description:    This file contains main functions and application data receive
handler function.


Copyright 2012-2016 Texas Instruments Incorporated. All rights reserved.

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
PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
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
*******************************************************************************/

/* -----------------------------------------------------------------------------
*  Includes
* ------------------------------------------------------------------------------
*/
#include <ti/drivers/UART.h>

#include "stdio.h"
#include <string.h>

#include "Board.h"
#include "led.h"

#include "uip_rpl_process.h"
#include "mac_config.h"
#include "nhl_api.h"
#include "nhl_setup.h"
#include "bsp_aes.h"
#include "nv_params.h"


/* -----------------------------------------------------------------------------
*  Global Variables
* ------------------------------------------------------------------------------
*/
UART_Handle     uartHandle;


/***********************************************************************//******
 * @fn         exceptionHandler
 *
 * @brief      Default exception handler function register in .cfg
 *
 * @param       none
 *
 * @return      none
 ******************************************************************************/
void exceptionHandler()
{
    while(1){}
}

/***************************************************************************//**
 * @fn          APP_openUART
 *
 * @brief       Set up UART parameters and open UART
 *
 * @param       none
 *
 * @return      none
 ******************************************************************************/
void APP_openUART(void)
{
    UART_Params     params;
    UART_Params_init(&params);
    params.baudRate      = 460800;
    params.writeMode     = UART_MODE_BLOCKING;
    params.readMode     = UART_MODE_BLOCKING;
    params.writeDataMode = UART_DATA_BINARY;
    params.readDataMode  = UART_DATA_BINARY;
    params.readEcho      = UART_ECHO_OFF;
    params.readTimeout   = 100000;//1s
    params.writeTimeout   = 10000;//0.1s

    uartHandle = UART_open(0, &params);
    if (!uartHandle)
    {
        Task_exit();
    }
}

/***************************************************************************//**
 * @fn          APP_printRecvPacket
 *
 * @brief       Print received message to UART console
 *
 * @param[in]   pOriginatorAddr -- Pointer to the address of the originator
 * @param[in]   hops -- Number of hops
 * @param[in]   pPayload -- Pointer to the payload data
 * @param[in]   payloadLen -- Length of the payload
 *
 * @return      none
 ******************************************************************************/
void APP_printRecvPacket(ulpsmacaddr_t *pOriginatorAddr, uint8_t hops,
           const uint8_t *pPayload, uint16_t payloadLen)
{
    unsigned long time;
    uint16_t data,addr;
    int i;
    char msg1[]="\r\n";
    char msg2[20];
    
    time = clock_seconds();

    addr = pOriginatorAddr->u8[0] + (pOriginatorAddr->u8[1] << 8);

    sprintf(msg2,"%04x %04x %02d",addr, time, hops);
    UART_write(uartHandle, msg2, strlen(msg2));

    for(i = 0; i < payloadLen/2; i++)
    {
        memcpy(&data, pPayload, sizeof(data));
        pPayload += sizeof(data);
        sprintf(msg2," %04x", data);
        UART_write(uartHandle, msg2, strlen(msg2));
    }
    UART_write(uartHandle, msg1, strlen(msg1));
}

#if !ENABLE_HCT
/**************************************************************************//***
* @fn          NHL_dataIndCb
*
* @brief       Application data receive callback function.
*
* @param       pDataInd -- Pointer to NHL_McpsDataInd_t structure to be
*                           confirmed
*
* @return      none
*******************************************************************************/
void NHL_dataIndCb(NHL_McpsDataInd_t *pDataInd)
{
    ulpsmacaddr_t       pSenderAddr;
    uint8_t *pAppData;
    uint8_t dataLen;
  
    dataLen = pDataInd->pMsdu->datalen;
    pAppData = (uint8_t *) pDataInd->pMsdu->buf_aligned;
    pSenderAddr = pDataInd->mac.srcAddr;

    APP_printRecvPacket(&pSenderAddr, 0, pAppData, dataLen);
    BM_free_6tisch(pDataInd->pMsdu);
}
#endif

/*******************************************************************************
* @fn          APP_start
*
* @brief       Start the scan process
*
* @param       none
*
* @return      none
*******************************************************************************/
void APP_start(void)
{
   NHL_MlmeStartPAN_t startArg;

  NHL_setupStartArg(&startArg);
  NHL_startPAN(&startArg,NULL);
}

/**************************************************************************//***
 * @fn          main
 *
 * @brief       Main function
 *
 * @param       none
 *
 * @return      none
 *******************************************************************************
 */
void main(void)
{
#if !ENABLE_HCT
    Clock_Params clockParams;
    Clock_Params_init(&clockParams);
    clockParams.period = 0;
    clockParams.startFlag = true;
    Clock_create((Clock_FuncPtr)APP_start,1000, &clockParams, NULL);
#endif

    PIN_init(BoardGpioInitTable);
    LED_init();

#ifdef FEATURE_MAC_SECURITY
    bspAesInit(0);
#endif

    UART_init();
    APP_openUART();

    coap_config_init();
    rplConfig_init();
    set_defaultTschMacConfig();
    NVM_init(); //fengmo NVM
    /* Enable interrupts and start sysbios */
    BIOS_start();
}
