#include "harp.h"
#include "uip_rpl_process.h"
#include <string.h>
#include "config.h"
#include "coap.h"
#include "coap_common_app.h"
#ifndef LINUX_GATEWAY
#include "led.h"
#endif
HARP_child_t HARP_children[MAX_CHILDREN_NUM];
HARP_self_t HARP_self;

static uint8_t cur_sort_layer;

#ifndef LINUX_GATEWAY
#include "nhl_device_table.h"
extern uint8_t selfShortAddr;
extern NHL_DeviceTable_t NHL_deviceTable[NODE_MAX_NUM];
#endif

int sortAdjNodesByIfaceTsDec(const void *a, const void *b)
{
    uint8_t uint8_a = *((uint8_t *)a);
    uint8_t uint8_b = *((uint8_t *)b);
    uint8_t x, y = 0;
    for (uint8_t i = 0; i < MAX_CHILDREN_NUM; i++)
    {
        if (HARP_children[i].id == uint8_a)
            x = i;
        if (HARP_children[i].id == uint8_b)
            y = i;
    }
    return (HARP_children[y].iface[cur_sort_layer].ts - HARP_children[x].iface[cur_sort_layer].ts);
}

int sortChildrenByIfaceTs(const void *a, const void *b)
{
    return ((HARP_child_t *)b)->iface[cur_sort_layer].ts - ((HARP_child_t *)a)->iface[cur_sort_layer].ts;
}

int sortChildrenByIfaceTsInc(const void *a, const void *b)
{
    return ((HARP_child_t *)a)->iface[cur_sort_layer].ts - ((HARP_child_t *)b)->iface[cur_sort_layer].ts;
}

int sortChildrenByIfaceCh(const void *a, const void *b)
{
    return ((HARP_child_t *)b)->iface[cur_sort_layer].ch - ((HARP_child_t *)a)->iface[cur_sort_layer].ch;
}

#ifndef LINUX_GATEWAY
//HARP_mailbox_msg_t mmsg;
//mmsg.type = HARP_MAILBOX_DUMMY;
//Mailbox_post(harp_mailbox, &mmsg, BIOS_NO_WAIT);
void harp_task(void)
{
    while (1)
    {
        HARP_mailbox_msg_t msg;
        Mailbox_pend(harp_mailbox, &msg, BIOS_WAIT_FOREVER);
        switch (msg.type)
        {
        case HARP_MAILBOX_DUMMY:
            LED_set(2, 1);
            Task_sleep(20 * CLOCK_SECOND);
            LED_set(2, 0);
            break;
        case HARP_MAILBOX_SEND_INIT:
            send_init();
            break;
        case HARP_MAILBOX_SEND_IFACE:
            send_interface();
            break;
        case HARP_MAILBOX_SEND_SP:
            send_subpartition();
            break;
        case HARP_MAILBOX_SEND_SCH:
            distributed_scheduling();
            break;
        }
    }
}
#endif

void harp_init(unsigned char *data)
{
    memset(&HARP_self, 0, sizeof(HARP_self));
    memset(&HARP_children[0], 0, sizeof(HARP_child_t) * MAX_CHILDREN_NUM);

    HARP_self.layer = *(uint8_t *)(data);
    HARP_self.partition_center = *(uint16_t *)(data + 1);

#ifdef LINUX_GATEWAY
    HARP_self.partition_up_end = *(uint16_t *)(data + 3);
    HARP_self.id = 1;
    HARP_self.children_cnt = *(uint8_t *)(data + 5);
    printf("initiating %d children: ", HARP_self.children_cnt);
    for (uint8_t j = 0; j < HARP_self.children_cnt; j++)
    {
        printf("#%d, ", *(uint8_t *)(data + j + 6));
        HARP_children[j].id = *(uint8_t *)(data + j + 6);
    }
    printf("\n");
#else
    HARP_self.id = selfShortAddr;
    HARP_self.parent = (uint8_t)NHL_deviceTable[0].shortAddr;

    for (uint8_t i = 1; i < 5; i++)
    {
        if ((uint8_t)NHL_deviceTable[i].shortAddr != 0)
        {
            HARP_children[HARP_self.children_cnt].id = (uint8_t)NHL_deviceTable[i].shortAddr;
            HARP_self.children_cnt++;
        }
        else
        {
            break;
        }
    }
#endif
}

uint8_t abstractInterface()
{
#ifdef LINUX_GATEWAY
    printf("total traffic: %d\n", HARP_self.traffic_fwd);
#endif

    HARP_self.iface[HARP_self.layer].ts = HARP_self.traffic_fwd;
    if (HARP_self.traffic_fwd > 0)
        HARP_self.iface[HARP_self.layer].ch = 1;
    return HARP_self.traffic_fwd;
}

// using best-fit skyline packing
uint8_t interfaceComposition()
{
#ifdef LINUX_GATEWAY
    printf("Composited interface: ");
#endif
    for (uint8_t layer = HARP_self.layer; layer <= MAX_HOP; layer++)
    {
        uint16_t width = 0;
        uint8_t height = 0;



        uint8_t children_cnt = 0;
        for (uint8_t i = 0; i < HARP_self.children_cnt; i++)
            if (HARP_children[i].iface[layer].ts != 0)
                children_cnt++;
        if (children_cnt == 0)
            continue;
            
        // for qsort
        cur_sort_layer = layer;
        qsort(HARP_children, MAX_CHILDREN_NUM, sizeof(HARP_child_t), sortChildrenByIfaceTs);
        HARP_children[0].sp_rel[layer].ts_start = 0;
        HARP_children[0].sp_rel[layer].ts_end = HARP_children[0].iface[layer].ts;
        HARP_children[0].sp_rel[layer].ch_start = 0;
        HARP_children[0].sp_rel[layer].ch_end = 0 + HARP_children[0].iface[layer].ch;
        width = HARP_children[0].iface[layer].ts;
        HARP_skyline_t *skyline = (HARP_skyline_t *)malloc(sizeof(HARP_skyline_t));
        skyline->start = 0;
        skyline->end = width;
        skyline->width = width;
        skyline->height = HARP_children[0].iface[layer].ch;
        skyline->next = NULL;
        HARP_skyline_t *head = (HARP_skyline_t *)malloc(sizeof(HARP_skyline_t));
        head->next = skyline;
        int cnt = 0;
        while (cnt < children_cnt - 1)
        {
            HARP_skyline_t *tmp = head->next;
            while (skyline != NULL)
            {
                if (skyline->height < tmp->height)
                {
                    tmp = skyline;
                }
                skyline = skyline->next;
            }
            skyline = tmp;

            uint8_t hasFit = 0;
            for (int i = 1; i < children_cnt; i++)
            {
                if (HARP_children[i].sp_rel[layer].ts_start == 0 && HARP_children[i].sp_rel[layer].ts_end == 0 &&
                    skyline->width >= HARP_children[i].iface[layer].ts)
                {
                    cnt++;
                    hasFit = 1;
                    HARP_children[i].sp_rel[layer].ts_start = skyline->start;
                    HARP_children[i].sp_rel[layer].ts_end = skyline->start + HARP_children[i].iface[layer].ts;
                    HARP_children[i].sp_rel[layer].ch_start = skyline->height;
                    HARP_children[i].sp_rel[layer].ch_end = skyline->height + HARP_children[i].iface[layer].ch;

                    // printf("HARP_child_t #%d's relative sub-partition {%d, %d, %d, %d}\n", HARP_children[i].id, HARP_children[i].sp_rel[0].ts_start, HARP_children[i].sp_rel[0].ts_end,
                    //        HARP_children[i].sp_rel[0].ch_start, HARP_children[i].sp_rel[0].ch_end);

                    if (skyline->width > HARP_children[i].iface[layer].ts)
                    {
                        // the remaining part
                        HARP_skyline_t *new_skyline = (HARP_skyline_t *)malloc(sizeof(HARP_skyline_t));
                        new_skyline->start = skyline->start + HARP_children[i].iface[layer].ts;
                        new_skyline->end = skyline->end;
                        new_skyline->width = skyline->width - HARP_children[i].iface[layer].ts;
                        new_skyline->height = skyline->height;
                        new_skyline->prev = skyline;
                        new_skyline->next = skyline->next;

                        // the used part
                        skyline->end = skyline->start + HARP_children[i].iface[layer].ts;
                        skyline->width = HARP_children[i].iface[layer].ts;
                        skyline->height += HARP_children[i].iface[layer].ch;
                        skyline->next = new_skyline;
                    }
                    else
                    {
                        skyline->height += HARP_children[i].iface[layer].ch;
                    }

                    break;
                }
            }

            // wasted area
            if (!hasFit)
            {
                skyline->prev->end = skyline->end;
                skyline->prev->width += skyline->width;
                skyline->prev->next = skyline->next;
                if (skyline->next != NULL)
                    skyline->next->prev = skyline->prev;
                skyline = skyline->prev;
            }

            // merge
            HARP_skyline_t *ss = head->next;
            while (ss != NULL)
            {
                if (ss->width == 0)
                {
                    ss->prev = ss->next;
                    ss = ss->prev;
                }
                if (ss->next != NULL)
                {
                    if (ss->height == ss->next->height)
                    {
                        ss->width += ss->next->width;
                        ss->end = ss->next->end;
                        ss->next = ss->next->next;
                        if (ss->next != NULL)
                            ss->next->prev = ss;
                    }
                }
                ss = ss->next;
            }
        }
        HARP_skyline_t *s = head->next;
        while (s != NULL)
        {
            if (height < s->height)
                height = s->height;
            HARP_skyline_t *next = s->next;
            free(s);
            s = next;
        }

        HARP_self.iface[layer].ts = width;
        HARP_self.iface[layer].ch = height;
        if (height > MAX_CHANNEL)
        {
#ifdef LINUX_GATEWAY
            printf("[!] Exceed channel limit (%d), rotate the strip\n", MAX_CHANNEL);
#endif
            uint8_t width = MAX_CHANNEL;
            uint16_t height = 0;

            // reset
            for (int i = 0; i < MAX_CHILDREN_NUM; i++)
            {
                HARP_children[i].sp_rel[layer].ts_start = 0;
                HARP_children[i].sp_rel[layer].ts_end = 0;
                HARP_children[i].sp_rel[layer].ch_start = 0;
                HARP_children[i].sp_rel[layer].ch_end = 0;
            }
            qsort(HARP_children, MAX_CHILDREN_NUM, sizeof(HARP_child_t), sortChildrenByIfaceCh);

            HARP_skyline_t *skyline = (HARP_skyline_t *)malloc(sizeof(HARP_skyline_t));
            skyline->start = 0;
            skyline->end = width;
            skyline->width = width;
            skyline->height = 0;

            skyline->next = NULL;
            HARP_skyline_t *head = (HARP_skyline_t *)malloc(sizeof(HARP_skyline_t));
            head->next = skyline;
            // skyline->prev = head;

            int cnt = 0;
            while (cnt < children_cnt)
            {
                HARP_skyline_t *tmp = head->next;
                while (skyline != NULL)
                {
                    if (skyline->height < tmp->height)
                    {
                        tmp = skyline;
                    }
                    skyline = skyline->next;
                }
                skyline = tmp;

                uint8_t hasFit = 0;
                for (int i = 0; i < children_cnt; i++)
                {
                    if (HARP_children[i].sp_rel[layer].ts_start == 0 && HARP_children[i].sp_rel[layer].ts_end == 0 &&
                        skyline->width >= HARP_children[i].iface[layer].ch)
                    {
                        cnt++;
                        hasFit = 1;

                        HARP_children[i].sp_rel[layer].ts_start = skyline->height;
                        HARP_children[i].sp_rel[layer].ts_end = skyline->height + HARP_children[i].iface[layer].ts;
                        HARP_children[i].sp_rel[layer].ch_start = skyline->start;
                        HARP_children[i].sp_rel[layer].ch_end = skyline->start + HARP_children[i].iface[layer].ch;

                        // printf("HARP_child_t #%d's logical sub-partition {%d, %d, %d, %d}\n", HARP_children[i].id, HARP_children[i].sp_rel[0].ts_start, HARP_children[i].sp_rel[0].ts_end,
                        //        HARP_children[i].sp_rel[0].ch_start, HARP_children[i].sp_rel[0].ch_end);

                        if (skyline->width > HARP_children[i].iface[layer].ch)
                        {
                            // the remaining part
                            HARP_skyline_t *new_skyline = (HARP_skyline_t *)malloc(sizeof(HARP_skyline_t));
                            new_skyline->start = skyline->start + HARP_children[i].iface[layer].ch;
                            new_skyline->end = skyline->end;
                            new_skyline->width = skyline->width - HARP_children[i].iface[layer].ch;
                            new_skyline->height = skyline->height;
                            new_skyline->prev = skyline;
                            new_skyline->next = skyline->next;

                            // the used part
                            skyline->end = skyline->start + HARP_children[i].iface[layer].ch;
                            skyline->width = HARP_children[i].iface[layer].ch;
                            skyline->height += HARP_children[i].iface[layer].ts;
                            skyline->next = new_skyline;
                        }
                        else
                        {
                            skyline->height += HARP_children[i].iface[layer].ts;
                        }
                        break;
                    }
                }

                // wasted area
                if (!hasFit)
                {
                    skyline->prev->end = skyline->end;
                    skyline->prev->width += skyline->width;
                    skyline->prev->next = skyline->next;
                    if (skyline->next != NULL)
                        skyline->next->prev = skyline->prev;
                    skyline = skyline->prev;
                }

                // merge
                HARP_skyline_t *ss = head->next;
                while (ss != NULL)
                {
                    if (ss->width == 0)
                    {
                        ss->prev = ss->next;
                        ss = ss->prev;
                    }
                    if (ss->next != NULL)
                    {
                        if (ss->height == ss->next->height)
                        {
                            ss->width += ss->next->width;
                            ss->end = ss->next->end;
                            ss->next = ss->next->next;
                            if (ss->next != NULL)
                                ss->next->prev = ss;
                        }
                    }
                    ss = ss->next;
                }
            }
            HARP_skyline_t *s = head->next;
            while (s != NULL)
            {
                if (height < s->height)
                    height = s->height;
                HARP_skyline_t *next = s->next;
                free(s);
                s = next;
            }
            HARP_self.iface[layer].ts = height;
            HARP_self.iface[layer].ch = width;
        }

#ifdef LINUX_GATEWAY
        printf("l%d-{%d, %d}, ", layer, HARP_self.iface[layer].ts, HARP_self.iface[layer].ch);
#endif
    }
#ifdef LINUX_GATEWAY
    printf("\n");
#endif
    return 0;
}

uint8_t subpartitionAllocation()
{
#ifdef LINUX_GATEWAY
    uint16_t slot_idx = HARP_self.partition_up_end;
    uint8_t redundent_slot = 0;
    printf("root abs sp: ");
    for (uint8_t layer = 0; layer < MAX_HOP; layer++)
    {
        if (HARP_self.iface[layer].ts == 0)
            break;
        HARP_self.sp_abs[layer].ts_start = slot_idx - HARP_self.iface[layer].ts - redundent_slot;
        HARP_self.sp_abs[layer].ts_end = slot_idx;
        HARP_self.sp_abs[layer].ch_start = 1;
        HARP_self.sp_abs[layer].ch_end = 16;
        slot_idx -= HARP_self.iface[layer].ts + redundent_slot;
        printf("l%d-{%d, %d}, ", layer, HARP_self.sp_abs[layer].ts_start, HARP_self.sp_abs[layer].ts_end);
    }
    printf("\n");
#endif

    for (uint8_t i = 0; i < HARP_self.children_cnt; i++)
    {
        if (HARP_children[i].iface[HARP_self.layer + 1].ts == 0)
            continue;
#ifdef LINUX_GATEWAY
        printf("child #%d abs sp: ", HARP_children[i].id);
#endif
        for (uint8_t layer = HARP_self.layer + 1; layer <= MAX_HOP; layer++)
        {
            if (HARP_children[i].iface[layer].ts == 0)
                break;
            HARP_children[i].sp_abs[layer].ts_start = HARP_self.sp_abs[layer].ts_start + HARP_children[i].sp_rel[layer].ts_start;
            HARP_children[i].sp_abs[layer].ts_end = HARP_self.sp_abs[layer].ts_start + HARP_children[i].sp_rel[layer].ts_end;
            HARP_children[i].sp_abs[layer].ch_start = HARP_self.sp_abs[layer].ch_start + HARP_children[i].sp_rel[layer].ch_start;
            HARP_children[i].sp_abs[layer].ch_end = HARP_self.sp_abs[layer].ch_start + HARP_children[i].sp_rel[layer].ch_end;
#ifdef LINUX_GATEWAY
            printf("l%d-{%d, %d, %d, %d}, ", layer,
                   HARP_children[i].sp_abs[layer].ts_start, HARP_children[i].sp_abs[layer].ts_end,
                   HARP_children[i].sp_abs[layer].ch_start, HARP_children[i].sp_abs[layer].ch_end);
#endif
        }
#ifdef LINUX_GATEWAY
        printf("\n");
#endif
    }
    return 0;
}

uint8_t subpartitionAdjustment()
{
    cur_sort_layer = HARP_self.adjustingLayer;
    qsort(HARP_self.adjustingNodes, MAX_CHILDREN_NUM, sizeof(uint8_t), sortAdjNodesByIfaceTsDec);

    HARP_interface_t adjIface;
    for (uint8_t i = 0; i < MAX_CHILDREN_NUM; i++)
    {
        if (HARP_children[i].id == HARP_self.adjustingNodes[0])
        {
            adjIface.ts = HARP_children[i].iface[HARP_self.adjustingLayer].ts;
            adjIface.ch = HARP_children[i].iface[HARP_self.adjustingLayer].ch;
            break;
        }
    }

#ifdef LINUX_GATEWAY
    printf("*****adjusting nodes: [");
    for (uint8_t i = 0; i < MAX_CHILDREN_NUM; i++)
    {
        printf("%d,", HARP_self.adjustingNodes[i]);
    }
    printf("]\n");
    printf("to be adjusted iface: I_(%d,%d)=(%d, %d)\n",
           HARP_self.adjustingNodes[0], HARP_self.adjustingLayer, adjIface.ts, adjIface.ch);
#endif

    uint8_t idleRectangles[16][4];
    uint8_t rectCnt = findIdleRectangularAreas(idleRectangles);
    // printf("idle rectangles: ");
    // for (uint8_t i = 0; i < rectCnt; i++)
    // {
    //     printf("[%d,%d,%d,%d],", idleRectangles[i][0], idleRectangles[i][1],
    //            idleRectangles[i][2], idleRectangles[i][3]);
    // }
    // printf("\n");

    uint8_t found = 0;
    for (uint8_t i = 0; i < rectCnt; i++)
    {
        if ((idleRectangles[i][1] - idleRectangles[i][0]) >= adjIface.ts &&
            (idleRectangles[i][3] - idleRectangles[i][2]) >= adjIface.ch)
        {
            found = 1;
#ifdef LINUX_GATEWAY
            printf("fit place found, place #%d's iface@l%d (%d, %d) to [%d,%d,%d,%d]\n",
                   HARP_self.adjustingNodes[0], HARP_self.adjustingLayer, adjIface.ts, adjIface.ch,
                   idleRectangles[i][0], idleRectangles[i][1], idleRectangles[i][2], idleRectangles[i][3]);
#endif
            // update rel_sp
            for (uint8_t c = 0; c < MAX_CHILDREN_NUM; c++)
            {
                if (HARP_self.adjustingNodes[0] == HARP_children[c].id)
                {
                    HARP_children[c].sp_rel[HARP_self.adjustingLayer].ts_start = idleRectangles[i][0];
                    HARP_children[c].sp_rel[HARP_self.adjustingLayer].ts_end = idleRectangles[i][0] + adjIface.ts;
                    HARP_children[c].sp_rel[HARP_self.adjustingLayer].ch_start = idleRectangles[i][2];
                    HARP_children[c].sp_rel[HARP_self.adjustingLayer].ch_end = idleRectangles[i][2] + adjIface.ch;
                    break;
                }
            }
            for (uint8_t j = 0; j < MAX_CHILDREN_NUM - 1; j++)
            {
                HARP_self.adjustingNodes[j] = HARP_self.adjustingNodes[j + 1];
            }
            HARP_self.adjustingNodesCnt--;
            return 1;
        }
    }

#ifdef LINUX_GATEWAY
    printf("not found, try to relocate another sp first\n");
#endif

    if (!found)
    {
        cur_sort_layer = HARP_self.adjustingLayer;
        qsort(HARP_children, MAX_CHILDREN_NUM, sizeof(HARP_child_t), sortChildrenByIfaceTsInc);

        for (uint8_t c = 0; c < MAX_CHILDREN_NUM; c++)
        {
            uint8_t skip = 0;
            if (HARP_children[c].iface[HARP_self.adjustingLayer].ts == 0)
            {
                skip = 1;
            }
            else
            {
                for (uint8_t j = 0; j < HARP_self.adjustingNodesCnt; j++)
                {
                    if (HARP_children[c].id == HARP_self.adjustingNodes[j])
                    {
                        skip = 1;
                        break;
                    }
                }
            }
            if (skip)
                continue;
#ifdef LINUX_GATEWAY
            printf("trying to move %d\n", HARP_children[c].id);
#endif
            HARP_self.adjustingNodes[HARP_self.adjustingNodesCnt] = HARP_children[c].id;
            HARP_self.adjustingNodesCnt++;

            uint8_t rectCnt = findIdleRectangularAreas(idleRectangles);

            for (uint8_t i = 0; i < rectCnt; i++)
            {
                if ((idleRectangles[i][1] - idleRectangles[i][0]) >= adjIface.ts &&
                    (idleRectangles[i][3] - idleRectangles[i][2]) >= adjIface.ch)
                {
#ifdef LINUX_GATEWAY
                    printf("fit place found by moving #%d away, place #%d's iface@l%d (%d, %d) to [%d,%d,%d,%d]\n", HARP_children[c].id,
                           HARP_self.adjustingNodes[0], HARP_self.adjustingLayer, adjIface.ts, adjIface.ch,
                           idleRectangles[i][0], idleRectangles[i][1], idleRectangles[i][2], idleRectangles[i][3]);
#endif
                    // update rel_sp
                    for (uint8_t cc = 0; cc < MAX_CHILDREN_NUM; cc++)
                    {
                        if (HARP_self.adjustingNodes[0] != 0 && HARP_self.adjustingNodes[0] == HARP_children[cc].id)
                        {
                            HARP_children[cc].sp_rel[HARP_self.adjustingLayer].ts_start = idleRectangles[i][0];
                            HARP_children[cc].sp_rel[HARP_self.adjustingLayer].ts_end = idleRectangles[i][0] + adjIface.ts;
                            HARP_children[cc].sp_rel[HARP_self.adjustingLayer].ch_start = idleRectangles[i][2];
                            HARP_children[cc].sp_rel[HARP_self.adjustingLayer].ch_end = idleRectangles[i][2] + adjIface.ch;
                            break;
                        }
                    }
                    for (uint8_t j = 0; j < MAX_CHILDREN_NUM - 1; j++)
                    {
                        HARP_self.adjustingNodes[j] = HARP_self.adjustingNodes[j + 1];
                    }
                    HARP_self.adjustingNodesCnt--;
                    return 1;
                }
            }
        }
    }
    return 1;
}

uint8_t findIdleRectangularAreas(uint8_t idleRectangles[16][4])
{
    uint32_t rectBitmap[MAX_CHANNEL];
    uint8_t rectCnt = 0;
    uint8_t layer = HARP_self.adjustingLayer;

    for (uint8_t i = 0; i < MAX_CHILDREN_NUM; i++)
    {
        uint8_t skip = 0;
        if (HARP_children[i].iface[layer].ts == 0)
        {
            skip = 1;
        }
        else
        {
            for (uint8_t j = 0; j < HARP_self.adjustingNodesCnt; j++)
            {
                if (HARP_children[i].id == HARP_self.adjustingNodes[j])
                {
                    skip = 1;
                    break;
                }
            }
        }
        if (skip)
            continue;

        for (uint8_t y = HARP_children[i].sp_rel[layer].ch_start; y < HARP_children[i].sp_rel[layer].ch_end; y++)
        {
            for (uint8_t x = HARP_children[i].sp_rel[layer].ts_start; x < HARP_children[i].sp_rel[layer].ts_end; x++)
            {
                rectBitmap[y] |= 0x80000000 >> x;
            }
        }
    }

    // for (int8_t i = HARP_self.iface[layer].ch - 1; i >= 0; i--)
    // {
    //     print_binary(rectBitmap[i]);
    //     printf("\n");
    // }

    for (uint8_t yCur = 0; yCur < HARP_self.iface[layer].ch; yCur++)
    {
        for (uint8_t xCur = 0; xCur < HARP_self.iface[layer].ts; xCur++)
        {
            if ((rectBitmap[yCur] << xCur & 0x80000000) == 0)
            {
                uint8_t xStart = xCur;
                uint8_t xEnd = xCur;
                uint8_t yStart = yCur;
                uint8_t yEnd = yCur;
                for (uint8_t yy = yCur; yy < HARP_self.iface[layer].ch; yy++)
                {
                    if ((rectBitmap[yy] << xCur & 0x80000000) != 0)
                    {
                        yEnd = yy;
                        break;
                    }
                    if (yy == HARP_self.iface[layer].ch - 1)
                    {
                        yEnd = yy + 1;
                    }
                }
                for (uint8_t xx = xCur; xx < HARP_self.iface[layer].ts; xx++)
                {
                    uint8_t allZero = 1;
                    for (uint8_t yyy = yCur; yyy < yEnd; yyy++)
                    {
                        if ((rectBitmap[yyy] << xx & 0x80000000) != 0)
                        {
                            allZero = 0;
                        }
                    }
                    if (allZero == 1)
                        xEnd++;
                }

                uint8_t duplicated = 0;
                for (uint8_t i = 0; i < rectCnt; i++)
                {
                    if (xStart >= idleRectangles[i][0] && xEnd <= idleRectangles[i][1] &&
                        yStart >= idleRectangles[i][2] && yEnd <= idleRectangles[i][3])
                    {
                        duplicated = 1;
                        break;
                    }
                }
                if (!duplicated)
                {
                    // printf("rectCnt: %d\n",rectCnt);
                    idleRectangles[rectCnt][0] = xStart;
                    idleRectangles[rectCnt][1] = xEnd;
                    idleRectangles[rectCnt][2] = yStart;
                    idleRectangles[rectCnt][3] = yEnd;
                    rectCnt++;
                }
            }
        }
    }
    return rectCnt;
}

// int main()
// {
//     initChildren();
//     interfaceComposition();
//     // subpartitionAllocation();
//     return 0;
// }