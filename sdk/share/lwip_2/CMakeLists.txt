if (CMAKE_BUILD_TYPE STREQUAL Debug)
    add_definitions(
        -DLWIP_DEBUG
        )
endif()

include_directories(
    $ENV{CFG_PLATFORM}
	${PROJECT_SOURCE_DIR}/sdk/include/lwip
	${PROJECT_SOURCE_DIR}/sdk/include/netif
	
	#lwip/apps
	${PROJECT_SOURCE_DIR}/sdk/include/lwip/apps
	
	#lwip/compat
	${PROJECT_SOURCE_DIR}/sdk/include/lwip/compat/posix
	${PROJECT_SOURCE_DIR}/sdk/include/lwip/compat/posix/arpa
	${PROJECT_SOURCE_DIR}/sdk/include/lwip/compat/posix/sys
	${PROJECT_SOURCE_DIR}/sdk/include/lwip/compat/stdc/errno
	
	#lwip/priv
	${PROJECT_SOURCE_DIR}/sdk/include/lwip/priv
	
	#lwip/prot
	${PROJECT_SOURCE_DIR}/sdk/include/lwip/prot
    
    #netif/ppp
	${PROJECT_SOURCE_DIR}/sdk/include/netif/ppp
	${PROJECT_SOURCE_DIR}/sdk/include/netif/ppp/polarssl
    )

add_library(lwip_2 STATIC
    #api/
	api/api_lib.c
    api/api_msg.c
    api/err.c
	api/if_api.c
    api/netbuf.c
    api/netdb.c
    api/netifapi.c
    api/sockets.c
    api/tcpip.c
	
	#core/ipv4/
    core/ipv4/autoip.c
	core/ipv4/dhcp.c
	core/ipv4/etharp.c
    core/ipv4/icmp.c
    core/ipv4/igmp.c
	core/ipv4/ip4.c
    core/ipv4/ip4_addr.c
    core/ipv4/ip4_frag.c
	
	#core/snmp/
    #core/snmp/asn1_dec.c
    #core/snmp/asn1_enc.c
    #core/snmp/mib_structs.c
    #core/snmp/mib2.c
    #core/snmp/msg_in.c
    #core/snmp/msg_out.c
    
	#core/
	core/altcp.c
	core/altcp_alloc.c
	core/altcp_tcp.c
	core/def.c
    core/dns.c
	core/inet_chksum.c
    core/init.c
	core/ip.c
    core/mem.c
    core/memp.c
    core/netif.c
	core/packet.c
    core/pbuf.c
    core/raw.c
    core/stats.c
    core/sys.c
    core/tcp.c
    core/tcp_in.c
    core/tcp_out.c
    core/timeouts.c
    core/udp.c

	#netif/
	netif/bridgeif.c
	netif/bridgeif_fdb.c
	netif/ethernet.c
    netif/slipif.c
	netif/zepif.c
    
    #netif/ppp
    netif/ppp/auth.c
    netif/ppp/ccp.c
    netif/ppp/chap_ms.c
    netif/ppp/chap-md5.c
    netif/ppp/chap-new.c
    netif/ppp/demand.c
    netif/ppp/eap.c
    netif/ppp/ecp.c
    netif/ppp/eui64.c
    netif/ppp/fsm.c
    netif/ppp/ipcp.c
    #netif/ppp/ipv6cp.c
    netif/ppp/lcp.c
    netif/ppp/magic.c
    netif/ppp/mppe.c
    netif/ppp/multilink.c
    netif/ppp/polarssl
    netif/ppp/ppp.c
    netif/ppp/pppapi.c
    netif/ppp/pppcrypt.c
    netif/ppp/pppoe.c
    #netif/ppp/pppol2tp.c
    #netif/ppp/pppos.c
    netif/ppp/upap.c
    netif/ppp/utils.c
    netif/ppp/vj.c
    
    #netif/ppp/polarssl
    netif/ppp/polarssl/arc4.c
    netif/ppp/polarssl/des.c
    netif/ppp/polarssl/md4.c
    netif/ppp/polarssl/md5.c
    netif/ppp/polarssl/sha1.c
    )

add_library(lwip_2 STATIC
    $ENV{CFG_PLATFORM}/wifipktif.c
    $ENV{CFG_PLATFORM}/wifi_apif.c
    $ENV{CFG_PLATFORM}/sys_arch.c
    )

if ($ENV{CFG_PLATFORM} STREQUAL openrtos)

    if (DEFINED CFG_NET_ETHERNET AND NOT DEFINED CFG_NET_AMEBA_SDIO)
    add_library(lwip_2 STATIC
        $ENV{CFG_PLATFORM}/pktif.c
    )
    endif()

    if (DEFINED CFG_NET_AMEBA_SDIO)
    add_library(lwip_2 STATIC
        $ENV{CFG_PLATFORM}/ameba_sdio_if.c
    )
    endif()

    if (DEFINED CFG_USB_ECM OR DEFINED CFG_USB_ECM_EX)
    add_library(lwip_2 STATIC
        $ENV{CFG_PLATFORM}/ecmif.c
    )
    endif()

    if (DEFINED CFG_USBH_CD_CDCECM_HCC)
    add_library(lwip_2 STATIC
        $ENV{CFG_PLATFORM}/usbh_hccecmif.c
    )
    endif()

    if (DEFINED CFG_USBD_NCM)
    add_library(lwip_2 STATIC
        $ENV{CFG_PLATFORM}/usbd_ncmif.c
    )
    endif()

else()

    add_library(lwip_2 STATIC
        $ENV{CFG_PLATFORM}/pktdrv.c
        $ENV{CFG_PLATFORM}/pktif.c
    )

endif()
