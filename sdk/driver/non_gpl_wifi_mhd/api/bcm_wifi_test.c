#include <pthread.h>
#include <string.h>
#include <sys/ioctl.h>
#include "openrtos/FreeRTOS.h"
#include "openrtos/task.h"
#include "lwip/tcpip.h"
#include "ith/ith_gpio.h"
#include "ite/ite_sd.h"
#include "wifi_nvram_image.h"

char *wifi_nvram_ptr = (char *)wifi_nvram_image;
int wifi_nvram_size = (int)sizeof(wifi_nvram_image);

/*=====================WIFI=====================*/
#define WIFI_OOB_PIN_NUM         26  // 38   //WL_HOST_WAKE
#define WIFI_WLREG_ON_PIN_NUM    25  // 40   //WIFI_EN_GPIO

#define MK_CNTRY(a, b, rev)                          \
   (((unsigned char)(a)) + (((unsigned char)(b)) << 8) + \
   (((unsigned short)(rev)) << 16))

extern int wifi_api_test(int argc, char **argv);
extern const char *get_security_string_by_index(uint8_t security);
extern int lwip_ping(int argc, char **argv);
extern void lwip_iperf_tcp(int argc, char **argv);
extern void lwip_iperf_udp(int argc, char **argv);

//TEST_MODE
#define SOFTAP_TEST_MODE			0
#define STA_TEST_MODE				1
#define STA_LOOP_TEST_MODE			2
#define STA_IPERF_TEST_MODE			3
#define STA_SCAN_TEST_MODE			4
#define MODULE_INIT_EXIT_STA_LOOP_TEST_MODE			5

//IPERF_TEST_MODE
#define IPERF_TCP_CLIENT_TEST		0
#define IPERF_TCP_SERVER_TEST		1
#define IPERF_UDP_CLIENT_TEST		2
#define IPERF_UDP_SERVER_TEST		3

#define TEST_MODE					STA_IPERF_TEST_MODE //SOFTAP_TEST_MODE
#define IPERF_TEST_MODE				IPERF_TCP_SERVER_TEST	

#if (TEST_MODE == SOFTAP_TEST_MODE)	
//Softap settings
char softap_ssid[] = "iteAP";
char softap_passwd[] = "12345678";
char *softap_security = "2";		//0-open, 1-wpa_psk_aes, 2-wpa2_psk_aes
char *softap_channel = "12";
char client_IP_Addr[] = "192.168.1.2";	
#else
//Sta settings
char sta_ssid[] = "TOTOLINK";
char sta_passwd[] = "12345678";
char *sta_security = "2";			//0-open, 1-wpa_psk_aes, 2-wpa2_psk_aes
char *sta_channel = "11";
int max_test_count = 300000;				//For STA_LOOP_TEST_MODE, STA_SCAN_TEST_MODE, MODULE_INIT_EXIT_STA_LOOP_TEST_MODE
#endif
	
char iperf_PC_IP_Addr[] = "192.168.11.4";
int argc;
char *p;

void osal_host_rtos_get_mhdtask_settings( uint32_t *priority, uint32_t *stack_size )
{
    *priority = 2; 
    *stack_size = (80000);
}


// 0: client mode , 1 : ap mode
int nWifiMode = 0;
void wifi_drv_on(int nSD)
{
	int ret;

    tcpip_init(NULL, NULL);
    mhd_gpio_regon_register(CFG_NET_APXXXX_WIFI_REG_ON_PIN_NUM);
    mhd_gpio_oob_register(CFG_NET_APXXXX_WIFI_OOB_PIN_NUM, ITH_GPIO_PULL_UP);

    #ifdef CFG_NET_WIFI_SDIO_NGPL_AP6256
        mhd_set_country_code(MK_CNTRY( 'C', 'N', 38 )); 
    #endif

#ifdef CFG_NET_WIFI_SDIO_NGPL_AP6203
        mhd_set_country_code(MK_CNTRY( 'C', 'N', 0 )); 
#endif

    //mhd_set_country_code(MK_CNTRY( 'C', 'N', 0 ));
    //mhd_set_country_code(MK_CNTRY( 'J', 'P', 3 ));
    if (nSD == 0)
        mhd_sdio_controller_index_register( SD_0 );
    else if(nSD == 1)
        mhd_sdio_controller_index_register( SD_1 );

    ret = mhd_module_init();  
    
    printf("------------------ BUILD : %s %s %d %d %d ------------------\n", __DATE__, __TIME__,nSD,CFG_NET_APXXXX_WIFI_OOB_PIN_NUM,CFG_NET_APXXXX_WIFI_REG_ON_PIN_NUM);

    if(ret)
        return;

    sleep(1);   

    #ifdef CFG_NET_WIFI_SDIO_NGPL_AP6256
	mhd_get_ampdu_hostreorder_support();
	mhd_set_ampdu_hostreorder_support(false);
    #endif
	
    #ifdef CFG_NET_WIFI_SDIO_NGPL_AP6203
	mhd_get_ampdu_hostreorder_support();
	mhd_set_ampdu_hostreorder_support(false);        
    #else
        //mhd_set_wifi_11n_support(false);
    #endif
}

void wifi_on()
{
    mhd_module_init();
}

void wifi_off()
{

  if(nWifiMode == 0) 
  {
      mhd_sta_disconnect( 1 );
      mhd_sta_network_down(); 
      sleep(1);

  } else if(nWifiMode ==1) {
      //mhd_softap_stop_dhcpd();
      mhd_softap_stop( 1 );   
      sleep(1);      
  }
  mhd_module_exit();

}

// 0: client mode , 1 : ap mode
void wifi_set_mode(int nMode)
{
    if (nMode ==0)
        nWifiMode = nMode;
    else if (nMode ==1)
        nWifiMode = nMode;

}

void bcm_drv_init(void) 
{
	int ret;
	tcpip_init(NULL, NULL);
	mhd_gpio_regon_register(WIFI_WLREG_ON_PIN_NUM);
	mhd_gpio_oob_register(WIFI_OOB_PIN_NUM, ITH_GPIO_PULL_UP);
	//mhd_set_country_code(MK_CNTRY( 'C', 'N', 0 ));
	mhd_set_country_code(MK_CNTRY( 'J', 'P', 3 ));
	
	mhd_sdio_controller_index_register( SD_0 );
	
	ret = mhd_module_init();  
	printf("------------------ BUILD : %s %s ------------------\n", __DATE__, __TIME__);
	
	if(ret)
		return;

	sleep(1);	

#if (TEST_MODE == STA_SCAN_TEST_MODE)
	for(int i=0; i<max_test_count; i++) 
	{
		argc = 2;	
		char *argv[] = {"api", "2"};
		wifi_api_test(argc, argv);	
		sleep(1);
	}
#else
#if (TEST_MODE == SOFTAP_TEST_MODE)	
	//--------- softap mode testing  --------- 		
	int max_delay = 60;
	argc = 6;
	//softap start
	char *argv[] = {"api", "6", softap_ssid, softap_passwd, softap_security, softap_channel};
	printf("SSDI = %s\nPassword = %s\nSecurity = %s\nChannel = %s\n", argv[2], argv[3], get_security_string_by_index(strtol(argv[4], &p, 10)), argv[5]);
	wifi_api_test(argc, argv);

	for(int i=0; i<max_delay; i++)
	{
		printf("delay %d sec (%d)\n", i+1, max_delay);
		sleep(1);
	}
	//ping test
	char *ping_argv[] = {"ping", client_IP_Addr};
	lwip_ping(argc, ping_argv);

	//get mac list
	argv[1] = "8";		
	wifi_api_test(argc, argv);
	
	sleep(1);	
	//softap stop
	argv[1] = "7";		
	wifi_api_test(argc, argv);	

	//------- softap mode testing end -------- 
#else 
	//---------   sta mode testing   --------- 
	argc = 6;	
	char *argv[] = {"api", "0", sta_ssid, sta_passwd, sta_security, sta_channel};
	printf("SSDI = %s\nPassword = %s\nSecurity = %s\nChannel = %s\n\n", argv[2], argv[3], get_security_string_by_index(strtol(argv[4], &p, 10)), argv[5]);
	
#if (TEST_MODE == STA_TEST_MODE) || (TEST_MODE == STA_IPERF_TEST_MODE)
		//connect
		wifi_api_test(argc, argv);
		
		//get rssi
		argv[1] = "3";
		wifi_api_test(argc, argv);		
#else	
	for(int i=0; i<max_test_count; i++) 
	{
		printf("----- sta loop test %d (Max: %d) -----\n", i+1, max_test_count);
		argc = 6;
		argv[1] = "0";
		wifi_api_test(argc, argv);

		argc = 2;
		//ping test
		char *ping_argv[] = {"ping", iperf_PC_IP_Addr};
		lwip_ping(argc, ping_argv);
		//get rssi
		argv[1] = "3";
		wifi_api_test(argc, argv);
		//get rate
		argv[1] = "4";
		wifi_api_test(argc, argv);
		//get mac addr
		argv[1] = "5";
		wifi_api_test(argc, argv);
		//scan
		argv[1] = "2";
		wifi_api_test(argc, argv);
		//ping test
		lwip_ping(argc, ping_argv);
#if (TEST_MODE == STA_LOOP_TEST_MODE)
		//disconnect
		argv[1] = "1";
		wifi_api_test(argc, argv);
		sleep(1);
#else
	/* MODULE_INIT_EXIT_STA_LOOP_TEST_MODE */
		//module exit
		argv[1] = "9";
		wifi_api_test(argc, argv);
		
		//module init
		argv[1] = "10";
		ret = wifi_api_test(argc, argv);		
		if(ret)
			return;
		
		sleep(1);	
#endif
	}		
#endif	//#if (TEST_MODE == STA_TEST_MODE) || (TEST_MODE == STA_IPERF_TEST_MODE)
	//--------- sta mode testing end --------- 
#endif

#if (TEST_MODE == STA_IPERF_TEST_MODE)
	//---------    iperf testing     --------- 
#if (IPERF_TEST_MODE == IPERF_TCP_CLIENT_TEST) || (IPERF_TEST_MODE == IPERF_TCP_SERVER_TEST)
	/*	iperf TCP testing  */
#if (IPERF_TEST_MODE == IPERF_TCP_CLIENT_TEST)			
	/*	iperf TCP client testing  */
	char *iperf_argv[] = {"iperf_tcp", "-c", iperf_PC_IP_Addr, "-t", "60", "-i", "60"};
	argc = 7;
	lwip_iperf_tcp(argc, iperf_argv);
#else
	/*	iperf TCP server testing  */
	char *iperf_argv[] = {"iperf_tcp", "-s", "-i", "5"};
	argc = 4;
	lwip_iperf_tcp(argc, iperf_argv);
#endif
#else
	/*	iperf UDP testing  */		
#if (IPERF_TEST_MODE == IPERF_UDP_CLIENT_TEST)	
	/*	iperf UDP client testing  */
	char *iperf_argv[] = {"iperf_udp", "-c", iperf_PC_IP_Addr, "-t", "60", "-i", "60", "-b", "40M"};
	argc = 9;
	lwip_iperf_udp(argc, iperf_argv);
#else
	/*	iperf UDP server testing  */
	char *iperf_argv[] = {"iperf_udp", "-s", "-i", "5"};
	argc = 4;
	lwip_iperf_udp(argc, iperf_argv);
#endif	
#endif
	//---------   iperf testing end  --------- 
#endif	//#if (TEST_MODE == STA_IPERF_TEST_MODE)
#endif	//#if (TEST_MODE == STA_SCAN_TEST_MODE)
	printf(">>>>>>>> ok\n");
	while(1)
	{
		sleep(1);
	}	
}


