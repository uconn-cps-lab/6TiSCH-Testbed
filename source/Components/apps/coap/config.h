#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "lib/list.h"
#include "lib/memb.h"
#include "lib/random.h"

#include "net/tcpip.h"
#include "net/uip.h"
#include "net/uip-fw.h"
#include "net/udp-simple-socket.h"
#include "net/uip-icmp6.h"
#include "net/uip-ds6.h"

#include "sys/clock.h"

#define FIREFOX_COAP_MOD 0
#define COAP_RESOURCE_CHECK_TIME 1 // check every 10 slots
#define COAP_SUBSCRIBE_FREQ 10 // check every slot
//#define COAP_SENSOR_RESOURCE_DIRTY_FREQ 1
#define COAP_NW_PERF_RESOURCE_DIRTY_FREQ 6*100

#define COAP_TIMER 1
#define COAP_DATA 2
#if FEATURE_DTLS
#define COAP_DTLS_TIMER 3
#endif
#define COAP_RESTART_SCAN 4
#define COAP_START_SCAN 5
#define COAP_MAC_SEC_TIMER 6
#define COAP_DTLS_MAC_SEC_CONNECT 7
#define COAP_DELETE_ALL_OBSERVERS 8

#define WITH_CONTIKI 1
//#define WITHOUT_OBSERVE
#ifndef NDEBUG
#define NDEBUG
#endif
#ifdef BYTE_ORDER
#undef BYTE_ORDER
#endif
#define BYTE_ORDER UIP_LITTLE_ENDIAN
#define UINT_MAX   4294967295U

/** Below definitions are important for static memory allocations **/
#ifdef LINUX_GATEWAY
#define HAVE_SYSLOG_H
#else
#define COAP_MAX_PDU_SIZE 164
#endif

/** Number of resources that can be handled by a CoAP server in addition to
 * @c /.well-known/core */
#define COAP_MAX_RESOURCES 10

/** Number of attributes that can be handled (should be at least
 * @c 2 * COAP_MAX_RESOURCES. to carry the content type and the
 * resource type. */
#define COAP_MAX_ATTRIBUTES 8

/**
 * Number of PDUs that can be stored simultaneously. This number
 * includes both, the PDUs stored for retransmission as well as the
 * PDUs received. Beware that choosing a too small value can lead to
 * many retransmissions to be dealt with.
 */
#define COAP_PDU_MAXCNT 4

/**
 * Maximum number of subscriptions. Every additional subscriber costs
 * 36 B.
 */
#ifdef HARP_MINIMAL
#define COAP_MAX_SUBSCRIBERS 1
#else 
#define COAP_MAX_SUBSCRIBERS 2
#endif
/**
 * Number of notifications that may be sent non-confirmable before a
 * confirmable message is sent to detect if observers are alive. The
 * maximum allowed value here is @c 15.
 */
#define COAP_OBS_MAX_NON   10

/**
 * Number of confirmable notifications that may fail (i.e. time out
 * without being ACKed) before an observer is removed. The maximum
 * value for COAP_OBS_MAX_FAIL is @c 3.
 */
#define COAP_OBS_MAX_FAIL  3

#ifdef WITH_JSON
// JSON
typedef struct {
	uint8_t outbuf[COAP_MAX_PDU_SIZE];
	uint8_t outbuf_pos;
} json_output; //Header file

#endif /* WITH_JSON */

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0
#define HAVE_STRNLEN 1
#define HAVE_SNPRINTF 1

/* there is no file-oriented output */
#define COAP_DEBUG_FD NULL
#define COAP_ERR_FD   NULL

#if defined(PLATFORM) && PLATFORM == PLATFORM_MC1322X
/* Redbee econotags get a special treatment here: endianness is set
 * explicitly, and
 */
#ifdef BYTE_ORDER
#undef BYTE_ORDER
#endif
#define BYTE_ORDER UIP_LITTLE_ENDIAN

#undef HAVE_ASSERT_H
#define HAVE_UNISTD_H
#endif /* defined(PLATFORM) && PLATFORM == PLATFORM_MC1322X */

#if defined(TMOTE_SKY)
/* Need to set the byte order for TMote Sky explicitely */
#ifdef BYTE_ORDER
#undef BYTE_ORDER
#endif
#define BYTE_ORDER UIP_LITTLE_ENDIAN

typedef int ssize_t;
#undef HAVE_ASSERT_H
#endif /* defined(TMOTE_SKY) */

#ifndef BYTE_ORDER
# ifdef UIP_CONF_BYTE_ORDER
#ifdef BYTE_ORDER
#undef BYTE_ORDER
#endif
#  define BYTE_ORDER UIP_CONF_BYTE_ORDER
# else
#  error "UIP_CONF_BYTE_ORDER not defined"
# endif /* UIP_CONF_BYTE_ORDER */
#endif /* BYTE_ORDER */

/* Define assert() as emtpy directive unless HAVE_ASSERT_H is given. */
#ifndef HAVE_ASSERT_H
#undef assert
# define assert(x)
#endif

#define ntohs uip_ntohs
#define htons uip_htons

#define PROCESS_CONTEXT_BEGIN
#define PROCESS_CONTEXT_END

#endif /* _CONFIG_H_ */

