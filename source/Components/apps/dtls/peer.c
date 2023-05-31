/* dtls -- a very basic DTLS implementation
 *
 * Copyright (C) 2011--2013 Olaf Bergmann <bergmann@tzi.org>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/******************************************************************************
*
* Copyright (c) 2017 -2018 Texas Instruments Inc.  All rights reserved.
*
* DESCRIPTION:
*
* HISTORY:
*
*
******************************************************************************/

#include "global.h"
#include "peer.h"
#include "debug.h"
#include "lib/memb.h"

extern uint32_t dtls_context_lock();
extern void dtls_context_unlock(uint32_t key);

MEMB(peer_storage, dtls_peer_t, DTLS_PEER_MAX);

void
peer_init()
{
   memb_init(&peer_storage);
}

static inline dtls_peer_t *
dtls_malloc_peer()
{
   return memb_alloc(&peer_storage);
}

void
dtls_free_peer(dtls_peer_t *peer)
{
   uint32_t lock = dtls_context_lock();
   dtls_handshake_free(peer->handshake_params);
   dtls_security_free(peer->security_params[0]);
   dtls_security_free(peer->security_params[1]);
   memb_free(&peer_storage, peer);
   dtls_context_unlock(lock);
}

dtls_peer_t *
dtls_new_peer(const session_t *session)
{
   dtls_peer_t *peer;

   uint32_t lock = dtls_context_lock();
   peer = dtls_malloc_peer();
   dtls_context_unlock(lock);

   if (peer)
   {
      memset(peer, 0, sizeof(dtls_peer_t));
      peer->session = *session;
      lock = dtls_context_lock();
      peer->security_params[0] = dtls_security_new();
      dtls_context_unlock(lock);
      if (!peer->security_params[0])
      {
         dtls_free_peer(peer);
         return NULL;
      }

#ifndef DTLS_WITH_TIRTOS
      dtls_dsrv_log_addr(DTLS_LOG_DEBUG, "dtls_new_peer", session);
#endif
  }
  return peer;
}
