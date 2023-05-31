#ifndef __NV_PARAMS_H__
#define __NV_PARAMS_H__

#ifndef NV_PARAMS_VERSION
#define NV_PARAMS_VERSION   (30)
#endif

#define COAP_DTLS_UDP_PORT    (20220)
#define COAP_MAC_SEC_UDP_PORT (20224)

struct structNvParams
{
   uint8_t mac_psk[16];
   uint16_t panid;
   uint16_t assoc_req_timeout_sec;
   uint16_t slotframe_size;
   uint16_t keepAlive_period;
   uint16_t coap_resource_check_time;
   uint16_t coap_port;
   uint16_t coap_dtls_port;
   uint16_t tx_power;
   uint8_t bcn_chan_0;
   uint8_t bcn_ch_mode;
   uint8_t scan_interval;
   uint8_t num_shared_slot;
   uint8_t fixed_channel_num;
   uint8_t rpl_dio_doublings;
   uint8_t coap_obs_max_non;
   uint8_t coap_default_response_timeout;
   uint8_t debug_level;
   uint8_t phy_mode;
   uint8_t fixed_parent;
   uint16_t sensor_id;

   uint32_t checkSum;   //Must be the last field of the structure
};


#define nvParams_name_mpsk "mpsk"
#define nvParams_name_pnid "pnid"
#define nvParams_name_bch0 "bch0"
#define nvParams_name_assoc_req_timeout_sec "asrqto"
#define nvParams_name_slotframe_size "slfs"
#define nvParams_name_keepAlive_period "kalp"
#define nvParams_name_coap_resource_check_time "coarct"
#define nvParams_name_coap_port "coappt"
#define nvParams_name_coap_dtls_port "codtpt"
#define nvParams_name_tx_power "txpw"
#define nvParams_name_bcn_ch_mode "bchmo"
#define nvParams_name_scan_interval "scani"
#define nvParams_name_num_shared_slot "nssl"
#define nvParams_name_fixed_channel_num "fxcn"
#define nvParams_name_rpl_dio_doublings "rpldd"
#define nvParams_name_coap_obs_max_non "coaomx"
#define nvParams_name_coap_default_response_timeout "codfto"
#define nvParams_name_debug_level "dbgl"
#define nvParams_name_phy_mode "phym"
#define nvParams_name_fixed_parent "fxpa"

extern struct structNvParams nvParams;

int NVM_writeParams(struct structNvParams *nvParams_p);
int NVM_readParams(struct structNvParams *nvParams_p);
void NVM_init();
void NVM_sync();
void NVM_update();

struct structCoapConfig
{
   uint8_t coap_obs_max_non;
   uint8_t coap_default_response_timeout;
   uint16_t coap_resource_check_time;
   uint16_t coap_port;
   uint16_t coap_dtls_port;
   uint16_t coap_mac_sec_port;
};

extern struct structCoapConfig coapConfig;

struct structRplConfig
{
   uint8_t rpl_dio_doublings;
};

extern struct structRplConfig rplConfig;

void coap_config_init();
void rplConfig_init();
#endif //__NV_PARAMS_H__
