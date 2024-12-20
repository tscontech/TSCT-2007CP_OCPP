#include <sys/ioctl.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>
#include "lwip/ip.h"
#include "lwip/dns.h"
#include "ite/itp.h"
#include "iniparser/iniparser.h"
#include "ctrlboard.h"
#ifdef CFG_NET_WIFI
#include "wifiMgr.h"
#endif
#define DHCP_TIMEOUT_MSEC (60 * 1000) //60sec

static bool bNetwrokThread = false;

static bool networkIsReady;
static int networkSocket;
bool networkToReset;

#ifdef CFG_NET_WIFI
static WIFI_MGR_SETTING gWifiSetting;
#endif

static  struct timeval tvStart = {0, 0}, tvEnd = {0, 0};
// wifi init
static int gInit =0;

bool TSCT_NetworkIsReady(void)
{
    return networkIsReady;
}

static uint8_t ConvAsci2Hex(char* asci_buf){
    uint8_t hex_buf;

    if(*asci_buf >= '0' && *asci_buf <= '9')
        hex_buf = (uint8_t)(*asci_buf) - '0';
    else if(*asci_buf >= 'A' && *asci_buf <= 'F')
        hex_buf = (uint8_t)(*asci_buf) - 'A' + 0xa;
    else if(*asci_buf >= 'a' && *asci_buf <= 'f')
        hex_buf = (uint8_t)(*asci_buf) - 'a' + 0xa;
    else
        hex_buf = 0;

    return hex_buf;
}

static void CstIpStrCpy(char *sDst, char *Src)
{
	char *saveptr;
	sprintf(&sDst[0], "%3d", atoi(strtok_r(Src, ".", &saveptr)));
	theConfig.ipaddr[3] = '.';
	sprintf(&sDst[4], "%3d", atoi(strtok_r(NULL, ".", &saveptr)));
	theConfig.ipaddr[7] = '.';
	sprintf(&sDst[8], "%3d", atoi(strtok_r(NULL, ".", &saveptr)));
	theConfig.ipaddr[11] = '.';
	sprintf(&sDst[12], "%3d", atoi(strtok_r(NULL, ".", &saveptr)));
}

#define DHCP_TIMEOUT_MSEC (5 * 1000) //5sec

void ResetEthernet(void) {
			
    ITPEthernetSetting setting;
    ITPEthernetInfo info;
    unsigned long mscnt = 0;
    char buf[16], *saveptr;
    uint8_t mac[6];

    memset(&setting, 0, sizeof (ITPEthernetSetting));

    setting.index = 0;
    // dhcp

    printf("theconfig.dhcp : %d\r\n",theConfig.dhcp);
    if(theConfig.dhcp == 0)
        setting.dhcp =false;
    else
        setting.dhcp =true;

        //setting.dhcp =false;

    // autoip
    setting.autoip = 0;

    // ipaddr
    strcpy(buf, theConfig.ipaddr);
    setting.ipaddr[0] = atoi(strtok_r(buf, ".", &saveptr));
    setting.ipaddr[1] = atoi(strtok_r(NULL, ".", &saveptr));
    setting.ipaddr[2] = atoi(strtok_r(NULL, ".", &saveptr));
    setting.ipaddr[3] = atoi(strtok_r(NULL, " ", &saveptr));
    printf("setting.ipaddr : %d %d %d %d\r\n", setting.ipaddr[0], setting.ipaddr[1], setting.ipaddr[2], setting.ipaddr[3]);
    // netmask
    strcpy(buf, theConfig.netmask);
    setting.netmask[0] = atoi(strtok_r(buf, ".", &saveptr));
    setting.netmask[1] = atoi(strtok_r(NULL, ".", &saveptr));
    setting.netmask[2] = atoi(strtok_r(NULL, ".", &saveptr));
    setting.netmask[3] = atoi(strtok_r(NULL, " ", &saveptr));

    // gateway
    strcpy(buf, theConfig.gw);
    setting.gw[0] = atoi(strtok_r(buf, ".", &saveptr));
    setting.gw[1] = atoi(strtok_r(NULL, ".", &saveptr));
    setting.gw[2] = atoi(strtok_r(NULL, ".", &saveptr));
    setting.gw[3] = atoi(strtok_r(NULL, " ", &saveptr));

    for(int i=0;i<6;i++)    {
        mac[i] = ((ConvAsci2Hex(theConfig.chargermac + (3*i)) << 4) & 0xf0) | (ConvAsci2Hex(theConfig.chargermac + (3*i)+1) & 0x0f);
    }

    if (memcmp(info.hardwareAddress, mac, 6))   {
        printf("\nStored MAC is different\r\n");
        ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_WIRTE_MAC, mac);
        sleep(1);
    }

    ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_RESET, &setting);

    printf("[Network] Wait ethernet cable to plugin\r\n ");
    while (!ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_IS_CONNECTED, NULL) && bGloAdminStatus == false)  {
        sleep(1);
        putchar('.');
        fflush(stdout);
    }
/*
    info.index = 0;
    ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_GET_INFO, &info);

    printf("config mac : %x:%02x:%02x:%02x:%02x:%02x\r\nstore mac : %02x:%02x:%02x:%02x:%02x:%02x\r\n",\
     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], info.hardwareAddress[0], info.hardwareAddress[1], info.hardwareAddress[2], info.hardwareAddress[3], info.hardwareAddress[4], info.hardwareAddress[5]);
	
    
    printf("info.address %d.%d.%d.%d \r\nsetting.ipaddr %d.%d.%d.%d\r\n", ((info.address >> 0)&0xff), ((info.address >> 8)&0xff), ((info.address >> 16)&0xff), ((info.address >> 24)&0xff),setting.ipaddr[0],setting.ipaddr[1],setting.ipaddr[2],setting.ipaddr[3]);
    printf("mac %02x:%02x:%02x:%02x:%02x:%02x\r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
*/	
    if(setting.dhcp == 1)
    {
        printf("\nWait DHCP settings");
        while (!ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_IS_AVAIL, NULL))  {
            char* ip;

	        usleep(100000);
	        mscnt += 100;

            putchar('@');
            fflush(stdout);

            if (mscnt >= DHCP_TIMEOUT_MSEC)
	        {
	            printf("\nDHCP timeout, use default settings\n");
	            setting.dhcp = setting.autoip = 0;
	            ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_RESET, &setting);
	            break;
	        }
        }
        networkIsReady = false;
    }

    for(int i=0;i<500;i++)
	{
		usleep(100*1000);
	    if (ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_IS_AVAIL, NULL))
	    {
	        char ip[16] = {0};

	        info.index = 0;
	        ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_GET_INFO, &info);
	        ipaddr_ntoa_r((const ip_addr_t*)&info.address, ip, sizeof(ip));

			if(setting.dhcp )
			{			
				if(ip != NULL)
				{
					char gw[16] = {0};
					char netmask[16] = {0};
				
					memset(theConfig.ipaddr, 0, 16);
					strncpy(theConfig.ipaddr, ip, strlen(ip));

					memset(theConfig.gw, 0, 16);
					ipaddr_ntoa_r((const ip_addr_t*)&info.gw, gw, sizeof(gw));			
					strncpy(theConfig.gw, gw, strlen(gw));

					memset(theConfig.netmask, 0, 16);
					ipaddr_ntoa_r((const ip_addr_t*)&info.netmask, netmask, sizeof(netmask));			
					strncpy(theConfig.netmask, netmask, strlen(netmask));

					networkIsReady = true;
					break;
				}
			}
			else
			{
				networkIsReady = true;
				break;
			}
	    }
	}

	printf("ResetEthernet : dhcp:%d networkIsReady:%d \n", theConfig.dhcp, networkIsReady);

	printf("IP address: %s \n", theConfig.ipaddr);
	printf("gw address: %s \n", theConfig.gw);
	printf("netmask: %s \n", theConfig.netmask);
}

static void* xNetworkTask(void* arg) {
	ResetEthernet();

	for (;;) {
		networkIsReady = ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_IS_CONNECTED, NULL);
		if (networkToReset) 		{
			ResetEthernet();
			networkToReset = false;
		}
		/*
		if(!networkIsReady){
            NetworkReset();
		}*/		
        //usleep(100*1000);
		sleep(2);
	}
	return NULL;
}

void xNetworkInit(void) {
    pthread_t task;

    networkIsReady = false;
	networkToReset = false;
    pthread_create(&task, NULL, xNetworkTask, NULL);
}

void TSCT_NetworkExit(void)
{
	int reset_var = 0x1;
	
	ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_OFF, (void *)&reset_var);	
	ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_DISABLE, NULL);
	ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_EXIT, NULL);
}

void NetworkReset(void)
{
    networkToReset  = true;	
    networkIsReady = false;
}