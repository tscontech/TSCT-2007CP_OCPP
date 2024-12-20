if (DEFINED CFG_NET_WIFI_SDIO_NGPL_8723DS)
  
	INCLUDE_DIRECTORIES(
	    ${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_8723d
        ${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_8723d/include
		${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_8723d/api/wifi/		
		${PROJECT_SOURCE_DIR}/sdk/driver/usb/inc
		${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_8723d/os/customer_rtos/
		${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_8723d/os/os_dep/include/
        ${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_8723d/platform/include
		${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_8723d/platform/rtwlan_bsp/
		${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_8723d/platform/sdio/core/
		${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_8723d/platform/sdio/include/
		${PROJECT_SOURCE_DIR}/sdk/driver/sd/mmc/include
		${PROJECT_SOURCE_DIR}/sdk/share/dhcps/
		${PROJECT_SOURCE_DIR}/sdk/include/lwip/        
        
		)

    LINK_LIBRARIES(${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_8723d/wlan_lib/wlan_lib.a)    

		
	ADD_LIBRARY(non_gpl_wifi_8723d STATIC
		api/wifi/wifi_conf.c
		api/wifi/wifi_ind.c
		api/wifi/wifi_promisc.c
		api/wifi/wifi_util.c
		api/wifi_interactive_mode.c
		
		os/customer_rtos/customer_rtos_service.c
		os/os_dep/device_lock.c

		os/os_dep/net_stack_intf.c
		os/os_dep/osdep_service.c    
		
		platform/rtwlan_bsp/rtwlan_bsp.c
		platform/sdio/wifi_io.c

		)
		
	target_link_libraries(non_gpl_wifi_8723d ${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_8723d/wlan_lib/wlan_lib.a)
endif()