/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * PAL WiFi functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "itp_cfg.h"
#include "lwip/dhcp.h"
#include "lwip/autoip.h"
#include "lwip/tcpip.h"
#include "ite/ite_wifi.h"
#include "ite/ite_usbex.h"

/* === Function Declaration === */
err_t wifiif_init(struct netif *netif);
void  wifiif_shutdown(struct netif *netif);
void  wifiif_poll(struct netif *netif);
void  wifiif_ctrl(struct netif *netif, struct net_device_config* cfg);
void  wifiif_info(struct netif *netif, struct net_device_info* info);

/* === Global Variable === */
static struct dhcp              wifiNetifDhcp;
static struct timeval           wifiLastTime;

static int                      WifiCurrentState = 0, WifiAddNetIf = 0, wifiMscnt, sleep_notify = 2;
static bool                     wifiInited = false, wifiDhcp = false, linkUp = false;
static ip_addr_t                wifiIpaddr, wifiNetmask, wifiGw;

/* === Macro === */
#define STASTATE                1
#define APSTATE                 2

#if defined (CFG_NET_WIFI_SDIO_NGPL) || defined (CFG_NET_WIFI_SDIO_NGPL_8723DS)
/* ==================== RTK 8189FTV/8723DS NETIF INIT ==================== */
/* ======================== WIFI SDIO type: Start ======================== */
#include <../non_gpl_wifi/include/autoconf_rtos.h>
#include <../non_gpl_wifi/include/wifi_constants.h>


/* === Macro === */
#define CFG_NET_WIFI_IPADDR     "192.168.1.1" // test example
#define CFG_NET_WIFI_NETMASK    "255.255.255.0"
#define CFG_NET_WIFI_GATEWAY    "192.168.1.1"


/* === Extern Functions === */
extern struct netif xnetif[NET_IF_NUM]; //extern netif from wifi_conf.c
extern uint8_t* LwIP_GetIP(struct netif *pnetif);
extern uint8_t* LwIP_GetMASK(struct netif *pnetif);
extern uint8_t* LwIP_GetMASK(struct netif *pnetif);
extern uint8_t* LwIP_GetMAC(struct netif *pnetif);
extern int      wifi_off(void);
extern int      wext_get_ssid(const char *ifname, unsigned char *ssid);


/* === Global Variables === */
static struct netif *keepNetif = NULL; // Keep add Netif


#ifndef WLAN0_NAME
  #define WLAN0_NAME		"wlan0"
#endif

// This function initializes all network interfaces
static void itpWifiNgplLwipInit(struct netif *netif)
{
    ip_addr_t ipaddr, netmask, gw;

    ip_addr_set_zero(&gw);
    ip_addr_set_zero(&ipaddr);
    ip_addr_set_zero(&netmask);
    printf("[Itp Wifi] itpWifiNgplLwipInit netif(0x%X)\n", netif);

    netif_add(netif, &ipaddr, &netmask, &gw, NULL, wifiif_init, tcpip_input);
    netif_set_default(netif);
#if LWIP_NETIF_REMOVE_CALLBACK
    netif_set_remove_callback(netif, wifiif_shutdown);
#endif
    dhcp_set_struct(netif, &wifiNetifDhcp);
}

static int WifiRead(int file, char *ptr, int len, void* info)
{
    struct net_device_info* netinfo = (struct net_device_info*)ptr;
    netinfo->infoType = WLAN_INFO_SCAN_GET;
    wifiif_info(keepNetif, netinfo);
    return 0;
}

static void WifiSleepNotify(int idx)
{
    sleep_notify = idx; // [initial:2], [sleep -> wakeup:1] , [wakeup -> sleep:0]
    if (sleep_notify == sleep_to_wakeup)
        wifiInited = true;
    printf("###sleep notify: %d\n", sleep_notify);
}

static void WifiStartDHCP(void* setting)
{
    ip_addr_t ipaddr, netmask, gw;

    ip_addr_set_zero(&gw);
    ip_addr_set_zero(&ipaddr);
    ip_addr_set_zero(&netmask);

    printf("dhcp_start\n");

    netif_set_addr(keepNetif, &ipaddr, &netmask, &gw);

    dhcp_start(keepNetif);

    wifiDhcp = true;
    gettimeofday(&wifiLastTime, NULL);
    wifiMscnt = 0;
}

static void WifiReset(ITPEthernetSetting* setting)
{

        netif_set_down(keepNetif);

        IP4_ADDR(&wifiIpaddr, setting->ipaddr[0], setting->ipaddr[1], setting->ipaddr[2], setting->ipaddr[3]);
        IP4_ADDR(&wifiNetmask, setting->netmask[0], setting->netmask[1], setting->netmask[2], setting->netmask[3]);
        IP4_ADDR(&wifiGw, setting->gw[0], setting->gw[1], setting->gw[2], setting->gw[3]);

        netif_set_addr(keepNetif, &wifiIpaddr, &wifiNetmask, &wifiGw);
        netif_set_up(keepNetif);
}

/* No Use for NGPL SDIO WIFI */
void WifiConnect(int type)
{
    //no use, for compiler
}

void WifiDisconnect(int type)
{
    //no use, for compiler
}

static void WifiGetInfo(ITPWifiInfo* info)
{
	static struct net_device_info netinfo = {0};
	unsigned char *mac ;
	memset(info, 0, sizeof (ITPWifiInfo));

	netinfo.infoType = WLAN_INFO_AP;
	strcpy(info->displayName, "wlan0");

	if(keepNetif)
	{
		mac = LwIP_GetMAC(keepNetif);

              ip_addr_t * ip_mask  = (ip_addr_t *)LwIP_GetMASK(keepNetif); 
              info->netmask = ip_mask->addr;  
              ip_addr_t * ip_t =  (ip_addr_t *) LwIP_GetIP(keepNetif); 
              info->address  = ip_t->addr;
              ip_addr_t * gw_t =  (ip_addr_t *) LwIP_GetGW(keepNetif);
              info->gateway= gw_t->addr;

              info->active    = info->address? 1 : 0; 
#if 0
              char ip[16] = {0};
              char mask[16] = {0};
              ipaddr_ntoa_r((const ip_addr_t*)ip_t,ip,sizeof(ip));
              ipaddr_ntoa_r((const ip_addr_t*)ip_mask,mask,sizeof(mask));

              printf("info = %s %s \n",ip,mask);
#endif              
    }

	info->hardwareAddress[0] = mac[0];
	info->hardwareAddress[1] = mac[1];
	info->hardwareAddress[2] = mac[2];
	info->hardwareAddress[3] = mac[3];
	info->hardwareAddress[4] = mac[4];
	info->hardwareAddress[5] = mac[5];

	info->rfQualityQuant = netinfo.avgQuant;
	info->rfQualityRSSI = netinfo.avgRSSI;
	strcpy(info->name, "wlan0");

	printf("%s\n",__func__);
}

/*************************/
static int WifiIoctl(int file, unsigned long request, void* ptr, void* info)
{
    switch (request)
    {
        case ITP_IOCTL_IS_AVAIL:
            if(WifiCurrentState == STASTATE)
            {
                unsigned char xnetif_set_up, xnetif_set_link_up;

                xnetif_set_up       = keepNetif->flags & NETIF_FLAG_UP; //SW stack ready (Get DHCP IP)
                xnetif_set_link_up  = keepNetif->flags & NETIF_FLAG_LINK_UP; //HW stack ready (Link layer OK)
               // printf("\n\r[ITP_WIFI][SDIO Type] NETIF(0x%X) FLAG UP(%d)/FLAG LINK UP(%d).\n", keepNetif, xnetif_set_up, xnetif_set_link_up);

                return xnetif_set_up && xnetif_set_link_up;
            }
            break;

        case ITP_IOCTL_IS_CONNECTED:
            if(WifiCurrentState == STASTATE)
            {
                int condition[3];
                char essid[33];

                /* There are 3 conditions for Connected */
                condition[0] = (wext_get_ssid(WLAN0_NAME, (unsigned char *) essid)>0)? 1 : 0; /* SSID len > 0 */
                condition[1] = keepNetif->flags & NETIF_FLAG_UP ? 1 : 0; /* SW stack ready (DHCP OK) */
                condition[2] = keepNetif->ip_addr.addr? 1 : 0;   /* Already hook IP into netif */

                printf("[ITP_WIFI] Connected cond: [1] SSID len %s 0, [2] DHCP is %s, [3] Netif %s IP\n",
                    condition[0] ? ">":"=",
                    condition[1] ? "OK":"not OK",
                    condition[2] ? "have":"have no");


                /* Check SSID was set */
                if(condition[0]) {
                    return condition[1] && condition[2];
                } else {
                    printf("\n\r[ITP_WIFI][SDIO Type] WIFI disconnected and have NO SSID.\n");
                    return -1;
                }
            }
            else if(WifiCurrentState == APSTATE)
            {
                printf("[ITP_WIFI][SDIO Type] ITP_IOCTL_IS_CONNECTED is useless for HostAP mode, ignore its.\n");
                return 0;
            }
            else
                return -1;
            break;

        case ITP_IOCTL_WIFI_START_DHCP:
        	if(WifiCurrentState == STASTATE)
                WifiStartDHCP((void*)ptr);
            break;

        case ITP_IOCTL_WIFI_STOP_DHCP:
            if(WifiCurrentState == STASTATE)
                dhcp_release(keepNetif);
                dhcp_stop(keepNetif);
            break;

        case ITP_IOCTL_WIFI_ADD_NETIF:
            keepNetif = ptr;
            itpWifiNgplLwipInit(ptr);
            break;

        case ITP_IOCTL_INIT:
            printf("[ITP_WIFI][SDIO Type] ITP_IOCTL_INIT \n");
            mmpRtlWifiDriverCmdTest();
            break;

        case ITP_IOCTL_EXIT:
            wifi_off();
            break;

        case ITP_IOCTL_RESET:
        WifiReset((void*)ptr);
        break;

        case ITP_IOCTL_SLEEP:
            WifiSleepNotify((int)ptr);
            break;

        case ITP_IOCTL_WIFI_SLEEP_STATUS:
        	return sleep_notify;


        case ITP_IOCTL_GET_INFO:
            {
                WifiGetInfo((ITPWifiInfo*)ptr);
            }
            break;

        case ITP_IOCTL_WIFIAP_ENABLE:
            {
                ip_addr_t ipaddr, netmask, gw;
#if defined(CFG_NET_ETHERNET) && defined(CFG_NET_WIFI)
                //itpWifiLwipInitNetif();
#endif
                ipaddr_aton(CFG_NET_WIFI_IPADDR, &ipaddr);
                ipaddr_aton(CFG_NET_WIFI_NETMASK, &netmask);
                ipaddr_aton(CFG_NET_WIFI_GATEWAY, &gw);

            if (keepNetif){
                netif_set_addr(keepNetif, &ipaddr, &netmask, &gw);
                netif_set_up(keepNetif);
                dhcp_start(keepNetif);
            } else {
                printf("[ITP_WIFI][SDIO Type] Error of netif is NULL \n");
            }

                WifiCurrentState = APSTATE;
                printf("[ITP_WIFI][SDIO Type] ITP_IOCTL_WIFIAP_ENABLE \n");
            }
            break;

        case ITP_IOCTL_WIFIAP_DISABLE:
            printf("[ITP_WIFI][SDIO Type] ITP_IOCTL_WIFIAP_DISABLE is useless for 8189F SDIO WIFI, ignore its.\n");
            break;

        case ITP_IOCTL_ENABLE:
            WifiCurrentState = STASTATE;
#ifdef CFG_NET_LWIP_2
            // Double check the netif is up or not
            if (!netif_is_up(keepNetif))
                netif_set_up(keepNetif);
#endif
            break;

        case ITP_IOCTL_DISABLE:
            {
                ip_addr_t ipaddr, netmask, gw;

                ip_addr_set_zero(&gw);
                ip_addr_set_zero(&ipaddr);
                ip_addr_set_zero(&netmask);

                netif_set_addr(keepNetif, &ipaddr, &netmask, &gw);
                netif_set_down(keepNetif);

                printf("[ITP_WIFI][SDIO Type] ITP_IOCTL_DISABLE(STA Mode) \n");
                wifiInited = false;
                linkUp = false;
                WifiCurrentState = 0;
            }
            break;

        case ITP_IOCTL_WIFI_NETIF_SHUTDOWN:
            netif_set_link_down(keepNetif); //close MAC & DMA
#if defined(CFG_NET_WIFI) && defined(CFG_NET_ETHERNET)
            netif_remove(keepNetif);
#else
            netif_remove_all(); //1. call netif_set_down   2.wifiif_shutdown by callback
#endif
            break;

        default:
            errno = (ITP_DEVICE_WIFI << ITP_DEVICE_ERRNO_BIT) | 1;
            printf("[ITP_WIFI][SDIO Type] ERROR : ITP_IOCTL (%d)\n", request);
            return -1;
    }
    return 0;
}

const ITPDevice itpDeviceWifi =
{
    ":wifi",
    itpOpenDefault,
    itpCloseDefault,
    WifiRead,
    itpWriteDefault,
    itpLseekDefault,
    WifiIoctl,
    NULL
};

/* ===================== WIFI SDIO type: End ===================== */


/* =================================================== */

#else

/* ===================== WIFI USB type: Start ===================== */

/* === Macro === */
#define WIFI_TEST_INPUT_DONGLE  15

/* === Static Varible === */
static struct netif             wifiNetif = {0};
static bool                     wifiConnected = false;
static pthread_t                wifi_init_task;


static int WifiRead(int file, char *ptr, int len, void* info)
{
    struct net_device_info* netinfo = (struct net_device_info*)ptr;
    netinfo->infoType = WLAN_INFO_SCAN_GET;
    wifiif_info(&wifiNetif, netinfo);
    return 0;
}

static void WifiSleepNotify(int idx)
{
    sleep_notify = idx; // [initial:2], [sleep -> wakeup:1] , [wakeup -> sleep:0]
    if (sleep_notify == sleep_to_wakeup)
        wifiInited = true;
    printf("###sleep notify: %d\n", sleep_notify);
}

// This function initializes all network interfaces
void itpWifiLwipInit(void)
{
    ip_addr_t ipaddr, netmask, gw;

    ip_addr_set_zero(&gw);
    ip_addr_set_zero(&ipaddr);
    ip_addr_set_zero(&netmask);

#if defined(CFG_NET_WIFI_MAY_NOT_EXIST)
    // check wifi dongle status
    if (!wifiConnected){
        printf("[Itp Wifi] itpWifiLwipInit no wifi dongle \n");
        return;
    }
#endif

    netif_add(&wifiNetif, &ipaddr, &netmask, &gw, NULL, wifiif_init, tcpip_input);
    //printf("itpWifiLwipInit don't netif_set_default  \n\n");
#if !defined(CFG_NET_ETHERNET)
    netif_set_default(&wifiNetif);
#endif
#if LWIP_NETIF_REMOVE_CALLBACK
    netif_set_remove_callback(&wifiNetif, wifiif_shutdown);
#endif
    WifiAddNetIf = 1;
    dhcp_set_struct(&wifiNetif, &wifiNetifDhcp);
}


// This function initializes all network interfaces
void itpWifiLwipInitNetif(void)
{
    ip_addr_t ipaddr, netmask, gw;
    int timeout = 0;
    ip_addr_set_zero(&gw);
    ip_addr_set_zero(&ipaddr);
    ip_addr_set_zero(&netmask);

    // avoid wifi dirver not initializes
    do {
        timeout++;
        if (timeout>10)
            break;
        usleep(100*1000);
        printf("Net device not ready , wait %d \n",timeout);
    } while (!smNetGetDevice());

    netif_add(&wifiNetif, &ipaddr, &netmask, &gw, NULL, wifiif_init, tcpip_input);
    printf("itpWifiLwipInitNetif %d \n\n",WifiAddNetIf);
    WifiAddNetIf = 1;
    netif_set_default(&wifiNetif);
    dhcp_set_struct(&wifiNetif, &wifiNetifDhcp);
}

static void WifiScan(void)
{
    static struct net_device_info info = {0};
    info.infoType = WLAN_INFO_SCAN;
    wifiif_info(&wifiNetif, &info);
}

static int WifiState(void)
{
    static struct net_device_info info = {0};
    info.infoType = WLAN_INFO_WIFI_STATE;
    wifiif_info(&wifiNetif, &info);
    return info.driverState;
}

static int WifiStaNum(void)
{
    static struct net_device_info info = {0};
    info.infoType = WLAN_INFO_WIFI_STATE;
    wifiif_info(&wifiNetif, &info);
    return info.staCount;
}

static int WifiBestChannel(void)
{
    static struct net_device_info info = {0};
    info.infoType = WLAN_INFO_WIFI_STATE;
    wifiif_info(&wifiNetif, &info);
    return info.channelId;
}


static void WifiapGetInfo(ITPWifiInfo* info)
{
    static struct net_device_info netinfo = {0};
    memset(info, 0, sizeof (ITPWifiInfo));

    netinfo.infoType = WLAN_INFO_AP;
    wifiif_info(&wifiNetif, &netinfo);

    info->active = 1;//wifiapNetif.ip_addr.addr ? 1 : 0;
    info->address = wifiNetif.ip_addr.addr;
    info->netmask = wifiNetif.netmask.addr;
    strcpy(info->displayName, "wlan0");

    info->hardwareAddress[0] = netinfo.staMacAddr[0];
    info->hardwareAddress[1] = netinfo.staMacAddr[1];
    info->hardwareAddress[2] = netinfo.staMacAddr[2];
    info->hardwareAddress[3] = netinfo.staMacAddr[3];
    info->hardwareAddress[4] = netinfo.staMacAddr[4];
    info->hardwareAddress[5] = netinfo.staMacAddr[5];

    strcpy(info->name, "wlan0");
}

static void WifiGetInfo(ITPWifiInfo* info)
{
    static struct net_device_info netinfo = {0};
    memset(info, 0, sizeof (ITPWifiInfo));

#ifndef CFG_NET_WIFI_WPA
    if (!(netif_is_link_up(&wifiNetif)))
    {
        LOG_ERR "wifi is not init yet\n" LOG_END
        return;
    }
#endif

    netinfo.infoType = WLAN_INFO_AP;
    wifiif_info(&wifiNetif, &netinfo);

    info->active = wifiNetif.ip_addr.addr ? 1 : 0;
    info->address = wifiNetif.ip_addr.addr;
    info->netmask = wifiNetif.netmask.addr;
    strcpy(info->displayName, "wlan0");

    info->hardwareAddress[0] = netinfo.staMacAddr[0];
    info->hardwareAddress[1] = netinfo.staMacAddr[1];
    info->hardwareAddress[2] = netinfo.staMacAddr[2];
    info->hardwareAddress[3] = netinfo.staMacAddr[3];
    info->hardwareAddress[4] = netinfo.staMacAddr[4];
    info->hardwareAddress[5] = netinfo.staMacAddr[5];
    info->rfQualityQuant = netinfo.avgQuant;
    info->rfQualityRSSI = netinfo.avgRSSI;
    strcpy(info->name, "wlan0");
}

static void WifiLinkHidden(struct net_device_info* setting)
{
    if (!(wifiInited))
    {
        LOG_ERR "wifi is not init yet\n" LOG_END
        return;
    }

    setting->infoType = WLAN_INFO_LINK_HIDDEN;
    wifiif_info(&wifiNetif, setting);
}

static void WifiLinkAP(void* setting)
{
    struct net_device_config *driverConfig = (struct net_device_config*)setting;
    if (!(wifiInited))
    {
        LOG_ERR "wifi is not init yet\n" LOG_END
        return;
    }

#ifndef CFG_NET_WIFI_WPA
    wifiif_ctrl(&wifiNetif, driverConfig);
    usleep(1000*1000*10);
#endif
}

static void WifiStartDHCP(void* setting)
{
    ip_addr_t ipaddr, netmask, gw;

    ip_addr_set_zero(&gw);
    ip_addr_set_zero(&ipaddr);
    ip_addr_set_zero(&netmask);

    printf("dhcp_start\n");

    netif_set_addr(&wifiNetif, &ipaddr, &netmask, &gw);

    dhcp_start(&wifiNetif);

    wifiDhcp = true;
    gettimeofday(&wifiLastTime, NULL);
    wifiMscnt = 0;
}

static void WifiReset(ITPEthernetSetting* setting)
{
    if (setting->dhcp || setting->autoip)
    {
        if (!setting->dhcp && wifiDhcp)
        {
            dhcp_stop(&wifiNetif);
            wifiDhcp = false;
        }

        if (setting->dhcp && !wifiDhcp)
        {
            dhcp_set_struct(&wifiNetif, &wifiNetifDhcp);
            dhcp_start(&wifiNetif);
            wifiDhcp = true;
        }

        gettimeofday(&wifiLastTime, NULL);
        wifiMscnt = 0;
    }
    else
    {
        if (wifiDhcp)
            dhcp_stop(&wifiNetif);

        netif_set_down(&wifiNetif);

        IP4_ADDR(&wifiIpaddr, setting->ipaddr[0], setting->ipaddr[1], setting->ipaddr[2], setting->ipaddr[3]);
        IP4_ADDR(&wifiNetmask, setting->netmask[0], setting->netmask[1], setting->netmask[2], setting->netmask[3]);
        IP4_ADDR(&wifiGw, setting->gw[0], setting->gw[1], setting->gw[2], setting->gw[3]);

        netif_set_addr(&wifiNetif, &wifiIpaddr, &wifiNetmask, &wifiGw);
        netif_set_up(&wifiNetif);
        wifiDhcp     = false;
    }
}

static void WifiPoll(void)
{
    if ((wifiNetif.ip_addr.addr == 0) && (wifiDhcp))
    {
        struct timeval currTime;

        gettimeofday(&currTime, NULL);
        if (itpTimevalDiff(&wifiLastTime, &currTime) >= DHCP_FINE_TIMER_MSECS)
        {
            if (wifiDhcp)
                dhcp_fine_tmr();

            wifiLastTime = currTime;
            wifiMscnt += DHCP_FINE_TIMER_MSECS;
        }
        else if (wifiMscnt >= DHCP_COARSE_TIMER_MSECS)
        {
            if (wifiDhcp)
                dhcp_coarse_tmr();

            wifiMscnt = 0;
        }
    }

    if(linkUp == true)
    {
        if (!(wifiNetif.flags & NETIF_FLAG_LINK_UP))
        {
    	    linkUp = false;
        }
    }
}

#if CFG_NET_WIFI_POLL_INTERVAL > 0

static void WifiPollHandler(timer_t timerid, int arg)
{
    WifiPoll();
}
#endif // CFG_NET_WIFI_POLL_INTERVAL > 0

#ifdef CFG_NET_WIFI_REDEFINE
static void* WifiInitTask(void* arg)
{
    int result = 0;

    wifiInited = false;
    linkUp = false;

#if CFG_NET_WIFI_POLL_INTERVAL > 0
    {
        timer_t timer;
        struct itimerspec value;
        timer_create(CLOCK_REALTIME, NULL, &timer);
        timer_connect(timer, (VOIDFUNCPTR)WifiPollHandler, 0);
        value.it_value.tv_sec = value.it_interval.tv_sec = 0;
        value.it_value.tv_nsec = value.it_interval.tv_nsec = CFG_NET_WIFI_POLL_INTERVAL * 1000000;
        timer_settime(timer, 0, &value, NULL);
    }
#endif // CFG_NET_WIFI_POLL_INTERVAL > 0

    //wait wifi connected
    while(!wifiConnected)
    {
        printf("[%s] Wait wifi initialize\n", __FUNCTION__);
        usleep(1000*1000);
    }

    // init socket device
    itpRegisterDevice(ITP_DEVICE_SOCKET, &itpDeviceSocket);
    ioctl(ITP_DEVICE_SOCKET, ITP_IOCTL_INIT, NULL);
}
#endif

static void WifiInit(void)
{
#if defined(CFG_NET_WIFI_MAY_NOT_EXIST)
    int nTemp = 0;
#endif
#ifdef CFG_NET_WIFI_REDEFINE
    if (!wifi_init_task)
    {
        pthread_attr_t attr;

        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&wifi_init_task, &attr, WifiInitTask, NULL);
    }
#else
    int result = 0;

    wifiInited = false;
    linkUp = false;

    //wait wifi connected
	while(!wifiConnected)
    {
        printf("wait wifi connected\n");
        usleep(1000*100);
#if defined(CFG_NET_WIFI_MAY_NOT_EXIST)
        // if there is no wifi dongle , return
        nTemp++;
        if (nTemp>WIFI_TEST_INPUT_DONGLE){
            printf("[Itp Wifi] WifiInit no wifi dongle \n");
            return;
        }
#endif
    }

#if CFG_NET_WIFI_POLL_INTERVAL > 0
    {
        timer_t timer;
        struct itimerspec value;
        timer_create(CLOCK_REALTIME, NULL, &timer);
        timer_connect(timer, (VOIDFUNCPTR)WifiPollHandler, 0);
        value.it_value.tv_sec = value.it_interval.tv_sec = 0;
        value.it_value.tv_nsec = value.it_interval.tv_nsec = CFG_NET_WIFI_POLL_INTERVAL * 1000000;
        timer_settime(timer, 0, &value, NULL);
    }
#endif // CFG_NET_WIFI_POLL_INTERVAL > 0
#endif
}

extern int wifiif_ioctrl(struct netif *netif, void* ptr,int cmd);
static int WifiIoctl(int file, unsigned long request, void* ptr, void* info)
{
    if ((SIOCIWFIRST <= request) && (request <= SIOCIWLAST))
    {
        int ret = 0;
        ret = wifiif_ioctrl(&wifiNetif, (void*)ptr,request);
        return ret;
    }

    if(request == (SIOCIWFIRSTPRIV+30) || request == (SIOCIWFIRSTPRIV+28))
    {
        int ret = 0;
        ret = wifiif_ioctrl(&wifiNetif, (void*)ptr,request);
        return ret;
    }



    switch (request)
    {
        case ITP_IOCTL_POLL:
        WifiPoll();
        break;

        case ITP_IOCTL_IS_AVAIL:
        if(WifiCurrentState == STASTATE)
        {
            return wifiNetif.ip_addr.addr? 1 : 0;
        }
        else if(WifiCurrentState == APSTATE)
        {
            return 1;
        }
        return 0;

        case ITP_IOCTL_IS_CONNECTED:
        if(WifiCurrentState == STASTATE)
        {
            static struct net_device_info netinfo = {0};

            if (!(wifiInited))
            {
                LOG_ERR "wifi is not init yet\n" LOG_END
                return 0;
            }

            netinfo.infoType = WLAN_INFO_LINK;
            wifiif_info(&wifiNetif, &netinfo);
            if(netinfo.linkInfo == WLAN_LINK_ON)
            {
                linkUp = true;
            }

            return linkUp;
        }
        else if(WifiCurrentState == APSTATE)
        {
            return 1;
        }
        return 0;

        case ITP_IOCTL_IS_DEVICE_READY:
        return wifiConnected;

        case ITP_IOCTL_SLEEP:
        WifiSleepNotify((int)ptr);
        break;

        case ITP_IOCTL_WIFI_SLEEP_STATUS:
        return sleep_notify;

        case ITP_IOCTL_GET_INFO:
        if(WifiCurrentState == STASTATE)
        {
            WifiGetInfo((ITPWifiInfo*)ptr);
        }
        else if(WifiCurrentState == APSTATE)
        {
            WifiapGetInfo((ITPWifiInfo*)ptr);
        }
        else
        {
            WifiGetInfo((ITPWifiInfo*)ptr);
        }
        break;

        case ITP_IOCTL_WIFIAP_GET_INFO:
        WifiapGetInfo((ITPWifiInfo*)ptr);
        break;

        case ITP_IOCTL_INIT:
        WifiInit();
        break;

#if defined(CFG_NET_WIFI_USB)
        case ITP_IOCTL_WIFI_DRV_REINIT:
        mmpRtlWifiDriverRegister();
        wifiInited = true;
        break;
#endif

        case ITP_IOCTL_WIFI_HIDDEN_SSID:
            WifiLinkHidden((struct net_device_info*)ptr);
        break;

        case ITP_IOCTL_SCAN:
        WifiScan();
        break;

        case ITP_IOCTL_RESET:
        WifiReset((void*)ptr);
        break;

        case ITP_IOCTL_ENABLE:
        printf("[itp wifi] ITP_IOCTL_ENABLE \n");
        netif_set_up(&wifiNetif);
        wifiInited = true;
        WifiCurrentState = STASTATE;
        break;

        case ITP_IOCTL_DISABLE:
        {
            ip_addr_t ipaddr, netmask, gw;

            ip_addr_set_zero(&gw);
            ip_addr_set_zero(&ipaddr);
            ip_addr_set_zero(&netmask);

            netif_set_addr(&wifiNetif, &ipaddr, &netmask, &gw);
            netif_set_down(&wifiNetif);
            printf("[itp wifi] ITP_IOCTL_DISABLE \n");
            wifiInited = false;
            linkUp = false;
            WifiCurrentState = 0;
        }
        break;

        case ITP_IOCTL_WIFI_NETIF_SHUTDOWN:
        netif_set_link_down(&wifiNetif); //close MAC & DMA
#if defined(CFG_NET_WIFI) && defined(CFG_NET_ETHERNET)
        netif_remove(&wifiNetif);
#else
        netif_remove_all(); //1. call netif_set_down   2.wifiif_shutdown by callback
#endif
        break;

        case ITP_IOCTL_RESET_DEFAULT:
        printf("itp wifi set netif  default \n");
        netif_set_default(&wifiNetif);
        break;


#ifdef CFG_NET_WIFI_HOSTAPD
        case ITP_IOCTL_WIFIAP_ENABLE:
        {
            ip_addr_t ipaddr, netmask, gw;
            printf("%s ITP_IOCTL_WIFIAP_ENABLE \n",__FUNCTION__);
#if defined(CFG_NET_ETHERNET) && defined(CFG_NET_WIFI)
            itpWifiLwipInitNetif();
#endif
            ipaddr_aton(CFG_NET_WIFI_IPADDR, &ipaddr);
            ipaddr_aton(CFG_NET_WIFI_NETMASK, &netmask);
            ipaddr_aton(CFG_NET_WIFI_GATEWAY, &gw);
            //ip4_addr1(&ipaddr);

            netif_set_addr(&wifiNetif, &ipaddr, &netmask, &gw);

            netif_set_up(&wifiNetif);
            WifiCurrentState = APSTATE;
            printf(" - %s  ITP_IOCTL_WIFIAP_ENABLE \n",__FUNCTION__);
        }
        break;

        case ITP_IOCTL_WIFIAP_DISABLE:
        {
            ip_addr_t ipaddr, netmask, gw;

            ip_addr_set_zero(&gw);
            ip_addr_set_zero(&ipaddr);
            ip_addr_set_zero(&netmask);

            netif_set_addr(&wifiNetif, &ipaddr, &netmask, &gw);
            netif_set_down(&wifiNetif);
            WifiCurrentState = 0;
        }
        break;
#endif
        case ITP_IOCTL_WIFI_LINK_AP:
        WifiLinkAP((void*)ptr);
        break;

        case ITP_IOCTL_WIFI_START_DHCP:
        if(WifiCurrentState == STASTATE)
            WifiStartDHCP((void*)ptr);
        break;

        case ITP_IOCTL_WIFI_STOP_DHCP:
        if(WifiCurrentState == STASTATE)
            dhcp_stop(&wifiNetif);
        break;

        case ITP_IOCTL_RENEW_DHCP:
        if(WifiCurrentState == STASTATE)
            dhcp_network_changed(&wifiNetif);
        break;

        case ITP_IOCTL_WIFI_STATE:
        return WifiState();

        case ITP_IOCTL_WIFI_STANUM:
        return WifiStaNum() - 2;

        case ITP_IOCTL_WIFI_BEST_CHANNEL:
        return WifiBestChannel();

        case ITP_IOCTL_WIFI_MODE:
        return WifiCurrentState;

        case ITP_IOCTL_WIFI_ADD_NETIF:
            itpWifiLwipInitNetif();
        break;

        case ITP_IOCTL_WIFI_NETIF_STATUS:
            return WifiAddNetIf;

        case ITP_IOCTL_WIFI_GET_NET_DEVICE:
        if (smNetGetDevice() != NULL)
            return 1;
        else
            return 0;

        default:
            errno = (ITP_DEVICE_WIFI << ITP_DEVICE_ERRNO_BIT) | 1;
            return -1;
    }
    return 0;
}

const ITPDevice itpDeviceWifi =
{
    ":wifi",
    itpOpenDefault,
    itpCloseDefault,
    WifiRead,
    itpWriteDefault,
    itpLseekDefault,
    WifiIoctl,
    NULL
};

void WifiConnect(int type)
{
    printf("WifiConnect\n");
    if(USB_DEVICE_WIFI(type))
    {
        printf(" USB : WIFI device is interted!! \n");
        wifiConnected = true;
    }
}

void WifiDisconnect(int type)
{
    if(USB_DEVICE_WIFI(type))
    {
        printf(" USB : WIFI device is disconnected!\n");
        ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_STOP_DHCP, NULL);
        ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_DISABLE, NULL);
        ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_NETIF_SHUTDOWN, NULL);
        tasklet_deinit();
        usleep(20000);
        iteTimerCtrlTerminate();
        usleep(20000);
        wifiConnected = false;
        wifiInited = false;
    }
}

/* ===================== WIFI USB type: End ===================== */

#endif
