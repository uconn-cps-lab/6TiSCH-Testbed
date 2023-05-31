/************************************************************************/
/* Contiki-specific parameters                                          */
/************************************************************************/

#ifndef _PLATFORM_H_
#define _PLATFORM_H_ 1

#ifdef WITH_CONTIKI //CONTIKI

/* global constants for constrained devices running Contiki */
#ifndef DTLS_PEER_MAX
/** The maximum number DTLS peers (i.e. sessions). */
#ifdef LINUX_GATEWAY
#define DTLS_PEER_MAX 32
#else
#  define DTLS_PEER_MAX 10
#endif
#endif

#ifndef DTLS_HANDSHAKE_MAX
/** The maximum number of concurrent DTLS handshakes. */

#ifdef LINUX_GATEWAY
#define DTLS_HANDSHAKE_MAX 32
#else
#define DTLS_HANDSHAKE_MAX 10
#endif
#endif

#ifndef DTLS_SECURITY_MAX
/** The maximum number of concurrently used cipher keys */
#define DTLS_SECURITY_MAX (DTLS_PEER_MAX + DTLS_HANDSHAKE_MAX)
#endif

#ifndef DTLS_HASH_MAX
/** The maximum number of hash functions that can be used in parallel. */
#define DTLS_HASH_MAX (3 * DTLS_PEER_MAX)
#endif

/************************************************************************/
/* Specific Contiki platforms                                           */
/************************************************************************/

#ifdef CONTIKI_TARGET_CC2538DK
#  include "platform-specific/config-cc2538dk.h"
#endif /* CONTIKI_TARGET_CC2538DK */

#endif /* CONTIKI */

#endif /* _PLATFORM_H_ */
