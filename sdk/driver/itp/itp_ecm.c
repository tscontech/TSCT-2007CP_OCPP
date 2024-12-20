/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * PAL Ethernet functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include <sys/ioctl.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "lwip/autoip.h"
#include "lwip/dns.h"
#include "netif/etharp.h"
#include "itp_cfg.h"
#include "ite/itp.h"
#ifdef CFG_NET_LWIP_2
#include "netif/bridgeif.h" //eason
#ifdef CFG_NET_LWIP_PPPOE
#include "netif/ppp/pppoe.h"
#endif
#endif


err_t ecmif_init(struct netif *netif);
void  ecmif_shutdown(struct netif *netif);
void  ecmif_poll(struct netif *netif);

static struct netif   ethNetifs[1];
static struct dhcp    ethNetifDhcp;

static bool           ethInited, ethDhcp;
static struct timeval ethLastTime;
static int            ethMscnt;
static timer_t        ethTimer;
static volatile bool  ethInPollingFunc;


// This function initializes all network interfaces
static void itpEcmLwipInit(void)
{
    struct netif *xnetif;
    ip_addr_t    ipaddr, netmask, gw;

    ip_addr_set_zero(&ipaddr);
    ip_addr_set_zero(&netmask);
    ip_addr_set_zero(&gw);

    xnetif = &ethNetifs[0];

    netif_set_default(netif_add(xnetif, &ipaddr, &netmask, &gw, NULL, ecmif_init, tcpip_input));
#if LWIP_NETIF_REMOVE_CALLBACK
    netif_set_remove_callback(xnetif, ecmif_shutdown);
#endif
    netif_set_up(xnetif);

    ethInited = true;
}

static void EcmGetInfo(ITPEthernetInfo *info)
{
    struct netif *xnetif;

    if (ethInited == false)
    {
        LOG_ERR "ethernet is not init yet\n" LOG_END
        return;
    }

    if (info->index >= 1)
    {
        LOG_ERR "Out of ecm: %d\n", info->index LOG_END
        return;
    }

    xnetif = &ethNetifs[info->index];

    if (xnetif->flags & NETIF_FLAG_LINK_UP)
        info->flags |= ITP_ETH_LINKUP;

    if (xnetif->ip_addr.addr)
        info->flags |= ITP_ETH_ACTIVE;

    info->address = xnetif->ip_addr.addr;
    info->netmask = xnetif->netmask.addr;
    sprintf(info->displayName, "Ecm%d", info->index);
    sprintf(info->name, "ecm%d", info->index);
}

static void EcmPoll(void)
{
    struct netif *xnetif = &ethNetifs[0];

    if (!ethInited)
        return;

    if ((xnetif->ip_addr.addr == 0)
        && (ethDhcp
            ))
    {
        struct timeval currTime;

        gettimeofday(&currTime, NULL);
        if (itpTimevalDiff(&ethLastTime, &currTime) >= DHCP_FINE_TIMER_MSECS)
        {
            if (ethDhcp)
                dhcp_fine_tmr();

            ethLastTime = currTime;
            ethMscnt   += DHCP_FINE_TIMER_MSECS;
        }
        else if (ethMscnt >= DHCP_COARSE_TIMER_MSECS)
        {
            if (ethDhcp)
                dhcp_coarse_tmr();

            ethMscnt = 0;
        }
    }

    // check for packets and link status
    ecmif_poll(xnetif);
}

#define CFG_NET_ECM_POLL_INTERVAL  50

#if CFG_NET_ECM_POLL_INTERVAL > 0

static void EcmPollHandler(timer_t timerid, int arg)
{
    ethInPollingFunc = true;
    EcmPoll();
    ethInPollingFunc = false;
}

#endif // CFG_NET_ECM_POLL_INTERVAL > 0

static int EcmInit(void)
{
#if CFG_NET_ECM_POLL_INTERVAL > 0
    {
        struct itimerspec value;
        timer_create(CLOCK_REALTIME, NULL, &ethTimer);
        timer_connect(ethTimer, (VOIDFUNCPTR)EcmPollHandler, 0);
        value.it_value.tv_sec = value.it_interval.tv_sec  = 0;
        value.it_value.tv_nsec = value.it_interval.tv_nsec = CFG_NET_ECM_POLL_INTERVAL * 1000000;
        timer_settime(ethTimer, 0, &value, NULL);
    }
#endif // CFG_NET_ECM_POLL_INTERVAL > 0

    itpEcmLwipInit();

    return 0;
}

static void EcmExit(void)
{
    int i;
    struct netif *xnetif = &ethNetifs[0];

    timer_delete(ethTimer);
    while (ethInPollingFunc)
        usleep(1000);

    netif_set_down(xnetif);
    netif_remove(xnetif);
}

static void EcmReset(ITPEthernetSetting *setting)
{
    struct netif *xnetif;
    ip_addr_t    ipaddr, netmask, gw;

    if (setting->index >= 1)
    {
        LOG_ERR "Out of ecm: %d\n", setting->index LOG_END
        return;
    }

    xnetif = &ethNetifs[setting->index];

    xnetif->ip_addr.addr = 0;

    if (setting->dhcp || setting->autoip)
    {
        if (!setting->dhcp && ethDhcp)
        {
            dhcp_stop(xnetif);
            ethDhcp = false;
        }

        if (setting->dhcp)
        {
            dhcp_set_struct(xnetif, &ethNetifDhcp);
            dhcp_start(xnetif);
            ethDhcp = true;
        }

        gettimeofday(&ethLastTime, NULL);
        ethMscnt = 0;
        return;
    }
    else
    {
        if (ethDhcp)
            dhcp_stop(xnetif);

        ethDhcp = false;
    }
}

static int EcmIoctl(int file, unsigned long request, void *ptr, void *info)
{
    switch (request)
    {
    case ITP_IOCTL_POLL:
        EcmPoll();
        break;

    case ITP_IOCTL_IS_AVAIL:
        return ethNetifs[0].ip_addr.addr ? 1 : 0;

    case ITP_IOCTL_IS_CONNECTED:
        return netif_is_link_up(&ethNetifs[0]);


    case ITP_IOCTL_GET_INFO:
        EcmGetInfo((ITPEthernetInfo *)ptr);
        break;

    case ITP_IOCTL_INIT:
        return EcmInit();
        break;

    case ITP_IOCTL_EXIT:
        EcmExit();
        break;

    case ITP_IOCTL_RESET:
        EcmReset((ITPEthernetSetting *)ptr);
        break;

    case ITP_IOCTL_RESET_DEFAULT:
        printf("itp ethernet set netif 0 default \n");
        netif_set_default(&ethNetifs[0]);
        break;

    case ITP_IOCTL_ENABLE:
        netif_set_link_up(&ethNetifs[0]);
        break;

    case ITP_IOCTL_DISABLE:
        netif_set_link_down(&ethNetifs[0]);
        break;

	case ITP_IOCTL_SLEEP:
        netif_set_down(&ethNetifs[0]);
        netif_remove(&ethNetifs[0]);
		break;

	case ITP_IOCTL_RESUME:
        itpEcmLwipInit();
		break;
	
    default:
        errno = (ITP_DEVICE_ECM_EX << ITP_DEVICE_ERRNO_BIT) | 1;
        return -1;
    }
    return 0;
}

const ITPDevice itpDeviceUsbEcmex =
{
    ":ecmex",
    itpOpenDefault,
    itpCloseDefault,
    itpReadDefault,
    itpWriteDefault,
    itpLseekDefault,
    EcmIoctl,
    NULL
};