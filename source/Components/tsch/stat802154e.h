/*
* Copyright (c) 2010-2014 Texas Instruments Incorporated
*
* All rights reserved not granted herein.
* Limited License. 
*
* Texas Instruments Incorporated grants a world-wide, royalty-free,
* non-exclusive license under copyrights and patents it now or hereafter
* owns or controls to make, have made, use, import, offer to sell and sell ("Utilize")
* this software subject to the terms herein.  With respect to the foregoing patent
*license, such license is granted  solely to the extent that any such patent is necessary
* to Utilize the software alone.  The patent license shall not apply to any combinations which
* include this software, other than combinations with devices manufactured by or for TI (“TI Devices”). 
* No hardware patent is licensed hereunder.
*
* Redistributions must preserve existing copyright notices and reproduce this license (including the
* above copyright notice and the disclaimer and (if applicable) source code license limitations below)
* in the documentation and/or other materials provided with the distribution
*
* Redistribution and use in binary form, without modification, are permitted provided that the following
* conditions are met:
*
*       * No reverse engineering, decompilation, or disassembly of this software is permitted with respect to any
*     software provided in binary form.
*       * any redistribution and use are licensed by TI for use only with TI Devices.
*       * Nothing shall obligate TI to provide you with source code for the software licensed and provided to you in object code.
*
* If software source code is provided to you, modification and redistribution of the source code are permitted
* provided that the following conditions are met:
*
*   * any redistribution and use of the source code, including any resulting derivative works, are licensed by
*     TI for use only with TI Devices.
*   * any redistribution and use of any object code compiled from the source code and any resulting derivative
*     works, are licensed by TI for use only with TI Devices.
*
* Neither the name of Texas Instruments Incorporated nor the names of its suppliers may be used to endorse or
* promote products derived from this software without specific prior written permission.
*
* DISCLAIMER.
*
* THIS SOFTWARE IS PROVIDED BY TI AND TI’S LICENSORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
* BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL TI AND TI’S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
/*
 *  ====================== stat802154e.h =============================================
 *  
 */

#ifndef __STAT802154E_H__
#define __STAT802154E_H__

#include "ulpsmac.h"

/*---------------------------------------------------------------------------*/
#define STAT802154E_TRUE    ULPSMAC_TRUE
#define STAT802154E_FALSE   ULPSMAC_FALSE

/*---------------------------------------------------------------------------*/
typedef enum
{
  /* generic status code */
  STAT802154E_SUCCESS                         = 0,

  /* MAC internal implementation status code */
  STAT802154E_ERR                             = 0x80, //try later
  STAT802154E_ERR_FATAL                       = 0x81, //fatal
  STAT802154E_ERR_BUSY                        = 0x82, //device is busy

  STAT802154E_INVALID_INPUT                   =0x83,  //inputs are invalid
  // 802.15.4 defined status code
  // 802.15.4 enum definition in 
  // section 7.1.17
  //STAT802154E_BEACON_LOSS                     = 0xe0,
  STAT802154E_CHANNEL_ACCESS_FAILURE          = 0xe1,
  //STAT802154E_DENIED                          = 0xe2,
  //STAT802154E_DISABLE_TRX_FAILURE             = 0xe3,
  STAT802154E_FRAME_TOO_LONG                  = 0xe5,

  //STAT802154E_IMPROPER_KEY_TYPE               = 0xdc,
  //STAT802154E_IMPROPER_SECURITY_LEVEL         = 0xdd,
  //STAT802154E_COUNTER_ERROR                   = 0xdb,

  STAT802154E_INVALID_ADDRESS                 = 0xf5,
  STAT802154E_INVALID_GTS                     = 0xe6,
  //STAT802154E_INVALID_HANDLE                  = 0xe7,
  STAT802154E_INVALID_INDEX                   = 0xf9,
  STAT802154E_INVALID_PARAMETER               = 0xe8,
  //STAT802154E_LIMIT_REACHED                   = 0xfa,
  STAT802154E_NO_ACK                          = 0xe9,
  //STAT802154E_NO_BEACON                       = 0xea,
  STAT802154E_NO_DATA                         = 0xeb,
  STAT802154E_NO_SHORT_ADDRESS                = 0xec,
  //STAT802154E_ON_TIME_TOO_LONG                = 0xf6,
  //STAT802154E_OUT_OF_CAP                      = 0xed,
  //STAT802154E_PAN_ID_CONFLICT                 = 0xee,
  //STAT802154E_PAST_TIME                       = 0xf7,
  STAT802154E_READ_ONLY                       = 0xfb,
  //STAT802154E_REALIGNMENT                     = 0xef,
  STAT802154E_SCAN_IN_PROGRESS                = 0xfc,
  //STAT802154E_SECURITY_ERROR                  = 0xe4,
  //STAT802154E_SUPERFRAME_OVERLAP              = 0xfd,
  //STAT802154E_TRACKING_OFF                    = 0xf8,
  //STAT802154E_TRANSACTION_EXPIRED             = 0xf0,
  STAT802154E_TRANSACTION_OVERFLOW            = 0xf1,
  //STAT802154E_TX_ACTIVE                       = 0xf2,
  //STAT802154E_UNAVAILABLE_KEY                 = 0xf3,
  STAT802154E_UNSUPPORTED_ATTRIBUTE           = 0xf4,
  //STAT802154E_UNSUPPORTED_LEGACY              = 0xde,
  //STAT802154E_UNSUPPORTED_SECURITY            = 0xdf,


  /* 802.15.4e specific extensions */
  //STAT802154E_BAD_CHANNEL                          = 0xd0, //chn probe has detected bad channel
  STAT802154E_SLOTFRAME_NOT_FOUND                  = 0xd1, // slotframehandle reference not found
  STAT802154E_MAX_SLOTFRAMES_EXCEEDED              = 0xd2, // device cannot add more slotframes
  STAT802154E_UNKNOWN_LINK                         = 0xd3, // link handle reference not found
  STAT802154E_MAX_LINKS_EXCEEDED                   = 0xd4, // device cannot add more links
  STAT802154E_NO_SYNC                              = 0xd5, // cannot start TSCH mode before sync

} STAT802154E_status_t;

#endif /* ___STAT802154E_H__ */
