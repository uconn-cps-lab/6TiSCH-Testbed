/*******************************************************************************
 *  Copyright (c) 2017 Texas Instruments Inc.
 *  All Rights Reserved This program is the confidential and proprietary
 *  product of Texas Instruments Inc.  Any Unauthorized use, reproduction or
 *  transfer of this program is strictly prohibited.
 *******************************************************************************
  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
 *******************************************************************************
 * FILE PURPOSE:
 *
 *******************************************************************************
 * DESCRIPTION:
 *
 *******************************************************************************
 * HISTORY:
 *
 ******************************************************************************/
#include <sys/socket.h> /*  socket definitions        */
#include <sys/types.h>  /*  socket types              */
#include <arpa/inet.h>  /*  inet (3) funtions         */
#include <stdlib.h>
#include <assert.h>
#include <unistd.h> /*  misc. UNIX functions      */
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <sched.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>

#include "comm.h"
#include "nm.h"

#include "net/uip.h"
#include "rpl/rpl-private.h"

int nm_sock=0;
pthread_t nm_pthread;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void NM_send(char*json){
    while(nm_sock==0){
        printf("Network Manager yet connected!\n");
        sleep(1);
    }
    char json_socket[1000];
    size_t json_size=strlen(json);
    json_size=sprintf(json_socket,"%d#%s",json_size,json);
    send(nm_sock, json_socket, json_size, MSG_NOSIGNAL);
}

static void NM_recv(char*json){
    while(nm_sock==0){
        printf("Network Manager yet connected!\n");
        sleep(1);
    }
    char c;
    size_t json_size=0;
    while(recv(nm_sock, &c, 1, MSG_WAITALL),c!='#'){
        json_size=json_size*10+c-'0';
    }
    recv(nm_sock, json, json_size, MSG_WAITALL);
    json[json_size]=0;
}
/*****************************************************************************
 *  FUNCTION: NM_requestGatewayAddress
 *
 *  DESCRIPTION: Request a short address for the gateway
 *
 *  PARAMETERS:
 *
 *  RETURNS: the new gateway address
 *
 *****************************************************************************/
uint16_t NM_requestGatewayAddress()
{
   extern int isdigit (int c);
   char json[1000];
   char*p;
   uint16_t retVal = 1;

   sprintf(json, "{\"type\":\"new_gw_addr\"}");
   pthread_mutex_lock(&mutex);
   NM_send(json);
   NM_recv(json);
   pthread_mutex_unlock(&mutex);

   p=json;
   if (isdigit(*p))
   {
      long val = strtol(p, &p, 10); // Read a number, ...
      retVal = val;
   }

   printf("-o-o-o-New gateway address: %d\n", retVal);
   return(retVal);
}
/*****************************************************************************
 *  FUNCTION: NM_requestNodeAddress
 *
 *  DESCRIPTION: Request a short address for a new node
 *
 *  PARAMETERS: in_out: req
 *
 *  RETURNS:
 *
 *****************************************************************************/
void NM_requestNodeAddress(NM_schedule_request_t* req)
{
   extern int isdigit (int c);
   char json[1000];
   char*p;

   sprintf(json, "{\"type\":\"new_node_addr\",\"eui64\":\"%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\",\"shortAddr\":%d,\"coordAddr\":%d}",
        req->eui64[0], req->eui64[1], req->eui64[2], req->eui64[3],
        req->eui64[4], req->eui64[5], req->eui64[6], req->eui64[7],
        req->shortAddr,req->coordAddr);
   pthread_mutex_lock(&mutex);
   NM_send(json);
   NM_recv(json);
   pthread_mutex_unlock(&mutex);

   p=json;
   if (isdigit(*p))
   {
      long val = strtol(p, &p, 10); // Read a number, ...
      req->shortAddr = val;
   }
}

/*****************************************************************************
 *  FUNCTION: NM_schedule
 *
 *  DESCRIPTION: Associate request, initiated by the new node
 *
 *  PARAMETERS: in: req
 *              in_out: res
 *
 *  RETURNS:
 *
 *****************************************************************************/
void NM_schedule(NM_schedule_request_t* req, NM_schedule_response_t* res)
{
    char json[1000];
    sprintf(json, "{\"type\":\"new_node\",\"eui64\":\"%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\",\"shortAddr\":%d,\"coordAddr\":%d}",
        req->eui64[0], req->eui64[1], req->eui64[2], req->eui64[3],
        req->eui64[4], req->eui64[5], req->eui64[6], req->eui64[7],
        req->shortAddr,req->coordAddr);
    pthread_mutex_lock(&mutex);
    NM_send(json);
    NM_recv(json);
    pthread_mutex_unlock(&mutex);

    int number[100];
    int nnumber=0;
    char*p=json;
    while (*p) { // While there are more characters to process...
   	  extern int isdigit (int c);
        if (isdigit(*p)) { // Upon finding a digit, ...
            long val = strtol(p, &p, 10); // Read a number, ...
            number[nnumber++]=val;
            if(nnumber>=100){
                fprintf(stderr,"Schedule Array Overflow (>=100)\n");
                fflush(stderr);
                exit(1);
            }
        } else { // Otherwise, move on to the next character.
            p++;
        }
    }
    int num_cells=(nnumber-3)/5;
    res->status=number[0];
    res->suggestion=number[1];
    
    if(num_cells!=number[2]){//sanity check
        fprintf(stderr,"Schedule response from webapp.js incorrect\n");
        fprintf(stderr,"%s\n",json);
        fflush(stderr);
        exit(1);
    }else if(num_cells>res->num_cells){
        fprintf(stderr,"Schedule Array Overflow (>=res->num_cells)\n");
        fflush(stderr);
        exit(1);
    }else{
        res->num_cells=num_cells;
        int i;
        for(i=0;i<num_cells;++i){
            res->cell_list[i].slotOffset=number[i*5+3];
            res->cell_list[i].channelOffset=number[i*5+4];
            res->cell_list[i].period=number[i*5+5];
            res->cell_list[i].offset=number[i*5+6];
            res->cell_list[i].linkOption=number[i*5+7];
        }
    }
}

/*****************************************************************************
 *  FUNCTION: NM_report_dio
 *
 *  DESCRIPTION: Report dio message to NM
 *
 *  PARAMETERS:
 *
 *  RETURNS:
 *
 *****************************************************************************/
void NM_report_dio()
{
    char json[1000];
    sprintf(json, "{\"type\":\"dio_report\"}");
    pthread_mutex_lock(&mutex);
    NM_send(json);
    pthread_mutex_unlock(&mutex);
}
/*****************************************************************************
 *  FUNCTION: NM_report_asn()
 *
 *  DESCRIPTION: Report last known ASN to NM, 32bit truncated
 *
 *  PARAMETERS:
 *
 *  RETURNS:
 *
 *****************************************************************************/
void NM_report_asn(uint64_t asn)
{
    char json[1000];
    sprintf(json, "{\"type\":\"asn_report\",\"asn\":%u}", (uint32_t)asn);
    pthread_mutex_lock(&mutex);
    NM_send(json);
    pthread_mutex_unlock(&mutex);
}

/*****************************************************************************
 *  FUNCTION: NM_report_dao
 *
 *  DESCRIPTION: Report dao message to NM
 *
 *  PARAMETERS:
 *
 *  RETURNS:
 *
 *****************************************************************************/
static void fillipv6string(char*strbuf,uip_ipaddr_t*addr){
    sprintf(strbuf, "\"%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\"", addr->u8[0], addr->u8[1], addr->u8[2], addr->u8[3],
       addr->u8[4], addr->u8[5], addr->u8[6], addr->u8[7],
       addr->u8[8], addr->u8[9], addr->u8[10], addr->u8[11],
       addr->u8[12], addr->u8[13], addr->u8[14], addr->u8[15]);
}
void NM_report_dao(uip_ipaddr_t* transit, uip_ipaddr_t* target_addr, uint8_t* target_flag, uint16_t lifetime)
{
    char json[1000];
    char temp[1000];
    char sender_string[1000];
    char parent_string[1000];
    char candidate_substring[1000];
    char candidate_string[1000]={};
    int i;
    memset(parent_string, 0, sizeof(parent_string));
    memset(candidate_substring, 0, sizeof(candidate_substring));
    memset(candidate_string, 0, sizeof(candidate_string));
    fillipv6string(parent_string,transit);
    for (i = 0; i < DAO_MAX_TARGET; ++i) {
        if(target_flag[i]==RPL_RESERVED_SENDER){
            fillipv6string(sender_string,&target_addr[i]);
        }else if(target_flag[i]==RPL_RESERVED_CANDIDATE){
            fillipv6string(candidate_substring,&target_addr[i]);
            strcat(candidate_string, ",");
            strcat(candidate_string, candidate_substring);
        }
    }
    int len=0;
    len+=sprintf(json, "{\"type\":\"dao_report\", \
\"sender\":%s, \
\"parent\":%s, \
\"candidate\":[%s], \
\"lifetime\":%d}",
        sender_string,
        parent_string,
        candidate_string+1,
        lifetime);
//    puts(json);
    struct timeval  tv;
    gettimeofday(&tv, NULL);
//    printf("Timestamp: %.3f\n", (tv.tv_sec) + (tv.tv_usec) / 1000000.0);
    pthread_mutex_lock(&mutex);
    NM_send(json);
    pthread_mutex_unlock(&mutex);
/*    printf("DAO reported: target: %02x%02x", target[0], target[1]);
    int i;
    for(i=2; i<16; i+=2){
        printf(":%02x%02x", target[i], target[i+1]);
    }
    printf(", parent: %02x%02x", parent[0], parent[1]);
    for(i=2; i<16; i+=2){
        printf(":%02x%02x", parent[i], parent[i+1]);
    }
    printf(", lifetime: %d\n", lifetime);*/
}

/*****************************************************************************
 *  FUNCTION: NM_set_minimal_config
 *
 *  DESCRIPTION: Get minimal configuration from NM
 *
 *  PARAMETERS:
 *
 *  RETURNS:
 *
 *****************************************************************************/
void NM_set_minimal_config(COMM_setSysCfg_s*config)
{
    char json[1000];
    sprintf(json, "{\"type\":\"set_minimal_config\",\"slot_frame_size\":%d,\"number_shared_slot\":%d}",
        config->slot_frame_size,
        config->number_shared_slot);
    pthread_mutex_lock(&mutex);
    NM_send(json);
    pthread_mutex_unlock(&mutex);
}


/*****************************************************************************
 *  FUNCTION: NM_server_thread_entry_fn
 *
 *  DESCRIPTION: Handle incoming connection
 *
 *  PARAMETERS:
 *
 *  RETURNS:
 *
 *****************************************************************************/
void *NM_server_thread_entry_fn(void * data_p)
{
    int sockfd;
    struct sockaddr_in serv_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error allocate socket");
        exit(1);
    }
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(NM_PORT);
    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error bind socket");
        exit(1);
    }
    listen(sockfd, 5);
    while (1) {
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(sockfd, &rfds);
        int ready = select(sockfd+1, &rfds, NULL, NULL, &tv);
        if(ready>0){
            nm_sock = accept(sockfd, NULL, NULL);
            if(nm_sock < 0){
                perror("Error on accept");
            }else{
                printf("Network Manager connected\n");
            }
        }
    }
    shutdown(sockfd,SHUT_WR);
    close(sockfd);
    return(NULL);
}

/*****************************************************************************
 *  FUNCTION: NM_init
 *
 *  DESCRIPTION: Initialize the NM server (create the server socket thread)
 *
 *  PARAMETERS: 
 *
 *  RETURNS:
 *
 *****************************************************************************/
int NM_init(void)
{
    pthread_create(&nm_pthread, NULL, NM_server_thread_entry_fn, NULL);
    return 1;
}
