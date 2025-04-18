set(PROJ_NAME ${CMAKE_PROJECT_NAME})

if (DEFINED CFG_BOOTLOADER_ENABLE)
    set(CMAKE_PROJECT_NAME bootloader)
    add_subdirectory(../bootloader ../bootloader)
endif()

set(CMAKE_PROJECT_NAME ${PROJ_NAME})

if (DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6256 OR DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6236 OR DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6181)
SET (WIFI_PATH
    ${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_mhd/wlan_lib
)
endif()

if (DEFINED CFG_WATCHDOG_ENABLE)
    add_definitions(
        -DCFG_WATCHDOG_ENABLE
    )
endif()

add_definitions(
    -DCFG_WEBSERVER_PORT=${CFG_WEBSERVER_PORT}
    -DCFG_WEBSERVER_PORT_STR="${CFG_WEBSERVER_PORT}"
    -DCFG_HW_VERSION="${CFG_HW_VERSION}"
    -DCFG_UPGRADE_FTP_URL="${CFG_UPGRADE_FTP_URL}"
    )

if (DEFINED CFG_CHECK_FILES_CRC_ON_BOOTING)
    add_definitions(
        -DCFG_CHECK_FILES_CRC_ON_BOOTING
        )
endif()

if (DEFINED CFG_PLAY_VIDEO_ON_BOOTING)
    add_definitions(
        -DCFG_PLAY_VIDEO_ON_BOOTING
        -DCFG_BOOT_VIDEO_BGCOLOR=${CFG_BOOT_VIDEO_BGCOLOR}
        -DCFG_BOOT_VIDEO_VOLUME=${CFG_BOOT_VIDEO_VOLUME}
        )
endif()

if (DEFINED CFG_ENABLE_ROTATE)
    add_definitions(
        -DCFG_ENABLE_ROTATE
        )
endif()

if (DEFINED CFG_BOOT_VIDEO_ENABLE_WINDOW_MODE)
    add_definitions(
        -DCFG_BOOT_VIDEO_ENABLE_WINDOW_MODE
        -DCFG_VIDEO_WINDOW_X_POS=${CFG_VIDEO_WINDOW_X_POS}
        -DCFG_VIDEO_WINDOW_Y_POS=${CFG_VIDEO_WINDOW_Y_POS}
        -DCFG_VIDEO_WINDOW_WIDTH=${CFG_VIDEO_WINDOW_WIDTH}
        -DCFG_VIDEO_WINDOW_HEIGHT=${CFG_VIDEO_WINDOW_HEIGHT}
        )
endif()

if (DEFINED CFG_SCREENSHOT_ENABLE)
    add_definitions(
        -DCFG_SCREENSHOT_ENABLE
        )
endif()

if (DEFINED CFG_UPGRADE_GUI)
    add_definitions(
        -DCFG_UPGRADE_GUI
        )
    execute_process(COMMAND dataconv
        -x ${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/${CFG_ITU_PATH}/upgrade.itu
        -o ${CMAKE_CURRENT_BINARY_DIR}/upgrade.inc
        )
endif()

if (DEFINED CFG_UPGRADE_DOWNLOAD_STORAGE)
    add_definitions(
        -DCFG_UPGRADE_DOWNLOAD_STORAGE
        -DCFG_UPGRADE_DOWNLOAD_POS=${CFG_UPGRADE_DOWNLOAD_POS}
        )

    ITE_ADD_DEFINITIONS_IF_DEFINED(CFG_UPGRADE_DOWNLOAD_NAND)
    ITE_ADD_DEFINITIONS_IF_DEFINED(CFG_UPGRADE_DOWNLOAD_NOR)
    ITE_ADD_DEFINITIONS_IF_DEFINED(CFG_UPGRADE_DOWNLOAD_SD0)
    ITE_ADD_DEFINITIONS_IF_DEFINED(CFG_UPGRADE_DOWNLOAD_SD1)
endif()

if (DEFINED CFG_DYNAMIC_LOAD_TP_MODULE)
    add_definitions(
        -DCFG_DYNAMIC_LOAD_TP_MODULE
        )      
        
    if (DEFINED CFG_TOUCH_ENABLE)

    	unset(modules)
    	foreach (m ${CFG_TOUCH_MODULE})
    	    set(modules "${modules} ${m}")
    	endforeach()
    
        add_definitions(
            -DCFG_CTRLBOARD_TOUCH_MODULE="${modules}"
            ) 
    endif()
endif()

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
if (DEFINED CFG_NET_WIFI_SDIO_NGPL)
	${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi/include/
    ${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi/api/wifi/
endif()
if (DEFINED CFG_NET_WIFI_SDIO_NGPL_8723DS)
	${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_8723d/include/
    ${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_8723d/api/wifi/
endif()
	)
if (DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6256 OR DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6236 OR DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6181)	
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
		${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_mhd/include	
)
endif()

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/sdk/driver/mmp/include
    ${PROJECT_SOURCE_DIR}/sdk/share/audio_mgr
    ${PROJECT_SOURCE_DIR}/sdk/share/ffmpeg
	${PROJECT_SOURCE_DIR}/sdk/share/lwip_ftpd
    ${PROJECT_SOURCE_DIR}/sdk/share/microhttpd
    ${PROJECT_SOURCE_DIR}/sdk/share/microhttpd/include
    ${PROJECT_SOURCE_DIR}/sdk/share/ping/  
    ${PROJECT_SOURCE_DIR}/sdk/share/wifi_mgr/ 
    ${PROJECT_SOURCE_DIR}/sdk/driver/itv
    ${PROJECT_SOURCE_DIR}/sdk/share/qrencode/

if (CFG_CHIP_FAMILY STREQUAL 9070)
    include_directories(
        ${PROJECT_SOURCE_DIR}/sdk/share/itu
        ${PROJECT_SOURCE_DIR}/sdk/share/itu/it9070
    )
else()
    include_directories(
        ${PROJECT_SOURCE_DIR}/sdk/share/itu
        ${PROJECT_SOURCE_DIR}/sdk/share/itu/it9850
    )
endif()
    )
add_executable(${CMAKE_PROJECT_NAME}
    aria.c
    audio.c
    config.c
	cstrtc.c
    ctrlboard.h
    external.c
    external_process.c
    function_table.c
    main.c
    photo.c
    py_ime.c
    reset.c
    scene.c
    scene.h
    screen.c
    storage.c
    string.c
    upgrade.c
    backup.c
    tsctcommon.c
    cstcontrolpilot.c
    cstwatthourmeter.c
    cstmagneticcontactor.c
    cstled.c
    cstbuzzer.c
    cstemergencybutton.c
    cstcardreader.c
    cstac220.c
    cstbacklight.c
    cstlog.c
    #tsctclient.c
    cstordersequence.c
    cstping.c
    layer_common.c
    #tsctpacket.c
    tsctfaultmanage.c
    tsctwsclient.c
    ocpp_cmd.c
    tsctparsing.c
    tsctobd.c
    tsctsecc.c
    temperature_humidity.c
    #qrencode
    tsctqrencode/bitstream.c
    tsctqrencode/mask.c
    tsctqrencode/mmask.c
    tsctqrencode/mqrspec.c
    tsctqrencode/qrenc.c
    tsctqrencode/qrencode.c
    tsctqrencode/qrinput.c
    tsctqrencode/qrspec.c
    tsctqrencode/rsecc.c
    tsctqrencode/split.c
    )

# layers
include(layer.cmake)

if (DEFINED CFG_NET_ENABLE)
    add_executable(${CMAKE_PROJECT_NAME}
		network_func.c
		network_main.c
		network_config.h
        webserver.c
        network.c    
        )
endif()

if (DEFINED CFG_NET_ETHERNET)
    add_executable(${CMAKE_PROJECT_NAME}
        network_ethernet.c
        )
endif()

if (DEFINED CFG_NET_WIFI)
    add_executable(${CMAKE_PROJECT_NAME}
        network_wifi.c
        )
endif()

# upgrade fw from uart
if (DEFINED CFG_UPGRADE_FROM_UART_RUN_TIME)
    add_executable(${CMAKE_PROJECT_NAME}
        upgrade_uart.c
        )
endif()

# boot video
if (DEFINED CFG_PLAY_VIDEO_ON_BOOTING)
	add_executable(${CMAKE_PROJECT_NAME}
		video.c
		)
		
	file(COPY
		${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/media/boot/boot.mp4
		DESTINATION ${CMAKE_BINARY_DIR}/data/private/media
		)
endif()

if (DEFINED CFG_PLAY_MJPEG_ON_BOOTING)
	add_executable(${CMAKE_PROJECT_NAME}
		video_mjpeg.c
		)
		
	file(COPY
        ${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/media/boot/${CFG_BOOT_MJPEG_FILE}
		DESTINATION ${CMAKE_BINARY_DIR}/data/private/media
		)
endif()

# files
configure_file(${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/ctrlboard.ini ${CMAKE_BINARY_DIR}/data/public/ctrlboard.ini COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/unitcost.dat ${CMAKE_BINARY_DIR}/data/public/unitcost.dat COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/userinfo.dat ${CMAKE_BINARY_DIR}/data/public/userinfo.dat COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/notsent.dat ${CMAKE_BINARY_DIR}/data/public/notsent.dat COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/daydata.dat ${CMAKE_BINARY_DIR}/data/public/daydata.dat COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/chargend.dat ${CMAKE_BINARY_DIR}/data/public/chargend.dat COPYONLY)
file(COPY
    ${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/sounds
    DESTINATION ${CMAKE_BINARY_DIR}/data/private
    )

if (DEFINED CFG_LCD_MULTIPLE)
    file(COPY
        ${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/itu
        DESTINATION ${CMAKE_BINARY_DIR}/data/private
        PATTERN "*.xml" EXCLUDE
        PATTERN "*.bak" EXCLUDE
        PATTERN "*.xls" EXCLUDE
        PATTERN "*_rotate" EXCLUDE
        PATTERN "upgrade.itu" EXCLUDE
        )
else()
    file(GLOB files ${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/${CFG_ITU_PATH}/*.*)
    file(COPY
        ${files}
        DESTINATION ${CMAKE_BINARY_DIR}/data/private
        PATTERN "*.xml" EXCLUDE
        PATTERN "*.bak" EXCLUDE    
        )
endif()

#if (DEFINED CFG_NET_WIFI_WPA_ENABLE)
#configure_file(
#    ${PROJECT_SOURCE_DIR}/sdk/share/wifi_hostapd/hostapd_wpa.conf.in
#    ${CMAKE_BINARY_DIR}/data/public/hostapd.conf
#    NEWLINE_STYLE UNIX
#    )
#else()
#configure_file(
#    ${PROJECT_SOURCE_DIR}/sdk/share/wifi_hostapd/hostapd.conf.in
#    ${CMAKE_BINARY_DIR}/data/public/hostapd.conf
#    NEWLINE_STYLE UNIX
#    )
#endif()

# backups
configure_file(${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/ctrlboard.ini ${CMAKE_BINARY_DIR}/data/private/backup/${CFG_PUBLIC_DRIVE}/ctrlboard.ini COPYONLY)
# crc
if (DEFINED CFG_CHECK_FILES_CRC_ON_BOOTING)
    file(REMOVE ${CMAKE_BINARY_DIR}/data/private/backup/${CFG_PUBLIC_DRIVE}/ite_crc.dat)
    execute_process(COMMAND makecrc -c ${CMAKE_BINARY_DIR}/data/public/ite_crc.dat -i ${CMAKE_BINARY_DIR}/data/private/backup/${CFG_PUBLIC_DRIVE})
    execute_process(COMMAND makecrc -v ${CMAKE_BINARY_DIR}/data/public/ite_crc.dat -i ${CMAKE_BINARY_DIR}/data/private/backup/${CFG_PUBLIC_DRIVE})
    file(COPY ${CMAKE_BINARY_DIR}/data/public/ite_crc.dat DESTINATION ${CMAKE_BINARY_DIR}/data/private/backup/${CFG_PUBLIC_DRIVE})
endif()

# font
file(COPY
    ${PROJECT_SOURCE_DIR}/data/font/${CFG_FONT_FILENAME}
    DESTINATION ${CMAKE_BINARY_DIR}/data/private/font
    )

# web
file(GLOB files ${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/web/*.*)
foreach (src ${files})
    string(REPLACE "${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/web/" "${CMAKE_BINARY_DIR}/data/private/web/" dest ${src})
    configure_file(${src} ${dest} COPYONLY)
endforeach()

# screen saver
file(COPY
    ${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/screensaver.jpg
    DESTINATION ${CMAKE_BINARY_DIR}/data/public
    )

# codec
if (DEFINED CFG_COMPRESS_AUDIO_PLUGIN AND DEFINED CFG_AUDIO_ENABLE)
    configure_file(${CMAKE_BINARY_DIR}/lib/sm32/mp3.codecs ${CMAKE_BINARY_DIR}/data/private/codec/mp3.codecs COPYONLY)
#    configure_file(${CMAKE_BINARY_DIR}/lib/sm32/aac.codecs ${CMAKE_BINARY_DIR}/data/private/codec/aac.codecs COPYONLY)    
#    configure_file(${CMAKE_BINARY_DIR}/lib/sm32/wma.codecs ${CMAKE_BINARY_DIR}/data/private/codec/wma.codecs COPYONLY)
    configure_file(${CMAKE_BINARY_DIR}/lib/sm32/wave.codecs ${CMAKE_BINARY_DIR}/data/private/codec/wave.codecs COPYONLY)
endif()

#if (DEFINED CFG_NET_WIFI_WPA_ENABLE)
#configure_file(
#    ${PROJECT_SOURCE_DIR}/sdk/share/wifi_hostapd/hostapd_wpa.conf.in
#    ${CMAKE_BINARY_DIR}/data/private/backup/${CFG_PUBLIC_DRIVE}/hostapd.conf
#    NEWLINE_STYLE UNIX
#    )
#else()    
#configure_file(
#    ${PROJECT_SOURCE_DIR}/sdk/share/wifi_hostapd/hostapd.conf.in
#    ${CMAKE_BINARY_DIR}/data/private/backup/${CFG_PUBLIC_DRIVE}/hostapd.conf
#    NEWLINE_STYLE UNIX
#    )
#endif()

# media
file(GLOB files ${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/media/*.*)
foreach (src ${files})
   string(REPLACE "${PROJECT_SOURCE_DIR}/project/${CMAKE_PROJECT_NAME}/media/" "${CMAKE_BINARY_DIR}/data/public/media/" dest ${src})
   configure_file(${src} ${dest} COPYONLY)
endforeach()

if (DEFINED CFG_DYNAMIC_LOAD_TP_MODULE)
    add_executable(${CMAKE_PROJECT_NAME}
        load_tp_module.c 
        )
endif()

if (DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6256)
    if (DEFINED CFG_NET_LWIP)
        set (PREBUILD_LIB 
            ${WIFI_PATH}/libmhd4345x-LwIP_1_4_1.a
        )   
    endif()
    if (DEFINED CFG_NET_LWIP_2)
        set (PREBUILD_LIB 
            ${WIFI_PATH}/libmhd4345x-LwIP_2_1_2.a
        )
    endif()     
endif()

if (DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6236)
    if (DEFINED CFG_NET_LWIP)
        set (PREBUILD_LIB 
            ${WIFI_PATH}/libmhd4343x-LwIP_1_4_1.a
        )   
    endif()
    if (DEFINED CFG_NET_LWIP_2)
        set (PREBUILD_LIB 
            ${WIFI_PATH}/libmhd4343x-LwIP_2_1_2.a
        )
    endif()     
endif()

if (DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6212)
    if (DEFINED CFG_NET_LWIP)
        set (PREBUILD_LIB 
            ${WIFI_PATH}/libmhd43438-LwIP_1_4_1.a
        )   
    endif()
    if (DEFINED CFG_NET_LWIP_2)
        set (PREBUILD_LIB 
            ${WIFI_PATH}/libmhd43438-LwIP_2_1_2.a
        )
    endif()     
endif()

if (DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6203)
    if (DEFINED CFG_NET_LWIP)
        set (PREBUILD_LIB 
            ${WIFI_PATH}/libmhd4301x-LwIP_1_4_1.a
        )   
    endif()
    if (DEFINED CFG_NET_LWIP_2)
        set (PREBUILD_LIB 
            ${WIFI_PATH}/libmhd4301x-LwIP_2_1_2.a
        )
    endif()     
endif()

if (DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6256 OR 
    DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6236 OR 
    DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6212 OR 
    DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6203)
    file(COPY ${PREBUILD_LIB}
        DESTINATION ${CMAKE_BINARY_DIR}/lib/${CFG_CPU_NAME}/
    )
endif()

# build
include(${PROJECT_SOURCE_DIR}/sdk/build.cmake)
