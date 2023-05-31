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

#include "mqtt_common.h"

static void free_mqp(struct mqtt_packet *mqp)
{
        if((0 == --mqp->n_refs) && (mqp->free))
                mqp->free(mqp);

        return;
}

void mqp_free(struct mqtt_packet *mqp)
{
        free_mqp(mqp);
}

static void reset_mqp(struct mqtt_packet *mqp)
{
        /* Fields not handled here are meant to be left unaltered. */
        mqp->fh_byte1 = 0;
        mqp->msg_id   = 0;
        mqp->fh_len   = 0;
        mqp->vh_len   = 0;
        mqp->pl_len   = 0;
}

void mqp_reset(struct mqtt_packet *mqp)
{
        reset_mqp(mqp);
}

void mqp_init(struct mqtt_packet *mqp, u8 offset)
{
        reset_mqp(mqp);

        mqp->offset = offset;
        mqp->n_refs = 1;
        mqp->next   = NULL;
}

static i32 buf_wr_utf8(u8 *buf, const struct utf8_string *utf8)
{
        u8 *ref = buf;

        buf += buf_wr_nbo_2B(buf, utf8->length);
        buf += buf_wr_nbytes(buf, utf8->buffer, utf8->length);

        return buf - ref;
}

i32 mqp_buf_wr_utf8(u8 *buf, const struct utf8_string *utf8)
{
        return buf_wr_utf8(buf, utf8);
}

#define MAX_REMLEN_BYTES  (MAX_FH_LEN - 1)

static i32 buf_tail_wr_remlen(u8 *buf, u32 remlen)
{
        u8 val[MAX_REMLEN_BYTES], i = 0;

        do {
                val[i] = remlen & 0x7F; /* MOD 128 */
                remlen = remlen >> 7;   /* DIV 128 */

                if(remlen)
                        val[i] |= 0x80;
                
                i++;

        } while(remlen);

        buf_wr_nbytes(buf + MAX_REMLEN_BYTES - i, val, i);

        return i;       /* # bytes written in buf */
}

i32 mqp_buf_tail_wr_remlen(u8 *buf, u32 remlen)
{
        return buf_tail_wr_remlen(buf, remlen);
}

static i32 buf_rd_remlen(u8 *buf, u32 *remlen)
{
        u32 val = 0, mul = 0;
        u8 i = 0;

        do {
                val += (buf[i] & 0x7F) << mul;
                mul += 7;

        } while((buf[i++] & 0x80)       && 
                (i < MAX_REMLEN_BYTES));

        *remlen = val;

        /* Return -1 if value was not found */
        return (buf[i - 1] & 0x80)? -1 : i;
}

i32 mqp_buf_rd_remlen(u8 *buf, u32 *remlen)
{
        return buf_rd_remlen(buf, remlen);
}

i32 
mqp_pub_append_topic(struct mqtt_packet *mqp, const struct utf8_string *topic,
                     u16 msg_id)
{
        u8 *buf = MQP_VHEADER_BUF(mqp), *ref = buf;

        if(0 != mqp->vh_len) 
                return -1; /* Topic has been already added */
        
        if(MQP_FREEBUF_LEN(mqp) < (topic + msg_id? 2 : 0))
                return MQP_ERR_PKT_LEN; /* Can't WR topic */

        buf += buf_wr_utf8(buf, topic);

        if(0 != msg_id) {  /* MSG ID 0 indicates ==> QoS0 */
                mqp->msg_id = msg_id;
                buf += buf_wr_nbo_2B(buf, mqp->msg_id);
        }

        mqp->vh_len += buf - ref;

        return buf - ref;
}

i32 mqp_pub_append_data(struct mqtt_packet *mqp, const u8 *data_buf,
                        u32 data_len)
{
        u8 *buf = MQP_PAYLOAD_BUF(mqp) + mqp->pl_len, *ref = buf;

        if(0 == mqp->vh_len)
                return -1;    /* Must include topic first */
        
        if(MQP_FREEBUF_LEN(mqp) < data_len)
                return MQP_ERR_PKT_LEN; /* Can't WR topic */

        /* Including payload info for PUBLISH */
        buf += buf_wr_nbytes(buf, data_buf, data_len);
        mqp->pl_len += buf - ref;
        
        return buf - ref;
}

bool mqp_proc_msg_id_ack_rx(struct mqtt_packet *mqp_raw, bool has_pl) // TBD
{
        u8 *buf = MQP_VHEADER_BUF(mqp_raw);

        if(mqp_raw->pl_len < 2) /* New */
                return false;   /* Bytes for MSG ID not available */

        buf += buf_rd_nbo_2B(buf, &mqp_raw->msg_id);
        mqp_raw->vh_len += 2;
        mqp_raw->pl_len -= 2;

        return (has_pl ^ (!!mqp_raw->pl_len))? false : true;
}

bool mqp_proc_pub_rx(struct mqtt_packet *mqp_raw)
{
        u8 *buf = MQP_VHEADER_BUF(mqp_raw), *ref = buf;
        u16 tmp = 0;

        if(mqp_raw->pl_len < (buf - ref + 2))    /* Length Check  */
                return false; /* Inadequate data for further work */

        buf += buf_rd_nbo_2B(buf, &tmp);         /* Topic  Length */
        buf += tmp;                              /* Topic Content */  

        if(mqp_raw->pl_len < (buf - ref + 2))    /* Length Check  */
                return false; /* Inadequate data for further work */ //TBD

        if(MQTT_QOS0 != ENUM_QOS((mqp_raw->fh_byte1) & 0x06))
                buf += buf_rd_nbo_2B(buf, &mqp_raw->msg_id);

        mqp_raw->vh_len += buf - ref;
        mqp_raw->pl_len -= buf - ref;

        return true;
}

bool mqp_ack_wlist_append(struct mqtt_ack_wlist *list, 
                          struct mqtt_packet    *elem)
{
        elem->next = NULL;                

        if(list->tail) {
                list->tail->next = elem;
                list->tail = elem;
        } else {
                list->tail = elem;
                list->head = elem;
        }

        return true;
}

struct mqtt_packet *mqp_ack_wlist_remove(struct mqtt_ack_wlist *list,
                                          u16 msg_id)
{
        struct mqtt_packet *elem = list->head, *prev = NULL;

        while(elem) {
                if(msg_id == elem->msg_id) {
                        if(prev)
                                prev->next = elem->next;
                        else
                                list->head = elem->next;
                        
                        if(NULL == list->head)
                                list->tail = NULL;
                        
                        break;
                }

                prev = elem;
                elem = elem->next;
        }

        return elem;
}

void mqp_ack_wlist_purge(struct mqtt_ack_wlist *list)
{
        struct mqtt_packet *elem = list->head;
        
        while(elem) {
                struct mqtt_packet *next = elem->next;
                free_mqp(elem);
                elem = next;
        }

        list->head = NULL;
        list->tail = NULL;

        return;
}

i32 mqp_prep_fh(struct mqtt_packet *mqp, u8 flags)
{
        u32 remlen = mqp->vh_len + mqp->pl_len;
        u8 *buf    = mqp->buffer + mqp->offset;
        u8 *ref    = buf;

        buf -= buf_tail_wr_remlen(buf - MAX_REMLEN_BYTES, remlen);

        buf -= 1; /* Make space for FH Byte1 */        
        mqp->fh_byte1 = *buf = MAKE_FH_BYTE1(mqp->msg_type, flags);

        mqp->fh_len   = ref - buf;
        mqp->offset  -= ref - buf;

        return ref - buf;
}


