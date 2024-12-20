/*
 * Copyright 2018, Broadcom Inc.
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Inc.;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Inc.
 */

#ifndef INCLUDED_MHD_API_H
#define INCLUDED_MHD_API_H

#include <stdint.h>
#include "mhd_constants.h"          /* For mhd_result_t */

#define PM1_POWERSAVE_MODE          ( 1 )
#define PM2_POWERSAVE_MODE          ( 2 )
#define NO_POWERSAVE_MODE           ( 0 )

typedef struct 
{
    uint8_t octet[6]; /* Unique 6-byte MAC address */
} mhd_mac_t;

typedef struct 
{
    uint8_t length;    /* SSID length */
    uint8_t value[32]; /* SSID name (AP name)  */
} mhd_ssid_t;

typedef struct 
{
    char ssid[32];
    char bssid[6];
    uint32_t channel;
    uint32_t security;
    uint32_t rssi;
    char ccode[4];
} mhd_ap_info_t;

typedef struct 
{
    mhd_ssid_t SSID;
    uint32_t security;
    uint8_t channel;
    uint8_t security_key_length;
    char security_key[ 64 ];
    mhd_mac_t BSSID;
} mhd_ap_connect_info_t;

typedef void (*mhd_link_callback_t)(void);

extern void mhd_sdio_controller_index_register( uint32_t index );
extern void mhd_set_country_code( uint32_t country );
extern int mhd_module_init( void );
extern int mhd_module_exit( void );

extern int mhd_start_scan( void );
extern int mhd_get_scan_results( mhd_ap_info_t *results, int *num );

extern void mhd_set_wifi_11n_support(int enable);

// station connects to ap. 0:success, 1:failed
// security: 0-open, 1-wpa_psk_aes, 2-wpa2_psk_aes
extern int mhd_sta_connect( const char *ssid, char *bssid, uint8_t security, const char *password, uint8_t channel );
extern int mhd_sta_disconnect( uint8_t force );

extern int mhd_sta_network_up( uint32_t ip, uint32_t gateway, uint32_t netmask );
extern int mhd_sta_network_down( void );

extern int mhd_sta_register_link_callback( mhd_link_callback_t link_up_cb, mhd_link_callback_t link_down_cb );
extern int mhd_sta_deregister_link_callback( mhd_link_callback_t link_up_cb, mhd_link_callback_t link_down_cb );

extern int mhd_sta_get_rssi( void );
extern int mhd_sta_get_rate( void );
extern int mhd_sta_get_mac_address( mhd_mac_t *mac );

extern uint32_t mhd_sta_ipv4_ipaddr( void );
extern uint32_t mhd_sta_ipv4_gateway( void );
extern uint32_t mhd_sta_ipv4_netmask( void );

extern int mhd_sta_set_powersave( uint8_t mode, uint8_t time_ms );
extern int mhd_sta_get_powersave( uint8_t *mode, uint8_t *time_ms );

extern int mhd_sta_set_bcn_li_dtim( uint8_t dtim );
extern int mhd_sta_get_bcn_li_dtim( void );
extern int mhd_sta_set_dtim_interval( int dtim_interval_ms );

extern void mhd_get_ampdu_hostreorder_support();
extern void mhd_set_ampdu_hostreorder_support(int enable);

// ssid:  less than 32 bytes
// password: less than 32 bytes
// security: 0-open, 1-wpa_psk_aes, 2-wpa2_psk_aes
// channel: 1~13
extern int mhd_softap_start( const unsigned char *ssid, const char *password, uint8_t security, uint8_t channel );
extern int mhd_softap_stop( uint8_t force );

extern int mhd_softap_get_mac_address( mhd_mac_t *mac );

extern int mhd_softap_set_hidden( int enable );
extern int mhd_softap_get_hidden( void );

extern int mhd_softap_start_dhcpd( uint32_t ip_address);
extern int mhd_softap_stop_dhcpd( void );

extern int mhd_softap_get_mac_list( mhd_mac_t *mac_list, uint32_t *count );
extern int mhd_softap_get_rssi( mhd_mac_t *mac_addr );
extern int mhd_softap_deauth_assoc_sta(const mhd_mac_t* mac);

typedef void (*mhd_client_callback_t)(mhd_mac_t);
int mhd_softap_register_client_callback( mhd_client_callback_t client_assoc_cb, mhd_client_callback_t client_disassoc_cb );

extern int mhd_wifi_get_channel( int interface, uint32_t* channel );
extern int mhd_wifi_set_channel( int interface, uint32_t channel );
extern int mhd_wifi_get_max_associations( uint32_t* max_assoc );
extern int mhd_wifi_set_max_associations( uint32_t max_assoc );

extern void host_rtos_delay_milliseconds( uint32_t num_ms );

#endif /* ifndef INCLUDED_MHD_API_H */
