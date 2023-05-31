/*
* Copyright (c) 2010 - 2014 Texas Instruments Incorporated
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
 *  ====================== ulpsmacbuf.h =============================================
 */


#ifndef __ULPSMACBUF_H__
#define __ULPSMACBUF_H__

#include "net/packetbuf.h"

#include "ulpsmacaddr.h"

/* \brief      The size of the packetbuf, in bytes */
#define ULPSMACBUF_DATA_SIZE  PACKETBUF_SIZE

/* \brief      The size of the packetbuf header, in bytes */
#define ULPSMACBUF_HDR_SIZE   PACKETBUF_HDR_SIZE

#define ULPSMACBUF_ATTR_BIT   PACKETBUF_ATTR_BIT
#define ULPSMACBUF_ATTR_BYTE  PACKETBUF_ATTR_BYTE

#define ULPSMACBUF_NUM_ADDRS  2                          //PACKETBUF_NUM_ADDRS
#define ULPSMACBUF_ADDRSIZE   sizeof(ulpsmacaddr_t) //PACKETBUF_ADDRSIZE

typedef packetbuf_attr_t      ulpsmacbuf_attr_t;

struct ulpsmacbuf_attr {
  ulpsmacbuf_attr_t val;
};

struct ulpsmacbuf_addr {
  ulpsmacaddr_t addr;
};


#define ULPSMACBUF_NUM_ATTRS     (ULPSMACBUF_ATTR_MAX - ULPSMACBUF_NUM_ADDRS)
#define ULPSMACBUF_ADDR_FIRST    ULPSMACBUF_ADDR_SENDER
#define ULPSMACBUF_IS_ADDR(type) ((type) >= ULPSMACBUF_ADDR_FIRST)

#define ULPSMACBUF_ATTRIBUTES(...) { __VA_ARGS__ ULPSMACBUF_ATTR_LAST }
#define ULPSMACBUF_ATTR_LAST { ULPSMACBUF_ATTR_NONE, 0 }

#define ULPSMACBUF_ATTR_PACKET_TYPE_BEACON       0x00
#define ULPSMACBUF_ATTR_PACKET_TYPE_DATA         0x01
#define ULPSMACBUF_ATTR_PACKET_TYPE_ACK          0x02
#define ULPSMACBUF_ATTR_PACKET_TYPE_COMMAND      0x03
#define ULPSMACBUF_ATTR_PACKET_TYPE_LL           0x04
#define ULPSMACBUF_ATTR_PACKET_TYPE_MULTIPURPOSE 0x05

#define ULPSMACBUF_ATTR_PACKET_TYPE_TSCH_ADV     ULPSMACBUF_ATTR_PACKET_TYPE_BEACON

#define ULPSMACBUF_ATTR_ADDR_MODE_LONG           ULPSMACADDR_MODE_LONG
#define ULPSMACBUF_ATTR_ADDR_MODE_SHORT          ULPSMACADDR_MODE_SHORT

#define ULPSMACBUF_ATTR_MAC_SEQNO_INVALID        0xffff

#define ULPSMACBUF_ATTR_ACKREQ_FALSE             0x00
#define ULPSMACBUF_ATTR_ACKREQ_TRUE              0x01

#define ULPSMACBUF_ATTR_ACKNACK_ACK              0x00
#define ULPSMACBUF_ATTR_ACKNACK_NACK             0x01

// ULPSMACBUF_DATA_SIZE should be less than 255 since we use 8 bits for len/ptr
struct ulpsmacbuf {
  uint8_t datalen; //buflen;
  uint8_t dataoffset; //bufptr;
  uint16_t buf_aligned[(128) / 2 ];
};

#endif /* __ULPSMACBUF_H__ */

/** @} */
/** @} */
