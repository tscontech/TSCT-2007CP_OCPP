if (DEFINED CFG_NET_WIFI_SDIO_NGPL OR DEFINED CFG_NET_WIFI_SDIO_NGPL_8723DS OR DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6256 OR DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6236 OR DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6212 OR DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6203 OR DEFINED CFG_NET_WIFI_8188EUS)

	INCLUDE_DIRECTORIES(
			${PROJECT_SOURCE_DIR}/sdk/share/wifi_mgr/
			${PROJECT_SOURCE_DIR}/sdk/share/dhcps/
			${PROJECT_SOURCE_DIR}/sdk/include/lwip/       
		)
	if (DEFINED CFG_NET_WIFI_SDIO_NGPL)
		INCLUDE_DIRECTORIES(
			${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi/include/
			${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi/api/wifi/
		)
	endif()
	
	if (DEFINED CFG_NET_WIFI_SDIO_NGPL_8723DS)	
		INCLUDE_DIRECTORIES(
			${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_8723d/include/
			${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_8723d/api/wifi/
		)
	endif()

	if (DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6256 OR DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6236 OR DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6212 OR DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6203)
			INCLUDE_DIRECTORIES(
			${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_mhd/include/
		)
	endif()    

	if (DEFINED CFG_NET_WIFI_SDIO_NGPL OR DEFINED CFG_NET_WIFI_SDIO_NGPL_8723DS)
	add_library(wifi_mgr STATIC
	    wifiMgr_ngpl.c
	    wifiMgr.h
	    )
	elseif(DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6256 OR DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6236 OR DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6212 OR DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6203)
	add_library(wifi_mgr STATIC
	    wifiMgr_ngpl_mhd.c
	    wifiMgr.h
	    )
	else()
	add_library(wifi_mgr STATIC
	    wifiMgr.c
	    wifiMgr.h
	    )
	endif()    
else()
add_library(wifi_mgr STATIC
    wifiMgr.c
    wifiMgr.h
    ) 
endif()  