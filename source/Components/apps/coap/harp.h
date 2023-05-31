#ifndef HARP_H
#define HARP_H

// hierarchical respource partitioning
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MAX_CHANNEL 15

#define HARP_SEND_DELAY 200 // delay between each harp packet, unit: slot

#ifdef LINUX_GATEWAY
#define MAX_CHILDREN_NUM 50
#else
#define MAX_CHILDREN_NUM 5
#endif
#define MAX_HOP 5 // start from 0
#define TRAFFIC_UNIT 1

#define HARP_MAILBOX_DUMMY 0x01
#define HARP_MAILBOX_SEND_INIT 0x02
#define HARP_MAILBOX_SEND_IFACE 0x03
#define HARP_MAILBOX_SEND_SP 0x04
#define HARP_MAILBOX_SEND_SCH 0x05

typedef struct
{
    uint16_t ts;
    uint8_t ch;
} HARP_interface_t;

typedef struct
{
    uint16_t ts_start;
    uint16_t ts_end;
    uint8_t ch_start;
    uint8_t ch_end;
} HARP_subpartition_t;

typedef struct
{
    uint8_t id;
    uint8_t traffic;
    HARP_interface_t iface[MAX_HOP + 1];
    HARP_subpartition_t sp_rel[MAX_HOP + 1];
    HARP_subpartition_t sp_abs[MAX_HOP + 1];
} HARP_child_t;

typedef struct
{
    uint8_t id;
    uint8_t parent;
    uint8_t layer;
    uint8_t children_cnt;
    uint8_t recv_iface_cnt;
    uint16_t partition_center; // to copy an uplink cell to a downlink cell
    uint16_t partition_up_end; // uplink partition end (right edge)
    uint8_t traffic_fwd;
    uint8_t adjustingNodes[MAX_CHILDREN_NUM];
    int8_t adjustingNodesCnt;
    uint8_t adjustingLayer;
    uint8_t relocatedCnt;
    HARP_interface_t iface[MAX_HOP + 1];
    HARP_subpartition_t sp_abs[MAX_HOP + 1];
} HARP_self_t;

typedef struct __HARP_skyline_t
{
    uint16_t start;
    uint16_t end;
    uint16_t width;
    uint16_t height;
    struct __HARP_skyline_t *prev;
    struct __HARP_skyline_t *next;
} HARP_skyline_t;

#ifndef LINUX_GATEWAY
typedef struct __HARP_mailbox_msg_t
{
    uint8_t type;
} HARP_mailbox_msg_t;

void harp_task(void);
#endif

void harp_init(unsigned char *data);
uint8_t abstractInterface();
uint8_t interfaceComposition();
uint8_t subpartitionAllocation();
uint8_t subpartitionAdjustment();
uint8_t findIdleRectangularAreas(uint8_t idleRectangles[16][4]);
#endif