/******************************************************************************
*
*   Copyright (C) 2014 Texas Instruments Incorporated
*
*   All rights reserved. Property of Texas Instruments Incorporated.
*   Restricted rights to use, duplicate or disclose this code are
*   granted through contract.
*
*   The program may not be used without the written permission of
*   Texas Instruments Incorporated or against the terms and conditions
*   stipulated in the agreement under which this program has been supplied,
*   and under no circumstances can it be used with non-TI connectivity device.
*
******************************************************************************/

#include "mqtt_client.h"

static void (*atomic_enbl)(void) = NULL;
static void (*atomic_dsbl)(void) = NULL;

#define ATOMIC_TAKE() if(atomic_enbl) atomic_enbl();
#define ATOMIC_GIVE() if(atomic_dsbl) atomic_dsbl();

//#define DEBUG 1
#define USR_INFO dbg_prn

#ifdef DEBUG 
#define DBG_INFO(I, ...) dbg_prn(I, ##__VA_ARGS__)
#else  
#define DBG_INFO(I, ...)
#endif

static i32 (*dbg_prn)(const i8 *fmt, ...) = NULL;
static const struct device_net_services *net_ops = NULL;
static struct mqtt_client_msg_cbs app_obj, *app_cbs = NULL;

static u16 msg_id = 0xFFFF;

static inline u16 assign_new_msg_id()
{
  return msg_id += 2;
}

enum client_state {

    WAIT_INIT_STATE,
    INIT_DONE_STATE = 0x01,
    CONNECTED_STATE
};
// Robert comment for conn_pl_utfs
//  [0]: point to client ID buffer
//  [1]: will top
//  [2]: will message
//  [3]: user name name
//  [4]: password
//
struct client_desc {

    const struct utf8_string    *conn_pl_utf8s[5]; /* Ref: CONNECT Payload */
    u8                           will_opts;

    enum client_state            state;

    void                        *net;

    /* Wait-List for Level 1 ACK(s) such  as  PUBACK, PUBREC, UN / SUBACK */
    struct mqtt_ack_wlist       *qos_ack1_wl;
    /* Wait-List for Level 2 ACK(s) such  as  PUBCOMP */
    struct mqtt_ack_wlist       *qos_ack2_wl;

    bool                         mqtt_mode31;

    bool                         await_connack;
    bool                         await_pingrsp;
    bool                         clean_session;

    u32                          ka_secs;
    u32                          send_ts; /* Last send time-stamp (secs) */

    u32                          nwconn_info;
    i8                          *server_addr;
    u16                          port_number;
};

static struct mqtt_ack_wlist qos_ack1_wl = {NULL, NULL};
static struct mqtt_ack_wlist qos_ack2_wl = {NULL, NULL};
static struct client_desc client_obj = 
        {{NULL, NULL, NULL, NULL, NULL}, 0, /* Connect Payload and flags */
         WAIT_INIT_STATE, NULL,             /* Client state and  network */
         &qos_ack1_wl, &qos_ack2_wl,        /* Wait list awaiting ACK(s) */
         false, false, false, false,        /* All boolean members init  */
         0, 0,                              /* Keep-Alive and time-stamp */
         0, NULL, 0};                       /* Server Addr & associates  */

static struct client_desc *client = NULL;

static void do_net_close(void)
{  
  net_ops->close(client->net);
  client->net = NULL;

  client->state = INIT_DONE_STATE;

  if(client->await_connack)
          client->await_connack = false;

  if(client->await_pingrsp)
          client->await_pingrsp = false;

  if(client->clean_session) {  /* Session Terminated: clean-up */
          DBG_INFO("CleanSession:1 - cleaning QoS1 / 2 traces\n\r");
          mqp_ack_wlist_purge(client->qos_ack2_wl);
          mqp_ack_wlist_purge(client->qos_ack1_wl);
  }

  return;
}

static inline i32 len_err_free_mqp(struct mqtt_packet *mqp)
{
  mqp_free(mqp);
  return MQP_ERR_PKT_LEN;
}

static i32 buf_send(const u8 *buf, u32 len)
{        
        i32 rv = net_ops->send(client->net, buf, len);
        if(rv <= 0) {
                USR_INFO("Fatal Send Error.\n\r");
                do_net_close();
        } else {
                client->send_ts = net_ops->time();
        }

        return rv;
}

static bool is_connected()
{
        return (client->state == CONNECTED_STATE)? true : false;
}

u16 mqtt_client_new_msg_id()
{
        return assign_new_msg_id();
}

bool mqtt_client_is_connected()
{
        return is_connected();
}

static i32 wr_connect_pl(u8 *buf, u32 fsz, u8 *conn_opts)
{
        u8 *ref = buf, i = 0, j;
        const struct utf8_string *utf8 = client->conn_pl_utf8s[0];
        u8 utf8_opts[] = {0x00, WILL_CONFIG_VAL, 0x00, 
                          USER_NAME_OPVAL, PASS_WORD_OPVAL};

        if(NULL == utf8) { /* Absence of Client ID requires special handling */
                if(fsz < 2)
                        return MQP_ERR_PKT_LEN;

                buf += buf_wr_nbo_2B(buf, 0); /* No Client ID: send 0 Length */
                i = 1;                        /* Client ID has been  handled */
        }
        
        for(j = i; j < 5; j++) {
                utf8 = client->conn_pl_utf8s[j];
                if(NULL == utf8)
                        continue;
                
                if(fsz < (buf - ref + utf8->length))
                        return MQP_ERR_PKT_LEN;   /* Payload: no space left */

                buf += mqp_buf_wr_utf8(buf, utf8);
                *conn_opts |= utf8_opts[j];     /* Enable appropriate flags */
        }

        return buf - ref;
}

/* Define protocol information for the supported versions */
static u8 mqtt310[] = {0x00, 0x06, 'M', 'Q', 'I', 's', 'd', 'p', 0x03};
static u8 mqtt311[] = {0x00, 0x04, 'M', 'Q', 'T', 'T', 0x04};

static inline u16 get_connect_vh_len(void)
{
        return (client->mqtt_mode31? sizeof(mqtt310) : sizeof(mqtt311)) + 3;
}

static i32 wr_connect_vh(u8 *buf, u16 ka_secs, u8 conn_opts)
{
        u8  *ref = buf;

        if(client->mqtt_mode31) 
                buf += buf_wr_nbytes(buf, mqtt310, sizeof(mqtt310));
        else
                buf += buf_wr_nbytes(buf, mqtt311, sizeof(mqtt311));

        *buf++ = conn_opts; 
        buf += buf_wr_nbo_2B(buf, ka_secs);

        return buf - ref;
}

static i32 try_net_connect(void)
{
        if((INIT_DONE_STATE != client->state) || client->await_connack)
                return MQP_ERR_NOT_DEF;

        if(NULL == net_ops)
                return MQP_ERR_NET_OPS;
        
        client->net = net_ops->open(client->nwconn_info, client->server_addr,
                                    client->port_number);

        return (NULL == client->net)? MQP_ERR_NETWORK : 0;
}

static i32 connect_msg_send(bool clean_session, u16 ka_secs)
{
        struct mqtt_packet *mqp = mqp_client_send_alloc(MQTT_CONNECT);
        u8 *buf, *ref, conn_opts = clean_session? CLEAN_START_VAL : 0;
        i32 rv = MQP_ERR_PKT_LEN; 
        u32 fsz; /* Free buffer size in PKT */ 
        u16 vhl = get_connect_vh_len();

        if(NULL == mqp)
                return MQP_ERR_PKT_AVL;
        
        rv = try_net_connect();
        if(rv < 0)
                goto mqtt_connect_msg_send_exit1;      /* No network to broker */

        fsz = MQP_FREEBUF_LEN(mqp);
        if(fsz < vhl)
                goto mqtt_connect_msg_send_exit1;      /* No space for VAR HDR */
        
        mqp->vh_len = vhl;               /* Reserve buffer for variable header */
        buf = ref = MQP_PAYLOAD_BUF(mqp);/* Get started to incorporate payload */
                
        rv = wr_connect_pl(buf, fsz - vhl, &conn_opts);   /* Payload  Contents */
        if(rv < 0)
                goto mqtt_connect_msg_send_exit1;         /* Payload WR failed */

        buf += rv;
        mqp->pl_len = buf - ref;

        wr_connect_vh(ref - vhl, ka_secs, client->will_opts | conn_opts);/* VH */
        
        mqp_prep_fh(mqp, MAKE_FH_FLAGS(false, MQTT_QOS0, false));/* Fix Header */
        
        ref = MQP_FHEADER_BUF(mqp);
        rv  = buf_send(ref, buf - ref);
        if(rv > 0) {    /* Successfully sent CONNECT msg - now do housekeeping */
                client->await_connack = true;
                client->clean_session = clean_session;
                client->ka_secs       = ka_secs;
        }
        
 mqtt_connect_msg_send_exit1:
        if(mqp)
                mqp_free(mqp);
        
        return rv;
}

i32 mqtt_connect_msg_send(bool clean_session, u16 ka_secs)
{
        return connect_msg_send(clean_session, ka_secs);
}

static void ack_wl_append_atomic(struct mqtt_ack_wlist *wl, 
                                 struct mqtt_packet   *mqp)
{
        ATOMIC_TAKE();
        mqp_ack_wlist_append(wl, mqp);
        ATOMIC_GIVE();
}

static void ack_wl_append(struct mqtt_packet *mqp)
{
        switch(ENUM_QOS(mqp->fh_byte1 & 0x0F)) {
                
        case MQTT_QOS1: 
                ack_wl_append_atomic(client->qos_ack1_wl, mqp);
                break;

        case MQTT_QOS2:
                ack_wl_append_atomic(client->qos_ack2_wl, mqp);
                break;

        default:
                break;
        }

        return;
}

/* 
   To be used for the following messages: PUBLISH, SUBSCRIBE, UNSUBSCRIBE
   Dispatches msg to broker over socket. Frees-up MQP, in case, MSG has QoS0 or
   if client-lib allocated MQP encounters an error in dispatch.
   Returns, on success, number of bytes transfered, otherwise -1 
*/
static i32 _msg_dispatch(struct mqtt_packet *mqp, enum mqtt_qos qos,
                         bool retain, bool local_mqp)
{
        i32 rv = MQP_ERR_NETWORK;
        bool free_flag = true;

        if(false == is_connected())
                return MQP_ERR_NOTCONN;            /* CONNECT to broker first */
        
        mqp_prep_fh(mqp, MAKE_FH_FLAGS(false, qos, retain));
        
        rv = buf_send(MQP_FHEADER_BUF(mqp), MQP_CONTENT_LEN(mqp));
        if(rv <= 0)
                goto _msg_dispatch_exit1;

        rv = mqp->msg_id;
        /* MQTT 3.1.1: only PUB messages are required to be resent or retried */
        if(client->mqtt_mode31 || (MQTT_PUBLISH == mqp->msg_type))
                if(MQTT_QOS0 != qos) {
                        ack_wl_append(mqp);    /* Needs ACK, put in wait-list */
                        free_flag  = false;    /* Yes, this MQP can not freed */
                }

_msg_dispatch_exit1:
        if(free_flag)                          /* Nothing to wait from broker */
                if((rv > 0) || local_mqp)      /* Error: Don't free app's MQP */
                        mqp_free(mqp);
                
        return rv;
}

inline static i32 msg_dispatch(struct mqtt_packet *mqp, enum mqtt_qos qos,
                               bool retain, bool local_mqp)
{
        if(NULL == mqp)
                return MQP_ERR_FNPARAM;
        
        return _msg_dispatch(mqp, qos, retain, local_mqp);
}

i32 mqtt_client_pub_msg_send(const struct utf8_string *topic, const u8 *data_buf,
                             u32 data_len, enum mqtt_qos qos, bool retain)
{
        struct mqtt_packet *mqp = NULL;

        if((NULL == topic) || ((!!data_buf) ^ (!!data_len)))
                return MQP_ERR_FNPARAM;

        mqp = mqp_client_send_alloc(MQTT_PUBLISH);
        if(NULL == mqp)
                return MQP_ERR_PKT_AVL;

        if((0 > mqp_pub_append_topic(mqp, topic, qos? assign_new_msg_id(): 0)) ||
           (data_len && (0 > mqp_pub_append_data(mqp, data_buf, data_len)))) 
                return len_err_free_mqp(mqp);

        return _msg_dispatch(mqp, qos, retain, true);
}

i32 mqtt_client_pub_msg_dispatch(struct mqtt_packet *mqp, enum mqtt_qos qos,
                                 bool retain)
{
        return msg_dispatch(mqp, qos, retain, false);
}


static i32 tail_incorp_msg_id(struct mqtt_packet *mqp)
{
        u8 *buf = MQP_FHEADER_BUF(mqp) + mqp->vh_len;

        if(0 == mqp->msg_id) {
                mqp->msg_id  = assign_new_msg_id();
                buf += buf_wr_nbo_2B(buf, mqp->msg_id);
                mqp->vh_len += 2;

                return 2;
        }

        return 0;
}

i32 mqtt_sub_msg_send(const struct utf8_strqos *qos_topics, u32 count)
{
        u8 *buf, *ref;
        struct mqtt_packet *mqp;
        u32 i, fsz /* Free buffer size in PKT */;

        if(NULL == qos_topics)
                return MQP_ERR_FNPARAM;

        mqp = mqp_client_send_alloc(MQTT_SUBSCRIBE);
        if(NULL == mqp)
                return MQP_ERR_PKT_AVL;

        buf  = MQP_VHEADER_BUF(mqp);
        fsz  = MQP_FREEBUF_LEN(mqp);         /*  Free Buffer Len */
        if(fsz < 2)
                return len_err_free_mqp(mqp);/* MSG-ID: no space */

        buf += tail_incorp_msg_id(mqp);
        ref  = buf;
        fsz -= 2;

        for(i = 0; i < count; i++) {
                const struct utf8_strqos *qos_top = qos_topics + i;
                u32 len = qos_top->length;

                if(fsz < (buf - ref + len + 1))    /* Has space? */
                        return len_err_free_mqp(mqp);    /* Nope */

                buf += buf_wr_nbo_2B(buf, len);
                buf += buf_wr_nbytes(buf, qos_top->buffer, len);

                *buf++ = QOS_VALUE(qos_top->qosreq);
        }

        mqp->pl_len = buf - ref;           /* Len of topics data */

        return _msg_dispatch(mqp, MQTT_QOS1, false, true);
}

i32 mqtt_sub_msg_dispatch(struct mqtt_packet *mqp)
{
        return msg_dispatch(mqp, MQTT_QOS1, false, false);
}

i32 mqtt_unsub_msg_send(const struct utf8_string *topics, u32 count)
{
        struct mqtt_packet *mqp;
        u8 *buf, *ref;
        u32 i, fsz /* Free buffer size in PKT */;

        if(NULL == topics)
                return MQP_ERR_FNPARAM;
        
        mqp = mqp_client_send_alloc(MQTT_UNSUBSCRIBE);
        if(NULL == mqp)
                return MQP_ERR_PKT_AVL;
        
        buf  = MQP_VHEADER_BUF(mqp);
        fsz  = MQP_FREEBUF_LEN(mqp);         /*  Free Buffer Len */
        if(fsz < 2)
                return len_err_free_mqp(mqp);/* MSG-ID: no space */

        buf += tail_incorp_msg_id(mqp);      /* push new  MSG-ID */
        ref  = buf;
        fsz -= 2;
        
        for(i = 0; i < count; i++) {      /*   write all topics */
                const struct utf8_string *topic = topics + i;

                if(fsz < (buf - ref + topic->length)) 
                        return len_err_free_mqp(mqp); /* No buf */
                
                buf += mqp_buf_wr_utf8(buf, topic);
        }

        mqp->pl_len = buf - ref;         /* len of topics data */

        return _msg_dispatch(mqp, MQTT_QOS1, false, true);
}

u16 mqtt_unsub_msg_dispatch(struct mqtt_packet *mqp)
{
        return msg_dispatch(mqp, MQTT_QOS1, false, false);
}

static i32 vh_msg_send(u8 msg_type, enum mqtt_qos qos, bool has_vh, u16 vh_data)
{
        u8  buf[4];
        u32 len = 2;

        if(false == is_connected())
                return MQP_ERR_NOTCONN;
        
        buf[0] = MAKE_FH_BYTE1(msg_type, MAKE_FH_FLAGS(false, qos, false));
        buf[1] = has_vh ? 2 : 0;
        
        if(has_vh)
                len += buf_wr_nbo_2B(buf + 2, vh_data);

        return buf_send(buf, len);
}

i32 mqtt_pingreq_send(void)
{
        i32 rv = vh_msg_send(MQTT_PINGREQ, MQTT_QOS0, false, 0);
        if(rv > 0) {
                DBG_INFO("PINGREQ has been sent to broker.\n\r");
                client->await_pingrsp = true;
        }

        return rv;                
}

i32 mqtt_disconn_send(void)
{
        if(false == is_connected())
                return MQP_ERR_NOTCONN;

        vh_msg_send(MQTT_DISCONNECT, MQTT_QOS0, false, 0);
        
        do_net_close(); /* Close network connection with Server */

        return 0;
}        

/*------------------------------------------------------------------------------
 *  RX Functions
 *------------------------------------------------------------------------------
 */
static bool ack_wlist_mqp_dispatch(struct mqtt_ack_wlist *wlist)
{
        struct mqtt_packet *mqp = NULL;
        bool rv = true;

        for(mqp = wlist->head; mqp && (true == rv); mqp = mqp->next) {

                u8 *buf = MQP_FHEADER_BUF(mqp);
                mqp->fh_byte1 = *buf |= DUP_FLAG_VAL(true);

                if(buf_send(buf, MQP_CONTENT_LEN(mqp)) <= 0)
                        rv = false;
        }

        return rv;
}

static bool acks_reminder_send(void)
{
        bool rv = false;

        DBG_INFO("Re-sending QoS1 & Qos2 msgs for which ACKS are awaited.\n\r");
        
        rv = ack_wlist_mqp_dispatch(client->qos_ack2_wl);
        if(true == rv)
                rv = ack_wlist_mqp_dispatch(client->qos_ack1_wl);

        return rv;
}

static
struct mqtt_packet* ack_wl_remove_atomic(struct mqtt_ack_wlist *wl, u16 msg_id)
{
        struct mqtt_packet *mqp;

        ATOMIC_TAKE();
        mqp = mqp_ack_wlist_remove(wl, msg_id); 
        ATOMIC_GIVE();

        return mqp;
}
                                 

static bool ack_wlist_remove(u8 msg_type, u16 msg_id)
{
        /* Note: Significant changes will be made in next revision of software*/ 
        /* Note: Next revision of software will have QoS based implementation */
        struct mqtt_packet *mqp = NULL;

        switch(msg_type) {

        case MQTT_PUBACK:
        case MQTT_SUBACK:
        case MQTT_UNSUBACK:
                mqp = ack_wl_remove_atomic(client->qos_ack1_wl, msg_id);
                break;

        case MQTT_PUBCOMP:
                mqp = ack_wl_remove_atomic(client->qos_ack2_wl, msg_id);
                break;

        case MQTT_CONNACK:                        
        case MQTT_PINGRSP:
        default: 
                break;
        }

        if(mqp) {
                mqp_free(mqp);
                DBG_INFO("ACK rcvd: Msg/ID: 0x%02x/0x%04x \n\r", msg_type, msg_id);
                return true;
        }
               
        USR_INFO("Err: Unexpected ACK Msg/ID 0x%02x/0x%04x\n\r", msg_type, msg_id);
        return false;
}

/* 
   Process ACK Message from Broker. 
   Returns true on success, otherwise false.
   Used for: PUBACK, PUBREC, PUBCOMP, PUBREL, SUBACK and UNSUBACK
*/
static bool proc_ack_msg_rx(struct mqtt_packet *mqp_raw, bool has_pl)
{
        u16 msg_id = 0;
        u8  msg_type = 0, *buf;
        u32 len = 0;

        if(false == mqp_proc_msg_id_ack_rx(mqp_raw, has_pl))
                return false; /* Problem in contents received from broker */

        msg_id   = mqp_raw->msg_id;
        msg_type = mqp_raw->msg_type;

        if(client->mqtt_mode31 || (MQTT_PUBACK == msg_type))
                if(false == ack_wlist_remove(msg_type, msg_id))
                        return false;/* Problemm the ACK was not awaited */

        len      = mqp_raw->pl_len; 
        buf      = MQP_PAYLOAD_BUF(mqp_raw);

        if(app_cbs && app_cbs->notify_ack)
                app_cbs->notify_ack(msg_type, msg_id, len? buf : NULL, len);

        return true;
}

static bool proc_pub_qos_rx(enum mqtt_qos qos, u16 msg_id)
{
        bool rv = false;

        switch(qos) {
        case MQTT_QOS0: 
                rv = true;
                break;
                
        case MQTT_QOS1: 
                if(vh_msg_send(MQTT_PUBACK, MQTT_QOS0, true, msg_id) > 0)
                        rv = true;
                break;

        case MQTT_QOS2: 
        default:
                break;
        }
        
        return rv;
}

static bool proc_pub_msg_rx(struct mqtt_packet *mqp_raw)
{
        bool rv = mqp_proc_pub_rx(mqp_raw);
        u8 B = mqp_raw->fh_byte1;
        
        if(false == rv)
                return rv;

        /* Assess PUB packet for QOS and dispatch ACK for the QoS > 0 */
        proc_pub_qos_rx(ENUM_QOS(B), mqp_raw->msg_id); 

        /* QoS obliation completed, present the PUBLISH packet to app */
        if(app_cbs && app_cbs->publish_rx) {
                /* App has chosen the callback method to  receive PKT */
                mqp_raw->n_refs++;   /* Make app owner of this packet */
                if(app_cbs->publish_rx(BOOL_DUP(B), ENUM_QOS(B), 
                                       BOOL_RETAIN(B), mqp_raw)) {
                        /* App has no use of PKT any more, so free it */
                        mqp_raw->n_refs--;     /* Take back ownership */
                }
        }
        
        return true;
}

static bool proc_connack_rx(struct mqtt_packet *mqp_raw)
{
        u8 *buf = MQP_VHEADER_BUF(mqp_raw);

        mqp_raw->vh_len += 2;
        mqp_raw->pl_len -= 2;

        if(0 != mqp_raw->pl_len)
                return false;      /* There is no payload in CONNACK message */ 
        
        client->await_connack = false;

        if(0 == VHB_CONNACK_RC(buf)) {
                client->state = CONNECTED_STATE; /* Now connected to Broker */
        } else {
                /* Broker didn't allow connection */
                net_ops->close(client->net);
                client->net = NULL; // TBD??? handle network error 
        }

        if(client->state == CONNECTED_STATE) {
                if(client->clean_session) {       /* Start on a clean slate */
                        DBG_INFO("CleanSession:1 - purge prev QoS 1/2 reqs.\n\r");
                        mqp_ack_wlist_purge(client->qos_ack2_wl);
                        mqp_ack_wlist_purge(client->qos_ack1_wl);
                } else {
                        /* Continued session: send pending QoS 1/2 packets */
                        if(false == acks_reminder_send())
                                return false;
                }
        }

        if(app_cbs && app_cbs->notify_ack)
                app_cbs->notify_ack(mqp_raw->msg_type, 0, buf, 2);

        return true;
}

static bool proc_pingrsp_rx(struct mqtt_packet *mqp_raw)
{
        bool rv;

        /* Mark true: contents are correct and it followed request from app
           False: App doesn't need to know about MQTT Lib initiated PINGREQ */
        rv = ((0 == mqp_raw->pl_len) && client->await_pingrsp)? true : false;
        if(rv) {
                client->await_pingrsp = false;
                if(app_cbs && app_cbs->notify_ack)
                        app_cbs->notify_ack(mqp_raw->msg_type, 0, NULL, 0);
        }

        return rv;
}

static bool init_done_state_rx(struct mqtt_packet *mqp_raw)
{
        bool rv = false;

        switch(mqp_raw->msg_type) {
                
        case MQTT_CONNACK: 
                rv = proc_connack_rx(mqp_raw); /* Changes state to CONNECTED */
                break;

        default:
                break;
        }

        return rv;
}

static bool connected_state_rx(struct mqtt_packet *mqp_raw)
{
        bool rv = false;

        switch(mqp_raw->msg_type) {
                
        case MQTT_PUBREC:
        case MQTT_PUBREL:
        case MQTT_PUBCOMP:
                break;  /* Need to implement QoS2 routines */

        case MQTT_PUBACK:
        case MQTT_UNSUBACK:
                rv = proc_ack_msg_rx(mqp_raw, false);
                break;

        case MQTT_PINGRSP:
                rv = proc_pingrsp_rx(mqp_raw);
                break;
                
        case MQTT_SUBACK:
                rv = proc_ack_msg_rx(mqp_raw, true);
                break;
                
        case MQTT_PUBLISH:
                rv = proc_pub_msg_rx(mqp_raw);
                break;

        case MQTT_CONNACK: /* not expected */
        default:
                break;
        }
        
        return rv;
}

static bool process_recv(struct mqtt_packet *mqp_raw)
{
        bool rv = false;

        switch(client->state) {

        case INIT_DONE_STATE: 
                rv = init_done_state_rx(mqp_raw); break;

        case CONNECTED_STATE:
                rv = connected_state_rx(mqp_raw); break;

        default:
                break;
        }

        DBG_INFO("Processing of Msg Type 0x%02x is %s\n\r", mqp_raw->msg_type,
                 rv ? "Successful" : "Failed");
                 
        return rv;
}


/*  
    Since, the network connection is a TCP socket stream, it is imperative that
    adequate checks are put in place to identify a MQTT packet and isolate it
    for further processing. The intent of following routine is to read just one 
    packet from continuous stream and leave rest for the next iteration.
*/
static i32 mqp_recv(struct mqtt_packet *mqp, u32 wait_secs, bool *timed_out)
{
        u32 remlen = 0;
        u8 ref[MAX_FH_LEN], *buf = ref, fh_len = 2;

        i32 len = net_ops->recv(client->net, buf, fh_len, wait_secs, timed_out);
        if(len != 2) 
                return len; /* NW Error: Must recv two bytes in a packet */

        buf++;
        while((*buf++ & 0x80) && (fh_len++ < MAX_FH_LEN)) {/* Get all FH */
                len = net_ops->recv(client->net, buf, 1, wait_secs, timed_out);
                if(len < 1)
                        return len;
        }

        if(-1 == mqp_buf_rd_remlen(buf - (fh_len - 1), &remlen))
                return MQP_ERR_NOT_DEF; /* Shouldn't happen */;

        if(remlen) {/* Try to read all data that follows FH */
                if(mqp->maxlen < (remlen + fh_len))
                        return MQP_ERR_PKT_LEN;/* Inadequate free buffer */
                
                len = net_ops->recv(client->net, MQP_FHEADER_BUF(mqp) + fh_len,
                                    remlen, wait_secs, timed_out);
                if(len != remlen)
                        return (len > 0)? MQP_ERR_NOT_DEF : len;
        }

        /* Set up MQTT Packet for received data from broker */
        buf_wr_nbytes(mqp->buffer + mqp->offset, ref, fh_len);
        mqp->fh_byte1 = *ref;
        mqp->msg_type = MSG_TYPE(*ref);
        mqp->fh_len   = fh_len;
        mqp->pl_len   = remlen;

        return fh_len + remlen;
}

#define TIME_OUT_ERR                                                    \
        "Timeout, Abort RX!!; Not rcvd either PINGRSP or awaited acks"
#define RX_FATAL_ERR "Fatal Socket Error, Abort RX!!"

static i32 ka_sequence(u32 *secs2wait, bool *ping_sent)
{
        u32 ka_secs = client->ka_secs, ttka /* Time to Keep Alive */;
        u32 diff = net_ops->time() -  client->send_ts;
        ttka = (diff >= ka_secs) ? 0 : ka_secs - diff;
        if(0 == ttka) {
                /* KA Boundary: Broker, R U There?  Send a PING Message */
                i32 len = vh_msg_send(MQTT_PINGREQ, MQTT_QOS0, false, 0);
                *ping_sent = true;
                return len;/* Error, if any, will be assessed by caller */
        }

        if(ttka < *secs2wait)
                *secs2wait = ttka;

        return 1;
}

/* 
   MQTT 3.1.1 implementation
   -------------------------

   Keep Alive Time is maxmimum interval within which a client should send a
   packet to broker. If there are either no packets to be sent to broker or
   no retries left, then client is expected to a send a PINGREQ within Keep
   Alive Time. Broker should respond by sending PINGRSP with-in reasonable
   time of 'wait_secs'. If Keep Alive Time is set as 0, then client is not
   expected to be disconnected because of in-activity of MQTT messages.
   The value of 'wait_secs' is assumed to be quite smaller than 'ka_secs'
*/

static i32 ka_and_recv(struct mqtt_packet *mqp, u32 wait_secs)
{
        i32 len = -1;
        bool timed_out = false, await_ping = false;
        
        if(NULL == client->net)
                return MQP_ERR_NOTCONN;

        while(1) {
                u32 secs2wait = wait_secs;
                if(client->ka_secs) {
                        len = ka_sequence(&secs2wait, &await_ping);
                        if(len <= 0)
                                return len;
                }
                
                len = mqp_recv(mqp, secs2wait, &timed_out);
                if(len > 0) {                       /* Has received a packet */
                        USR_INFO("Rcvd: Msg Type 0x%02x\n\r", mqp->msg_type);
                        if(await_ping && (MQTT_PINGRSP == mqp->msg_type)) {
                                mqp_reset(mqp);
                                await_ping = false;
                                continue;
                        }

                        return len;
                }

                if(0 == len)
                        break; /* Quit: Server has closed network connection */

                if((false == timed_out) || await_ping || client->await_connack)
                        break; /* Quit: unreachable broker or NOT a timeoout */

                /* It is a time-out error: was a smaller time-out scheduled? */
                if(secs2wait != wait_secs)
                        continue;/* Yes, it is the KA boundary, send PINGREQ */

                return MQP_ERR_TIMEOUT;
        }

        USR_INFO("RX Abort [%d]: TimeOut %u, Await-Ping %u, Await-CONNACK %u\n\r",
                 len, timed_out, await_ping, client->await_connack);
                 
        do_net_close();              /* Close network connection with Server */
        return len;
}

i32 mqtt_client_await_msg(u8 msg_type, struct mqtt_packet *mqp, u32 wait_secs)
{
        i32 rv = -1;

        if(app_cbs)
                return MQP_ERR_NOT_DEF;

        if(NULL == mqp)
                return MQP_ERR_FNPARAM;
        
        do {
                mqp_reset(mqp);              /* Ensures a clean working buffer */

                rv = ka_and_recv(mqp, wait_secs);
                if(rv <= 0)
                        break;
                
        } while(((0x00 != msg_type) && (msg_type != mqp->msg_type)) ||
                (false == process_recv(mqp)));/* Do Loop: Bad or Ignore packet */
        
        return rv;
}

i32 mqtt_client_run(u32 wait_secs)
{
        i32 rv = -1;
        struct mqtt_packet *mqp = NULL;

        if(NULL == app_cbs)
                return MQP_ERR_NOT_DEF;
                
        do {
                mqp = mqp_client_recv_alloc(0);
                if(NULL == mqp) {
                        rv = MQP_ERR_PKT_AVL;
                        break;
                }

                rv = ka_and_recv(mqp, wait_secs);
                if(rv > 0)
                        process_recv(mqp);  /* process received message */

                mqp_free(mqp);

        } while(rv > 0);
        
        return rv;
}

/*------------------------------------------------------------------------------
 * Deduced and / or Utility MQTT API(s) for use by applications
 *------------------------------------------------------------------------------
 */
i32 mqtt_connect(bool flag, u16 ka_secs, u32 wait_secs)
{
        i32 rv;
        struct mqtt_packet mqp, *rx_mqp = &mqp;
        u8 buf[4];

        mqp_buffer_attach(rx_mqp, buf, 4, 0);

        if(((rv = connect_msg_send(flag, ka_secs)) <= 0) ||
           ((rv = ka_and_recv(rx_mqp,  wait_secs)) <= 0)) {

                return (0 == rv)? MQP_ERR_NETWORK : rv;
        }

        return  proc_connack_rx(rx_mqp)? 
                MQP_CONNACK_RC(rx_mqp) :
                MQP_ERR_NETWORK;
}

/*------------------------------------------------------------------------------
 * Buffer Pool and management, other registrations and initialization.
 *------------------------------------------------------------------------------
 */
static struct mqtt_packet *free_list = NULL;

inline static struct mqtt_packet *mqp_alloc_atomic(void)
{
        struct mqtt_packet *mqp = NULL;

        ATOMIC_TAKE();
        mqp = free_list;
        if(mqp)
                free_list = mqp->next;
        ATOMIC_GIVE();

        return mqp;
}

struct mqtt_packet *mqp_client_alloc(u8 msg_type, u8 offset)
{
        struct mqtt_packet *mqp = mqp_alloc_atomic();
        if(NULL == mqp) {
                USR_INFO("Fatal: No MQP alloc for msg type 0x%02x\n\r", msg_type);
                return NULL;
        }

        mqp_init(mqp, offset);
        mqp->msg_type = msg_type;

        return mqp;
}

/* Do not use this routine with-in this file */
static void free_mqp(struct mqtt_packet *mqp)
{
        ATOMIC_TAKE();
        mqp->next = free_list;
        free_list = mqp;
        ATOMIC_GIVE();
}

i32 mqtt_client_register_buffers(u32 num_mqp, struct mqtt_packet *mqp_vec,
                                 u32 buf_len, u8 *buf_vec)
{
        u32 i, j;

        if((0 == num_mqp) || (0 == buf_len) || free_list)
                return -1;
        
        for(i = 0, j = 0; i < num_mqp; i++, j += buf_len) {
                struct mqtt_packet *mqp = mqp_vec + i;
                
                mqp->buffer = buf_vec + j;
                mqp->maxlen = buf_len;

                mqp->free   = free_mqp;
                mqp->next   = free_list;
                free_list   = mqp;
        }

        return 0;
}

i32 mqtt_client_register_will(const struct utf8_string  *will_top,
                              const struct utf8_string  *will_msg,
                              enum mqtt_qos will_qos, bool retain)
{
        u8 B = 0;
        
        /* Both should be either NULL or valid */
        if((!!will_top) ^ (!!will_msg))
                return -1; /* Not a good combo */

        if(will_top) {
                B = QOS_VALUE(will_qos) << 3;
                if(retain)
                        B |= WILL_RETAIN_VAL;
        }
                
        client->conn_pl_utf8s[1] = will_top;
        client->conn_pl_utf8s[2] = will_msg;

        client->will_opts = B;
        
        return 0;
}

i32 mqtt_client_register_info(const struct utf8_string *client_id,
                              const struct utf8_string *user_name,
                              const struct utf8_string *pass_word)
{
        client->conn_pl_utf8s[0] = client_id;
        client->conn_pl_utf8s[3] = user_name;
        client->conn_pl_utf8s[4] = user_name? pass_word : NULL;

        return 0;
}

i32 mqtt_client_register_net_svc(const struct device_net_services *net)
{
        if(net && net->open && net->close && net->send &&
           net->recv && net->time) {
                net_ops = net;
                return 0;
        }

        return -1;
}

i32 mqtt_client_lib_init(const struct mqtt_client_lib_cfg  *cfg,
                         const struct mqtt_client_msg_cbs  *cbs)
{
        if((NULL == cfg) || (NULL == cfg->dbg_print))
                return -1;

        client              = &client_obj;
        client->state       = INIT_DONE_STATE;
        
        client->mqtt_mode31 = cfg->mqtt_mode31;

        client->nwconn_info = cfg->nwconn_info;
        client->server_addr = cfg->server_addr;
        client->port_number = cfg->port_number;
        
        if(cbs) {
                app_cbs             = &app_obj;
                app_cbs->publish_rx = cbs->publish_rx;
                app_cbs->notify_ack = cbs->notify_ack;
        }
                
        dbg_prn                 = cfg->dbg_print;

        atomic_enbl             = cfg->atomic_enbl;
        atomic_dsbl             = cfg->atomic_dsbl;

        return 0;
}

