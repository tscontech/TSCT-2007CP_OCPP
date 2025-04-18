//----------------------------------------------------------------------------//
//#include <flash/stm32_flash.h>
#if !defined(CONFIG_MBED_ENABLED)
//#include "main.h"
//#include <lwip_netconf.h>
//#include "dhcps.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#endif
#include <stdlib.h>
#include <sys/ioctl.h>
#include "wifi_conf.h"
//#include "wifi_util.h"
//#include "wifi_ind.h"
#include "osdep_service.h"
#include <device_lock.h>
#if LWIP_AUTOIP
#include "lwip/autoip.h"
static struct autoip  ethNetifAutoip;
static bool           ethAutoip;
#endif
#include "ite/itp.h"

#if CONFIG_EXAMPLE_WLAN_FAST_CONNECT || CONFIG_JD_SMART
#include "wlan_fast_connect/example_wlan_fast_connect.h"
#if CONFIG_FAST_DHCP
#include "flash_api.h"
#include "device_lock.h"
#endif
#endif
#if CONFIG_EXAMPLE_UART_ATCMD || CONFIG_EXAMPLE_SPI_ATCMD
#include "at_cmd/atcmd_wifi.h"
#endif
#if CONFIG_INIC_EN
extern int inic_start(void);
extern int inic_stop(void);
#endif

/******************************************************
 *                    Constants
 ******************************************************/
#define SCAN_USE_SEMAPHORE	0

#define RTW_JOIN_TIMEOUT 15000

#define JOIN_ASSOCIATED             (uint32_t)(1 << 0)
#define JOIN_AUTHENTICATED          (uint32_t)(1 << 1)
#define JOIN_LINK_READY             (uint32_t)(1 << 2)
#define JOIN_SECURITY_COMPLETE      (uint32_t)(1 << 3)
#define JOIN_COMPLETE               (uint32_t)(1 << 4)
#define JOIN_NO_NETWORKS            (uint32_t)(1 << 5)
#define JOIN_WRONG_SECURITY         (uint32_t)(1 << 6)
#define JOIN_HANDSHAKE_DONE         (uint32_t)(1 << 7)
#define JOIN_SIMPLE_CONFIG          (uint32_t)(1 << 8)
#define JOIN_AIRKISS                (uint32_t)(1 << 9)
#define JOIN_CONNECTING             (uint32_t)(1 << 10)

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *               Variables Declarations
 ******************************************************/

#if !defined(CONFIG_MBED_ENABLED)
struct netif xnetif[NET_IF_NUM];
static struct dhcp wifiNetifDhcp;
#endif

/******************************************************
 *               Variables Definitions
 ******************************************************/
static internal_scan_handler_t scan_result_handler_ptr = {0, 0, 0, RTW_FALSE, 0, 0, 0, 0, 0};
static internal_join_result_t* join_user_data;
static unsigned char ap_bssid[6];
#if CONFIG_WIFI_IND_USE_THREAD
static void* disconnect_sema = NULL;
#endif
#if defined(CONFIG_MBED_ENABLED)
rtw_mode_t wifi_mode = RTW_MODE_STA;
#endif

rtw_mode_t wifi_mode = RTW_MODE_STA;

int error_flag = RTW_UNKNOWN;
uint32_t rtw_join_status;
#if ATCMD_VER == ATVER_2
extern unsigned char dhcp_mode_sta;
#endif
/* The flag to check if wifi init is completed */
static int _wifi_is_on = 0;
/******************************************************
 *               Variables Definitions
 ******************************************************/

#ifndef WLAN0_NAME
  #define WLAN0_NAME		"wlan0"
#endif
#ifndef WLAN1_NAME
  #define WLAN1_NAME		"wlan1"
#endif
/* Give default value if not defined */
#ifndef NET_IF_NUM
#ifdef CONFIG_CONCURRENT_MODE
#define NET_IF_NUM 2
#else
#define NET_IF_NUM 1
#endif
#endif

/*Static IP ADDRESS*/
#ifndef IP_ADDR0
#define IP_ADDR0   192
#define IP_ADDR1   168
#define IP_ADDR2   1
#define IP_ADDR3   80
#endif

/*NETMASK*/
#ifndef NETMASK_ADDR0
#define NETMASK_ADDR0   255
#define NETMASK_ADDR1   255
#define NETMASK_ADDR2   255
#define NETMASK_ADDR3   0
#endif

/*Gateway Address*/
#ifndef GW_ADDR0
#define GW_ADDR0   192
#define GW_ADDR1   168
#define GW_ADDR2   1
#define GW_ADDR3   1
#endif

/*Static IP ADDRESS*/
#ifndef AP_IP_ADDR0
#define AP_IP_ADDR0   192
#define AP_IP_ADDR1   168
#define AP_IP_ADDR2   43
#define AP_IP_ADDR3   1
#endif

/*NETMASK*/
#ifndef AP_NETMASK_ADDR0
#define AP_NETMASK_ADDR0   255
#define AP_NETMASK_ADDR1   255
#define AP_NETMASK_ADDR2   255
#define AP_NETMASK_ADDR3   0
#endif

/*Gateway Address*/
#ifndef AP_GW_ADDR0
#define AP_GW_ADDR0   192
#define AP_GW_ADDR1   168
#define AP_GW_ADDR2   43
#define AP_GW_ADDR3   1  
#endif

#if CONFIG_AUTO_RECONNECT
#ifndef AUTO_RECONNECT_COUNT
#define AUTO_RECONNECT_COUNT	8
#endif
#ifndef AUTO_RECONNECT_INTERVAL
#define AUTO_RECONNECT_INTERVAL	5	// in sec
#endif
#endif
int multi_scan = 0;
/******************************************************
 *               Function Definitions
 ******************************************************/

#if CONFIG_WLAN

extern unsigned char is_promisc_enabled(void);
extern int promisc_set(rtw_rcr_level_t enabled, void (*callback)(unsigned char*, unsigned int, void*), unsigned char len_used);
extern unsigned char _is_promisc_enabled(void);
extern int wext_get_drv_ability(const char *ifname, unsigned int *ability);
//----------------------------------------------------------------------------//
static int wifi_connect_local(rtw_network_info_t *pWifi)
{
	int ret = 0;

	if(is_promisc_enabled())
		promisc_set(0, NULL, 0);

	/* lock 4s to forbid suspend under linking */
	rtw_wakelock_timeout(4 *1000);

	if(!pWifi) return -1;
	switch(pWifi->security_type){
		case RTW_SECURITY_OPEN:
			ret = wext_set_key_ext(WLAN0_NAME, IW_ENCODE_ALG_NONE, NULL, 0, 0, 0, 0, NULL, 0);
			break;
		case RTW_SECURITY_WEP_PSK:
		case RTW_SECURITY_WEP_SHARED:
			ret = wext_set_auth_param(WLAN0_NAME, IW_AUTH_80211_AUTH_ALG, IW_AUTH_ALG_SHARED_KEY);
			if(ret == 0)
				ret = wext_set_key_ext(WLAN0_NAME, IW_ENCODE_ALG_WEP, NULL, pWifi->key_id, 1 /* set tx key */, 0, 0, pWifi->password, pWifi->password_len);
			break;
		case RTW_SECURITY_WPA_TKIP_PSK:
		case RTW_SECURITY_WPA2_TKIP_PSK:
			ret = wext_set_auth_param(WLAN0_NAME, IW_AUTH_80211_AUTH_ALG, IW_AUTH_ALG_OPEN_SYSTEM);
			if(ret == 0)
				ret = wext_set_key_ext(WLAN0_NAME, IW_ENCODE_ALG_TKIP, NULL, 0, 0, 0, 0, NULL, 0);
			if(ret == 0)
				ret = wext_set_passphrase(WLAN0_NAME, pWifi->password, pWifi->password_len);
			break;
		case RTW_SECURITY_WPA_AES_PSK:
		case RTW_SECURITY_WPA2_AES_PSK:
		case RTW_SECURITY_WPA2_MIXED_PSK:
		case RTW_SECURITY_WPA_WPA2_MIXED_PSK:
			ret = wext_set_auth_param(WLAN0_NAME, IW_AUTH_80211_AUTH_ALG, IW_AUTH_ALG_OPEN_SYSTEM);
			if(ret == 0)
				ret = wext_set_key_ext(WLAN0_NAME, IW_ENCODE_ALG_CCMP, NULL, 0, 0, 0, 0, NULL, 0);
			if(ret == 0)
				ret = wext_set_passphrase(WLAN0_NAME, pWifi->password, pWifi->password_len);
			break;
		default:
			ret = -1;
			RTW_API_INFO("\n\rWIFICONF: security type(0x%x) is not supported.\n\r", pWifi->security_type);
			break;
	}
	if(ret == 0)
		ret = wext_set_ssid(WLAN0_NAME, pWifi->ssid.val, pWifi->ssid.len);
	return ret;
}

static int wifi_connect_bssid_local(rtw_network_info_t *pWifi)
{
	int ret = 0;
	u8 bssid[12] = {0};

	if(is_promisc_enabled())
		promisc_set(0, NULL, 0);

	/* lock 4s to forbid suspend under linking */
	rtw_wakelock_timeout(4 *1000);

	if(!pWifi) return -1;
	switch(pWifi->security_type){
		case RTW_SECURITY_OPEN:
			ret = wext_set_key_ext(WLAN0_NAME, IW_ENCODE_ALG_NONE, NULL, 0, 0, 0, 0, NULL, 0);
			break;
		case RTW_SECURITY_WEP_PSK:
		case RTW_SECURITY_WEP_SHARED:
			ret = wext_set_auth_param(WLAN0_NAME, IW_AUTH_80211_AUTH_ALG, IW_AUTH_ALG_SHARED_KEY);
			if(ret == 0)
				ret = wext_set_key_ext(WLAN0_NAME, IW_ENCODE_ALG_WEP, NULL, pWifi->key_id, 1 /* set tx key */, 0, 0, pWifi->password, pWifi->password_len);
			break;
		case RTW_SECURITY_WPA_TKIP_PSK:
		case RTW_SECURITY_WPA2_TKIP_PSK:
			ret = wext_set_auth_param(WLAN0_NAME, IW_AUTH_80211_AUTH_ALG, IW_AUTH_ALG_OPEN_SYSTEM);
			if(ret == 0)
				ret = wext_set_key_ext(WLAN0_NAME, IW_ENCODE_ALG_TKIP, NULL, 0, 0, 0, 0, NULL, 0);
			if(ret == 0)
				ret = wext_set_passphrase(WLAN0_NAME, pWifi->password, pWifi->password_len);
			break;
		case RTW_SECURITY_WPA_AES_PSK:
		case RTW_SECURITY_WPA2_AES_PSK:
		case RTW_SECURITY_WPA2_MIXED_PSK:
			ret = wext_set_auth_param(WLAN0_NAME, IW_AUTH_80211_AUTH_ALG, IW_AUTH_ALG_OPEN_SYSTEM);
			if(ret == 0)
				ret = wext_set_key_ext(WLAN0_NAME, IW_ENCODE_ALG_CCMP, NULL, 0, 0, 0, 0, NULL, 0);
			if(ret == 0)
				ret = wext_set_passphrase(WLAN0_NAME, pWifi->password, pWifi->password_len);
			break;
		default:
			ret = -1;
			RTW_API_INFO("\n\rWIFICONF: security type(0x%x) is not supported.\n\r", pWifi->security_type);
			break;
	}
	if(ret == 0){
		memcpy(bssid, pWifi->bssid.octet, ETH_ALEN);
		if(pWifi->ssid.len){
			bssid[ETH_ALEN] = '#';
			bssid[ETH_ALEN + 1] = '@';
			memcpy(bssid + ETH_ALEN + 2, &pWifi, sizeof(pWifi));
		}
		ret = wext_set_bssid(WLAN0_NAME, bssid);
	}
	return ret;
}

void wifi_rx_beacon_hdl( char* buf, int buf_len, int flags, void* userdata) {
	//RTW_API_INFO("Beacon!\n");
}


static void wifi_no_network_hdl(char* buf, int buf_len, int flags, void* userdata)
{
  if(join_user_data!=NULL)
 	 rtw_join_status = JOIN_NO_NETWORKS | JOIN_CONNECTING;
}

static void wifi_connected_hdl( char* buf, int buf_len, int flags, void* userdata)
{
	rtw_memcpy(ap_bssid, buf, ETH_ALEN);

#ifdef CONFIG_ENABLE_EAP
	if(get_eap_phase()){
		rtw_join_status = JOIN_COMPLETE | JOIN_SECURITY_COMPLETE | JOIN_ASSOCIATED | JOIN_AUTHENTICATED | JOIN_LINK_READY | JOIN_CONNECTING;
		return;
	}
#endif /* CONFIG_ENABLE_EAP */

	if((join_user_data!=NULL)&&((join_user_data->network_info.security_type == RTW_SECURITY_OPEN) ||
		(join_user_data->network_info.security_type == RTW_SECURITY_WEP_PSK) ||
		(join_user_data->network_info.security_type == RTW_SECURITY_WEP_SHARED))){
		rtw_join_status = JOIN_COMPLETE | JOIN_SECURITY_COMPLETE | JOIN_ASSOCIATED | JOIN_AUTHENTICATED | JOIN_LINK_READY | JOIN_CONNECTING;
		rtw_up_sema(&join_user_data->join_sema);
	}else if((join_user_data!=NULL)&&((join_user_data->network_info.security_type == RTW_SECURITY_WPA2_AES_PSK) )){
		rtw_join_status = JOIN_COMPLETE | JOIN_SECURITY_COMPLETE | JOIN_ASSOCIATED | JOIN_AUTHENTICATED | JOIN_LINK_READY | JOIN_CONNECTING;
	}
}
static void wifi_handshake_done_hdl( char* buf, int buf_len, int flags, void* userdata)
{
	rtw_join_status = JOIN_COMPLETE | JOIN_SECURITY_COMPLETE | JOIN_ASSOCIATED | JOIN_AUTHENTICATED | JOIN_LINK_READY|JOIN_HANDSHAKE_DONE | JOIN_CONNECTING;
	if(join_user_data != NULL)
		rtw_up_sema(&join_user_data->join_sema);
}
extern void dhcp_stop(struct netif *netif);
static void wifi_disconn_hdl( char* buf, int buf_len, int flags, void* userdata)
{
#define REASON_4WAY_HNDSHK_TIMEOUT 15
	u16 disconn_reason;
	/* buf detail: mac addr + disconn_reason, buf_len = ETH_ALEN+2*/
	disconn_reason =*(u16*)(buf+6);

	if(join_user_data != NULL){
		if(join_user_data->network_info.security_type == RTW_SECURITY_OPEN){

			if(rtw_join_status & JOIN_NO_NETWORKS)
				error_flag = RTW_NONE_NETWORK;

		}else if(join_user_data->network_info.security_type == RTW_SECURITY_WEP_PSK){

			if(rtw_join_status & JOIN_NO_NETWORKS)
				error_flag = RTW_NONE_NETWORK;

			else if(rtw_join_status == 0)
		 		error_flag = RTW_CONNECT_FAIL;

		}else if(join_user_data->network_info.security_type == RTW_SECURITY_WPA2_AES_PSK){

			if(rtw_join_status & JOIN_NO_NETWORKS)
				error_flag = RTW_NONE_NETWORK;

			else if(rtw_join_status == 0)
		 		error_flag = RTW_CONNECT_FAIL;

			else if(rtw_join_status == (JOIN_COMPLETE | JOIN_SECURITY_COMPLETE | JOIN_ASSOCIATED | JOIN_AUTHENTICATED | JOIN_LINK_READY | JOIN_CONNECTING))
			{
				 if(disconn_reason == REASON_4WAY_HNDSHK_TIMEOUT)
					error_flag = RTW_4WAY_HANDSHAKE_TIMEOUT;
				else
					error_flag = RTW_WRONG_PASSWORD;
			}
		}

	}else{
		if(error_flag == RTW_NO_ERROR) //wifi_disconn_hdl will be dispatched one more time after join_user_data = NULL add by frankie
			error_flag = RTW_UNKNOWN;
	}

#if CONFIG_LWIP_LAYER
#if defined(CONFIG_MBED_ENABLED) || defined(PLATFORM_CUSTOMER_RTOS)
	//TODO
#else
    if (disconn_reason == 65535){
        netif_set_down(&xnetif[0]); //eason add
    }
	dhcp_stop(&xnetif[0]);
#endif
#endif

	if(join_user_data != NULL)
		rtw_up_sema(&join_user_data->join_sema);
	//RTW_API_INFO("\r\nWiFi Disconnect. Error flag is %d.\n", error_flag);

// Need to use sema to make sure wifi_disconn_hdl invoked before setting join_user_data when connecting to another AP
#if CONFIG_WIFI_IND_USE_THREAD
	if(disconnect_sema != NULL){
		rtw_up_sema(&disconnect_sema);
	}
#endif
}

#if CONFIG_EXAMPLE_WLAN_FAST_CONNECT || CONFIG_JD_SMART
#define WLAN0_NAME	      "wlan0"

struct wlan_fast_reconnect wifi_data_to_flash;

#if CONFIG_FAST_DHCP
	u8 is_the_same_ap = 0;
#endif

void restore_wifi_info_to_flash()
{
	u32 channel = 0;
	u8 index = 0;
	u8 *ifname[1] = {WLAN0_NAME};
	rtw_wifi_setting_t setting;
	//struct security_priv *psecuritypriv = &padapter->securitypriv;
	//WLAN_BSSID_EX  *pcur_bss = pmlmepriv->cur_network.network;

	rtw_memset(&wifi_data_to_flash,0,sizeof(struct wlan_fast_reconnect));

	if(p_write_reconnect_ptr){
		if(wifi_get_setting((const char*)ifname[0],&setting) || setting.mode == RTW_MODE_AP){
			RTW_API_INFO("\r\n %s():wifi_get_setting fail or ap mode", __func__);
			return;
		}
		channel = setting.channel;

#if CONFIG_FAST_DHCP
		flash_t		flash;
		struct wlan_fast_reconnect data;

		device_mutex_lock(RT_DEV_LOCK_FLASH);
		flash_stream_read(&flash, FAST_RECONNECT_DATA, sizeof(struct wlan_fast_reconnect), (uint8_t *)&data);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);

		if(strncmp(data.psk_essid,setting.ssid,strlen((char const*)setting.ssid)) == 0){
			is_the_same_ap = 1;
		}else{
			is_the_same_ap = 0;
		}
#endif

		rtw_memset(psk_essid[index], 0, sizeof(psk_essid[index]));
		strncpy((char*)psk_essid[index], (char const*)setting.ssid, strlen((char const*)setting.ssid));
		switch(setting.security_type){
			case RTW_SECURITY_OPEN:
			    rtw_memset(psk_passphrase[index], 0, sizeof(psk_passphrase[index]));
			    rtw_memset(wpa_global_PSK[index], 0, sizeof(wpa_global_PSK[index]));
			    wifi_data_to_flash.security_type = RTW_SECURITY_OPEN;
			    break;
			case RTW_SECURITY_WEP_PSK:
			    channel |= (setting.key_idx) << 28;
			    rtw_memset(psk_passphrase[index], 0, sizeof(psk_passphrase[index]));
    			    rtw_memset(wpa_global_PSK[index], 0, sizeof(wpa_global_PSK[index]));
			    rtw_memcpy(psk_passphrase[index], setting.password, sizeof(psk_passphrase[index]));
			    wifi_data_to_flash.security_type = RTW_SECURITY_WEP_PSK;
			    break;
			case RTW_SECURITY_WPA_TKIP_PSK:
				wifi_data_to_flash.security_type = RTW_SECURITY_WPA_TKIP_PSK;
                break;
			case RTW_SECURITY_WPA2_AES_PSK:
				wifi_data_to_flash.security_type = RTW_SECURITY_WPA2_AES_PSK;
			    break;
			default:
			    break;
		}

		memcpy(wifi_data_to_flash.psk_essid, psk_essid[index], sizeof(wifi_data_to_flash.psk_essid));
		if (strlen((char const*)psk_passphrase64) == 64) {
			memcpy(wifi_data_to_flash.psk_passphrase, psk_passphrase64, sizeof(wifi_data_to_flash.psk_passphrase));
		} else {
			memcpy(wifi_data_to_flash.psk_passphrase, psk_passphrase[index], sizeof(wifi_data_to_flash.psk_passphrase));
		}

		//For avoiding 13Bytes password of WEP ascii replacing WPA that will cause four-way handshaking failed because of not generating global_psk.
		if(setting.security_type == RTW_SECURITY_WEP_PSK)
        rtw_memset(psk_passphrase[index], 0, sizeof(psk_passphrase[index]));

		memcpy(wifi_data_to_flash.wpa_global_PSK, wpa_global_PSK[index], sizeof(wifi_data_to_flash.wpa_global_PSK));
		memcpy(&(wifi_data_to_flash.channel), &channel, 4);

#if CONFIG_FAST_DHCP == 0
		//call callback function in user program
		p_write_reconnect_ptr((u8 *)&wifi_data_to_flash, sizeof(struct wlan_fast_reconnect));
#endif

	}
}

#endif

//----------------------------------------------------------------------------//
int wifi_connect(
	char 				*ssid,
	rtw_security_t	security_type,
	char 				*password,
	int 				ssid_len,
	int 				password_len,
	int 				key_id,
	void 				*semaphore)
{
	_sema join_semaphore;
	rtw_result_t result = RTW_SUCCESS;
	u8 wep_hex = 0;
	u8 wep_pwd[14] = {0};

	if(rtw_join_status & JOIN_CONNECTING){
		if(wifi_disconnect() < 0){
			RTW_API_INFO("\nwifi_disconnect Operation failed!");
			return RTW_ERROR;
		}
		while(rtw_join_status & JOIN_CONNECTING){
			rtw_msleep_os(1);
		}
	}

	if(rtw_join_status & JOIN_SIMPLE_CONFIG || rtw_join_status & JOIN_AIRKISS){
		return RTW_BUSY;
	}

	error_flag = RTW_UNKNOWN ;//clear for last connect status
	rtw_memset(ap_bssid, 0, ETH_ALEN);
	if ( ( ( ( password_len >  RTW_MAX_PSK_LEN ) ||
             ( password_len <  RTW_MIN_PSK_LEN ) ) &&
           ( ( security_type == RTW_SECURITY_WPA_TKIP_PSK ) ||
             ( security_type == RTW_SECURITY_WPA_AES_PSK ) ||
             ( security_type == RTW_SECURITY_WPA2_AES_PSK ) ||
             ( security_type == RTW_SECURITY_WPA2_TKIP_PSK ) ||
             ( security_type == RTW_SECURITY_WPA2_MIXED_PSK ) ) )) {
             error_flag = RTW_WRONG_PASSWORD;
		return RTW_INVALID_KEY;
	}

	if ((security_type == RTW_SECURITY_WEP_PSK)||
		(security_type ==RTW_SECURITY_WEP_SHARED)) {
		if ((password_len != 5) && (password_len != 13) &&
		    (password_len != 10)&& (password_len != 26)) {
		    	error_flag = RTW_WRONG_PASSWORD;
			return RTW_INVALID_KEY;
		} else {

			if(password_len == 10) {

				u32 p[5] = {0};
				u8 i = 0;
				sscanf((const char*)password, "%02x%02x%02x%02x%02x", &p[0], &p[1], &p[2], &p[3], &p[4]);
				for(i=0; i< 5; i++)
					wep_pwd[i] = (u8)p[i];
				wep_pwd[5] = '\0';
				password_len = 5;
				wep_hex = 1;
			} else if (password_len == 26) {
				u32 p[13] = {0};
				u8 i = 0;
				sscanf((const char*)password, "%02x%02x%02x%02x%02x%02x%02x"\
					 "%02x%02x%02x%02x%02x%02x", &p[0], &p[1], &p[2], &p[3], &p[4],\
					  &p[5], &p[6], &p[7], &p[8], &p[9], &p[10], &p[11], &p[12]);
				for(i=0; i< 13; i++)
					wep_pwd[i] = (u8)p[i];
				wep_pwd[13] = '\0';
				password_len = 13;
				wep_hex = 1;
			}
		}
	}

	internal_join_result_t *join_result = (internal_join_result_t *)rtw_zmalloc(sizeof(internal_join_result_t));
	if(!join_result) {
		return RTW_NOMEM;
	}

	join_result->network_info.ssid.len = ssid_len > 32 ? 32 : ssid_len;
	rtw_memcpy(join_result->network_info.ssid.val, ssid, ssid_len);

	join_result->network_info.password_len = password_len;
	if(password_len) {
		/* add \0 to the end */
		join_result->network_info.password = rtw_zmalloc(password_len + 1);
		if(!join_result->network_info.password) {
			result =(rtw_result_t) RTW_NOMEM;
			goto error;
		}
		if (0 == wep_hex)
			rtw_memcpy(join_result->network_info.password, password, password_len);
		else
			rtw_memcpy(join_result->network_info.password, wep_pwd, password_len);

	}

	join_result->network_info.security_type = security_type;
	join_result->network_info.key_id = key_id;

	if(semaphore == NULL) {
		rtw_init_sema( &join_result->join_sema, 0 );
		if(!join_result->join_sema){
			result =(rtw_result_t) RTW_NORESOURCE;
			goto error;
		}
		join_semaphore = join_result->join_sema;
	} else {
		join_result->join_sema = semaphore;
	}
	wifi_reg_event_handler(WIFI_EVENT_NO_NETWORK,wifi_no_network_hdl,NULL);
	wifi_reg_event_handler(WIFI_EVENT_CONNECT, wifi_connected_hdl, NULL);
	wifi_reg_event_handler(WIFI_EVENT_DISCONNECT, wifi_disconn_hdl, NULL);
	wifi_reg_event_handler(WIFI_EVENT_FOURWAY_HANDSHAKE_DONE, wifi_handshake_done_hdl, NULL);

// if is connected to ap, would trigger disconn_hdl but need to make sure it is invoked before setting join_user_data
#if CONFIG_WIFI_IND_USE_THREAD
	if(wifi_is_connected_to_ap() == RTW_SUCCESS){
		rtw_init_sema( &disconnect_sema, 0 );
	}
#endif

	rtw_join_status = JOIN_CONNECTING;
	wifi_connect_local(&join_result->network_info);

#if CONFIG_WIFI_IND_USE_THREAD
	if(disconnect_sema != NULL){
		rtw_down_timeout_sema(&disconnect_sema, 50);
		rtw_free_sema( &disconnect_sema);
	}
#endif

	join_user_data = join_result;

	if(semaphore == NULL) {
// for eap connection, timeout should be longer (default value in wpa_supplicant: 60s)
#ifdef CONFIG_ENABLE_EAP
		if(get_eap_phase()){
			if(rtw_down_timeout_sema( &join_result->join_sema, 60000 ) == RTW_FALSE) {
				RTW_API_INFO("RTW API: Join bss timeout\r\n");
				if(password_len) {
					rtw_free(join_result->network_info.password);
				}
				result = RTW_TIMEOUT;
				goto error;
			} else {
				if(wifi_is_connected_to_ap( ) != RTW_SUCCESS) {
					result = RTW_ERROR;
					goto error;
				}
			}
		}
		else
#endif
		if(rtw_down_timeout_sema( &join_result->join_sema, RTW_JOIN_TIMEOUT ) == RTW_FALSE) {
			RTW_API_INFO("RTW API: Join bss timeout\r\n");
			if(password_len) {
				rtw_free(join_result->network_info.password);
			}
			result = RTW_TIMEOUT;
			goto error;
		} else {
			if(join_result->network_info.password_len) {
				rtw_free(join_result->network_info.password);
			}
			if(wifi_is_connected_to_ap( ) != RTW_SUCCESS) {
				result = RTW_ERROR;
				goto error;
			}
		}
	}

	result = RTW_SUCCESS;
#if CONFIG_LWIP_LAYER
#if defined(CONFIG_MBED_ENABLED) || defined(PLATFORM_CUSTOMER_RTOS)
	//TODO
#else
	netif_set_link_up(&xnetif[0]);
#endif
#endif

#if CONFIG_EXAMPLE_WLAN_FAST_CONNECT || CONFIG_JD_SMART
	restore_wifi_info_to_flash();
#endif

error:
	if(semaphore == NULL){
		rtw_free_sema( &join_semaphore);
	}
	join_user_data = NULL;
	rtw_free((u8*)join_result);
	wifi_unreg_event_handler(WIFI_EVENT_CONNECT, wifi_connected_hdl);
	wifi_unreg_event_handler(WIFI_EVENT_NO_NETWORK,wifi_no_network_hdl);
	wifi_unreg_event_handler(WIFI_EVENT_FOURWAY_HANDSHAKE_DONE, wifi_handshake_done_hdl);
	rtw_join_status &= ~JOIN_CONNECTING;
	return result;
}

int wifi_connect_bssid(
	unsigned char 		bssid[ETH_ALEN],
	char 				*ssid,
	rtw_security_t	security_type,
	char 				*password,
	int 				bssid_len,
	int 				ssid_len,
	int 				password_len,
	int 				key_id,
	void 				*semaphore)
{
	_sema join_semaphore;
	rtw_result_t result = RTW_SUCCESS;

	if(rtw_join_status & JOIN_CONNECTING){
		if(wifi_disconnect() < 0){
			RTW_API_INFO("\nwifi_disconnect Operation failed!");
			return RTW_ERROR;
		}
		while(rtw_join_status & JOIN_CONNECTING){
			rtw_mdelay_os(1);
		}
	}

	if(rtw_join_status & JOIN_SIMPLE_CONFIG || rtw_join_status & JOIN_AIRKISS){
		return RTW_BUSY;
	}

	error_flag = RTW_UNKNOWN;//clear for last connect status
	rtw_memset(ap_bssid, 0, ETH_ALEN);
	internal_join_result_t *join_result = (internal_join_result_t *)rtw_zmalloc(sizeof(internal_join_result_t));
	if(!join_result) {
		return RTW_NOMEM;
	}
	if(ssid_len && ssid){
		join_result->network_info.ssid.len = ssid_len > 32 ? 32 : ssid_len;
		rtw_memcpy(join_result->network_info.ssid.val, ssid, ssid_len);
	}
	rtw_memcpy(join_result->network_info.bssid.octet, bssid, bssid_len);

	if ( ( ( ( password_len >  RTW_MAX_PSK_LEN ) ||
             ( password_len <  RTW_MIN_PSK_LEN ) ) &&
           ( ( security_type == RTW_SECURITY_WPA_TKIP_PSK ) ||
             ( security_type == RTW_SECURITY_WPA_AES_PSK ) ||
             ( security_type == RTW_SECURITY_WPA2_AES_PSK ) ||
             ( security_type == RTW_SECURITY_WPA2_TKIP_PSK ) ||
             ( security_type == RTW_SECURITY_WPA2_MIXED_PSK ) ) )||
             (((password_len != 5)&& (password_len != 13))&&
             ((security_type == RTW_SECURITY_WEP_PSK)||
             (security_type ==RTW_SECURITY_WEP_SHARED ) ))) {
		return RTW_INVALID_KEY;
	}
	join_result->network_info.password_len = password_len;
	if(password_len) {
		/* add \0 to the end */
		join_result->network_info.password = rtw_zmalloc(password_len + 1);
		if(!join_result->network_info.password) {
			return RTW_NOMEM;
		}
		rtw_memcpy(join_result->network_info.password, password, password_len);
	}

		join_result->network_info.security_type = security_type;
		join_result->network_info.key_id = key_id;

	if(semaphore == NULL) {
		rtw_init_sema( &join_result->join_sema, 0 );
		if(!join_result->join_sema){
		    return RTW_NORESOURCE;
		}
		join_semaphore = join_result->join_sema;
	} else {
		join_result->join_sema = semaphore;
	}
	wifi_reg_event_handler(WIFI_EVENT_NO_NETWORK,wifi_no_network_hdl,NULL);
	wifi_reg_event_handler(WIFI_EVENT_CONNECT, wifi_connected_hdl, NULL);
	wifi_reg_event_handler(WIFI_EVENT_DISCONNECT, wifi_disconn_hdl, NULL);
	wifi_reg_event_handler(WIFI_EVENT_FOURWAY_HANDSHAKE_DONE, wifi_handshake_done_hdl, NULL);

	rtw_join_status = JOIN_CONNECTING;
	wifi_connect_bssid_local(&join_result->network_info);

	join_user_data = join_result;

	if(semaphore == NULL) {
		if(rtw_down_timeout_sema( &join_result->join_sema, RTW_JOIN_TIMEOUT ) == RTW_FALSE) {
			RTW_API_INFO("RTW API: Join bss timeout\r\n");
			if(password_len) {
				rtw_free(join_result->network_info.password);
			}
			rtw_free((u8*)join_result);
			rtw_free_sema( &join_semaphore);
			result = RTW_TIMEOUT;
			goto error;
		} else {
			rtw_free_sema( &join_semaphore );
			if(join_result->network_info.password_len) {
				rtw_free(join_result->network_info.password);
			}
			rtw_free((u8*)join_result);
			if( wifi_is_connected_to_ap( ) != RTW_SUCCESS) {
				result = RTW_ERROR;
				goto error;
			}
		}
	}

	result = RTW_SUCCESS;

#if CONFIG_LWIP_LAYER
#if defined(CONFIG_MBED_ENABLED) || defined(PLATFORM_CUSTOMER_RTOS)
	//TODO
#else
	netif_set_link_up(&xnetif[0]);
#endif
#endif

#if CONFIG_EXAMPLE_WLAN_FAST_CONNECT || CONFIG_JD_SMART
	restore_wifi_info_to_flash();
#endif

error:
	join_user_data = NULL;
	wifi_unreg_event_handler(WIFI_EVENT_CONNECT, wifi_connected_hdl);
	wifi_unreg_event_handler(WIFI_EVENT_NO_NETWORK,wifi_no_network_hdl);
	wifi_unreg_event_handler(WIFI_EVENT_FOURWAY_HANDSHAKE_DONE, wifi_handshake_done_hdl);
	rtw_join_status &= ~JOIN_CONNECTING;
	return result;
}

int wifi_disconnect(void)
{
	int ret = 0;

	//set MAC address last byte to 1 since driver will filter the mac with all 0x00 or 0xff
	//add extra 2 zero byte for check of #@ in wext_set_bssid()
	const __u8 null_bssid[ETH_ALEN + 2] = {0, 0, 0, 0, 0, 1, 0, 0};

	if (wext_set_bssid(WLAN0_NAME, null_bssid) < 0){
		RTW_API_INFO("\n\rWEXT: Failed to set bogus BSSID to disconnect");
		ret = -1;
	}

    /* Set down the LWIP netif flag */
    netif_set_down(&xnetif[0]); //Eason add
	return ret;
}

//----------------------------------------------------------------------------//
int wifi_is_connected_to_ap( void )
{
	return rltk_wlan_is_connected_to_ap();
}

//----------------------------------------------------------------------------//
int wifi_is_up(rtw_interface_t interface)
{
    switch (interface)
	{
		case RTW_AP_INTERFACE:
			switch (wifi_mode)
			{
				case RTW_MODE_STA_AP:
					return (rltk_wlan_running(WLAN1_IDX) && _wifi_is_on);
				case RTW_MODE_STA:
					return 0;
				default:
					return (rltk_wlan_running(WLAN0_IDX) && _wifi_is_on);
			}
		case RTW_STA_INTERFACE:
			switch (wifi_mode)
			{
				case RTW_MODE_AP:
					return 0;
				default:
					return (rltk_wlan_running(WLAN0_IDX) && _wifi_is_on);
			}
		default:
			return 0;
    }
}

int wifi_is_ready_to_transceive(rtw_interface_t interface)
{
    switch ( interface )
    {
        case RTW_AP_INTERFACE:
            return ( wifi_is_up(interface) == RTW_TRUE ) ? RTW_SUCCESS : RTW_ERROR;

        case RTW_STA_INTERFACE:
            switch ( error_flag)
            {
		  case RTW_NO_ERROR:
			return RTW_SUCCESS;

		  default:
			return RTW_ERROR;
            }
        default:
            return RTW_ERROR;
    }
}

//----------------------------------------------------------------------------//
int wifi_set_mac_address(char * mac)
{
	char buf[13+17+1];
	rtw_memset(buf, 0, sizeof(buf));
	snprintf(buf, 13+17, "write_mac %s", mac);
	return wext_private_command(WLAN0_NAME, buf, 0);
}

int wifi_get_mac_address(char * mac)
{
	int ret = 0;
	char buf[32];
	rtw_memset(buf, 0, sizeof(buf));
	rtw_memcpy(buf, "read_mac", 8);
	ret = wext_private_command_with_retval(WLAN0_NAME, buf, buf, 32);
	strcpy(mac, buf);
	return ret;
}

//----------------------------------------------------------------------------//
int wifi_enable_powersave(void)
{
	return wext_enable_powersave(WLAN0_NAME, 1, 1);
}

int wifi_disable_powersave(void)
{
	return wext_disable_powersave(WLAN0_NAME);
}

#if 0 //Not ready
//----------------------------------------------------------------------------//
int wifi_get_txpower(int *poweridx)
{
	int ret = 0;
	char buf[11];

	rtw_memset(buf, 0, sizeof(buf));
	rtw_memcpy(buf, "txpower", 11);
	ret = wext_private_command_with_retval(WLAN0_NAME, buf, buf, 11);
	sscanf(buf, "%d", poweridx);

	return ret;
}

int wifi_set_txpower(int poweridx)
{
	int ret = 0;
	char buf[24];

	rtw_memset(buf, 0, sizeof(buf));
	snprintf(buf, 24, "txpower patha=%d", poweridx);
	ret = wext_private_command(WLAN0_NAME, buf, 0);

	return ret;
}
#endif

//----------------------------------------------------------------------------//
int wifi_get_associated_client_list(void * client_list_buffer, uint16_t buffer_length)
{
	const char * ifname = WLAN0_NAME;
	int ret = 0;
	char buf[25];

	if(wifi_mode == RTW_MODE_STA_AP) {
		ifname = WLAN1_NAME;
	}

	rtw_memset(buf, 0, sizeof(buf));
	snprintf(buf, 25, "get_client_list %x", client_list_buffer);
	ret = wext_private_command(ifname, buf, 0);

	return ret;
}

//----------------------------------------------------------------------------//
int wifi_get_ap_bssid(unsigned char *bssid)
{
	if( RTW_SUCCESS == wifi_is_ready_to_transceive(RTW_STA_INTERFACE)){
		rtw_memcpy(bssid, ap_bssid, ETH_ALEN);
		return RTW_SUCCESS;
	}
	return RTW_ERROR;
}

//----------------------------------------------------------------------------//
int wifi_get_ap_info(rtw_bss_info_t * ap_info, rtw_security_t* security)
{
	const char * ifname = WLAN0_NAME;
	int ret = 0;
	char buf[24];

	if(wifi_mode == RTW_MODE_STA_AP) {
		ifname = WLAN1_NAME;
	}

	rtw_memset(buf, 0, sizeof(buf));
	snprintf(buf, 24, "get_ap_info %x", ap_info);
	ret = wext_private_command(ifname, buf, 0);

	snprintf(buf, 24, "get_security");
	ret = wext_private_command_with_retval(ifname, buf, buf, 24);
	sscanf(buf, "%d", security);

	return ret;
}

int wifi_get_drv_ability(unsigned int *ability)
{
	return wext_get_drv_ability(WLAN0_NAME, ability);
}

//----------------------------------------------------------------------------//
int wifi_set_country(rtw_country_code_t country_code)
{
	int ret;

	ret = wext_set_country(WLAN0_NAME, country_code);

	return ret;
}

//----------------------------------------------------------------------------//
int wifi_set_channel_plan(uint8_t channel_plan)
{
	const char * ifname = WLAN0_NAME;
	int ret = 0;
	char buf[24];

	rtw_memset(buf, 0, sizeof(buf));
	snprintf(buf, 24, "set_ch_plan %x", channel_plan);
	ret = wext_private_command(ifname, buf, 0);
	return ret;
}

int wifi_get_channel_plan(uint8_t *channel_plan)
{
	int ret = 0;
	char buf[24];
	char *ptmp;

	rtw_memset(buf, 0, sizeof(buf));
	rtw_memcpy(buf, "get_ch_plan", 11);
	ret = wext_private_command_with_retval(WLAN0_NAME, buf, buf, 24);
	*channel_plan = strtoul(buf, &ptmp, 16);
	return ret;
}

//----------------------------------------------------------------------------//
extern int rltk_wlan_get_sta_max_data_rate(u8 *inidata_rate);
int wifi_get_sta_max_data_rate(OUT u8 * inidata_rate)
{
	return rltk_wlan_get_sta_max_data_rate(inidata_rate);
}

//----------------------------------------------------------------------------//
int wifi_get_rssi(int *pRSSI)
{
	return wext_get_rssi(WLAN0_NAME, pRSSI);
}

//----------------------------------------------------------------------------//
int wifi_set_channel(int channel)
{
	return wext_set_channel(WLAN0_NAME, channel);
}

int wifi_get_channel(int *channel)
{
	return wext_get_channel(WLAN0_NAME, (u8*)channel);
}

//----------------------------------------------------------------------------//

 int wifi_register_multicast_address(rtw_mac_t *mac)
 {
 	return wext_register_multicast_address(WLAN0_NAME, mac);
 }
#ifdef CONFIG_MULTICAST
 int wifi_enable_multicast_address_filter(void)
{
	return wext_enable_multicast_address_filter(WLAN0_NAME);
}

int wifi_disable_multicast_address_filter(void)
{
	return wext_disable_multicast_address_filter(WLAN0_NAME);
}
#endif

int wifi_unregister_multicast_address(rtw_mac_t *mac)
{
	return wext_unregister_multicast_address(WLAN0_NAME, mac);
}

//----------------------------------------------------------------------------//
_WEAK void wifi_set_mib(void)
{
	// adaptivity
	wext_set_adaptivity(RTW_ADAPTIVITY_DISABLE);
}

//----------------------------------------------------------------------------//
_WEAK void wifi_set_country_code(void)
{
	//wifi_set_country(RTW_COUNTRY_US);
}

//----------------------------------------------------------------------------//
int wifi_rf_on(void)
{
	int ret;
	ret = rltk_wlan_rf_on();
	return ret;
}

//----------------------------------------------------------------------------//
int wifi_rf_off(void)
{
	int ret;
	ret = rltk_wlan_rf_off();
	return ret;
}

//----------------------------------------------------------------------------//
int wifi_on(rtw_mode_t mode)
{
	int ret = 1;
	int timeout = 20;
	int idx;
	int devnum = 1;
	static int event_init = 0;

	if(rltk_wlan_running(WLAN0_IDX)) {
		RTW_API_INFO("\n\rWIFI is already running");
		return 1;
	}

	if(event_init == 0){
		init_event_callback_list();
		event_init = 1;
	}

	wifi_mode = mode;

	if(mode == RTW_MODE_STA_AP)
		devnum = 2;

	// set wifi mib
	wifi_set_mib();
	RTW_API_INFO("\n\rInitializing WIFI ...");
	for(idx=0;idx<devnum;idx++){
		ret = rltk_wlan_init(idx, mode);
		if(ret <0)
			return ret;
	}
	for(idx=0;idx<devnum;idx++){
		device_mutex_lock(RT_DEV_LOCK_WLAN);
		ret = rltk_wlan_start(idx);
		if(ret == 0) _wifi_is_on = 1;
		device_mutex_unlock(RT_DEV_LOCK_WLAN);
		if(ret <0){
			RTW_API_INFO("\n\rERROR: Start WIFI Failed!");
			rltk_wlan_deinit();
			return ret;
		}
	}

	while(1) {
		if(rltk_wlan_running(devnum-1)) {
			RTW_API_INFO("\n\rWIFI initialized\n");
			wifi_set_country_code();
			break;
		}

		if(timeout == 0) {
			RTW_API_INFO("\n\rERROR: Init WIFI timeout!");
			break;
		}

		rtw_msleep_os(1000);
		timeout --;
	}

	#if CONFIG_LWIP_LAYER
	#if defined(CONFIG_MBED_ENABLED) || defined(PLATFORM_CUSTOMER_RTOS)
	//TODO
	#else
	printf("xnetif[0]: 0x%X\nxnetif[1]: 0x%X\n", &xnetif[0], &xnetif[1]);
	if(mode == RTW_MODE_STA){
		ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_ADD_NETIF, &xnetif[0]);
        if (ret == RTW_SUCCESS)
            netif_set_link_up(&xnetif[0]);
	}else if(mode == RTW_MODE_AP){
		ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_ADD_NETIF, &xnetif[0]);
		//ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFIAP_ENABLE, &xnetif[0]);
	}else if(mode == RTW_MODE_STA_AP) {
		ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_ADD_NETIF, &xnetif[1]);
		ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFIAP_ENABLE, &xnetif[1]);
		netif_set_up(&xnetif[1]);
		netif_set_link_up(&xnetif[1]);
		ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_ADD_NETIF, &xnetif[0]);
	}
	//netif_set_up(&xnetif[0]);
	//dhcp_set_struct(&xnetif[0], &wifiNetifDhcp);
	/*if(mode == RTW_MODE_AP)
		netif_set_link_up(&xnetif[0]);
	else	 if(mode == RTW_MODE_STA_AP) {
		netif_set_up(&xnetif[1]);
		netif_set_link_up(&xnetif[1]);
	}*/
	#endif
	#endif

#if CONFIG_INIC_EN
	inic_start();
#endif
	printf("wifi_disable_multicast_address_filter");
	wifi_disable_multicast_address_filter();

    printf("\r\n wifi_disable_powersave %d \n", wifi_disable_powersave());                

	return ret;
}

int wifi_off(void)
{
	int ret = 0;
	int timeout = 20;
	printf("wifi_off \n");
	if((rltk_wlan_running(WLAN0_IDX) == 0) &&
		(rltk_wlan_running(WLAN1_IDX) == 0)) {
		RTW_API_INFO("\n\rWIFI is not running");
		return 0;
	}
#if CONFIG_LWIP_LAYER
#if defined(CONFIG_MBED_ENABLED) || defined(PLATFORM_CUSTOMER_RTOS)
	//TODO
#else

//        ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_DISABLE, NULL);
//        ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_NETIF_SHUTDOWN, NULL);

	//dhcps_deinit();
	//LwIP_DHCP(0, DHCP_STOP);
	if (&xnetif[0]) {
            dhcp_stop(&xnetif[0]);
            netif_set_down(&xnetif[0]);
       }
        if (&xnetif[1]) {
//	    dhcp_stop(&xnetif[1]);
           netif_set_down(&xnetif[1]);
        }
#endif
#endif
#if defined(CONFIG_ENABLE_WPS_AP) && CONFIG_ENABLE_WPS_AP
	if((wifi_mode ==  RTW_MODE_AP) || (wifi_mode == RTW_MODE_STA_AP))
		wpas_wps_deinit();
#endif
	RTW_API_INFO("\n\rDeinitializing WIFI ...");
	device_mutex_lock(RT_DEV_LOCK_WLAN);
	rltk_wlan_deinit();
	_wifi_is_on = 0;
	device_mutex_unlock(RT_DEV_LOCK_WLAN);

	while(1) {
		if((rltk_wlan_running(WLAN0_IDX) == 0) &&
			(rltk_wlan_running(WLAN1_IDX) == 0)) {
			RTW_API_INFO("\n\rWIFI deinitialized");
			break;
		}

		if(timeout == 0) {
			RTW_API_INFO("\n\rERROR: Deinit WIFI timeout!");
			break;
		}

		rtw_msleep_os(1000);
		timeout --;
	}

	wifi_mode = RTW_MODE_NONE;
    
    ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_DISABLE, NULL);
    ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_NETIF_SHUTDOWN, NULL);

#if CONFIG_INIC_EN
	inic_stop();
#endif

	return ret;
}

int wifi_off_fastly(void)
{
#if CONFIG_LWIP_LAYER
#if defined(CONFIG_MBED_ENABLED) || defined(PLATFORM_CUSTOMER_RTOS)
	//TODO
#else
	//dhcps_deinit();
	//LwIP_DHCP(0, DHCP_STOP);
	dhcp_stop(&xnetif[0]);
#endif
#endif
	//RTW_API_INFO("\n\rDeinitializing WIFI ...");
	device_mutex_lock(RT_DEV_LOCK_WLAN);
	rltk_wlan_deinit_fastly();
	device_mutex_unlock(RT_DEV_LOCK_WLAN);
	return 0;
}


int wifi_set_power_mode(unsigned char ips_mode, unsigned char lps_mode)
{
	return wext_enable_powersave(WLAN0_NAME, ips_mode, lps_mode);
}

int wifi_set_tdma_param(unsigned char slot_period, unsigned char rfon_period_len_1, unsigned char rfon_period_len_2, unsigned char rfon_period_len_3)
{
	return wext_set_tdma_param(WLAN0_NAME, slot_period, rfon_period_len_1, rfon_period_len_2, rfon_period_len_3);
}

int wifi_set_lps_dtim(unsigned char dtim)
{
	return wext_set_lps_dtim(WLAN0_NAME, dtim);
}

int wifi_get_lps_dtim(unsigned char *dtim)
{
	return wext_get_lps_dtim(WLAN0_NAME, dtim);
}

// mode == 0: packet count, 1: enter directly, 2: tp threshold (default)
int wifi_set_lps_thresh(rtw_lps_thresh_t mode)
{
	return wext_set_lps_thresh(WLAN0_NAME, mode);
}

int wifi_set_beacon_mode(unsigned char mode) {
	return wext_set_beacon_mode(WLAN0_NAME, mode);
}

int wifi_set_lps_level(unsigned char lps_level) {
	return wext_set_lps_level(WLAN0_NAME, lps_level);
}
//----------------------------------------------------------------------------//
static void wifi_ap_sta_assoc_hdl( char* buf, int buf_len, int flags, void* userdata)
{
	//USER TODO
	printf("wifi_ap_sta_assoc_hdl \n");

}
static void wifi_ap_sta_disassoc_hdl( char* buf, int buf_len, int flags, void* userdata)
{
	//USER TODO
	printf("wifi_ap_sta_disassoc_hdl");
}

int wifi_get_last_error(void)
{
   return error_flag;
}


#if defined(CONFIG_ENABLE_WPS_AP) && CONFIG_ENABLE_WPS_AP
int wpas_wps_init(const char* ifname);
#endif

int wifi_set_mfp_support(unsigned char value)
{
	return wext_set_mfp_support(WLAN0_NAME, value);
}

int wifi_start_ap(
	char 				*ssid,
	rtw_security_t		security_type,
	char 				*password,
	int 				ssid_len,
	int 				password_len,
	int					channel)
{
	const char *ifname = WLAN0_NAME;
	int ret = 0;
#if defined (CONFIG_AP_MODE) && defined (CONFIG_NATIVEAP_MLME)
	if(wifi_mode == RTW_MODE_STA_AP) {
		ifname = WLAN1_NAME;
	}
    printf("wifi_start_ap %s %s\n",ssid,password);


	if(is_promisc_enabled())
		promisc_set(0, NULL, 0);

	wifi_reg_event_handler(WIFI_EVENT_STA_ASSOC, wifi_ap_sta_assoc_hdl, NULL);
	wifi_reg_event_handler(WIFI_EVENT_STA_DISASSOC, wifi_ap_sta_disassoc_hdl, NULL);

	ret = wext_set_mode(ifname, IW_MODE_MASTER);
	if(ret < 0) goto exit;
	ret = wext_set_channel(ifname, channel);	//Set channel before starting ap
	if(ret < 0) goto exit;

	switch(security_type) {
		case RTW_SECURITY_OPEN:
			break;
#ifdef CONFIG_FPGA
		case RTW_SECURITY_WEP_PSK:
			ret = wext_set_auth_param(ifname, IW_AUTH_80211_AUTH_ALG, IW_AUTH_ALG_OPEN_SYSTEM);
			if(ret == 0)
				ret = wext_set_key_ext(ifname, IW_ENCODE_ALG_WEP, NULL, 0, 1, 0, 0, (u8*)password, password_len);
			break;
		case RTW_SECURITY_WPA2_TKIP_PSK:
			ret = wext_set_auth_param(ifname, IW_AUTH_80211_AUTH_ALG, IW_AUTH_ALG_OPEN_SYSTEM);
			if(ret == 0)
				ret = wext_set_key_ext(ifname, IW_ENCODE_ALG_TKIP, NULL, 0, 0, 0, 0, NULL, 0);
			if(ret == 0)
				ret = wext_set_passphrase(ifname, (u8*)password, password_len);
			break;
#endif
		case RTW_SECURITY_WPA2_AES_PSK:
			ret = wext_set_auth_param(ifname, IW_AUTH_80211_AUTH_ALG, IW_AUTH_ALG_OPEN_SYSTEM);
			if(ret == 0)
				ret = wext_set_key_ext(ifname, IW_ENCODE_ALG_CCMP, NULL, 0, 0, 0, 0, NULL, 0);
			if(ret == 0)
				ret = wext_set_passphrase(ifname, (u8*)password, password_len);
			break;
#ifdef CONFIG_IEEE80211W
		case RTW_SECURITY_WPA2_AES_CMAC:
			ret = wext_set_auth_param(ifname, IW_AUTH_80211_AUTH_ALG, IW_AUTH_ALG_OPEN_SYSTEM);
			if(ret == 0)
				ret = wext_set_key_ext(ifname, IW_ENCODE_ALG_AES_CMAC, NULL, 0, 0, 0, 0, NULL, 0);
			if(ret == 0)
				ret = wext_set_passphrase(ifname, (u8*)password, password_len);
			break;
#endif
		default:
			ret = -1;
			RTW_API_INFO("\n\rWIFICONF: security type is not supported");
			break;
	}
	if(ret < 0) goto exit;

	ret = wext_set_ap_ssid(ifname, (u8*)ssid, ssid_len);
#if defined(CONFIG_ENABLE_WPS_AP) && CONFIG_ENABLE_WPS_AP
	wpas_wps_init(ifname);
#endif
#if CONFIG_LWIP_LAYER
#if defined(CONFIG_MBED_ENABLED) || defined(PLATFORM_CUSTOMER_RTOS)
	//TODO
#else
//	if(wifi_mode == RTW_MODE_STA_AP)
//		netif_set_link_up(&xnetif[1]);
//	else
		netif_set_link_up(&xnetif[0]);
#endif
#endif
exit:
#endif
	return ret;
}

extern int set_hidden_ssid(const char * ifname, u8 value);
int wifi_start_ap_with_hidden_ssid(
	char 				*ssid,
	rtw_security_t		security_type,
	char 				*password,
	int 				ssid_len,
	int 				password_len,
	int					channel)
{
	const char *ifname = WLAN0_NAME;
	int ret = 0;
#if defined (CONFIG_AP_MODE) && defined (CONFIG_NATIVEAP_MLME)
	if(wifi_mode == RTW_MODE_STA_AP) {
		ifname = WLAN1_NAME;
	}

	if(is_promisc_enabled())
		promisc_set(0, NULL, 0);

	wifi_reg_event_handler(WIFI_EVENT_STA_ASSOC, wifi_ap_sta_assoc_hdl, NULL);
	wifi_reg_event_handler(WIFI_EVENT_STA_DISASSOC, wifi_ap_sta_disassoc_hdl, NULL);

	ret = wext_set_mode(ifname, IW_MODE_MASTER);
	if(ret < 0) goto exit;
	ret = wext_set_channel(ifname, channel);	//Set channel before starting ap
	if(ret < 0) goto exit;

	switch(security_type) {
		case RTW_SECURITY_OPEN:
			break;
		case RTW_SECURITY_WPA2_AES_PSK:
			ret = wext_set_auth_param(ifname, IW_AUTH_80211_AUTH_ALG, IW_AUTH_ALG_OPEN_SYSTEM);
			if(ret == 0)
				ret = wext_set_key_ext(ifname, IW_ENCODE_ALG_CCMP, NULL, 0, 0, 0, 0, NULL, 0);
			if(ret == 0)
				ret = wext_set_passphrase(ifname, (u8*)password, password_len);
			break;
		default:
			ret = -1;
			RTW_API_INFO("\n\rWIFICONF: security type is not supported");
			break;
	}
	if(ret < 0) goto exit;

	ret = set_hidden_ssid(ifname, 1);
	if(ret < 0) goto exit;

	ret = wext_set_ap_ssid(ifname, (u8*)ssid, ssid_len);
#if defined(CONFIG_ENABLE_WPS_AP) && CONFIG_ENABLE_WPS_AP
	wpas_wps_init(ifname);
#endif
exit:
#endif
	return ret;
}

void wifi_scan_each_report_hdl( char* buf, int buf_len, int flags, void* userdata)
{
	int i =0;
	int j =0;
	int insert_pos = 0;
	rtw_scan_result_t** result_ptr = (rtw_scan_result_t**)buf;
	rtw_scan_result_t* temp = NULL;

	for(i=0; i<scan_result_handler_ptr.scan_cnt; i++){
		if(CMP_MAC(scan_result_handler_ptr.pap_details[i]->BSSID.octet, (*result_ptr)->BSSID.octet)){
			if((*result_ptr)->signal_strength > scan_result_handler_ptr.pap_details[i]->signal_strength){
				temp = scan_result_handler_ptr.pap_details[i];
				for(j = i-1; j >= 0; j--){
					if(scan_result_handler_ptr.pap_details[j]->signal_strength >= (*result_ptr)->signal_strength)
						break;
					else
						scan_result_handler_ptr.pap_details[j+1] = scan_result_handler_ptr.pap_details[j];
				}
				scan_result_handler_ptr.pap_details[j+1] = temp;
				scan_result_handler_ptr.pap_details[j+1]->signal_strength = (*result_ptr)->signal_strength;
			}
			memset(*result_ptr, 0, sizeof(rtw_scan_result_t));
			return;
		}
	}

	//scan_result_handler_ptr.scan_cnt++;

	if(scan_result_handler_ptr.scan_cnt >= scan_result_handler_ptr.max_ap_size){
		scan_result_handler_ptr.scan_cnt = scan_result_handler_ptr.max_ap_size;
		if((*result_ptr)->signal_strength > scan_result_handler_ptr.pap_details[scan_result_handler_ptr.max_ap_size-1]->signal_strength){
			rtw_memcpy(scan_result_handler_ptr.pap_details[scan_result_handler_ptr.max_ap_size-1], *result_ptr, sizeof(rtw_scan_result_t));
			temp = scan_result_handler_ptr.pap_details[scan_result_handler_ptr.max_ap_size -1];
			scan_result_handler_ptr.scan_cnt  = scan_result_handler_ptr.max_ap_size -1;
		}else
			return;
	}else{
		rtw_memcpy(&scan_result_handler_ptr.ap_details[scan_result_handler_ptr.scan_cnt], *result_ptr, sizeof(rtw_scan_result_t));
	}

	for(i=0; i< scan_result_handler_ptr.scan_cnt; i++){
		if((*result_ptr)->signal_strength > scan_result_handler_ptr.pap_details[i]->signal_strength)
			break;
	}
	insert_pos = i;

	for(i = scan_result_handler_ptr.scan_cnt; i>insert_pos; i--)
		scan_result_handler_ptr.pap_details[i] = scan_result_handler_ptr.pap_details[i-1];

	if(temp != NULL)
		scan_result_handler_ptr.pap_details[insert_pos] = temp;
	else
		scan_result_handler_ptr.pap_details[insert_pos] = &scan_result_handler_ptr.ap_details[scan_result_handler_ptr.scan_cnt];

	if(scan_result_handler_ptr.scan_cnt < scan_result_handler_ptr.max_ap_size)
		scan_result_handler_ptr.scan_cnt++;

	rtw_memset(*result_ptr, 0, sizeof(rtw_scan_result_t));
}

void wifi_scan_done_hdl( char* buf, int buf_len, int flags, void* userdata)
{
	int i = 0;
	rtw_scan_handler_result_t scan_result_report;

	for(i=0; i<scan_result_handler_ptr.scan_cnt; i++){
		rtw_memcpy(&scan_result_report.ap_details, scan_result_handler_ptr.pap_details[i], sizeof(rtw_scan_result_t));
		scan_result_report.scan_complete = scan_result_handler_ptr.scan_complete;
		scan_result_report.user_data = scan_result_handler_ptr.user_data;
		(*scan_result_handler_ptr.gscan_result_handler)(&scan_result_report);
	}

	scan_result_handler_ptr.scan_complete = RTW_TRUE;
	scan_result_report.scan_complete = RTW_TRUE;
	scan_result_report.user_data = scan_result_handler_ptr.user_data;
	(*scan_result_handler_ptr.gscan_result_handler)(&scan_result_report);

	rtw_free(scan_result_handler_ptr.ap_details);
	rtw_free(scan_result_handler_ptr.pap_details);
#if SCAN_USE_SEMAPHORE
	rtw_up_sema(&scan_result_handler_ptr.scan_semaphore);
#else
	scan_result_handler_ptr.scan_running = 0;
#endif
	wifi_unreg_event_handler(WIFI_EVENT_SCAN_RESULT_REPORT, wifi_scan_each_report_hdl);
	wifi_unreg_event_handler(WIFI_EVENT_SCAN_DONE, wifi_scan_done_hdl);
	return;
}

//int rtk_wifi_scan(char *buf, int buf_len, xSemaphoreHandle * semaphore)
int wifi_scan(rtw_scan_type_t                    scan_type,
				  rtw_bss_type_t                     bss_type,
				  void*                result_ptr)
{
	int ret;
	scan_buf_arg * pscan_buf;
	u16 flags = scan_type | (bss_type << 8);
	if(result_ptr != NULL){
		pscan_buf = (scan_buf_arg *)result_ptr;
		if(multi_scan)
			ret = wext_set_multiscan(WLAN0_NAME, (char*)pscan_buf->buf, pscan_buf->buf_len, flags);
		else
			ret = wext_set_scan(WLAN0_NAME, (char*)pscan_buf->buf, pscan_buf->buf_len, flags);
	}else{
		wifi_reg_event_handler(WIFI_EVENT_SCAN_RESULT_REPORT, wifi_scan_each_report_hdl, NULL);
		wifi_reg_event_handler(WIFI_EVENT_SCAN_DONE, wifi_scan_done_hdl, NULL);
		ret = wext_set_scan(WLAN0_NAME, NULL, 0, flags);
	}
	
	if(ret == 0) {
		if(result_ptr != NULL){
			ret = wext_get_scan(WLAN0_NAME, pscan_buf->buf, pscan_buf->buf_len);
		}
	}
	else if(ret == -1){
		if(result_ptr == NULL){
			wifi_unreg_event_handler(WIFI_EVENT_SCAN_RESULT_REPORT, wifi_scan_each_report_hdl);
			wifi_unreg_event_handler(WIFI_EVENT_SCAN_DONE, wifi_scan_done_hdl);
		}
	}
	return ret;
}
int wifi_scan_networks_with_multissid(int (results_handler)(char*buf, int buflen, char *ssid, void *user_data), 
	OUT void* user_data, IN int scan_buflen, IN scan_ssid* Ssid ,IN int num_ssid)
{
	int scan_cnt = 0, add_cnt = 0;
	scan_buf_arg scan_buf;
	int len = 0;
	int i = 0;
	int ret;
	scan_buf.buf_len = scan_buflen;
	scan_buf.buf = (char*)rtw_malloc(scan_buf.buf_len);
	if(!scan_buf.buf){
		RTW_API_INFO("\n\rERROR: Can't malloc memory(%d)", scan_buf.buf_len);
		return RTW_NOMEM;
	}
	//set ssid
	memset(scan_buf.buf, 0, scan_buf.buf_len);
	memcpy(scan_buf.buf, &num_ssid, sizeof(int));
	len += sizeof(int);
	for(i =0;i<num_ssid;i++){
		len += sizeof(int);
		len += Ssid[i].ssidlength;
	}
	if(len>scan_buf.buf_len){
		RTW_API_INFO("\r\n the scan ssid length is more scan_buf length");
		return RTW_ERROR;
	}
	len = sizeof(int);
	for(i =0;i<num_ssid;i++){
		memcpy(scan_buf.buf + len, &(Ssid[i].ssidlength), sizeof(int));
		len += sizeof(int);
		memcpy(scan_buf.buf + len, Ssid[i].ssid, Ssid[i].ssidlength);
		len += Ssid[i].ssidlength;
	}
	//Scan channel
	multi_scan = 1;
	if(scan_cnt = (wifi_scan(RTW_SCAN_TYPE_ACTIVE, RTW_BSS_TYPE_ANY, &scan_buf)) < 0){
		RTW_API_INFO("\n\rERROR: wifi scan failed");
		ret = RTW_ERROR;
	}else{
		if(NULL == results_handler)
		{
			int plen = 0;
			while(plen < scan_buf.buf_len){
				int len, rssi, ssid_len, i, security_mode;
				int wps_password_id;
				char *mac, *ssid;
				//u8 *security_mode;
				RTW_API_INFO("\n\r");
				// len
				len = (int)*(scan_buf.buf + plen);
				RTW_API_INFO("len = %d,\t", len);
				// check end
				if(len == 0) break;
				// mac
				mac = scan_buf.buf + plen + 1;
				RTW_API_INFO("mac = ");
				for(i=0; i<6; i++)
					RTW_API_INFO("%02x ", (u8)*(mac+i));
				RTW_API_INFO(",\t");
				// rssi
				rssi = *(int*)(scan_buf.buf + plen + 1 + 6);
				RTW_API_INFO(" rssi = %d,\t", rssi);
				// security_mode
				security_mode = (int)*(scan_buf.buf + plen + 1 + 6 + 4);
				switch (security_mode) {
					case IW_ENCODE_ALG_NONE:
						RTW_API_INFO("sec = open    ,\t");
						break;
					case IW_ENCODE_ALG_WEP:
						RTW_API_INFO("sec = wep     ,\t");
						break;
					case IW_ENCODE_ALG_CCMP:
						RTW_API_INFO("sec = wpa/wpa2,\t");
						break;
				}
				// password id
				wps_password_id = (int)*(scan_buf.buf + plen + 1 + 6 + 4 + 1);
				RTW_API_INFO("wps password id = %d,\t", wps_password_id);
				
				RTW_API_INFO("channel = %d,\t", *(scan_buf.buf + plen + 1 + 6 + 4 + 1 + 1));
				// ssid
				ssid_len = len - 1 - 6 - 4 - 1 - 1 - 1;
				ssid = scan_buf.buf + plen + 1 + 6 + 4 + 1 + 1 + 1;
				RTW_API_INFO("ssid = ");
				for(i=0; i<ssid_len; i++)
					RTW_API_INFO("%c", *(ssid+i));
				plen += len;
				add_cnt++;
			}

			RTW_API_INFO("\n\rwifi_scan: add count = %d, scan count = %d", add_cnt, scan_cnt);
		}
		ret = RTW_SUCCESS;
	}
	if(results_handler)
		//results_handler(scan_buf.buf, scan_buf.buf_len, ssid, user_data);
		
	if(scan_buf.buf)
		rtw_free(scan_buf.buf);
	multi_scan = 0;
	return ret;
}


int wifi_scan_networks_with_ssid(int (results_handler)(char*buf, int buflen, char *ssid, void *user_data), 
	OUT void* user_data, IN int scan_buflen, IN char* ssid, IN int ssid_len)
{
	int scan_cnt = 0, add_cnt = 0;
	scan_buf_arg scan_buf;
	int ret;

	scan_buf.buf_len = scan_buflen;
	scan_buf.buf = (char*)rtw_malloc(scan_buf.buf_len);
	if(!scan_buf.buf){
		RTW_API_INFO("\n\rERROR: Can't malloc memory(%d)", scan_buf.buf_len);
		return RTW_NOMEM;
	}
	//set ssid
	memset(scan_buf.buf, 0, scan_buf.buf_len);
	memcpy(scan_buf.buf, &ssid_len, sizeof(int));
	memcpy(scan_buf.buf+sizeof(int), ssid, ssid_len);

	//Scan channel	
	multi_scan = 0;
	if(scan_cnt = (wifi_scan(RTW_SCAN_TYPE_ACTIVE, RTW_BSS_TYPE_ANY, &scan_buf)) < 0){
		RTW_API_INFO("\n\rERROR: wifi scan failed");
		ret = RTW_ERROR;
	}else{
		if(NULL == results_handler)
		{
			int plen = 0;
			while(plen < scan_buf.buf_len){
				int len, rssi, ssid_len, i, security_mode;
				int wps_password_id;
				char *mac, *ssid;
				//u8 *security_mode;
				RTW_API_INFO("\n\r");
				// len
				len = (int)*(scan_buf.buf + plen);
				RTW_API_INFO("len = %d,\t", len);
				// check end
				if(len == 0) break;
				// mac
				mac = scan_buf.buf + plen + 1;
				RTW_API_INFO("mac = ");
				for(i=0; i<6; i++)
					RTW_API_INFO("%02x ", (u8)*(mac+i));
				RTW_API_INFO(",\t");
				// rssi
				rssi = *(int*)(scan_buf.buf + plen + 1 + 6);
				RTW_API_INFO(" rssi = %d,\t", rssi);
				// security_mode
				security_mode = (int)*(scan_buf.buf + plen + 1 + 6 + 4);
				switch (security_mode) {
					case IW_ENCODE_ALG_NONE:
						RTW_API_INFO("sec = open    ,\t");
						break;
					case IW_ENCODE_ALG_WEP:
						RTW_API_INFO("sec = wep     ,\t");
						break;
					case IW_ENCODE_ALG_CCMP:
						RTW_API_INFO("sec = wpa/wpa2,\t");
						break;
				}
				// password id
				wps_password_id = (int)*(scan_buf.buf + plen + 1 + 6 + 4 + 1);
				RTW_API_INFO("wps password id = %d,\t", wps_password_id);
				
				RTW_API_INFO("channel = %d,\t", *(scan_buf.buf + plen + 1 + 6 + 4 + 1 + 1));
				// ssid
				ssid_len = len - 1 - 6 - 4 - 1 - 1 - 1;
				ssid = scan_buf.buf + plen + 1 + 6 + 4 + 1 + 1 + 1;
				RTW_API_INFO("ssid = ");
				for(i=0; i<ssid_len; i++)
					RTW_API_INFO("%c", *(ssid+i));
				plen += len;
				add_cnt++;
			}

			RTW_API_INFO("\n\rwifi_scan: add count = %d, scan count = %d", add_cnt, scan_cnt);
		}
		ret = RTW_SUCCESS;
	}
	if(results_handler)
		results_handler(scan_buf.buf, scan_buf.buf_len, ssid, user_data);
		
	if(scan_buf.buf)
		rtw_free(scan_buf.buf);

	return ret;
}

int wifi_scan_networks(rtw_scan_result_handler_t results_handler, void* user_data)
{
	unsigned int max_ap_size = 64;

	/* lock 2s to forbid suspend under scan */
	rtw_wakelock_timeout(2*1000);
	
#if SCAN_USE_SEMAPHORE
	rtw_bool_t result;
	if(NULL == scan_result_handler_ptr.scan_semaphore)
		rtw_init_sema(&scan_result_handler_ptr.scan_semaphore, 1);
	
	scan_result_handler_ptr.scan_start_time = rtw_get_current_time();
	/* Initialise the semaphore that will prevent simultaneous access - cannot be a mutex, since
	* we don't want to allow the same thread to start a new scan */
	result = (rtw_bool_t)rtw_down_timeout_sema(&scan_result_handler_ptr.scan_semaphore, SCAN_LONGEST_WAIT_TIME);
	if ( result != RTW_TRUE )
	{
		/* Return error result, but set the semaphore to work the next time */
		rtw_up_sema(&scan_result_handler_ptr.scan_semaphore);
		return RTW_TIMEOUT;
	}
#else
	if(scan_result_handler_ptr.scan_running){
		int count = 100;
		while(scan_result_handler_ptr.scan_running && count > 0)
		{
			rtw_msleep_os(20);
			count --;
		}
		if(count == 0){
			RTW_API_INFO("\n\r[%d]WiFi: Scan is running. Wait 2s timeout.", rtw_get_current_time());
			return RTW_TIMEOUT;
		}
	}
	scan_result_handler_ptr.scan_start_time = rtw_get_current_time();
	scan_result_handler_ptr.scan_running = 1;
#endif

	scan_result_handler_ptr.gscan_result_handler = results_handler;
	
	scan_result_handler_ptr.max_ap_size = max_ap_size;
	scan_result_handler_ptr.ap_details = (rtw_scan_result_t*)rtw_zmalloc(max_ap_size*sizeof(rtw_scan_result_t));
	if(scan_result_handler_ptr.ap_details == NULL){
		goto err_exit;
	}
	rtw_memset(scan_result_handler_ptr.ap_details, 0, max_ap_size*sizeof(rtw_scan_result_t));

	scan_result_handler_ptr.pap_details = (rtw_scan_result_t**)rtw_zmalloc(max_ap_size*sizeof(rtw_scan_result_t*));
	if(scan_result_handler_ptr.pap_details == NULL)
		goto error2_with_result_ptr;
	rtw_memset(scan_result_handler_ptr.pap_details, 0, max_ap_size*sizeof(rtw_scan_result_t*));
	
	scan_result_handler_ptr.scan_cnt = 0;

	scan_result_handler_ptr.scan_complete = RTW_FALSE;
	scan_result_handler_ptr.user_data = user_data;

	if (wifi_scan( RTW_SCAN_COMMAMD<<4 | RTW_SCAN_TYPE_ACTIVE, RTW_BSS_TYPE_ANY, NULL) != RTW_SUCCESS)
	{
		goto error1_with_result_ptr;
	}

	return RTW_SUCCESS;

error1_with_result_ptr:
	rtw_free((u8*)scan_result_handler_ptr.pap_details);
	scan_result_handler_ptr.pap_details = NULL;
	
error2_with_result_ptr:
	rtw_free((u8*)scan_result_handler_ptr.ap_details);
	scan_result_handler_ptr.ap_details = NULL;

err_exit:
	rtw_memset((void *)&scan_result_handler_ptr, 0, sizeof(scan_result_handler_ptr));
	return RTW_ERROR;
}
//----------------------------------------------------------------------------//
int wifi_set_pscan_chan(__u8 * channel_list,__u8 * pscan_config, __u8 length)
{
	if(channel_list)
	    return wext_set_pscan_channel(WLAN0_NAME, channel_list, pscan_config, length);
	else
	    return -1;
}
//---------------------------------------------------------------------------//
int wifi_set_scan_reorderchan(__u8 * channel_list, __u8 length)//pscan_config should be PSCAN_REORDER_CHANNEL
{
	if(channel_list)
	    return wext_set_scan_reorderchannel(WLAN0_NAME, channel_list, length);
	else
	    return -1;
}

//----------------------------------------------------------------------------//
int wifi_get_setting(const char *ifname, rtw_wifi_setting_t *pSetting)
{
	int ret = 0;
	int mode = 0;
	unsigned short security = 0;

	memset(pSetting, 0, sizeof(rtw_wifi_setting_t));
	if(wext_get_mode(ifname, &mode) < 0)
		ret = -1;

	switch(mode) {
		case IW_MODE_MASTER:
			pSetting->mode = RTW_MODE_AP;
			break;
		case IW_MODE_INFRA:
		default:
			pSetting->mode = RTW_MODE_STA;
			break;
		//default:
			//RTW_API_INFO("\r\n%s(): Unknown mode %d\n", __func__, mode);
			//break;
	}

	if(wext_get_ssid(ifname, pSetting->ssid) < 0)
		ret = -1;
	if(wext_get_channel(ifname, &pSetting->channel) < 0)
		ret = -1;
	if(wext_get_enc_ext(ifname, &security, &pSetting->key_idx, pSetting->password) < 0)
		ret = -1;

	switch(security){
		case IW_ENCODE_ALG_NONE:
			pSetting->security_type = RTW_SECURITY_OPEN;
			break;
		case IW_ENCODE_ALG_WEP:
			pSetting->security_type = RTW_SECURITY_WEP_PSK;
			break;
		case IW_ENCODE_ALG_TKIP:
			pSetting->security_type = RTW_SECURITY_WPA_TKIP_PSK;
			break;
		case IW_ENCODE_ALG_CCMP:
			pSetting->security_type = RTW_SECURITY_WPA2_AES_PSK;
			break;
		default:
			break;
	}

	if(security == IW_ENCODE_ALG_TKIP || security == IW_ENCODE_ALG_CCMP)
		if(wext_get_passphrase(ifname, pSetting->password) < 0)
			ret = -1;

	return ret;
}
//----------------------------------------------------------------------------//
int wifi_show_setting(const char *ifname, rtw_wifi_setting_t *pSetting)
{
	int ret = 0;

	RTW_API_INFO("\n\r\nWIFI  %s Setting:",ifname);
	RTW_API_INFO("\n\r==============================");

	switch(pSetting->mode) {
		case RTW_MODE_AP:
			RTW_API_INFO("\n\r      MODE => AP");
			break;
		case RTW_MODE_STA:
			RTW_API_INFO("\n\r      MODE => STATION");
			break;
		default:
			RTW_API_INFO("\n\r      MODE => UNKNOWN");
	}
	RTW_API_INFO("\n\r      SSID => %s", pSetting->ssid);
	RTW_API_INFO("\n\r   CHANNEL => %d", pSetting->channel);

	switch(pSetting->security_type) {
		case RTW_SECURITY_OPEN:
			RTW_API_INFO("\n\r  SECURITY => OPEN");
			break;
		case RTW_SECURITY_WEP_PSK:
			RTW_API_INFO("\n\r  SECURITY => WEP");
			RTW_API_INFO("\n\r KEY INDEX => %d", pSetting->key_idx);
			break;
		case RTW_SECURITY_WPA_TKIP_PSK:
			RTW_API_INFO("\n\r  SECURITY => TKIP");
			break;
		case RTW_SECURITY_WPA2_AES_PSK:
			RTW_API_INFO("\n\r  SECURITY => AES");
			break;
		default:
			RTW_API_INFO("\n\r  SECURITY => UNKNOWN");
	}

	RTW_API_INFO("\n\r  PASSWORD => %s", pSetting->password);
	RTW_API_INFO("\n\r");
	return ret;
}

//----------------------------------------------------------------------------//
int wifi_set_network_mode(rtw_network_mode_t mode)
{
	if((mode == RTW_NETWORK_B) || (mode == RTW_NETWORK_BG) || (mode == RTW_NETWORK_BGN))
		return rltk_wlan_wireless_mode((unsigned char) mode);

	return -1;
}

int wifi_get_network_mode(rtw_network_mode_t *pmode)
{
	if(pmode != NULL)
		return rltk_wlan_get_wireless_mode((unsigned char *) pmode);

	return -1;
}

int wifi_set_wps_phase(unsigned char is_trigger_wps)
{
	return rltk_wlan_set_wps_phase(is_trigger_wps);
}

//----------------------------------------------------------------------------//
int wifi_set_promisc(rtw_rcr_level_t enabled, void (*callback)(unsigned char*, unsigned int, void*), unsigned char len_used)
{
	return promisc_set(enabled, callback, len_used);
}

void wifi_enter_promisc_mode(){
#ifdef CONFIG_PROMISC
	int mode = 0;
	unsigned char ssid[33];

	if(wifi_mode == RTW_MODE_STA_AP){
		wifi_off();
		rtw_msleep_os(20);
		wifi_on(RTW_MODE_PROMISC);
	}else{
		wext_get_mode(WLAN0_NAME, &mode);

		switch(mode) {
			case IW_MODE_MASTER:    //In AP mode
				//rltk_wlan_deinit();
				wifi_off();//modified by Chris Yang for iNIC
				rtw_msleep_os(20);
				//rltk_wlan_init(0, RTW_MODE_PROMISC);
				//rltk_wlan_start(0);
				wifi_on(RTW_MODE_PROMISC);
				break;
			case IW_MODE_INFRA:		//In STA mode
				if(wext_get_ssid(WLAN0_NAME, ssid) > 0)
					wifi_disconnect();
		}
	}
#endif
}

int wifi_restart_ap(
	unsigned char 		*ssid,
	rtw_security_t		security_type,
	unsigned char 		*password,
	int 				ssid_len,
	int 				password_len,
	int					channel)
{
#if defined (CONFIG_AP_MODE) && defined (CONFIG_NATIVEAP_MLME)
	unsigned char idx = 0;
#if CONFIG_LWIP_LAYER
#if defined(CONFIG_MBED_ENABLED) || defined(PLATFORM_CUSTOMER_RTOS)
	//TODO
#else
	ip_addr_t ipaddr;
	ip_addr_t netmask;
	ip_addr_t gw;
	struct netif * pnetif = &xnetif[0];
#endif
#endif
#ifdef  CONFIG_CONCURRENT_MODE
	rtw_wifi_setting_t setting;
	int sta_linked = 0;
#endif

	if(rltk_wlan_running(WLAN1_IDX)){
		idx = 1;
	}

	// stop dhcp server
#if CONFIG_LWIP_LAYER
#if defined(CONFIG_MBED_ENABLED) || defined(PLATFORM_CUSTOMER_RTOS)
	//TODO
#else
	//dhcps_deinit();
#endif
#endif

#ifdef  CONFIG_CONCURRENT_MODE
	if(idx > 0){
		sta_linked = wifi_get_setting(WLAN0_NAME, &setting);
		wifi_off();
		rtw_msleep_os(20);
		wifi_on(RTW_MODE_STA_AP);
	}
	else
#endif
	{
#if CONFIG_LWIP_LAYER
#if defined(CONFIG_MBED_ENABLED) || defined(PLATFORM_CUSTOMER_RTOS)
		//TODO
#else
#if LWIP_VERSION_MAJOR >= 2
		IP4_ADDR(ip_2_ip4(&ipaddr), GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
		IP4_ADDR(ip_2_ip4(&netmask), NETMASK_ADDR0, NETMASK_ADDR1 , NETMASK_ADDR2, NETMASK_ADDR3);
		IP4_ADDR(ip_2_ip4(&gw), GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
		netif_set_addr(pnetif, ip_2_ip4(&ipaddr), ip_2_ip4(&netmask),ip_2_ip4(&gw));
#else
		IP4_ADDR(&ipaddr, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
		IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1 , NETMASK_ADDR2, NETMASK_ADDR3);
		IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
		netif_set_addr(pnetif, &ipaddr, &netmask,&gw);
#endif
#endif
#endif
		wifi_off();
		rtw_msleep_os(20);
		wifi_on(RTW_MODE_AP);
	}
	// start ap
	if(wifi_start_ap((char*)ssid, security_type, (char*)password, ssid_len, password_len, channel) < 0) {
		RTW_API_INFO("\n\rERROR: Operation failed!");
		return -1;
	}

#if (INCLUDE_uxTaskGetStackHighWaterMark == 1)
	RTW_API_INFO("\r\nWebServer Thread: High Water Mark is %ld\n", uxTaskGetStackHighWaterMark(NULL));
#endif
#ifdef  CONFIG_CONCURRENT_MODE
	// connect to ap if wlan0 was linked with ap
	if(idx > 0 && sta_linked == 0){
		int ret;
		RTW_API_INFO("\r\nAP: ssid=%s", (char*)setting.ssid);
		RTW_API_INFO("\r\nAP: security_type=%d", setting.security_type);
		RTW_API_INFO("\r\nAP: password=%s", (char*)setting.password);
		RTW_API_INFO("\r\nAP: key_idx =%d\n", setting.key_idx);
		ret = wifi_connect((char*)setting.ssid,
									setting.security_type,
									(char*)setting.password,
									strlen((char*)setting.ssid),
									strlen((char*)setting.password),
									setting.key_idx,
									NULL);
#if CONFIG_DHCP_CLIENT
		if(ret == RTW_SUCCESS) {
			/* Start DHCPClient */
			LwIP_DHCP(0, DHCP_START);
		}
#endif
	}
#endif
#if defined(CONFIG_MBED_ENABLED)
	osThreadId_t id = osThreadGetId();
	RTW_API_INFO("\r\nWebServer Thread: High Water Mark is %ld\n", osThreadGetStackSpace(id));
#else
#if (INCLUDE_uxTaskGetStackHighWaterMark == 1)
	RTW_API_INFO("\r\nWebServer Thread: High Water Mark is %ld\n", uxTaskGetStackHighWaterMark(NULL));
#endif
#endif
#if CONFIG_LWIP_LAYER
#if defined(CONFIG_MBED_ENABLED) || defined(PLATFORM_CUSTOMER_RTOS)
	//TODO
#else
	// start dhcp server
	//dhcps_init(&xnetif[idx]);
#endif
#endif
#endif
	return 0;
}

#if CONFIG_AUTO_RECONNECT
extern void (*p_wlan_autoreconnect_hdl)(rtw_security_t, char*, int, char*, int, int);

struct wifi_autoreconnect_param {
	rtw_security_t security_type;
	char *ssid;
	int ssid_len;
	char *password;
	int password_len;
	int key_id;
};

#if defined(CONFIG_MBED_ENABLED) || defined(PLATFORM_CUSTOMER_RTOS)
void wifi_autoreconnect_hdl(rtw_security_t security_type,
                            char *ssid, int ssid_len,
                            char *password, int password_len,
                            int key_id)
{
	RTW_API_INFO("\n\r%s Not Implemented Yet!\n", __func__);
}
#else
static void wifi_autoreconnect_thread(void *param)
{
	int ret = RTW_ERROR;
	struct wifi_autoreconnect_param *reconnect_param = (struct wifi_autoreconnect_param *) param;
	RTW_API_INFO("\n\rauto reconnect ...\n");
	ret = wifi_connect(reconnect_param->ssid, reconnect_param->security_type, reconnect_param->password,
	                   reconnect_param->ssid_len, reconnect_param->password_len, reconnect_param->key_id, NULL);
#if CONFIG_LWIP_LAYER
	if(ret == RTW_SUCCESS) {
#if 0//ATCMD_VER == ATVER_2
		if (dhcp_mode_sta == 2){
			struct netif * pnetif = &xnetif[0];
			//LwIP_UseStaticIP(pnetif);
			//dhcps_init(pnetif);
                        dhcps_init();
		}
		else
#endif
		{
		    netif_set_up(&xnetif[0]); //eason add
			//LwIP_DHCP(0, DHCP_START);
			dhcp_stop(&xnetif[0]);
#if LWIP_AUTOIP
			/*uint8_t *ip = LwIP_GetIP(&xnetif[0]);
			if((ip[0] == 0) && (ip[1] == 0) && (ip[2] == 0) && (ip[3] == 0)) {
				RTW_API_INFO("\n\nIPv4 AUTOIP ...");
				LwIP_AUTOIP(&xnetif[0]);
			}*/
			autoip_set_struct(&xnetif[0], &ethNetifAutoip);
			autoip_start(&xnetif[0]);
#endif
		}
	}
#endif //#if CONFIG_LWIP_LAYER
	vTaskDelete(NULL);
}

void wifi_autoreconnect_hdl(rtw_security_t security_type,
                            char *ssid, int ssid_len,
                            char *password, int password_len,
                            int key_id)
{
	static struct wifi_autoreconnect_param param;
	param.security_type = security_type;
	param.ssid = ssid;
	param.ssid_len = ssid_len;
	param.password = password;
	param.password_len = password_len;
	param.key_id = key_id;
	xTaskCreate(wifi_autoreconnect_thread, (const char *)"wifi_autoreconnect", 512, &param, tskIDLE_PRIORITY + 1, NULL);
}
#endif

int wifi_config_autoreconnect(__u8 mode, __u8 retry_times, __u16 timeout)
{
    p_wlan_autoreconnect_hdl = wifi_autoreconnect_hdl;
    return wext_set_autoreconnect(WLAN0_NAME, mode, retry_times, timeout);
}

int wifi_set_autoreconnect(__u8 mode)
{
	p_wlan_autoreconnect_hdl = wifi_autoreconnect_hdl;
	return wifi_config_autoreconnect(mode, AUTO_RECONNECT_COUNT, AUTO_RECONNECT_INTERVAL);//default retry 8 times, timeout 5 seconds
}

int wifi_get_autoreconnect(__u8 *mode)
{
	return wext_get_autoreconnect(WLAN0_NAME, mode);
}
#endif

#if defined( CONFIG_ENABLE_AP_POLLING_CLIENT_ALIVE )&&( CONFIG_ENABLE_AP_POLLING_CLIENT_ALIVE == 1 )
extern void (*p_ap_polling_sta_hdl)(void *);
extern void (*p_ap_polling_sta_int_hdl)(void *, u16, u32 );
extern void ap_polling_sta_hdl(void *);
extern void ap_polling_sta_int_hdl(void *, u16, u32 );

void  wifi_set_ap_polling_sta(__u8 enabled)
{
	if(_TRUE == enabled)
	{
		p_ap_polling_sta_hdl = ap_polling_sta_hdl;
		p_ap_polling_sta_int_hdl = ap_polling_sta_int_hdl;
	}else
	{
		p_ap_polling_sta_hdl = NULL;
		p_ap_polling_sta_int_hdl = NULL;
	}
	return ;
}
#endif

#ifdef CONFIG_CUSTOM_IE
/*
 * Example for custom ie
 *
 * u8 test_1[] = {221, 2, 2, 2};
 * u8 test_2[] = {221, 2, 1, 1};
 * rtw_custom_ie_t buf[2] = {{test_1, PROBE_REQ},
 *		 {test_2, PROBE_RSP | BEACON}};
 * u8 buf_test2[] = {221, 2, 1, 3} ;
 * rtw_custom_ie_t buf_update = {buf_test2, PROBE_REQ};
 *
 * add ie list
 * static void cmd_add_ie(int argc, char **argv)
 * {
 *	 wifi_add_custom_ie((void *)buf, 2);
 * }
 *
 * update current ie
 * static void cmd_update_ie(int argc, char **argv)
 * {
 *	 wifi_update_custom_ie(&buf_update, 2);
 * }
 *
 * delete all ie
 * static void cmd_del_ie(int argc, char **argv)
 * {
 *	 wifi_del_custom_ie();
 * }
 */

int wifi_add_custom_ie(void *cus_ie, int ie_num)
{
	return wext_add_custom_ie(WLAN0_NAME, cus_ie, ie_num);
}


int wifi_update_custom_ie(void *cus_ie, int ie_index)
{
	return wext_update_custom_ie(WLAN0_NAME, cus_ie, ie_index);
}

int wifi_del_custom_ie()
{
	return wext_del_custom_ie(WLAN0_NAME);
}

#endif

#ifdef CONFIG_PROMISC
extern void promisc_init_packet_filter(void);
extern int promisc_add_packet_filter(u8 filter_id, rtw_packet_filter_pattern_t *patt, rtw_packet_filter_rule_t rule);
extern int promisc_enable_packet_filter(u8 filter_id);
extern int promisc_disable_packet_filter(u8 filter_id);
extern int promisc_remove_packet_filter(u8 filter_id);
void wifi_init_packet_filter()
{
	promisc_init_packet_filter();
}

int wifi_add_packet_filter(unsigned char filter_id, rtw_packet_filter_pattern_t *patt, rtw_packet_filter_rule_t rule)
{
	return promisc_add_packet_filter(filter_id, patt, rule);
}

int wifi_enable_packet_filter(unsigned char filter_id)
{
	return promisc_enable_packet_filter(filter_id);
}

int wifi_disable_packet_filter(unsigned char filter_id)
{
	return promisc_disable_packet_filter(filter_id);
}

int wifi_remove_packet_filter(unsigned char filter_id)
{
	return promisc_remove_packet_filter(filter_id);
}

extern void promisc_filter_by_ap_and_phone_mac(u8 enable, void *ap_mac, void *phone_mac);
void wifi_filter_by_ap_and_phone_mac(u8 enable, void *ap_mac, void *phone_mac)
{
	promisc_filter_by_ap_and_phone_mac(enable,ap_mac,phone_mac);
}
#endif

#ifdef CONFIG_AP_MODE
extern int wext_enable_forwarding(const char *ifname);
extern int wext_disable_forwarding(const char *ifname);
int wifi_enable_forwarding(void)
{
	return wext_enable_forwarding(WLAN0_NAME);
}

int wifi_disable_forwarding(void)
{
	return wext_disable_forwarding(WLAN0_NAME);
}
#endif

/* API to set flag for concurrent mode wlan1 issue_deauth when channel switched by wlan0
 * usage: wifi_set_ch_deauth(0) -> wlan0 wifi_connect -> wifi_set_ch_deauth(1)
 */
#ifdef CONFIG_CONCURRENT_MODE
int wifi_set_ch_deauth(__u8 enable)
{
	return wext_set_ch_deauth(WLAN1_NAME, enable);
}
#endif

void wifi_set_indicate_mgnt(int enable)
{
	wext_set_indicate_mgnt(enable);
	return;
}

#ifdef CONFIG_ANTENNA_DIVERSITY
int wifi_get_antenna_info(unsigned char *antenna)
{
	int ret = 0;

	char buf[32];
	rtw_memset(buf, 0, sizeof(buf));
	rtw_memcpy(buf, "get_ant_info", 12);
	ret = wext_private_command_with_retval(WLAN0_NAME, buf, buf, 32);
	sscanf(buf, "%d", antenna); // 0: main, 1: aux
	return ret;
}
#endif

#ifdef CONFIG_SW_MAILBOX_EN
int mailbox_to_wifi(u8 *data, u8 len)
{
	return wext_mailbox_to_wifi(WLAN0_NAME, data, len);
}
#endif
//----------------------------------------------------------------------------//
#ifdef CONFIG_WOWLAN
int wifi_wlan_redl_fw()
{
	int ret = 0;

	ret = wext_wlan_redl_fw(WLAN0_NAME);

	return ret;
}

int wifi_wowlan_ctrl(int enable)
{
	int ret = 0;

	ret = wext_wowlan_ctrl(WLAN0_NAME, enable);

	return ret;
}

#ifdef CONFIG_WOWLAN_CUSTOM_PATTERN
int wifi_wowlan_set_pattern(wowlan_pattern_t pattern)
{
	int ret = 0;

	ret = wext_wowlan_set_pattern(WLAN0_NAME, pattern);

	return ret;
}
#endif
int wifi_wowlan_unicast_wake_ctrl(unsigned char enable)
{
	int ret = 0;
	
	ret = wext_wowlan_unicast_wake_ctrl(WLAN0_NAME, enable);
	
	return ret;  
}
#endif
int wifi_enable_adaptivity(void)
{
	return wext_enable_adaptivity(WLAN0_NAME);
}

int wifi_disable_adaptivity(void)
{
	return wext_disable_adaptivity(WLAN0_NAME);
}

//----------------------------------------------------------------------------//
#endif	//#if CONFIG_WLAN
