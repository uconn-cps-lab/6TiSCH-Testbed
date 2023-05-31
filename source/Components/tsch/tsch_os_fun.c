/*
* Copyright (c) 2015 Texas Instruments Incorporated
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
 *  ====================== tsch_os_fun.c =============================================
 *  Generic OS dependent fcuntion Implementation
 */

//*****************************************************************************
// includes
//*****************************************************************************
#include "tsch_os_fun.h"

//*****************************************************************************
// functions
//*****************************************************************************

//! \brief     Post notification to corresponding event handler
//! \param[in] handler   Pointer to the event handler
//! \param[in] msg       Pointer to the message that will be passed along with
//!                      the notification
//! \param[in] timeout   Maximum wait time if no free space avaible for post
//! \param[in] type      Post type
bool SN_post(void *handle, void *msg, uint32_t timeout, uint8_t type)
{
    bool res = false;

#ifdef WITH_SYSBIOS
    if (handle)
    {
        if (type == NOTIF_TYPE_MAILBOX)
        {
            res = Mailbox_post((Mailbox_Handle ) handle, (Ptr) msg, timeout);
        }
        else if (type == NOTIF_TYPE_SEMAPHORE)
        {
            Semaphore_post((Semaphore_Handle ) handle);
            res = true;
        }
        else
        {
            res = false;
        }
#endif
    }
    return (res);
}

//! \brief     Pend on notification for corresponding event handler
//! \param[in] handler  Pointer to the event handler
//! \param[in] msg      Pointer to the message that will be passed along with
//!                     the notification
//! \param[in] timeout  Maximum pending time for notification
//! \param[in] type     Pend type. For SYSBIOS, the type could be either
//!                     NOTIF_TYPE_MAILBOX or NOTIF_TYPE_SEMAPHORE
bool SN_pend(void *handle, void *msg, uint32_t timeout, uint8_t type)
{
    bool res;

#ifdef WITH_SYSBIOS
    if (handle)
    {
        if (type == NOTIF_TYPE_MAILBOX)
        {
            res = Mailbox_pend((Mailbox_Handle ) handle, (Ptr) msg, timeout);
        }
        else if (type == NOTIF_TYPE_SEMAPHORE)
        {
            res = Semaphore_pend((Semaphore_Handle ) handle, timeout);
        }
        else
        {
            res = false;
        }
    }
#endif

    return (res);
}

//! \brief     Congfigure and start a clock event
//! \param[in] clock_handler   Handle for the clock event
//! \param[in] period          If period > 0, then the clock event is periodic.
//                             If period = 0, then the clock is one time clock.
//! \param[in] init_timeout    Initial timeout for the first expiration of the
//!                            clock event
void SN_clockSet(void* clock_handle, uint32_t period, uint32_t init_timeout)
{
#ifdef WITH_SYSBIOS
    if (clock_handle)
    {
        if ((period > 0) || (init_timeout > 0))
        {
            Clock_Handle clk = (Clock_Handle) clock_handle;
            Clock_stop(clk);

            if (init_timeout > 0)
            {
                Clock_setTimeout(clk, init_timeout);
            }

            if (period > 0)
            {
                Clock_setPeriod(clk, period);
            }

            Clock_start(clk);
        }
    }
#endif
}

//! \brief     Start a configured clock event
//! \param[in] clock_handler   Handle for the clock event
void SN_clockStart(void* clock_handle)
{
#ifdef WITH_SYSBIOS
    if (clock_handle)
    {
        Clock_Handle clk = (Clock_Handle) clock_handle;
        Clock_start(clk);
    }
#endif
}

//! \brief     Stop a configured clock event
//! \param[in] clock_handler   Handle for the clock event
void SN_clockStop(void* clock_handle)
{
#ifdef WITH_SYSBIOS
    if (clock_handle)
    {
        Clock_Handle clk = (Clock_Handle) clock_handle;
        Clock_stop(clk);
    }
#endif
}

//! \brief     Restart a configured clock event
//! \param[in] clock_handler   Handle for the clock event
void SN_clockRestart(void* clock_handle)
{
#ifdef WITH_SYSBIOS
    if (clock_handle)
    {
      SN_clockStop(clock_handle);
      SN_clockStart(clock_handle);
    }
#endif
}

//! \brief     Check  if a configured clock event is active
//! \param[in] clock_handler   Handle for the clock event
Bool SN_clockIsActive(void* clock_handle)
{
#ifdef WITH_SYSBIOS
    if (clock_handle)
    {
        Clock_Handle clk = (Clock_Handle) clock_handle;
        return Clock_isActive(clk);
    }
    else
    {
        return FALSE;
    }
#endif
}


//! \brief     Check  if a configured clock event is active
//! \param[in] clock_handler   Handle for the clock event
//! \param[in] func_ptr         Pointer to clock functon
//! \param[in] func_arg         Function argument
void SN_clockSetFunc(void* clock_handle, void(*func_ptr)(void*), void * func_arg )
{
#ifdef WITH_SYSBIOS
    if (clock_handle)
    {
        Clock_Handle clk = (Clock_Handle) clock_handle;
        Clock_FuncPtr fxn = (Clock_FuncPtr)func_ptr;
        UArg arg = (UArg) func_arg;

        Clock_setFunc(clk,fxn,arg);
    }
#endif
}


