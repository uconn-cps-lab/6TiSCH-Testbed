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
 *  ====================== tsch_api.h =============================================
 *  
 */

#ifndef __TSCH_API_H__
#define __TSCH_API_H__

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "mac_config.h"
#include "ulpsmacbuf.h"
#include "bm_api.h"
#include "rtimer.h"
/* ------------------------------------------------------------------------------------------------
 *                                           Constants
 * ------------------------------------------------------------------------------------------------
 */

/* MAC Callback Events */
#define MAC_MCPS_DATA_CNF           0x01    /* Data confirm */
#define MAC_MCPS_DATA_IND           0x02    /* Data indication */
#define MAC_MLME_ASSOCIATE_IND      0x03    /* Associate indication */
#define MAC_MLME_ASSOCIATE_CNF      0x04    /* Associate confirm */
#define MAC_MLME_DISASSOCIATE_IND   0x05    /* Disassociate indication */
#define MAC_MLME_DISASSOCIATE_CNF   0x06    /* Disassociate confirm */
#define MAC_MLME_BEACON_NOTIFY_IND  0x07    /* Beacon notify indication */
#define MAC_MLME_SCAN_CNF           0x08    /* Scan confirm */
#define MAC_MLME_COMM_STATUS_IND    0x09    /* Communication status indication */
#define MAC_MLME_BCN_CNF            0x0A
#define MAC_MLME_ASSOCIATE_IND_L2MH    0x0B   /* L2MH Associate indication */
#define MAC_MLME_ASSOCIATE_CNF_L2MH    0x0C   /* L2MH Associate confirm */
#define MAC_MLME_DISASSOCIATE_IND_L2MH    0x0D   /* L2MH Disassociate indication */
#define MAC_MLME_DISASSOCIATE_CNF_L2MH    0x0E   /* L2MH Disassociate confirm */
#define MAC_MLME_BEACONINFO_INDICATION    0x0F
#define MAC_MLME_BEACONINFO_CONFIRM       0x10
#define MAC_MLME_RESTART         0x11
#define MAC_MLME_BEACON_REQUEST    0x12
#define MAC_MLME_JOIN_IND          0x13
#define MAC_MLME_SHARE_SLOT_MODIFY    0x14

#define MAC_REG_DATA_EVENTS    0x01
#define MAC_REG_MLME_EVENTS    0x02

#define MAC_CB_ERR         0x00
#define MAC_CB_POST        0x01
#define MAC_CB_FREE_BUF    0x02

// Attach indication type
#define MAC_ATTACH_INDICATION_FROM_DIRECT_CONN_NODE              (0)
#define MAC_ATTACH_INDICATION_FROM_INTERMEDIATE_NODE             (1)
#define MAC_ATTACH_ASSO_REQUEST_FROM_LEAF_NODE                   (2)
#define MAC_ATTACH_ASSO_REQUEST_FROM_INTERMEDIATE_NODE           (3)

#define SCAN_CHANNELS_LEN 3
   
/* Status */
#define MAC_SUCCESS                 0x00  /* Operation successful */
#define MAC_CONDITIONAL_PASS        0x01  /* Pass the security level check conditionally as defined in 802.15.4-2011*/
#define MAC_UNAVAILABLE_DEVICE      0x02  /* Device look up procedure failed as defined in 802.15.4-2011*/
#define MAC_UNAVAILABLE_SECURITY_LEVEL 0x03 /* Security level look up procedure failed as defined in 802.15.4-2011*/
#define MAC_DEV_TABLE_FULL          0x04   /*No space in the MAC security device table to hold the new device*/
#define MAC_AUTOACK_PENDING_ALL_ON  0xFE  /* The AUTOPEND pending all is turned on */
#define MAC_AUTOACK_PENDING_ALL_OFF 0xFF  /* The AUTOPEND pending all is turned off */
#define MAC_BEACON_LOSS             0xE0  /* The beacon was lost following a synchronization request */
#define MAC_CHANNEL_ACCESS_FAILURE  0xE1  /* The operation or data request failed because of
                                             activity on the channel */
#define MAC_COUNTER_ERROR           0xDB  /* The frame counter puportedly applied by the originator of
                                             the received frame is invalid */
#define MAC_DENIED                  0xE2  /* The MAC was not able to enter low power mode. */
#define MAC_DISABLE_TRX_FAILURE     0xE3  /* Unused */
#define MAC_FRAME_TOO_LONG          0xE5  /* The received frame or frame resulting from an operation
                                             or data request is too long to be processed by the MAC */
#define MAC_IMPROPER_KEY_TYPE       0xDC  /* The key purportedly applied by the originator of the
                                             received frame is not allowed */
#define MAC_IMPROPER_SECURITY_LEVEL 0xDD  /* The security level purportedly applied by the originator of
                                             the received frame does not meet the minimum security level */
#define MAC_INVALID_ADDRESS         0xF5  /* The data request failed because neither the source address nor
                                             destination address parameters were present */
#define MAC_INVALID_GTS             0xE6  /* Unused */
#define MAC_INVALID_HANDLE          0xE7  /* The purge request contained an invalid handle */
#define MAC_INVALID_INDEX           0xF9  /* Unused */
#define MAC_INVALID_PARAMETER       0xE8  /* The API function parameter is out of range */
#define MAC_LIMIT_REACHED           0xFA  /* The scan terminated because the PAN descriptor storage limit
                                             was reached */
#define MAC_NO_ACK                  0xE9  /* The operation or data request failed because no
                                             acknowledgement was received */
#define MAC_NO_BEACON               0xEA  /* The scan request failed because no beacons were received or the
                                             orphan scan failed because no coordinator realignment was received */
#define MAC_NO_DATA                 0xEB  /* The associate request failed because no associate response was received
                                             or the poll request did not return any data */
#define MAC_NO_SHORT_ADDRESS        0xEC  /* The short address parameter of the start request was invalid */
#define MAC_ON_TIME_TOO_LONG        0xF6  /* Unused */
#define MAC_OUT_OF_CAP              0xED  /* Unused */
#define MAC_PAN_ID_CONFLICT         0xEE  /* A PAN identifier conflict has been detected and
                                             communicated to the PAN coordinator */
#define MAC_PAST_TIME               0xF7  /* Unused */
#define MAC_READ_ONLY               0xFB  /* A set request was issued with a read-only identifier */
#define MAC_REALIGNMENT             0xEF  /* A coordinator realignment command has been received */
#define MAC_SCAN_IN_PROGRESS        0xFC  /* The scan request failed because a scan is already in progress */
#define MAC_SECURITY_ERROR          0xE4  /* Cryptographic processing of the received secure frame failed */
#define MAC_SUPERFRAME_OVERLAP      0xFD  /* The beacon start time overlapped the coordinator transmission time */
#define MAC_TRACKING_OFF            0xF8  /* The start request failed because the device is not tracking
                                             the beacon of its coordinator */
#define MAC_TRANSACTION_EXPIRED     0xF0  /* The associate response, disassociate request, or indirect
                                             data transmission failed because the peer device did not respond
                                             before the transaction expired or was purged */
#define MAC_TRANSACTION_OVERFLOW    0xF1  /* The request failed because MAC data buffers are full */
#define MAC_TX_ACTIVE               0xF2  /* Unused */
#define MAC_UNAVAILABLE_KEY         0xF3  /* The operation or data request failed because the
                                             security key is not available */
#define MAC_UNSUPPORTED_ATTRIBUTE   0xF4  /* The set or get request failed because the attribute is not supported */
#define MAC_UNSUPPORTED_LEGACY      0xDE  /* The received frame was secured with legacy security which is
                                             not supported */
#define MAC_UNSUPPORTED_SECURITY    0xDF  /* The security of the received frame is not supported */
#define MAC_UNSUPPORTED             0x18  /* The operation is not supported in the current configuration */
#define MAC_BAD_STATE               0x19  /* The operation could not be performed in the current state */
#define MAC_NO_RESOURCES            0x1A  /* The operation could not be completed because no
                                             memory resources were available */


/* ------------------------------------------------------------------------------------------------
 *                                           Typedefs
 * ------------------------------------------------------------------------------------------------
 */

/* MAC event header type */
typedef struct
{
    uint8_t   event;              /* MAC event */
    uint8_t   status;             /* MAC status */
} tschEventHdr_t;

/* Common security type */
typedef macSec_t tschSec_t;

/* PAN descriptor type */
typedef struct
{
    ulpsmacaddr_t coordAddr;        /* The address of the coordinator sending the beacon */
    uint16_t      coordPanId;       /* The PAN ID of the network */
    uint8_t       coordAddrMode;    /* The address mode of the coordinator address */
    uint8_t       linkQuality;      /* The link quality of the received beacon */
    uint8_t       rssi;             /* The RSSI of the received beacon */
    uint32_t      timestamp;        /* The time at which the beacon was received, in backoffs */
    uint8_t       logicalChannel;   /* The logical channel of the network */
    bool          securityFailure;  /* Set to TRUE if there was an error in the security processing */
    tschSec_t     sec;              /* The security parameters for the received beacon frame */
    uint8_t       *pEnergyDetect;   /* Pointer to a buffer to store energy detect measurements */
} tschPanDesc_t;

typedef struct
{
    uint8_t panIdSuppressed;     /* frameControlOptions */
    uint8_t iesIncluded;        /* frameControlOptions */
    uint8_t seqNumSuppressed;   /* frameControlOptions */
} tschFrmCtrlOpt_t;

typedef struct
{
    uint8_t iesIncluded; /* frameControlOptions */
} tschCmdFrmCtrlOpt_t;

typedef struct
{
    tschEventHdr_t   hdr;
    uint8_t slotframeHandle;
} tschMlmeSetSlotframeConfirm_t;

typedef struct
{
    tschEventHdr_t   hdr;
    uint16_t linkHandle;
    uint8_t slotframeHandle;
} tschMlmeSetLinkConfirm_t;

/* MLME start request type */
typedef struct
{
    uint16_t        panId;               /* The PAN ID to use.  This parameter is ignored if panCoordinator is FALSE */
    tschSec_t       beaconSec;           /* Security parameters for the beacon frame */
    tschFrmCtrlOpt_t fco;
    BM_BUF_t *	    pIE_List;
    bool            panCoordinator;      /* Set to TRUE to start a network as PAN coordinator */
    uint8_t       logicalChannel;         /* The logical channel to use.  This parameter is ignored if panCoordinator is FALSE */
    uint8_t       channelPage;            /* The channel page to use.  This parameter is ignored if panCoordinator is FALSE */
    bool        batteryLifeExt;         /* If this value is TRUE, the receiver is disabled after MAC_BATT_LIFE_EXT_PERIODS
                                        full backoff periods following the interframe spacing period of the beacon frame */
    bool        coordRealignment;       /* Set to TRUE to transmit a coordinator realignment prior to changing
                                        the superframe configuration */
    tschSec_t    realignSec;            /* Security parameters for the coordinator realignment frame */
} tschMlmeStartReq_t;

typedef struct
{
    uint8_t mode;
} tschMlmeTschModeReq_t;

typedef struct
{
    uint8_t scanChannels[SCAN_CHANNELS_LEN];       /* Channels to scan */
    uint8_t scanType;                    /* The type of scan */
    uint8_t channelPage;                 /* The channel page on which to perform the scan */
    uint16_t scanDuration;              /* Scan time in seconds */
    tschSec_t  sec;                     /* The security parameters for scan */
    uint8_t maxResults;                 /* Max number of beacons to return for passive scan */
    uint8_t linkQualityThreshold;       /* Populate if link quality is above threshold */
    uint16_t panId;                     /* PANID to join. If 0x0000, free to join any PAN ID */
    tschPanDesc_t  *pPanDescriptor;  /* Pointer to a buffer to store PAN descriptors */  
} tschMlmeScanReq_t;



typedef struct
{
    uint8_t operation; /* ADD = 0, DELETE = 2, MODIFY = 3*/
    uint8_t slotframeHandle; /* unique identifier of the slotframe */
    uint16_t size; /* number of timeslots in the new slotframe*/
} tschMlmeSetSlotframeReq_t;

typedef struct
{
    uint8_t operation; /* ADD = 0, DELETE = 2, MODIFY = 3*/
    uint8_t slotframeHandle; /* slotframe handle of the slotframe to which the link is associated */
    uint16_t linkHandle;
    uint16_t timeslot; /* timeslot of link to be added,  see section 7.5.1.5.1 */
    uint16_t channelOffset; /* channel offset of link, see section 7.5.1.5.1 */
    uint8_t linkOptions; /* bitmap*/
    uint8_t linkType; /* NORMAL = FALSE, ADVERTISING = TRUE, indicates if link may be used to send advertisement beacon */
    uint16_t nodeAddr; /* address of neighbor device connected by link*/
    uint8_t period;
    uint8_t periodOffset;
} tschMlmeSetLinkReq_t;

typedef struct
{
    tschEventHdr_t   hdr;
    uint16_t panId;
    uint8_t srcAddrMode;
    uint8_t dstAddrMode;
    ulpsmacaddr_t srcAddr;
    ulpsmacaddr_t dstAddr;
    uint8_t reason;
} tschMlmeCommStatusIndication_t;

typedef struct
{
    tschEventHdr_t   hdr;                /* Event header contains the status of the scan request */
    uint8_t scanType;
    uint8_t channelPage;
    uint32_t unscannedChannels;
    uint8_t resultListSize;            /* The number of PAN descriptors returned in the results list */
    tschPanDesc_t  *pPanDescriptor;    /* The list of PAN descriptors, one for each beacon found */
} tschMlmeScanConfirm_t;

typedef struct
{
    ulpsmacaddr_t       dstAddr;
    tschSec_t           sec;
    uint8_t             dstAddrMode;
    uint8_t             bsnSuppression;
    uint8_t             beaconType;
    tschCmdFrmCtrlOpt_t fco;
    BM_BUF_t            *pIE_List;
} tschMlmeBeaconReq_t;

typedef struct
{
    tschEventHdr_t   hdr;
} tschMlmeBeaconConfirm_t;

typedef struct
{
    tschEventHdr_t   hdr;              /* The event header */
    tschFrmCtrlOpt_t fco;              /* The frame control option */
    tschPanDesc_t    panDescriptor;  /* The PAN descriptor for the received beacon */
    uint8_t  *pAddrList;             /* The list of device addresses for which the sender of the beacon has data */
    uint8_t   bsn;                    /* The beacon sequence number */
    uint8_t   pendAddrSpec;           /* The beacon pending address specification */
    uint8_t   beaconType;             /* beacon (0x00) or enhanced beacon (0x01)*/
} tschMlmeBeaconNotifyIndication_t;

typedef struct
{
    tschEventHdr_t hdr;
    ulpsmacaddr_t deviceAddress;
    tschSec_t sec;
    uint8_t capabilityInformation;
    tschCmdFrmCtrlOpt_t fco;
} tschMlmeAssociateIndication_t; /* after receive associate request */

typedef struct
{
    tschSec_t       sec;                /* security parameters */
    ulpsmacaddr_t   coordAddress;
    uint16_t        coordPanId;         /* PAN ID of PAN coord */
    uint8_t         coordAddrMode;
    uint8_t         capabilityInformation; /*device capabilities */
    tschCmdFrmCtrlOpt_t fco;
    BM_BUF_t         *pIE_List;
} tschMlmeAssociateReq_t; /* For sending associate request */

typedef struct
{
    ulpsmacaddr_t       destExtAddr;
    uint16_t            destShortAddr; //Need to change to short
    tschSec_t           sec;
    uint8_t             status;
    tschCmdFrmCtrlOpt_t fco;
    BM_BUF_t            *pIE_List;
} tschMlmeAssociateResp_t; /* For sending associate response */

typedef struct
{
    tschEventHdr_t hdr;
    uint16_t assocShortAddress;
    tschSec_t    sec;  /* security parameters */
    tschCmdFrmCtrlOpt_t fco;
} tschMlmeAssociateConfirm_t; /* After receive associate response */

typedef struct
{
    tschSec_t               sec;  /* security parameters */
    ulpsmacaddr_t           deviceAddress;
    uint8_t                 deviceAddrMode;
    uint16_t                devicePanId;
    uint8_t                 disassociateReason;
} tschMlmeDisassociateReq_t; /* For sending disassociate request */

typedef struct
{
    tschEventHdr_t  hdr;
    uint8_t         deviceAddrMode;
    ulpsmacaddr_t   deviceAddress;
    uint8_t         disassociateReason;
    tschSec_t       sec;  
} tschMlmeDisassociateIndication_t;

typedef struct
{
    tschEventHdr_t  hdr;
    uint16_t        devicePanId;
    uint8_t         deviceAddrMode;
    ulpsmacaddr_t   deviceAddress;
} tschMlmeDisassociateConfirm_t;

typedef struct
{
    uint16_t dstAddrShort; 
    uint16_t period;   
} tschMlmeKeepAliveReq_t;

typedef struct
{
    tschFrmCtrlOpt_t fco;
    ulpsmacaddr_t   dstAddr;
    uint16_t        dstPanId;
    uint8_t         srcAddrMode;
    uint8_t         dstAddrMode;
    uint8_t         mdsuHandle;
    uint8_t         txOptions;
    BM_BUF_t *      pIEList;
    uint8_t         power;
} tschDataReq_t;

typedef struct
{
    BM_BUF_t *      pMsdu;      /* buffer pointer to MSDU */
    tschSec_t       sec;        /* Security parameters, not support yet*/
    tschDataReq_t   dataReq;    /* Data request parameters*/
} tschMcpsDataReq_t;

typedef struct
{
    tschEventHdr_t hdr;
    uint8_t msduHandle;
    uint8_t numBackoffs;
    //Additional parameters to comply with NHL APIs
    uint32_t timestamp;
    uint8_t mpduLinkQuality; /* The link quality of the received ACK */
    uint8_t rssi; /* The RSSI of the received ACK */
    uint8_t correlation; /* The raw correlation value of the received ACK*/
} tschMcpsDataCnf_t;


typedef struct
{
    ulpsmacaddr_t       srcAddr;
    ulpsmacaddr_t       dstAddr;
    uint32_t            timestamp;
    BM_BUF_t *          pIEList;
    uint16_t            srcPanId;
    uint16_t            dstPanId;
    tschFrmCtrlOpt_t    fco;
    uint8_t             dsn;
    uint8_t             srcAddrMode;
    uint8_t             dstAddrMode;
    uint8_t             mpduLinkQuality;
    uint8_t             rssi;
} tschDataInd_t;

typedef struct
{
    tschEventHdr_t      hdr;
    BM_BUF_t            *pMsdu;
    tschSec_t       	sec;
    tschDataInd_t       mac;
    uint16_t            recv_asn; // Jiachen
} tschMcpsDataInd_t;

typedef struct
{
    tschEventHdr_t   hdr;
} tschMlmeRestart_t;

typedef struct
{
    tschEventHdr_t   hdr;
} tschMlmeBeaconRequest_t;

typedef struct
{
    tschEventHdr_t   hdr;
} tschMlmeShareSlotModify_t;

typedef struct
{
    tschEventHdr_t   hdr;
    uint16_t childShortAddr;
    uint16_t parentShortAddr;
} tschMlmeAssocRespIe_t;


#if ULPSMAC_L2MH
typedef struct
{
    uint16_t coordPanId; /* PAN ID of PAN coord */
    ulpsmacaddr_t coordAddress;
    uint8_t coordAddrMode;
    uint8_t capabilityInformation; /*device capabilities */
    ulpsmacaddr_t deviceExtAddr;
    uint16_t requestorShrtAddr;
    tschCmdFrmCtrlOpt_t fco;
}tschTiAssociateReqL2mh_t; /* For sending of layer 2 mulihop associate request */

typedef struct
{
    tschEventHdr_t hdr;
    ulpsmacaddr_t deviceAddress;
    uint16_t requestorShrtAddr;
    uint8_t capabilityInformation;
    uint8_t srcAddrMode;
    ulpsmacaddr_t srcAddr;
    tschCmdFrmCtrlOpt_t fco;
}tschTiAssociateIndicationL2mh_t; /* After receive layer 2 multihop associate request */

typedef struct
{
    ulpsmacaddr_t deviceExtAddr;
    uint16_t deviceShortAddr;
    uint16_t requestorShrtAddr;
    uint8_t status;
    uint8_t dstAddrMode;
    ulpsmacaddr_t dstAddr;
    tschCmdFrmCtrlOpt_t fco;
}tschTiAssociateRespL2mh_t; /* For sending layer 2 multihop associate response */

typedef struct
{
    tschEventHdr_t hdr;
    ulpsmacaddr_t deviceExtAddr;
    uint16_t assocShortAddress;
    uint16_t requestorShrtAddr;
    tschCmdFrmCtrlOpt_t fco;
}tschTiAssociateConfirmL2mh_t; /* After receive layer 2 multihop associate response */

typedef struct
{
    uint16_t coordPanId; /* PAN ID of PAN coord */
    ulpsmacaddr_t coordAddress;
    uint8_t coordAddrMode;
    uint8_t reason; /*device capabilities */
    ulpsmacaddr_t deviceExtAddr;
    uint16_t requestorShrtAddr;
    tschCmdFrmCtrlOpt_t fco;
}tschTiDisassociateReqL2mh_t; /* For sending layer 2 multihop disassoc notification */

typedef struct
{
    tschEventHdr_t hdr;
    ulpsmacaddr_t deviceExtAddress;
    uint16_t requestorShrtAddr;
    uint8_t reason;
    uint8_t srcAddrMode;
    ulpsmacaddr_t srcAddr;
    tschCmdFrmCtrlOpt_t fco;
}tschTiDisassociateIndicationL2mh_t; /* After receiver layer 2 multihop disassoc notification */

typedef struct
{
    ulpsmacaddr_t deviceExtAddr;
    uint16_t requestorShrtAddr;
    uint8_t status;
    uint8_t dstAddrMode;
    ulpsmacaddr_t dstAddr;
    tschCmdFrmCtrlOpt_t fco;
}tschTiDisassociateAckL2mh_t; /* For sending layer 2 multihop disassoc ACK */

typedef struct
{
    tschEventHdr_t hdr;
    ulpsmacaddr_t deviceExtAddr;
    uint16_t requestorShrtAddr;
    tschCmdFrmCtrlOpt_t fco;
}tschTiDisassociateConfirmL2mh_t; /* After receive layer 2 multihop disassoc ACK */
#endif


typedef union
{
    tschEventHdr_t            hdr;
} tschCbackEvent_t;


typedef void (*CbackFunc_t) (tschCbackEvent_t *arg, BM_BUF_t* usm_buf);

#ifdef FEATURE_MAC_SECURITY
#include "mac_security_api.h"
#else
#define MAC_KEY_SOURCE_MAX_LEN      8
#define MAC_SEC_LEVEL_NONE          0x00  /* No security is used */
#endif

/* ------------------------------------------------------------------------------------------------
 *                                           Global Variables
 * ------------------------------------------------------------------------------------------------
 */
extern CbackFunc_t MlmeCb;
extern CbackFunc_t McpsCb;

/* ------------------------------------------------------------------------------------------------
 *                                           Functions
 * ------------------------------------------------------------------------------------------------
 */
void TSCH_event_register(uint8_t reg_event, CbackFunc_t pCbFunc);
void TSCH_Init(void);
uint16_t TSCH_MlmeStartReq(tschMlmeStartReq_t* arg);
uint16_t TSCH_MlmeTschModeReq(tschMlmeTschModeReq_t* arg);
void TSCH_MlmeBeaconReq(tschMlmeBeaconReq_t* arg);
void TSCH_MlmeScanReq(tschMlmeScanReq_t* arg);
void TSCH_MlmeAssociateReq(tschMlmeAssociateReq_t* arg);
void TSCH_MlmeAssociateResp(tschMlmeAssociateResp_t* arg);
void TSCH_MlmeDisassociateReq(tschMlmeDisassociateReq_t* arg);
uint16_t TSCH_MlmeSetLinkReq(tschMlmeSetLinkReq_t* arg);
uint16_t TSCH_MlmeSetSlotframeReq(tschMlmeSetSlotframeReq_t* arg);
uint16_t TSCH_MlmeSetReq(uint16_t attrb_id, void* value);
uint16_t TSCH_MlmeGetReq(uint16_t attrb_id, void* value);
void TSCH_McpsDataReq(tschMcpsDataReq_t* arg);
uint16_t TSCH_MlmeKeepAliveReq(tschMlmeKeepAliveReq_t* arg);
void TSCH_TXM_send_ka_packet_handler(void *ptr);  //JIRA51
void TSCH_TXM_adjust_ka_timer_handler(rtimer_clock_t *ptr);  //JIRA51
void NHL_astat_req_handler();  //JIRA52

#if ULPSMAC_L2MH
void TSCH_TiAssociateReqL2mh(tschTiAssociateReqL2mh_t* arg, BM_BUF_t* usm_buf);
void TSCH_TiAssociateRespL2mh(tschTiAssociateRespL2mh_t* arg, BM_BUF_t* usm_buf);
void TSCH_TiDisassociateReqL2mh(tschTiDisassociateReqL2mh_t* arg, BM_BUF_t* usm_buf);
void TSCH_TiDisassociateAckL2mh(tschTiDisassociateAckL2mh_t* arg, BM_BUF_t* usm_buf);
#endif

#endif
