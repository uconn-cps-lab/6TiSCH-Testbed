/*
* Copyright (c) 2014 Texas Instruments Incorporated
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
* include this software, other than combinations with devices manufactured by or for TI (â€œTI Devicesâ€). 
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
* THIS SOFTWARE IS PROVIDED BY TI AND TIâ€™S LICENSORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
* BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL TI AND TIâ€™S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
/*
 *  ====================== clock.c =============================================
 *  Use TI-RTOS clock module to get time. 
 */

 
#include "sys/clock.h"
 
//In system ticks!
clock_time_t clock_time()
{
    return pltfrm_getTimeStamp();
}

clock_time_t clock_milisceconds()
{
    clock_time_t ret = clock_time()/(CLOCK_SECOND/1000);
    return ret;
}


unsigned long clock_seconds(void)
{
    clock_time_t ret = clock_time()/CLOCK_SECOND;
    return ret;
}

void
clock_init()
{

}

#define MAX32BITINT     (0xFFFFFFFF)
#define HALFMAX32BITINT (0x80000000)

long clock_delta(clock_time_t t1, clock_time_t t2)
{
   long rtv;
   uint32_t v1 = t1 & MAX32BITINT;
   uint32_t v2 = t2 & MAX32BITINT;
   uint32_t dv;

   if (v1 < v2)
   {
      dv = v2 - v1;
      if (dv >= HALFMAX32BITINT)
      {
         rtv = MAX32BITINT - dv + ((dv > HALFMAX32BITINT) ? 1 : 0);
      }
      else
      {
         rtv = -dv;
      }
   }
   else
   {
      dv = v1 - v2;
      if (dv >= HALFMAX32BITINT)
      {
         rtv = -(MAX32BITINT - dv) - ((dv > HALFMAX32BITINT) ? 1 : 0);
      }
      else
      {
         rtv = dv;
      }
   }

   return(rtv);
}
