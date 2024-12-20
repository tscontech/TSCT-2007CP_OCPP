#if (DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6256 OR DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6236 OR DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6212 OR DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6203)

    add_definitions(
        -DCFG_NET_APXXXX_WIFI_OOB_PIN_NUM="${CFG_NET_APXXXX_WIFI_OOB_PIN_NUM}"
        -DCFG_NET_APXXXX_WIFI_REG_ON_PIN_NUM="${CFG_NET_APXXXX_WIFI_REG_ON_PIN_NUM}"
    )    	 

	INCLUDE_DIRECTORIES(
  	${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_mhd/include
		${PROJECT_SOURCE_DIR}/sdk/driver/sd/mmc/include
		${PROJECT_SOURCE_DIR}/sdk/share/dhcps/
		${PROJECT_SOURCE_DIR}/sdk/include/lwip/        
        
		)

		
	ADD_LIBRARY(non_gpl_wifi_mhd STATIC
		api/bcm_api_test.c
		api/bcm_wifi_test.c

		)
		
#endif()