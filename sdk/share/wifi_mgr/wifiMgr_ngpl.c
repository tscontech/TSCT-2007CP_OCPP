#if defined(CFG_NET_WIFI_SDIO_NGPL) || defined(CFG_NET_WIFI_SDIO_NGPL_8723DS)
#include <sys/ioctl.h>
#include "lwip/dhcp.h"
#include "wifiMgr.h"
#include "ite/ite_wifi.h"


/* Extern Function */
extern void         dhcps_init(void);
extern void         dhcps_deinit(void);
extern uint8_t*     LwIP_GetIP(struct netif *pnetif);
extern TickType_t   xTaskGetTickCount(void);
extern struct netif xnetif[NET_IF_NUM];
extern  int gScanStart8189 ;

/* Global Variable */
static pthread_t                         ClientModeTask, ProcessTask;

static sem_t                             semConnectStart, semConnectStop;
static pthread_mutex_t                   mutexALWiFi, mutexIni, mutexMode;

static sem_t                             semWPSStart, semWPSStop;
static pthread_mutex_t                   mutexALWiFiWPS;

static WIFIMGR_CONNSTATE_E               wifi_conn_state    = WIFIMGR_CONNSTATE_STOP;
static WIFIMGR_WORKSTATE_E               wifi_work_state    = WIFIMGR_WORKSTATE_FAILED;
static WIFIMGR_ECODE_E                   wifi_conn_ecode    = WIFIMGR_ECODE_SET_DISCONNECT;

static WIFI_MGR_SCANAP_LIST              gWifiMgrApList[WIFI_SCAN_LIST_NUMBER] = {0};

static WIFI_MGR_SETTING                  gWifiMgrSetting    = {0};
static struct net_device_info            gScanApInfo        = {0};

static struct timeval                   tvDHCP1 = {0, 0}, tvDHCP2     = {0, 0};

extern u8 auto_reconnect_running;

/* WifiMgr flags */
static WIFI_MGR_VAR                      gWifiMgrVar        =
{
    .WIFI_Mode              = WIFIMGR_MODE_CLIENT,
    .MP_Mode                = 0,
    .Need_Set               = false,
    .Pre_Scan               = false,
    .Start_Scan             = false,
    .WIFI_Init_Ready        = false,
    .SoftAP_Hidden          = false,
    .SoftAP_Init_Ready      = false,
    .WIFI_Terminate         = false,
    .WPA_Terminate          = false,
    .Cancel_Connect         = false,
    .Cancel_WPS             = false,
    .Is_WIFI_Available      = false
};


/* ================================================= */
/*
/* Static Functions */
/*
/* ================================================= */


static void
_WifiMgr_Create_Worker_Thread(pthread_t task, void *(*start_routine)(void *), void *arg)
{
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    //pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, WIFI_STACK_SIZE);
    pthread_create(&task, &attr, start_routine, arg);
}


// sta connect to ap
static int
_WifiMgr_Sta_Connect_Process(void)
{
    ITPEthernetSetting setting;
    unsigned long tick1 = xTaskGetTickCount();
    unsigned long tick2, tick3;

    int nRet = WIFIMGR_ECODE_OK, cRet = RTW_PENDING;
    int is_connected = 0, dhcp_available = 0;
    int phase = 0, nSecurity = -1, nAPCount = 0;
    bool is_ssid_match = false;
    unsigned long connect_cnt = 0;
    unsigned long retry_connect_cnt = 0, retry_dhcp_cnt = 0;
    char *ssid, *password;
    WIFI_MGR_SCANAP_LIST pList[64];

    rtw_security_t      security_type;
    int 				ssid_len;
    int 				password_len;
    int 				key_id;
    void				*semaphore;

    if (gWifiMgrVar.Cancel_Connect) {
        goto end;
    }

    ssid            = gWifiMgrVar.Ssid;
    password        = gWifiMgrVar.Password;
    security_type   = gWifiMgrVar.SecurityMode;

    if (gWifiMgrVar.MP_Mode) {
        printf("[ WIFIMGR ] Is MP mode, connect to default SSID.\r\n");
        // SSID
        snprintf(ssid,                  32, "%s", CFG_NET_WIFI_MP_SSID);
        // Password
        snprintf(password, RTW_MAX_PSK_LEN, "%s", CFG_NET_WIFI_MP_PASSWORD);
#ifdef DTMF_DEC_HAS_SECUMODE
        // Security mode
        snprintf(security_type,          8, "%s", CFG_NET_WIFI_MP_SECURITY);
#endif
    }
    //printf("disable autoconnect %d \n",auto_reconnect_running);    
    wifi_set_autoreconnect(0);

retry_scan:

    /* === Connection STEP 1: Get AP list in currently === */
    if(gWifiMgrVar.Pre_Scan) {
        /* Select SSID after user do scanning on touch panel */
        memcpy(pList, gWifiMgrApList, sizeof(WIFI_MGR_SCANAP_LIST)*WIFI_SCAN_LIST_NUMBER);
        nAPCount = gScanApInfo.apCnt;
    } else {
        /* Get WIFI list while the Wifimgr want to reconnect old SSID */
        if (strcmp(gWifiMgrVar.Ssid, "") != 0)
            nAPCount = WifiMgr_Get_Scan_AP_Info(pList);
        else
            goto end;
    }


    /* === Connection STEP 2: Find out the SSID you want to connect === */
    for (int i = 0; i < nAPCount; i++) {
		/* Find the match SSID */
        if (!strcmp(ssid, pList[i].ssidName)) {
            is_ssid_match = true;
            printf("[ WIFIMGR ] Wanna connect to [%s], list is matched [%s(0x%X)]\n",
                ssid, pList[i].ssidName, pList[i].securityMode);
            if (pList[i].securityMode != 0){
                security_type = pList[i].securityMode;
            } else {
                security_type = RTW_SECURITY_OPEN;
            }
            break;
        }
    }

    gWifiMgrVar.SecurityMode = security_type ;

    ssid_len = strlen((const char *)ssid);
    password_len = strlen((const char *)password);
    key_id = 0;
    semaphore = NULL;

retry:

    if (gWifiMgrVar.Cancel_Connect) {
        nRet = WIFIMGR_ECODE_CONNECT_ERROR;
        gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_CONNECTING_FAIL);

        goto end;
    }

    /* === Connection STEP 3: Connect to AP if the SSID is matched. Otherwise, we set retry or doing something === */
    if (is_ssid_match) {
    cRet = wifi_connect(ssid,
    			security_type,
    			password,
    			ssid_len,
    			password_len,
    			key_id,
    			semaphore);

    /* Connection Retry Handler */
        if (cRet == RTW_SUCCESS) {
            printf("[ WIFIMGR ] wifi_connect results: success\n");

            /* Connect OK!! Reset count and go to DHCP phase */
            retry_connect_cnt = 0;
        } else if (cRet != RTW_SUCCESS && (retry_connect_cnt < WIFIMGR_RECONNTECT_TIME)) {
            printf("[ WIFIMGR ] wifi_connect results: RTW Error Code(%d)\n", cRet);

            WifiMgr_Sta_Disconnect();
            usleep(100*1000);

            if (cRet == RTW_TIMEOUT) {
                /* If SSID can be found but connection was TIMEOUT, try SCAN and CONNECT again. */
                retry_connect_cnt++;

                printf("[ WIFIMGR ] wifi connect retry (%d/%d) times - Join bss timeout, maybe signal were too weak\n", retry_connect_cnt, WIFIMGR_RECONNTECT_TIME);
                gWifiMgrVar.Pre_Scan = false; //Renew the scan list,  avoid security changing suddenly.

                goto retry_scan;
            } else if (cRet == RTW_ERROR) {
                /* If SSID can be found but connection was ERROR, try SCAN and CONNECT again. */
                retry_connect_cnt++;

                printf("[ WIFIMGR ] wifi connect retry (%d/%d) times - Maybe it were AP issues(AP power off/Dirty channel/Error PW/...etc)\n", retry_connect_cnt, WIFIMGR_RECONNTECT_TIME);
                gWifiMgrVar.Pre_Scan = false; //Renew the scan list,  avoid security changing suddenly.

                goto retry_scan;
            } else {
                /* Another connection error code, just try CONNECT again */
                retry_connect_cnt++;

                printf("[ WIFIMGR ] wifi connect have another errors - RTW Error Code(%d)\n", cRet);

                goto retry;
            }
        } else {
            /* Retry to scan or connect is over times */
            if (WIFIMGR_RECONNTECT_INFINITE) {
                gWifiMgrVar.Pre_Scan = false; //Renew the scan list,  avoid security changing suddenly.
                retry_connect_cnt = 0;

                goto retry_scan;
            }

            printf("[ WIFIMGR ] wifi connect retry failed, quit this SSID(%s)\n", ssid);
            nRet = WIFIMGR_ECODE_CONNECT_ERROR;
            gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_CONNECTING_FAIL);

            goto end;
        }
    } else {
        printf("[%s] SSID is NOT match, don't do connection process\n", __FUNCTION__);

        if (WIFIMGR_RECONNTECT_INFINITE) {
            /* If SSID can NOT be found, try scan again. */
            retry_connect_cnt++;
            gWifiMgrVar.Pre_Scan = false; //Renew the scan list,  avoid security changing suddenly.
            printf("[%s] wifi connect retry (%d/Infinite) times - SSID can NOT be found\n", __FUNCTION__, retry_connect_cnt);

            goto retry_scan;
        }

        /* SSID can NOT be found, you can handle something if you want(ex: goto retry_scan).  */
        if (retry_connect_cnt < WIFIMGR_RECONNTECT_TIME && !WIFIMGR_RECONNTECT_INFINITE) {
            /* If SSID can NOT be found, try scan again. */
            retry_connect_cnt++;
            printf("[%s] wifi connect retry (%d/%d) times - SSID can NOT be found\n", __FUNCTION__, retry_connect_cnt, WIFIMGR_RECONNTECT_TIME);

            goto retry_scan;
        } else {
            // connect to hidden ap
            cRet = wifi_connect(ssid,
    			security_type,
    			password,
    			ssid_len,
    			password_len,
    			key_id,
    			semaphore);
             if (cRet != RTW_SUCCESS){
                printf("[ WIFIMGR ] wifi connect retry failed, quit this SSID(%s)\n", ssid);
	            /* Give up..., Reset count and go to DHCP phase */
	            retry_connect_cnt = 0;
                gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_CONNECTING_FAIL);
                
                goto end;
            }

        }
    }

    // dhcp
    setting.dhcp        = gWifiMgrSetting.setting.dhcp;

    // autoip
    setting.autoip      = gWifiMgrSetting.setting.autoip;

    // ipaddr
    setting.ipaddr[0]   = gWifiMgrSetting.setting.ipaddr[0];
    setting.ipaddr[1]   = gWifiMgrSetting.setting.ipaddr[1];
    setting.ipaddr[2]   = gWifiMgrSetting.setting.ipaddr[2];
    setting.ipaddr[3]   = gWifiMgrSetting.setting.ipaddr[3];

    // netmask
    setting.netmask[0]  = gWifiMgrSetting.setting.netmask[0];
    setting.netmask[1]  = gWifiMgrSetting.setting.netmask[1];
    setting.netmask[2]  = gWifiMgrSetting.setting.netmask[2];
    setting.netmask[3]  = gWifiMgrSetting.setting.netmask[3];

    // gateway
    setting.gw[0]       = gWifiMgrSetting.setting.gw[0];
    setting.gw[1]       = gWifiMgrSetting.setting.gw[1];
    setting.gw[2]       = gWifiMgrSetting.setting.gw[2];
    setting.gw[3]       = gWifiMgrSetting.setting.gw[3];

    printf("[ WIFIMGR ] ssid     = %s\r\n", ssid);
    printf("[ WIFIMGR ] password = %s\r\n", password);
    printf("[ WIFIMGR ] security_type = %s(0x%x) \r\n",
        ((security_type == RTW_SECURITY_OPEN ) ? "Open" :
        (security_type == RTW_SECURITY_WEP_PSK ) ? "WEP" :
        (security_type == RTW_SECURITY_WPA_TKIP_PSK ) ? "WPA TKIP" :
        (security_type == RTW_SECURITY_WPA_AES_PSK ) ? "WPA AES" :
        (security_type == RTW_SECURITY_WPA2_AES_PSK ) ? "WPA2 AES" :
        (security_type == RTW_SECURITY_WPA2_TKIP_PSK ) ? "WPA2 TKIP" :
        (security_type == RTW_SECURITY_WPA2_MIXED_PSK ) ? "WPA2 Mixed" :
        (security_type == RTW_SECURITY_WPA_WPA2_MIXED_PSK ) ? "WPA/WPA2 PSK" :
        (security_type == RTW_SECURITY_WPA_TKIP_8021X ) ? "WPA TKIP 8021X" :
        (security_type == RTW_SECURITY_WPA_AES_8021X ) ? "WPA AES 8021X" :
        (security_type == RTW_SECURITY_WPA2_AES_8021X ) ? "WPA2 AES 8021X" :
        (security_type == RTW_SECURITY_WPA2_TKIP_8021X ) ? "WPA2 TKIP 8021X" :
        (security_type == RTW_SECURITY_WPA_WPA2_MIXED_8021X ) ? "WPA/WPA2 8021X" : "Unknown"), security_type);

    if (gWifiMgrVar.Cancel_Connect) {
        goto end;
    }

    if (strlen(ssid) == 0)
    {
        printf("[ WIFIMGR ] %s() L#%ld: Error! Wifi setting has no SSID\r\n", __FUNCTION__, __LINE__);
        nRet = WIFIMGR_ECODE_NO_SSID;
        goto end;
    }

#if defined(CFG_NET_ETHERNET) && defined(CFG_NET_WIFI_USB)
    printf("[WIFIMGR] check wifi netif %d \n",ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_NETIF_STATUS, NULL));
    // Check if the wifi netif is exist
    if (ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_NETIF_STATUS, NULL) == 0) {
        printf("[ WIFIMGR ] %s() L#%ld: wifi need to add netif !\r\n", __FUNCTION__, __LINE__);
        ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_ADD_NETIF, NULL);
    }
#endif

    if (gWifiMgrVar.Cancel_Connect) {
        goto end;
    }

    // Wait for connecting...
    printf("[ WIFIMGR ] [ DHCP %s ]Wait for connecting\n", setting.dhcp ? "Enable":"Disable");
    if (setting.dhcp) {
        ip_addr_set_zero(&xnetif[0].ip_addr);
        // Wait for DHCP setting...
        printf("[ WIFIMGR ] Wait for DHCP setting");

#ifdef CFG_NET_LWIP_2
        if (!netif_is_up(&xnetif[0]))
            netif_set_up(&xnetif[0]);
#endif

        dhcp_start(&xnetif[0]);
        tick3 = xTaskGetTickCount();
        unsigned char *ip = LwIP_GetIP(&xnetif[0]);
        printf("\r\n\n[ WIFIMGR ] IP set zero.\nGot IP after %dms.\n", (tick3-tick1));

        connect_cnt = WIFI_CONNECT_DHCP_COUNT;
        while (connect_cnt)
        {
            ip = LwIP_GetIP(&xnetif[0]);

            if (ip[0]!=0)
            {
                printf("\r\n[ WIFIMGR ] DHCP setting OK\r\n");
                dhcp_available = 1;
                gWifiMgrVar.Pre_Scan = false;
                break;
            }
            putchar('.');
            fflush(stdout);
            connect_cnt--;
            if (connect_cnt == 0)
            {
                if ((security_type != RTW_SECURITY_UNKNOWN || security_type != RTW_SECURITY_OPEN) &&
                    retry_dhcp_cnt < WIFIMGR_REOFFER_DHCP_TIME)
                {
                    printf("\r\n[ WIFIMGR ] %s() L#%ld: DHCP timeout! Goto retry(retry %d times).\r\n", __FUNCTION__, __LINE__, retry_dhcp_cnt);
                    retry_dhcp_cnt++;
                    goto retry;
                }
                else
                {
                    printf("\r\n[ WIFIMGR ] %s() L#%ld: DHCP timeout! connect fail!\r\n", __FUNCTION__, __LINE__);
                    WifiMgr_Sta_Cancel_Connect();
                    nRet = WIFIMGR_ECODE_DHCP_ERROR;
                    goto end;
                }
            }

            if (gWifiMgrVar.Cancel_Connect || gWifiMgrVar.WIFI_Terminate)
            {
                goto end;
            }

            usleep(100000);
        }

        if (gWifiMgrVar.Cancel_Connect)
        {
            goto end;
        }
    }
    else
    {
        printf("[ WIFIMGR ] Manual setting IP\n");
        ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_RESET, &setting);
        dhcp_available = 1;
        ioctl(ITP_DEVICE_WIFI_NGPL, ITP_IOCTL_GET_INFO, NULL);
    }

    if (dhcp_available)
    {
        //usleep(1000*1000*5); //workaround random miss frames issue for cisco router
        printf("[ WIFIMGR ] wifi_set_autoreconnect \n");
        wifi_set_autoreconnect(2);


        while (!ioctl(ITP_DEVICE_WIFI_NGPL, ITP_IOCTL_IS_AVAIL, NULL)){
            usleep(100*1000);
        }

        ioctl(ITP_DEVICE_WIFI_NGPL, ITP_IOCTL_GET_INFO, NULL);

        if (gWifiMgrSetting.wifiCallback)
            gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CONNECTION_FINISH);
    }

    // start dhcp count
    gettimeofday(&tvDHCP1, NULL);


end:
    if (gWifiMgrVar.Cancel_Connect)
    {
        printf("[ WIFIMGR ] %s() L#%ld: End. Cancel_Connect is set.\r\n", __FUNCTION__, __LINE__);

        if (gWifiMgrSetting.wifiCallback)
            gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_CONNECTING_CANCEL);
    }

	/* Sava last time connect info otherwise WIFI can't reconnect for wake up */
    if (gWifiMgrSetting.wifiCallback)
        gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_SLEEP_SAVE_INFO);

    return nRet;
}


static int
_WifiMgr_Sta_Connect_Post(void)
{
    int nRet = WIFIMGR_ECODE_OK;

    if (!gWifiMgrVar.WIFI_Init_Ready)
        return WIFIMGR_ECODE_NOT_INIT;

    pthread_mutex_lock(&mutexALWiFi);
    if (wifi_conn_state == WIFIMGR_CONNSTATE_STOP) {
        gWifiMgrVar.Need_Set = false;
        sem_post(&semConnectStart);
    }
    pthread_mutex_unlock(&mutexALWiFi);

    return nRet;
}


static inline int
_WifiMgr_Sta_Compare_AP_MAC(int list_1, int list_2)
{
    if (gWifiMgrApList[list_1].apMacAddr != NULL && gWifiMgrApList[list_2].apMacAddr != NULL){
        return (gWifiMgrApList[list_1].apMacAddr[0] == gWifiMgrApList[list_2].apMacAddr[0]) && \
                       (gWifiMgrApList[list_1].apMacAddr[1] == gWifiMgrApList[list_2].apMacAddr[1])&&\
                       (gWifiMgrApList[list_1].apMacAddr[2] == gWifiMgrApList[list_2].apMacAddr[2])&&\
                       (gWifiMgrApList[list_1].apMacAddr[3] == gWifiMgrApList[list_2].apMacAddr[3])&&\
                       (gWifiMgrApList[list_1].apMacAddr[4] == gWifiMgrApList[list_2].apMacAddr[4])&&\
                       (gWifiMgrApList[list_1].apMacAddr[5] == gWifiMgrApList[list_2].apMacAddr[5]);
    } else {
        return -1;
    }
}


static inline void
_WifiMgr_Sta_Entry_Info(char* ssid, char* password, unsigned long secumode)
{

    if (ssid){
        // SSID
        if(strlen(ssid)<WIFI_SSID_MAXLEN){
            strcpy(gWifiMgrVar.Ssid,ssid);
        } else {
            printf("[Wifi mgr]_WifiMgr_Sta_Entry_Info can't copy ssid %d \n",__LINE__);
        }
    }

    if (password){
        // Password
        if(strlen(password)<WIFI_PASSWORD_MAXLEN){
            strcpy(gWifiMgrVar.Password,password);
        } else {
            printf("[Wifi mgr]_WifiMgr_Sta_Entry_Info can't copy password %d \n",__LINE__);
        }
        
    }

    if (secumode){
        // Security mode
        gWifiMgrVar.SecurityMode = secumode;
    }
}


static void
_WifiMgr_Sta_List_Swap(int x, int y)
{
    WIFI_MGR_SCANAP_LIST temp;

    memcpy(&temp,&gWifiMgrApList[x],sizeof(WIFI_MGR_SCANAP_LIST));
    memcpy(&gWifiMgrApList[x],&gWifiMgrApList[y],sizeof(WIFI_MGR_SCANAP_LIST));
    memcpy(&gWifiMgrApList[y],&temp,sizeof(WIFI_MGR_SCANAP_LIST));
}


static void
_WifiMgr_Sta_List_Sort_Insert(int size)
{
    int i,j;
    for(i = 0; i < size; i++){
        for(j = i; j > 0; j--){
            if(gWifiMgrApList[j].rfQualityRSSI > gWifiMgrApList[j - 1].rfQualityRSSI){
                _WifiMgr_Sta_List_Swap(j, j-1);
            }
        }
    }
}


static int
_WifiMgr_Sta_Remove_Same_SSID(int size)
{
    int i, j, mac_cmp;
    WIFI_MGR_SCANAP_LIST gWifiMgrTempApList[WIFI_SCAN_LIST_NUMBER] = {0};

    if (size < 1){
        return size;
    }

    for (i=size-1 ; i>0 ; i--){
        for (j = i ; j >=0 ; j --){
            if (strcmp(gWifiMgrApList[i].ssidName , gWifiMgrApList[j].ssidName)==0 && i!=j){
                //set power =0 , if the same ssid
#if WIFIMGR_REMOVE_ALL_SAME_SSID
                if (gWifiMgrApList[i].rfQualityQuant < gWifiMgrApList[j].rfQualityQuant ? 1:0)
                    gWifiMgrApList[i].rfQualityQuant = 0;
                else
                    gWifiMgrApList[j].rfQualityQuant = 0;
#else
                if (_WifiMgr_Sta_Compare_AP_MAC(i, j))
                    gWifiMgrApList[i].rfQualityQuant = 0;
#endif
            }
        }
    }

    for (i = 0 , j =0 ; i < size ; i ++){
        if (gWifiMgrApList[i].rfQualityQuant > 0){
            memcpy(&gWifiMgrTempApList[j], &gWifiMgrApList[i], sizeof(WIFI_MGR_SCANAP_LIST));
            j++;
        }
    }

#if 0
    printf("RemoveSameSsid %d \n",j);
    for (i = 0; i < j; i++)
    {
        printf("[Wifi mgr] ssid = %32s, securityOn = %ld, securityMode = %ld, avgQuant = %d, avgRSSI = %d , <%02x:%02x:%02x:%02x:%02x:%02x>\r\n", gWifiMgrTempApList[i].ssidName, gWifiMgrTempApList[i].securityOn, gWifiMgrTempApList[i].securityMode,gWifiMgrTempApList[i].rfQualityQuant, gWifiMgrTempApList[i].rfQualityRSSI,
        gWifiMgrTempApList[i].apMacAddr[0], gWifiMgrTempApList[i].apMacAddr[1], gWifiMgrTempApList[i].apMacAddr[2], gWifiMgrTempApList[i].apMacAddr[3], gWifiMgrTempApList[i].apMacAddr[4], gWifiMgrTempApList[i].apMacAddr[5]);
    }
    printf("RemoveSameSsid -----\n\n");
#endif
    memset(gWifiMgrApList, 0,                  sizeof(WIFI_MGR_SCANAP_LIST)*WIFI_SCAN_LIST_NUMBER);
    memcpy(gWifiMgrApList, gWifiMgrTempApList, sizeof(WIFI_MGR_SCANAP_LIST)*WIFI_SCAN_LIST_NUMBER);
    return j;
}


static void
_WifiMgr_Sta_Scan_Result_Print( rtw_scan_result_t* record )
{
    printf( "%s\n ", __FUNCTION__);
    printf( "%s\t ", ( record->bss_type == RTW_BSS_TYPE_ADHOC ) ? "Adhoc" : "Infra" );
    // printf( MAC_FMT, MAC_ARG(record->BSSID.octet) );
    printf( " %d\t ", record->signal_strength );
    printf( " %d\t  ", record->channel );
    printf( " %d\t  ", record->wps_type );
    printf( "%s\t\t ", ( record->security == RTW_SECURITY_OPEN ) ? "Open" :
        (record->security == RTW_SECURITY_WEP_PSK ) ? "WEP" :
        (record->security == RTW_SECURITY_WPA_TKIP_PSK ) ? "WPA TKIP" :
        (record->security == RTW_SECURITY_WPA_AES_PSK ) ? "WPA AES" :
        (record->security == RTW_SECURITY_WPA2_AES_PSK ) ? "WPA2 AES" :
        (record->security == RTW_SECURITY_WPA2_TKIP_PSK ) ? "WPA2 TKIP" :
        (record->security == RTW_SECURITY_WPA2_MIXED_PSK ) ? "WPA2 Mixed" :
        (record->security == RTW_SECURITY_WPA_WPA2_MIXED_PSK ) ? "WPA/WPA2 PSK" :
        (record->security == RTW_SECURITY_WPA_TKIP_8021X ) ? "WPA TKIP 8021X" :
        (record->security == RTW_SECURITY_WPA_AES_8021X ) ? "WPA AES 8021X" :
        (record->security == RTW_SECURITY_WPA2_AES_8021X ) ? "WPA2 AES 8021X" :
        (record->security == RTW_SECURITY_WPA2_TKIP_8021X ) ? "WPA2 TKIP 8021X" :
        (record->security == RTW_SECURITY_WPA_WPA2_MIXED_8021X ) ? "WPA/WPA2 8021X" : "Unknown");

    printf( " %s ", record->SSID.val );
    printf( "\r\n" );
}


static rtw_result_t
_WifiMgr_Sta_Scan_Result_Handler( rtw_scan_handler_result_t* malloced_scan_result )
{
    static int ApNum = 0;
	unsigned char convert_quant;
    void *_user_data;
    struct net_device_info *apInfo = NULL;

    //	netDeviceInfo->apList[i].rfQualityRSSI = iwEvt.u.qual.level;
    //printf("====>%s: scan_complete(%d)\n", __FUNCTION__, malloced_scan_result->scan_complete);

    if (malloced_scan_result->scan_complete != RTW_TRUE) {
        rtw_scan_result_t* record = &malloced_scan_result->ap_details;
        record->SSID.val[record->SSID.len] = 0; /* Ensure the SSID is null terminated */

        _user_data = malloced_scan_result->user_data;
        apInfo = (struct net_device_info *)_user_data;

		/* SSID */
        if (record->SSID.len > 0 && record->SSID.len <= WIFI_SSID_MAXLEN) {
            memcpy(apInfo->apList[apInfo->apCnt].ssidName, record->SSID.val ,WIFI_SSID_MAXLEN);
        }

		/* MAC */
		memcpy(apInfo->apList[apInfo->apCnt].apMacAddr, record->BSSID.octet, 6*sizeof(unsigned char));

		/* Power Level in dBm*/
        if (record->signal_strength < 0)
            apInfo->apList[apInfo->apCnt].rfQualityRSSI =  record->signal_strength;

		/* Signal Quality */
		if (record->signal_strength <= -100)
			convert_quant = 0;
		else if(record->signal_strength >= -30)
			convert_quant = 100;
		else
			convert_quant = 150 + 1.67*(record->signal_strength);

		apInfo->apList[apInfo->apCnt].rfQualityQuant = convert_quant;

		/* Security */
        apInfo->apList[apInfo->apCnt].securityMode = record->security; //unsigned long

		/* Channel */
        apInfo->apList[apInfo->apCnt].channelId = record->channel ;

        //printf("apCnt %d, %s[%02x:%02x:%02x:%02x:%02x:%02x]\n", apInfo->apCnt, apInfo->apList[apInfo->apCnt].ssidName, MAC_ARG(apInfo->apList[apInfo->apCnt].apMacAddr));
        apInfo->apCnt ++;
        //RTW_API_INFO( "%d\t ", ++ApNum );
        //_WifiMgr_Sta_Scan_Result_Print(record);
    } else{
        ApNum = 0;
    }
    gWifiMgrVar.Start_Scan = false;
    return RTW_SUCCESS;
}


static int
_WifiMgr_Sta_Scan_Process(struct net_device_info *apInfo)
{
    int nRet = 0, scan_result;
    int nWifiState = 0;
    int i = 0;
    int nHideSsid = 0;
    if (!gWifiMgrVar.WIFI_Init_Ready) {
        printf("scanWifiAp  !WIFI_Init_Ready \n ");
        return WIFIMGR_ECODE_NOT_INIT;
    }
    //printf("disable autoconnect %d \n",auto_reconnect_running);    
    wifi_set_autoreconnect(0);
    usleep(200*1000);
    memset(apInfo, 0, sizeof(struct net_device_info));

    printf("[ WIFIMGR ] %s: Start to SCAN AP ==========================\r\n", __FUNCTION__);

	scan_result = wifi_scan_networks(_WifiMgr_Sta_Scan_Result_Handler, apInfo);

    if(scan_result == RTW_SUCCESS){
		gWifiMgrVar.Pre_Scan = true;
	}else{
        printf("\n\rERROR: wifi scan failed, result code(%d).\n", scan_result);
        return WIFIMGR_ECODE_NOT_INIT;
    }

    gWifiMgrVar.Start_Scan = true;

    while (1)
    {
        nWifiState = (int)gWifiMgrVar.Start_Scan;
        //printf("[Presentation]%s() nWifiState=0x%X\r\n", __FUNCTION__, nWifiState);
        if (nWifiState == 0)
        {
            // scan finish
            printf("[ WIFIMGR ] %s: Scan AP Finish!\r\n", __FUNCTION__);
            break;
        }
        
                
        usleep(100 * 1000);
    }

    printf("[ WIFIMGR ] %s: ScanApInfo.apCnt = %ld\r\n", __FUNCTION__, apInfo->apCnt);

    for (i = 0; i < apInfo->apCnt; i++)
    {
        unsigned int ssid_len = strlen(apInfo->apList[i].ssidName);
        /* Avoid the SSID length is shorter than 32, and the RSSI is less than 0. */
        if (ssid_len > 0 && ssid_len < 33 && apInfo->apList[i].rfQualityRSSI < 0)
        {
            gWifiMgrApList[i].channelId = apInfo->apList[i].channelId;
            gWifiMgrApList[i].operationMode = apInfo->apList[i].operationMode ;
            gWifiMgrApList[i].rfQualityQuant = apInfo->apList[i].rfQualityQuant;
            gWifiMgrApList[i].rfQualityRSSI = apInfo->apList[i].rfQualityRSSI;
            gWifiMgrApList[i].securityMode = apInfo->apList[i].securityMode;
            /* For security */
            memcpy(gWifiMgrApList[i].apMacAddr, apInfo->apList[i].apMacAddr, 6);
            memcpy(gWifiMgrApList[i].ssidName,  apInfo->apList[i].ssidName, 32);
        } else {
            nHideSsid ++;
        }
    }

#if WIFIMGR_SHOW_SCAN_LIST
    apInfo->apCnt = apInfo->apCnt - nHideSsid;

    _WifiMgr_Sta_List_Sort_Insert(apInfo->apCnt);

    apInfo->apCnt = _WifiMgr_Sta_Remove_Same_SSID(apInfo->apCnt);

    for (i = 0; i < apInfo->apCnt; i++)
    {
        printf("[ WIFIMGR ] SSID = %32s, securityMode =  %16s, avgQuant = %4d %%, power = %4d dBm , <%02X:%02X:%02X:%02X:%02X:%02X>\r\n",
			gWifiMgrApList[i].ssidName,
			//gWifiMgrApList[i].securityMode,
			((gWifiMgrApList[i].securityMode == RTW_SECURITY_OPEN ) ? "Open" :
			(gWifiMgrApList[i].securityMode == RTW_SECURITY_WEP_PSK ) ? "WEP" :
			(gWifiMgrApList[i].securityMode == RTW_SECURITY_WPA_TKIP_PSK ) ? "WPA TKIP" :
			(gWifiMgrApList[i].securityMode == RTW_SECURITY_WPA_AES_PSK ) ? "WPA AES" :
			(gWifiMgrApList[i].securityMode == RTW_SECURITY_WPA2_AES_PSK ) ? "WPA2 AES" :
			(gWifiMgrApList[i].securityMode == RTW_SECURITY_WPA2_TKIP_PSK ) ? "WPA2 TKIP" :
			(gWifiMgrApList[i].securityMode == RTW_SECURITY_WPA2_MIXED_PSK ) ? "WPA2 Mixed" :
            (gWifiMgrApList[i].securityMode == RTW_SECURITY_WPA_WPA2_MIXED_PSK ) ? "WPA/WPA2 PSK" :
            (gWifiMgrApList[i].securityMode == RTW_SECURITY_WPA_TKIP_8021X ) ? "WPA TKIP 8021X" :
            (gWifiMgrApList[i].securityMode == RTW_SECURITY_WPA_AES_8021X ) ? "WPA AES 8021X" :
            (gWifiMgrApList[i].securityMode == RTW_SECURITY_WPA2_AES_8021X ) ? "WPA2 AES 8021X" :
            (gWifiMgrApList[i].securityMode == RTW_SECURITY_WPA2_TKIP_8021X ) ? "WPA2 TKIP 8021X" :
            (gWifiMgrApList[i].securityMode == RTW_SECURITY_WPA_WPA2_MIXED_8021X ) ? "WPA/WPA2 8021X" : "Unknown"),
			gWifiMgrApList[i].rfQualityQuant, gWifiMgrApList[i].rfQualityRSSI,
        	gWifiMgrApList[i].apMacAddr[0], gWifiMgrApList[i].apMacAddr[1], gWifiMgrApList[i].apMacAddr[2],
        	gWifiMgrApList[i].apMacAddr[3], gWifiMgrApList[i].apMacAddr[4], gWifiMgrApList[i].apMacAddr[5]);
    }
#endif
    //printf("wifi_is_connected_to_ap %d \n",wifi_is_connected_to_ap());    
    if (wifi_is_connected_to_ap() ==0){
        wifi_set_autoreconnect(2);
    }
    printf("[ WIFIMGR ] %s: End to SCAN AP ============================\r\n", __FUNCTION__);
    return apInfo->apCnt;
}


static void*
_WifiMgr_Sta_Thread(void* arg)
{
    int nRet = WIFIMGR_ECODE_OK;
    gWifiMgrVar.IS_WifiMgr_Sta_Thread = true;
    while (1)
    {
        printf("[ WIFIMGR ] ClientModeThreadFunc\n");
        sem_wait(&semConnectStart);


        if (gWifiMgrVar.WIFI_Terminate) {
            gWifiMgrVar.IS_WifiMgr_Sta_Thread = false;
            printf("[ WIFIMGR ] terminate _WifiMgr_Sta_Thread(0) \n");
            break;
        }

        if (gWifiMgrVar.Need_Set){
            wifi_conn_state = WIFIMGR_CONNSTATE_SETTING;
            printf("[ WIFIMGR ] START to Set!\r\n");
            wifi_conn_ecode = nRet = WIFIMGR_ECODE_OK;

            gWifiMgrVar.Need_Set = false;
            printf("[ WIFIMGR ] Set finish!\r\n");
        }
        usleep(1000);
        printf("[ WIFIMGR ] nRet: %d\n", nRet);

		if (strcmp(gWifiMgrVar.Ssid, "") == 0)
			nRet = WIFIMGR_ECODE_NO_SSID;
		else
			nRet = WIFIMGR_ECODE_OK;

        if (nRet == WIFIMGR_ECODE_OK) {
            wifi_conn_state = WIFIMGR_CONNSTATE_CONNECTING;

            printf("[ WIFIMGR ] START to Connect!\r\n");

            gWifiMgrVar.Cancel_WPS = true;
            wifi_conn_ecode = _WifiMgr_Sta_Connect_Process();
            gWifiMgrVar.Cancel_WPS = false;
            printf("[ WIFIMGR ] Connect finish!\r\n");

        }
        wifi_conn_state = WIFIMGR_CONNSTATE_STOP;
        usleep(1000);
    }
end:
    wifi_conn_state = WIFIMGR_CONNSTATE_STOP;
    gWifiMgrVar.IS_WifiMgr_Sta_Thread = false;
    return NULL;
}


static inline void
_WifiMgr_HostAP_Entry_Info(char* ssid, char* password)
{
    if (ssid){
        // SSID
        snprintf(gWifiMgrVar.ApSsid, WIFI_SSID_MAXLEN, ssid);
    }
    if (password){
        // Password
        snprintf(gWifiMgrVar.ApPassword, WIFI_PASSWORD_MAXLEN, password);
    }
}


static int
_WifiMgr_HostAP_Init(void)
{
    int nRet = WIFIMGR_ECODE_OK;
    ITPWifiInfo wifiInfo;

    ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFIAP_ENABLE, NULL);

    if (gWifiMgrVar.SoftAP_Hidden){
        //wifi_start_ap_with_hidden_ssid("AP_Mode",RTW_SECURITY_WPA2_AES_PSK,"12345678",7,8,4);
        wifi_start_ap_with_hidden_ssid(gWifiMgrVar.ApSsid,RTW_SECURITY_WPA2_AES_PSK,gWifiMgrVar.ApPassword,strlen(gWifiMgrVar.ApSsid),strlen(gWifiMgrVar.ApPassword),4);
    } else {
        //wifi_start_ap("AP_Mode",RTW_SECURITY_WPA2_AES_PSK,"12345678",7,8,4);
        wifi_start_ap(gWifiMgrVar.ApSsid,RTW_SECURITY_WPA2_AES_PSK,gWifiMgrVar.ApPassword,strlen(gWifiMgrVar.ApSsid),strlen(gWifiMgrVar.ApPassword),4);
    }
    dhcps_init();

    usleep(1000);

    return nRet;
}


static int
_WifiMgr_HostAP_Terminate(void)
{
    int nRet = WIFIMGR_ECODE_OK;

    dhcps_deinit();
    usleep(1000*10);

    ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFIAP_DISABLE, NULL);
    usleep(1000*1000);

    return nRet;
}


static int
_WifiMgr_WPS_Connect_Post(void)
{
    int nRet = WIFIMGR_ECODE_OK;

    if (!gWifiMgrVar.WIFI_Init_Ready) {
        return WIFIMGR_ECODE_NOT_INIT;
    }

    pthread_mutex_lock(&mutexALWiFi);
    if (wifi_conn_state == WIFIMGR_CONNSTATE_STOP) {
        gWifiMgrVar.Need_Set = true;
        sem_post(&semWPSStart);
//        sem_post(&semConnectStart);
    }
    pthread_mutex_unlock(&mutexALWiFi);

    return nRet;
}


static int
_WifiMgr_WPS_Init(void)
{
    int nRet = WIFIMGR_ECODE_OK;
    struct net_device_config netCfg = {0};
    unsigned long connect_cnt = 0;
    int is_connected = 0, dhcp_available = 0;
    ITPWifiInfo wifiInfo;
    ITPEthernetSetting setting;

    struct net_device_config wpsNetCfg = {0};
    int len = 0;
    char ssid[WIFI_SSID_MAXLEN];
    char password[WIFI_PASSWORD_MAXLEN];

    if (gWifiMgrVar.Cancel_WPS) {
        goto end;
    }

    netCfg.operationMode = WLAN_MODE_STA;
    memset(netCfg.ssidName, 0, sizeof(netCfg.ssidName));
    netCfg.securitySuit.securityMode = WLAN_SEC_WPS;

    if (ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_IS_CONNECTED, NULL)) {

        usleep(1000*100);
        // dhcp
        setting.dhcp = 0;

        // autoip
        setting.autoip = 0;

        // ipaddr
        setting.ipaddr[0] =0;
        setting.ipaddr[1] = 0;
        setting.ipaddr[2] = 0;
        setting.ipaddr[3] = 0;

        // netmask
        setting.netmask[0] = 0;
        setting.netmask[1] = 0;
        setting.netmask[2] = 0;
        setting.netmask[3] = 0;

        // gateway
        setting.gw[0] = 0;
        setting.gw[1] = 0;
        setting.gw[2] = 0;
        setting.gw[3] = 0;

        ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_RESET, &setting);

    }


    if (gWifiMgrVar.Cancel_WPS)
    {
        goto end;
    }

    // Wait for connecting...
    printf("[WIFIMGR WPS] Wait for connecting");
    connect_cnt = WIFI_CONNECT_COUNT;
    while (connect_cnt)
    {
        putchar('.');
        fflush(stdout);
        connect_cnt--;
        if (connect_cnt == 0) {
            printf("\r\n[WIFIMGR WPS]%s() L#%ld: Timeout! Cannot connect to WIFI AP!\r\n", __FUNCTION__, __LINE__);
            break;
        }

        if (gWifiMgrVar.Cancel_WPS) {
            goto end;
        }

        usleep(100000);
    }

    if (!is_connected) {
        printf("[WIFIMGR WPS]%s() L#%ld: Error! Cannot connect to WiFi AP!\r\n", __FUNCTION__, __LINE__);
        nRet = WIFIMGR_ECODE_CONNECT_ERROR;
        goto end;
    }

    if (gWifiMgrVar.Cancel_WPS) {
        goto end;
    }

    // Wait for DHCP setting...
    printf("[WIFIMGR WPS] Wait for DHCP setting");
    ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_START_DHCP, NULL);
    connect_cnt = WIFI_CONNECT_COUNT;
    while (connect_cnt)
    {
        if (ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_IS_AVAIL, NULL)) {
            printf("\r\n[WIFIMGR WPS] DHCP setting OK\r\n");
            dhcp_available = 1;
            break;
        }
        putchar('.');
        fflush(stdout);
        connect_cnt--;
        if (connect_cnt == 0) {
            printf("\r\n[WIFIMGR WPS]%s() L#%ld: DHCP timeout! connect fail!\r\n", __FUNCTION__, __LINE__);
            nRet = WIFIMGR_ECODE_DHCP_ERROR;
            goto end;
        }

        if (gWifiMgrVar.Cancel_WPS) {
            goto end;
        }
        usleep(100000);
    }

    if (dhcp_available)
    {
        // trim the " char
        memset(ssid, 0, WIFI_SSID_MAXLEN);
        len = strlen(wpsNetCfg.ssidName);
        memcpy(ssid, wpsNetCfg.ssidName + 1, len - 2);
        memset(password, 0, WIFI_PASSWORD_MAXLEN);
        len = strlen(wpsNetCfg.securitySuit.preShareKey);
        memcpy(password, wpsNetCfg.securitySuit.preShareKey + 1, len - 2);

        printf("[WIFIMGR WPS] WPS Info:\r\n");
        printf("[WIFIMGR WPS] WPS SSID     = %s\r\n", ssid);
        printf("[WIFIMGR WPS] WPS Password = %s\r\n", password);
        printf("[WIFIMGR WPS] WPS Security = %ld\r\n", wpsNetCfg.securitySuit.securityMode);
    }

    end:

    if (gWifiMgrVar.Cancel_WPS)
    {
        printf("[WIFIMGR WPS]%s() L#%ld: End. gWifiMgrVar.Cancel_WPS is set.\r\n", __FUNCTION__, __LINE__);
    }

    return nRet;
}


static void*
_WifiMgr_WPS_Thread(void* arg)
{
    int nRet = WIFIMGR_ECODE_OK;

    while (1)
    {
        sem_wait(&semWPSStart);
        if (gWifiMgrVar.WIFI_Terminate) {
            printf("[ WIFIMGR ] terminate WifiMgr_WPS_ThreadFunc \n");
            break;
        }

        printf("[ WIFIMGR ] START to Connect WPS!\r\n");
        wifi_conn_ecode = _WifiMgr_WPS_Init();
        printf("[ WIFIMGR ] Connect WPS finish!\r\n");


        usleep(1000);
    }

    return NULL;
}

static void*
_WifiMgr_Main_Process_Thread(void *arg)
{
    int nRet = 0;
    int bIsAvail = 0, nWiFiConnState = 0, nWiFiConnEcode = 0;
    int nPlayState = 0;
    WIFIMGR_CONNSTATE_E gWifi_connstate = WIFIMGR_CONNSTATE_STOP;

    int nCheckCnt = WIFIMGR_CHECK_WIFI_MSEC;
    static struct timeval tv1 = {0, 0}, tv2 = {0, 0};
    static struct timeval tv3_temp = {0, 0}, tv4_temp = {0, 0};
    long temp_disconn_time = 0;
    int wifi_mode_now = 0, is_softap_ready = 1;


    gWifiMgrVar.Is_First_Connect = true;
    gWifiMgrVar.Is_Temp_Disconnect = false;
    gWifiMgrVar.No_Config_File = false;
    gWifiMgrVar.No_SSID = false;
    gWifiMgrVar.IS_WifiMgr_Main_Process_Thread= true;

    usleep(20000);
    printf("[ WIFIMGR ] ProcessThreadFunc\n");

    while (1)
    {
        nCheckCnt--;
        if (gWifiMgrVar.WIFI_Terminate) {
            printf("[ WIFIMGR ] terminate WifiMgr_Process_ThreadFunc \n");
            gWifiMgrVar.IS_WifiMgr_Main_Process_Thread = false;
            break;
        }

        usleep(1000);
        if (nCheckCnt == 0) {
            wifi_mode_now = WifiMgr_Get_WIFI_Mode();

            if (wifi_mode_now == WIFIMGR_MODE_SOFTAP){
                // Soft AP mode
                if (!gWifiMgrVar.SoftAP_Init_Ready) {
                    printf("[ WIFIMGR ] %s() L#%ld: is_softap_ready=%ld\r\n", __FUNCTION__, __LINE__, is_softap_ready);
                    if (is_softap_ready) {
                        gWifiMgrVar.SoftAP_Init_Ready = true;
                        gWifi_connstate = WIFIMGR_CONNSTATE_STOP;
                        if (gWifiMgrSetting.wifiCallback)
                            gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CONNECTION_FINISH);
                    }
                }
            }
            else
            {
                // Client mode
                if (gWifiMgrVar.Is_First_Connect) {
                    // First time connect when the system start up
                    nRet = _WifiMgr_Sta_Connect_Post();
                    if (nRet == WIFIMGR_ECODE_OK) {
                        gWifi_connstate = WIFIMGR_CONNSTATE_CONNECTING;
                    }
                    gWifiMgrVar.Is_First_Connect = false;

                    goto end;
                }

                if (gWifi_connstate == WIFIMGR_CONNSTATE_SETTING ||
                    gWifi_connstate == WIFIMGR_CONNSTATE_CONNECTING)
                {
                    nRet = WifiMgr_Get_Connect_State(&nWiFiConnState, &nWiFiConnEcode);
                    if (nWiFiConnState == WIFIMGR_CONNSTATE_STOP) {
                        gWifi_connstate = WIFIMGR_CONNSTATE_STOP;
                        // the connecting was finish
                        if (nWiFiConnEcode == WIFIMGR_ECODE_OK) {
                            //nRet = WifiMgr_Get_Sta_Work_State(&bIsAvail);
                            if (!WifiMgr_Get_Sta_Connect_Complete) {
                                // fail, restart the timer
                                gettimeofday(&tv1, NULL);
                            }
                        } else {
                            printf("[ WIFIMGR ] %s() L#%ld: Error! nWiFiConnEcode = 0%ld\r\n", __FUNCTION__, __LINE__, nWiFiConnEcode);

                            // connection has error
                            if (nWiFiConnEcode == WIFIMGR_ECODE_NO_INI_FILE) {
                                gWifiMgrVar.No_Config_File = true;
                            }
                            if (nWiFiConnEcode == WIFIMGR_ECODE_NO_SSID) {
                                gWifiMgrVar.No_SSID = true;
                            } else {
                                // fail, restart the timer
                                gettimeofday(&tv1, NULL);
                            }
                        }
                    }
                    goto end;
                }

                //nRet = WifiMgr_Get_Sta_Work_State(&bIsAvail);
                nRet = WifiMgr_Get_Connect_State(&nWiFiConnState, &nWiFiConnEcode);
            	if (ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_SLEEP_STATUS, NULL) && gWifiMgrVar.Is_Temp_Disconnect)
            		gWifiMgrVar.Is_Temp_Disconnect = false;
                
                if (WifiMgr_Get_Sta_Avalible())
                {
                    if (gWifiMgrVar.Is_Temp_Disconnect) {
                        gWifiMgrVar.Is_Temp_Disconnect = false;     // reset
                        printf("[ WIFIMGR ] %s() L#%ld: WiFi auto re-connected!\r\n", __FUNCTION__, __LINE__);
                        if (gWifiMgrSetting.wifiCallback)
                            gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_RECONNECTION);
                    }

                    if (!gWifiMgrVar.Is_WIFI_Available) {
                        // prev is not available, curr is available
                        gWifiMgrVar.Is_WIFI_Available = true;
                        gWifiMgrVar.No_Config_File = false;
                        gWifiMgrVar.No_SSID = false;
                        printf("[ WIFIMGR ] %s() L#%ld: WiFi auto re-connected!\r\n", __FUNCTION__, __LINE__);
                    }
                     gettimeofday(&tvDHCP2, NULL);
                     if (itpTimevalDiff(&tvDHCP1, &tvDHCP2) > WIFIMGR_DHCP_RENEW_MSEC) {
                         printf("====>Send DHCP Discover!!!!!\n");
                         printf("DHCP renew %d \n", itpTimevalDiff(&tvDHCP1, &tvDHCP2));
                         ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_RENEW_DHCP, NULL);
                         gettimeofday(&tvDHCP1, NULL);
                         gettimeofday(&tvDHCP2, NULL);
                     } else {
                    //     printf("DHCP wait renew  %d \n", itpTimevalDiff(&tvDHCP1, &tvDHCP2));
                     }

                    
                } else {
                    if (gWifiMgrVar.Is_WIFI_Available){
                        if (!gWifiMgrVar.Is_Temp_Disconnect && nWiFiConnEcode == WIFIMGR_ECODE_OK)
                        {
                            // first time detect
                            gWifiMgrVar.Is_Temp_Disconnect = true;
                            gettimeofday(&tv3_temp, NULL);
                            printf("[ WIFIMGR ] %s() L#%ld: WiFi temporary disconnected!%d %d\r\n", __FUNCTION__, __LINE__,nWiFiConnState,nWiFiConnEcode);
                            if (gWifiMgrSetting.wifiCallback)
                                gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_TEMP_DISCONNECT);
                        } else if (nWiFiConnEcode == WIFIMGR_ECODE_OK){
                            gettimeofday(&tv4_temp, NULL);
                            temp_disconn_time = itpTimevalDiff(&tv3_temp, &tv4_temp);
                            printf("[ WIFIMGR ] %s() L#%ld: temp disconnect time = %ld sec. %d %d\r\n", __FUNCTION__, __LINE__, temp_disconn_time / 1000 , nWiFiConnState,nWiFiConnEcode);
                            if (temp_disconn_time >= WIFIMGR_TEMPDISCONN_MSEC) {
                                printf("[ WIFIMGR ] %s() L#%ld: WiFi temporary disconnected over %ld sec. Services should be shut down.\r\n", __FUNCTION__, __LINE__, temp_disconn_time / 1000);
                                gWifiMgrVar.Is_Temp_Disconnect = false;     // reset

                                if (gWifiMgrSetting.wifiCallback)
                                    gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_DISCONNECT_30S);

                                // prev is available, curr is not available
                                gWifiMgrVar.Is_WIFI_Available = false;
                            }
                        }
                    }
                    else
                    {
                        // prev is not available, curr is not available
                        if (gWifiMgrVar.No_Config_File || gWifiMgrVar.No_SSID) {
                            // has no data to connect, skip
                            goto end;
                        }
                        nRet = WifiMgr_Get_Connect_State(&nWiFiConnState, &nWiFiConnEcode);
                        switch (nWiFiConnState)
                        {
                        case WIFIMGR_CONNSTATE_STOP:
                            gettimeofday(&tv2, NULL);
                            if (itpTimevalDiff(&tv1, &tv2) >= WIFIMGR_RECONNTECT_MSEC) {
                                //nRet = _WifiMgr_Sta_Connect_Post();
                                if (nRet == WIFIMGR_ECODE_OK) {
                                    gWifi_connstate = WIFIMGR_CONNSTATE_CONNECTING;
                                }
                            }
                            break;
                        case WIFIMGR_CONNSTATE_SETTING:
                            break;
                        case WIFIMGR_CONNSTATE_CONNECTING:
                            break;
                        }
                    }
                }
            }

    end:
            nCheckCnt = WIFIMGR_CHECK_WIFI_MSEC;
        }
    }
    gWifiMgrVar.IS_WifiMgr_Main_Process_Thread= false;
    return NULL;
}

static int
_WifiMgr_Enable_PowerSave(void)
{

    return wifi_enable_powersave();
}


static int
_WifiMgr_Disable_PowerSave(void)
{

    return wifi_disable_powersave();
}


/* ================================================= */




/* ================================================= */
/*
/* Static Functions */
/*
/* ================================================= */


int
WifiMgr_Get_Connect_State(int *conn_state, int *e_code)
{
    int nRet = WIFIMGR_ECODE_OK;

    if (!gWifiMgrVar.WIFI_Init_Ready) {
        *conn_state = 0;
        *e_code = WIFIMGR_ECODE_NOT_INIT;
        return WIFIMGR_ECODE_NOT_INIT;
    }

    pthread_mutex_lock(&mutexALWiFi);
    *conn_state = wifi_conn_state;
    *e_code = wifi_conn_ecode;
    pthread_mutex_unlock(&mutexALWiFi);

    return nRet;
}


/* Determine WifiMgr working states */
int
WifiMgr_Get_Sta_Work_State(int* work_state)
{
    int state = WIFIMGR_WORKSTATE_STANDBY;

    if (gWifiMgrVar.WIFI_Init_Ready) {
        /* LWIP SW/HW Stack */
        if (ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_IS_AVAIL, NULL)){
            pthread_mutex_lock(&mutexALWiFi);
            *work_state  = wifi_work_state;
            pthread_mutex_unlock(&mutexALWiFi);
        } else {
            *work_state  = WIFIMGR_WORKSTATE_STANDBY;
        }
    } else {
        *work_state      = WIFIMGR_WORKSTATE_NONINIT;
    }

    state = *work_state;

    return state;
}


int
WifiMgr_Get_Sta_Avalible(void)
{
    /* LWIP link detection  by SW/HW */
    return ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_IS_AVAIL, NULL);
}

int
WifiMgr_Get_Sta_Connect_Complete(void)
{
/*
      Confirm 3 conditions of connection :
        1. SSID len > 0
        2. SW stack ready (DHCP OK)
        3. Already hook IP into netif
*/
    return ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_IS_CONNECTED, NULL);
}


int
WifiMgr_Get_MAC_Address(unsigned char cMac[6])
{

    ITPWifiInfo wifiInfo;

    ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_GET_INFO, &wifiInfo);
    cMac[0] = wifiInfo.hardwareAddress[0];
    cMac[1] = wifiInfo.hardwareAddress[1];
    cMac[2] = wifiInfo.hardwareAddress[2];
    cMac[3] = wifiInfo.hardwareAddress[3];
    cMac[4] = wifiInfo.hardwareAddress[4];
    cMac[5] = wifiInfo.hardwareAddress[5];

    //printf("WifiMgr_Get_MAC_Address %0x:%0x:%0x:%0x:%0x:%0x   \n",cMac[0],cMac[1],cMac[2],cMac[3],cMac[4],cMac[5]);

    return 0;
}


int
WifiMgr_Get_WIFI_Mode(void)
{
    return gWifiMgrVar.WIFI_Mode;
}


int
WifiMgr_Get_Scan_AP_Info(WIFI_MGR_SCANAP_LIST* pList)
{
    int nApCount;

    if (!gWifiMgrVar.WIFI_Init_Ready) {
        printf("[ WIFIMGR ] WifiMgr_Get_Scan_AP_Info  !WIFI_Init_Ready \n ");
        return WIFIMGR_ECODE_NOT_INIT;
    }

    pthread_mutex_lock(&mutexMode);


    nApCount = _WifiMgr_Sta_Scan_Process(&gScanApInfo);
    memcpy(pList,gWifiMgrApList,sizeof(WIFI_MGR_SCANAP_LIST)*WIFI_SCAN_LIST_NUMBER);

    pthread_mutex_unlock(&mutexMode);

    printf("[ WIFIMGR ] WifiMgr_Get_Scan_AP_Info %d  \n",nApCount);

    return nApCount;

}


int
WifiMgr_Get_HostAP_Ready(void)
{
    printf("[ WIFIMGR ] WifiMgr_Get_HostAP_Ready: %d \n", wifi_is_ready_to_transceive(RTW_AP_INTERFACE));
    return wifi_is_ready_to_transceive(RTW_AP_INTERFACE);
}


int
WifiMgr_Get_HostAP_Device_Number(void)
{
    int stacount = 0;
    int client_number;
    struct {
        int    count;
        rtw_mac_t mac_list[3];
    } client_info;

	client_info.count = 3;
    wifi_get_associated_client_list(&client_info, sizeof(client_info));

#if 1
    //printf("\n\rWifiMgr_Get_WIFI_Mode:");
    //printf("\n\r==============================");

    if(client_info.count == 0){
       //printf("\n\rClient Num: 0\n\r");
   	}

    else
    {
        printf("\n\rClient Num: %d", client_info.count);
        printf("\n\r");
    }
#endif

    return client_info.count;
}


int
WifiMgr_Sta_Connect(char* ssid, char* password, char* secumode)
{
    int nRet = WIFIMGR_ECODE_OK;
    unsigned long secumode_8189f;

    if (!gWifiMgrVar.WIFI_Init_Ready) {
        return WIFIMGR_ECODE_NOT_INIT;
    }
    if (gWifiMgrVar.WIFI_Mode != WIFIMGR_MODE_CLIENT){
        printf("[ WIFIMGR ] WifiMgr_Sta_Connect need client mode to connect %d  #line %d  \n",gWifiMgrVar.WIFI_Mode,__LINE__);
        return WIFIMGR_ECODE_NOT_INIT;
    }

    secumode_8189f = WifiMgr_Secu_ITE_To_RTL(secumode);

    _WifiMgr_Sta_Entry_Info(ssid, password, secumode_8189f);

    if (wifi_conn_state == WIFIMGR_CONNSTATE_STOP) {
        gWifiMgrVar.Need_Set = false;
        sem_post(&semConnectStart);
    }
    gettimeofday(&tvDHCP1, NULL);
    gettimeofday(&tvDHCP2, NULL);

    return WIFIMGR_ECODE_OK;
}


int
WifiMgr_Sta_Disconnect(void)
{

    ITPEthernetSetting setting;

    int nRet = WIFIMGR_ECODE_OK;

    if (!gWifiMgrVar.WIFI_Init_Ready) {
        return WIFIMGR_ECODE_NOT_INIT;
    }

    if (wifi_conn_state == WIFIMGR_CONNSTATE_CONNECTING){
        WifiMgr_Sta_Cancel_Connect();
    }

	printf("[ WIFIMGR ] WifiMgr_Sta_Disconnect \n");

    if (ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_IS_CONNECTED, NULL)) {

        nRet = wifi_disconnect();

        usleep(1000*100);
        // dhcp
        setting.dhcp = 0;

        // autoip
        setting.autoip = 0;

        // ipaddr
        setting.ipaddr[0] = 0;
        setting.ipaddr[1] = 0;
        setting.ipaddr[2] = 0;
        setting.ipaddr[3] = 0;

        // netmask
        setting.netmask[0] = 0;
        setting.netmask[1] = 0;
        setting.netmask[2] = 0;
        setting.netmask[3] = 0;

        // gateway
        setting.gw[0] = 0;
        setting.gw[1] = 0;
        setting.gw[2] = 0;
        setting.gw[3] = 0;

        //ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_RESET, &setting);
    }

    printf("[ WIFIMGR ] WifiMgr_Sta_Disconnect end \n");
    wifi_conn_state = WIFIMGR_CONNSTATE_STOP;
    wifi_conn_ecode = WIFIMGR_ECODE_SET_DISCONNECT;
    usleep(1000*100);
    WifiMgr_Sta_Not_Cancel_Connect();
    gettimeofday(&tvDHCP1, NULL);
    gettimeofday(&tvDHCP2, NULL);
    
    return WIFIMGR_ECODE_OK;
}


int
WifiMgr_Sta_Sleep_Disconnect(void)
{
    ITPEthernetSetting setting;

    int nRet = WIFIMGR_ECODE_OK;

    if (!gWifiMgrVar.WIFI_Init_Ready) {
        return WIFIMGR_ECODE_NOT_INIT;
    }

	printf("[ WIFIMGR ] WifiMgr_Sta_Sleep_Disconnect \n");
    wifi_conn_state = WIFIMGR_CONNSTATE_STOP;
    wifi_conn_ecode = WIFIMGR_ECODE_SET_DISCONNECT;
    usleep(1000*100);

    return WIFIMGR_ECODE_OK;
}


void
WifiMgr_Sta_Switch(int status)
{
	gWifiMgrVar.Client_On_Off = status;
}


void
WifiMgr_Sta_Cancel_Connect(void)
{
    gWifiMgrVar.Cancel_Connect = true;
}


void
WifiMgr_Sta_Not_Cancel_Connect(void)
{
    gWifiMgrVar.Cancel_Connect = false;
}


void
WifiMgr_HostAP_Set_Hidden(void)
{
    gWifiMgrVar.SoftAP_Hidden = true;
}

unsigned long WifiMgr_Secu_ITE_To_RTL(char* ite_security_enum)
{
    unsigned long rtl_security_enum;

    if (WifiMgr_Get_WIFI_Mode() == WIFIMGR_MODE_SOFTAP)
        return RTW_SECURITY_UNKNOWN;

    /* Translate ITE WIFI security enum to RTL WIFI security enum*/
    if (strcmp(ite_security_enum, ITE_WIFI_SEC_OPEN) == 0)
        rtl_security_enum = RTW_SECURITY_OPEN;
    else if (strcmp(ite_security_enum, ITE_WIFI_SEC_WEP_PSK) == 0)
        rtl_security_enum = RTW_SECURITY_WEP_PSK;
    else if (strcmp(ite_security_enum, ITE_WIFI_SEC_WPA_TKIP_PSK) == 0)
        rtl_security_enum = RTW_SECURITY_WPA_TKIP_PSK;
    else if (strcmp(ite_security_enum, ITE_WIFI_SEC_WPA2_AES_PSK) == 0)
        rtl_security_enum = RTW_SECURITY_WPA2_AES_PSK;
    else if (strcmp(ite_security_enum, ITE_WIFI_SEC_WPA_AES_PSK) == 0)
        rtl_security_enum = RTW_SECURITY_WPA_AES_PSK;
    else if (strcmp(ite_security_enum, ITE_WIFI_SEC_WPA2_TKIP_PSK) == 0)
        rtl_security_enum = RTW_SECURITY_WPA2_TKIP_PSK;
    else if (strcmp(ite_security_enum, ITE_WIFI_SEC_WPA2_MIXED_PSK) == 0)
        rtl_security_enum = RTW_SECURITY_WPA2_MIXED_PSK;
    else if (strcmp(ite_security_enum, ITE_WIFI_SEC_WPA_WPA2_MIXED) == 0)
        rtl_security_enum = RTW_SECURITY_WPA_WPA2_MIXED_PSK;
    else if (strcmp(ite_security_enum, ITE_WIFI_SEC_WPS_SECURE) == 0)
        rtl_security_enum = RTW_SECURITY_WPS_SECURE;
    else if (strcmp(ite_security_enum, ITE_WIFI_SEC_WPA_TKIP_8021X) == 0)
        rtl_security_enum = RTW_SECURITY_WPA_TKIP_8021X;
    else if (strcmp(ite_security_enum, ITE_WIFI_SEC_WPA_AES_8021X) == 0)
        rtl_security_enum = RTW_SECURITY_WPA_AES_8021X;
    else if (strcmp(ite_security_enum, ITE_WIFI_SEC_WPA2_AES_8021X) == 0)
        rtl_security_enum = RTW_SECURITY_WPA2_AES_8021X;
    else if (strcmp(ite_security_enum, ITE_WIFI_SEC_WPA2_TKIP_8021X) == 0)
        rtl_security_enum = RTW_SECURITY_WPA2_TKIP_8021X;
    else if (strcmp(ite_security_enum, ITE_WIFI_SEC_WPA_WPA2_MIXED_8021X) == 0)
        rtl_security_enum = RTW_SECURITY_WPA_WPA2_MIXED_8021X;
    else
        rtl_security_enum = RTW_SECURITY_UNKNOWN;

    printf("[ WIFIMGR ] WifiMgr_Secu_ITE_To_RTL: ITE(%s) -> RTL(0x%x)\n", ite_security_enum, rtl_security_enum);

    return rtl_security_enum;
}


char* WifiMgr_Secu_RTL_To_ITE(unsigned long rtl_security_enum)
{
    static char ite_security_enum[3];

    /* Translate 8189ftv WIFI security enum to ITE WIFI security enum*/
    if (rtl_security_enum == RTW_SECURITY_OPEN)
        strcpy(ite_security_enum, "0");
    else if (rtl_security_enum == RTW_SECURITY_WEP_PSK)
        strcpy(ite_security_enum, "1");
    else if (rtl_security_enum == RTW_SECURITY_WPA_TKIP_PSK)
        strcpy(ite_security_enum, "2");
    else if (rtl_security_enum == RTW_SECURITY_WPA_AES_PSK)
        strcpy(ite_security_enum, "3");
    else if (rtl_security_enum == RTW_SECURITY_WPA2_AES_PSK)
        strcpy(ite_security_enum, "4");
    else if (rtl_security_enum == RTW_SECURITY_WPA2_TKIP_PSK)
        strcpy(ite_security_enum, "5");
    else if (rtl_security_enum == RTW_SECURITY_WPA2_MIXED_PSK)
        strcpy(ite_security_enum, "6");
    else if (rtl_security_enum == RTW_SECURITY_WPA_WPA2_MIXED_PSK)
        strcpy(ite_security_enum, "7");
    else if (rtl_security_enum == RTW_SECURITY_WPS_OPEN)
        strcpy(ite_security_enum, "8");
    else if (rtl_security_enum == RTW_SECURITY_WPS_SECURE)
        strcpy(ite_security_enum, "9");
    else if (rtl_security_enum == RTW_SECURITY_WPA_TKIP_8021X)
        strcpy(ite_security_enum, "11");
    else if (rtl_security_enum == RTW_SECURITY_WPA_AES_8021X)
        strcpy(ite_security_enum, "12");
    else if (rtl_security_enum == RTW_SECURITY_WPA2_AES_8021X)
        strcpy(ite_security_enum, "13");
    else if (rtl_security_enum == RTW_SECURITY_WPA2_TKIP_8021X)
        strcpy(ite_security_enum, "14");
    else if (rtl_security_enum == RTW_SECURITY_WPA_WPA2_MIXED_8021X)
        strcpy(ite_security_enum, "15");
    else
        strcpy(ite_security_enum, "NA");

    printf("[ WIFIMGR ] WifiMgr_Secu_RTL_To_ITE: RTL(0x%X) -> ITE(%s)\n", rtl_security_enum, ite_security_enum);

    return ite_security_enum;
}


int
WifiMgr_Init(WIFIMGR_MODE_E init_mode, int mp_mode,WIFI_MGR_SETTING wifiSetting)
{
    int nRet = WIFIMGR_ECODE_OK;
    pthread_attr_t attr, attr1,attr2;

    while(gWifiMgrVar.WIFI_Terminate){
         printf("[ WIFIMGR ] WifiMgr not finished yet \n");
         usleep(200*1000);
    }

    wifi_conn_state         = WIFIMGR_CONNSTATE_STOP;
    wifi_conn_ecode         = WIFIMGR_ECODE_SET_DISCONNECT;
    gWifiMgrVar.Need_Set    = false;
    gWifiMgrVar.MP_Mode     = mp_mode;

    gWifiMgrSetting.wifiCallback = wifiSetting.wifiCallback;

    if (init_mode ==WIFIMGR_MODE_CLIENT){
        _WifiMgr_Sta_Entry_Info(wifiSetting.ssid, wifiSetting.password, wifiSetting.secumode);
    } else if (init_mode ==WIFIMGR_MODE_SOFTAP){
        printf("[ WIFIMGR ] WIFI start AP: SSID(%s)/PW(%s) \n", wifiSetting.ssid, wifiSetting.password);
        _WifiMgr_HostAP_Entry_Info(wifiSetting.ssid, wifiSetting.password);
        _WifiMgr_HostAP_Init();
    }

    // default select dhcp
    gWifiMgrSetting.setting.dhcp = 1;
    if (wifiSetting.setting.ipaddr[0]>0){
        memcpy(&gWifiMgrSetting.setting,&wifiSetting.setting,sizeof(ITPEthernetSetting));
    }

    // init semaphore
    nRet = sem_init(&semConnectStart, 0, 0);
    if (nRet == -1) {
        printf("[ WIFIMGR ] ERROR, semConnectStart sem_init() fail!\r\n");
        nRet = WIFIMGR_ECODE_SEM_INIT;
        goto err_end;
    }

    nRet = sem_init(&semConnectStop, 0, 0);
    if (nRet == -1) {
        printf("[ WIFIMGR ] ERROR, semConnectStop sem_init() fail!\r\n");
        nRet = WIFIMGR_ECODE_SEM_INIT;
        goto err_end;
    }

    nRet = sem_init(&semWPSStart, 0, 0);
    if (nRet == -1) {
        printf("[ WIFIMGR ] ERROR, semWPSStart sem_init() fail!\r\n");
        nRet = WIFIMGR_ECODE_SEM_INIT;
        goto err_end;
    }

    nRet = sem_init(&semWPSStop, 0, 0);
    if (nRet == -1) {
        printf("[ WIFIMGR ] ERROR, semWPSStop sem_init() fail!\r\n");
        nRet = WIFIMGR_ECODE_SEM_INIT;
        goto err_end;
    }

    // init mutex
    nRet = pthread_mutex_init(&mutexALWiFi, NULL);
    if (nRet != 0) {
        printf("[ WIFIMGR ] ERROR, mutexALWiFi pthread_mutex_init() fail! nRet = %ld\r\n", nRet);
        nRet = WIFIMGR_ECODE_MUTEX_INIT;
        goto err_end;
    }

    nRet = pthread_mutex_init(&mutexALWiFiWPS, NULL);
    if (nRet != 0) {
        printf("[ WIFIMGR ] ERROR, mutexALWiFiWPS pthread_mutex_init() fail! nRet = %ld\r\n", nRet);
        nRet = WIFIMGR_ECODE_MUTEX_INIT;
        goto err_end;
    }

    nRet = pthread_mutex_init(&mutexIni, NULL);
    if (nRet != 0) {
        printf("[ WIFIMGR ] ERROR, mutexIni pthread_mutex_init() fail! nRet = %ld\r\n", nRet);
        nRet = WIFIMGR_ECODE_MUTEX_INIT;
        goto err_end;
    }

    nRet = pthread_mutex_init(&mutexMode, NULL);
    if (nRet != 0) {
        printf("[ WIFIMGR ] ERROR, mutexMode pthread_mutex_init() fail! nRet = %ld\r\n", nRet);
        nRet = WIFIMGR_ECODE_MUTEX_INIT;
        goto err_end;
    }

    // create thread
    printf("[ WIFIMGR ] Create thread \n");

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);    
    pthread_attr_setstacksize(&attr, WIFI_STACK_SIZE);
    pthread_create(&ClientModeTask, &attr, _WifiMgr_Sta_Thread, NULL);

    pthread_attr_init(&attr2);
    pthread_attr_setdetachstate(&attr2, PTHREAD_CREATE_DETACHED);    
    pthread_attr_setstacksize(&attr2, WIFI_STACK_SIZE);
    pthread_create(&ProcessTask, &attr2, _WifiMgr_Main_Process_Thread, NULL);

    gWifiMgrVar.WIFI_Mode = init_mode;
    printf("[ WIFIMGR ] %s() L#%ld: WIFI Mode = %ld\r\n", __FUNCTION__, __LINE__, gWifiMgrVar.WIFI_Mode);

    WifiMgr_Sta_Not_Cancel_Connect();
    gWifiMgrVar.Cancel_WPS      = false;

    gWifiMgrVar.WIFI_Init_Ready = true;
    gWifiMgrVar.WIFI_Terminate  = false;
end:
    return nRet;

err_end:
    pthread_mutex_destroy(&mutexMode);
    pthread_mutex_destroy(&mutexIni);
    pthread_mutex_destroy(&mutexALWiFiWPS);
    sem_destroy(&semWPSStop);
    sem_destroy(&semWPSStart);
    pthread_mutex_destroy(&mutexALWiFi);
    sem_destroy(&semConnectStop);
    sem_destroy(&semConnectStart);

    return nRet;
}



int WifiMgr_Terminate(void)
{
    int nRet = WIFIMGR_ECODE_OK;
    int nWait = 0;
    printf("[ WIFIMGR ] WifiMgr_Terminate: ====>start\n");
    if (!gWifiMgrVar.WIFI_Init_Ready) {
        return WIFIMGR_ECODE_NOT_INIT;
    }

    gWifiMgrVar.WPA_Terminate = true;
    gWifiMgrVar.Start_Scan    = false;
    if (gWifiMgrVar.WIFI_Mode == WIFIMGR_MODE_SOFTAP) {
        dhcps_deinit();
        usleep(1000*10);

        ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFIAP_DISABLE, NULL);
        usleep(1000*1000);

    } else {
        //client mode: clean "gWifiSetting" at network.c
        if (gWifiMgrSetting.wifiCallback && (gWifiMgrVar.Client_On_Off == WIFIMGR_SWITCH_OFF))
            gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_CLIENT_MODE_SLEEP_CLEAN_INFO);

        gWifiMgrVar.Cancel_WPS = true;
    }

    gWifiMgrVar.WIFI_Terminate = true;
    //wifi_off();


    sem_post(&semWPSStart);
    sem_post(&semConnectStart);

    usleep(2000);
//    pthread_join(ClientModeTask, NULL);
//    pthread_join(ProcessTask, NULL);

    pthread_mutex_destroy(&mutexMode);
    pthread_mutex_destroy(&mutexIni);
    pthread_mutex_destroy(&mutexALWiFiWPS);
    sem_destroy(&semWPSStop);
    sem_destroy(&semWPSStart);
    pthread_mutex_destroy(&mutexALWiFi);
    sem_destroy(&semConnectStop);
    sem_destroy(&semConnectStart);

    /* Initialize these flags */
    gWifiMgrVar.WIFI_Init_Ready = false;
    gWifiMgrVar.WIFI_Terminate = false;
    gWifiMgrVar.WPA_Terminate = false;
    gWifiMgrVar.SoftAP_Hidden = false;
    gWifiMgrVar.SoftAP_Init_Ready = false;
    WifiMgr_Sta_Not_Cancel_Connect();
    do {
        printf("[ WIFIMGR ] WifiMgr_Terminate wait WifiMgr_Main_Process_Thread %d WifiMgr_Sta_Thread %d \n",gWifiMgrVar.IS_WifiMgr_Main_Process_Thread,gWifiMgrVar.IS_WifiMgr_Sta_Thread);
        usleep(200*1000);
        nWait++;
        if (nWait>10)    
            break;

    } while(gWifiMgrVar.IS_WifiMgr_Main_Process_Thread == true || gWifiMgrVar.IS_WifiMgr_Sta_Thread == true);
    printf("[ WIFIMGR ] WifiMgr_Terminate: <====end\n");
    gettimeofday(&tvDHCP1, NULL);
    gettimeofday(&tvDHCP2, NULL);

    return nRet;
}


static int gCountT = 0;
static WIFI_MGR_SETTING gWifiSetting;
static  struct timeval gSwitchStart, gSwitchEnd;
static  struct timeval gSwitchCount;

int
_WifiMgr_Sta_HostAP_Switch_Calculate_Time(void)
{
    if (itpTimevalDiff(&gSwitchCount,&gSwitchEnd)> 1000){
        printf(" %d , %d  \n",itpTimevalDiff(&gSwitchCount,&gSwitchEnd)+(gCountT*1000),itpTimevalDiff(&gSwitchStart,&gSwitchEnd));
        gCountT++;
        gettimeofday(&gSwitchCount, NULL);
    }
}


static void*
_WifiMgr_Sta_HostAP_Switch_Thread(void *arg)
{
    int nTemp;

    printf("[ WIFIMGR ] WifiMgr_Sta_HostAP_Switch \n");

    if (!gWifiMgrVar.WIFI_Init_Ready) {
        //return WIFIMGR_ECODE_NOT_INIT;
        return NULL;
    }

    gWifiMgrVar.WIFI_Mode = WifiMgr_Get_WIFI_Mode();

    WifiMgr_Terminate();
    sleep(1);

    wifi_off();
    ioctl(ITP_DEVICE_SDIO, ITP_IOCTL_EXIT, NULL);
#if defined(CFG_GPIO_SD1_WIFI_POWER_ENABLE)
    ithLockMutex(ithStorMutex);

    ithSdSetGpioLow(ITH_STOR_SD1);

    ithWIFICardPowerOff();
    // wait 200 ms
    usleep(200 * 1000);

    ithWIFICardPowerOn();
    // wait 200 ms
    usleep(200 * 1000);

    ithUnlockMutex (ithStorMutex);
#endif
    ioctl(ITP_DEVICE_SDIO, ITP_IOCTL_INIT, NULL);

    gCountT = 0;
    gettimeofday(&gSwitchCount, NULL);

    if (gWifiMgrVar.WIFI_Mode == WIFIMGR_MODE_SOFTAP){
        // init client mode
        printf("[ WIFIMGR ] WifiMgr_Sta_HostAP_Switch init client  \n");
        wifi_on(RTW_MODE_STA);
        usleep(200 * 1000);

        nTemp = WifiMgr_Init(WIFIMGR_MODE_CLIENT, 0, gWifiSetting);
    } else {
        // init softap mode
		printf("[ WIFIMGR ] WifiMgr_Sta_HostAP_Switch init softap  \n");
		wifi_on(RTW_MODE_AP);
        usleep(200 * 1000);
		printf("[ WIFIMGR ] WifiMgr_Sta_HostAP_Switch init softap  done \n");
        nTemp = WifiMgr_Init(WIFIMGR_MODE_SOFTAP, 0, gWifiSetting);
    }

    if (gWifiMgrSetting.wifiCallback)
        gWifiMgrSetting.wifiCallback(WIFIMGR_STATE_CALLBACK_SWITCH_CLIENT_SOFTAP_FINISH);

    return 0;

}


int
WifiMgr_HostAP_First_Start(void)
{
    printf("[ WIFIMGR ] WifiMgr_HostAP_First_Start \n");

    wifi_off();
    sleep(1);
    wifi_on(RTW_MODE_AP);
}


int
WifiMgr_Sta_HostAP_Switch(WIFI_MGR_SETTING wifiSetting)
{
    int res;
    pthread_t task;
    pthread_attr_t attr;

    gettimeofday(&gSwitchStart, NULL);

    memcpy(&gWifiSetting,&wifiSetting,sizeof(WIFI_MGR_SETTING));

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, WIFI_STACK_SIZE);
    res = pthread_create(&task, &attr, _WifiMgr_Sta_HostAP_Switch_Thread, NULL);

    return res;
}

// disable auto reconnect 
int WifiMgr_Disable_Auto_Reconnect(int nDisable)
{


}




#elif defined(_WIN32)

int
WifiMgr_Get_Scan_AP_Info(WIFI_MGR_SCANAP_LIST* pList)
{
    return 0;
}


int
WifiMgr_Get_Connect_State(int *conn_state, int *e_code)
{
    int nRet = WIFIMGR_ECODE_OK;

    *conn_state = 0;
    *e_code = 0;


    return nRet;
}


int WifiMgr_Sta_Connect_AP(char* ssid, char* password, char* secumode)
{
    return WIFIMGR_ECODE_OK;
}


int WifiMgr_Sta_Disconnect()
{
    return WIFIMGR_ECODE_OK;
}


void WifiMgr_Sta_Cancel_Connect()
{
    return;
}
#endif

