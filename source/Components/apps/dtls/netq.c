/* netq.h -- Simple packet queue
 *
 * Copyright (C) 2010--2012 Olaf Bergmann <bergmann@tzi.org>
 *
 * This file is part of the library tinyDTLS. Please see the file
 * LICENSE for terms of use.
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

#include "dtls_config.h"
#include "debug.h"
#include "netq.h"

#ifdef HAVE_ASSERT_H
#include <assert.h>
#else
#ifndef assert
#warning "assertions are disabled"
#  define assert(x)
#endif
#endif

#include "t_list.h"

#include "lib/memb.h"

extern uint32_t dtls_context_lock();
extern void dtls_context_unlock(uint32_t key);

MEMB(netq_storage, netq_t, NETQ_MAXCNT);

static inline netq_t *
netq_malloc_node(size_t size) {
  return (netq_t *)memb_alloc(&netq_storage);
}

static inline void
netq_free_node(netq_t *node) {
   uint32_t lock = dtls_context_lock();
   memb_free(&netq_storage, node);
   dtls_context_unlock(lock);
}

void
netq_init() {
  memb_init(&netq_storage);
}

int 
netq_insert_node(list_t queue, netq_t *node) {
  netq_t *p;

  assert(queue);
  assert(node);

  uint32_t lock = dtls_context_lock();
  p = (netq_t *)list_head(queue);
  while(p && p->t <= node->t && list_item_next(p))
    p = list_item_next(p);

  if (p)
    list_insert(queue, p, node);
  else
    list_push(queue, node);
  dtls_context_unlock(lock);
  return 1;
}

netq_t *
netq_head(list_t queue) {
  if (!queue)
    return NULL;

  return list_head(queue);
}

netq_t *
netq_next(netq_t *p) {
  if (!p)
    return NULL;

  return list_item_next(p);
}

void
netq_remove(list_t queue, netq_t *p) {
  if (!queue || !p)
    return;

  list_remove(queue, p);
}

netq_t *netq_pop_first(list_t queue) {
  if (!queue)
    return NULL;

  return list_pop(queue);
}

netq_t *
netq_node_new(size_t size) {
  netq_t *node;

  uint32_t lock = dtls_context_lock();
  node = netq_malloc_node(size);
  dtls_context_unlock(lock);

#ifndef NDEBUG
  if (!node)
    dtls_warn("netq_node_new: malloc\n");
#endif

  if (node)
    memset(node, 0, sizeof(netq_t));

  return node;  
}

void 
netq_node_free(netq_t *node) {
  if (node)
    netq_free_node(node);
}

void 
netq_delete_all(list_t queue) {
  netq_t *p;
  if (queue) {
    uint32_t lock = dtls_context_lock();
    while((p = list_pop(queue)))
      netq_free_node(p); 
    dtls_context_unlock(lock);
  }
}

