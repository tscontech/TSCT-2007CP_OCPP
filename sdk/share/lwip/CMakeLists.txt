if (CMAKE_BUILD_TYPE STREQUAL Debug)
    add_definitions(
        -DLWIP_DEBUG
        )
endif()

include_directories(
    $ENV{CFG_PLATFORM}
    )

add_library(lwip STATIC
    api/api_lib.c
    api/api_msg.c
    api/err.c
    api/netbuf.c
    api/netdb.c
    api/netifapi.c
    api/sockets.c
    api/tcpip.c
    apps/lwiperf/lwiperf.c
	#apps/mqtt/mqtt.c
	#apps/mdns/mdns.c
	#apps/mdns/mdns_domain.c
	#apps/mdns/mdns_out.c
	#apps/netbiosns/netbiosns.c
	#apps/smtp/smtp.c
    core/ipv4/autoip.c
    core/ipv4/icmp.c
    core/ipv4/igmp.c
    core/ipv4/ip.c
    core/ipv4/inet.c
    core/ipv4/inet_chksum.c
    core/ipv4/ip_addr.c
    core/ipv4/ip_frag.c
    core/snmp/asn1_dec.c
    core/snmp/asn1_enc.c
    core/snmp/mib_structs.c
    core/snmp/mib2.c
    core/snmp/msg_in.c
    core/snmp/msg_out.c
    core/def.c
    core/dhcp.c
    core/dns.c
    core/init.c
    core/mem.c
    core/memp.c
    core/netif.c
    core/pbuf.c
    core/raw.c
    core/stats.c
    core/sys.c
    core/tcp.c
    core/tcp_in.c
    core/tcp_out.c
    core/timers.c
    core/udp.c
    core/packet.c
    netif/etharp.c
    netif/ethernetif.c
    netif/slipif.c
    )

add_library(lwip STATIC
    $ENV{CFG_PLATFORM}/wifipktif.c
    $ENV{CFG_PLATFORM}/wifi_apif.c
    $ENV{CFG_PLATFORM}/sys_arch.c
    )

if ($ENV{CFG_PLATFORM} STREQUAL openrtos)

    if (DEFINED CFG_NET_ETHERNET AND NOT DEFINED CFG_NET_AMEBA_SDIO)
    add_library(lwip STATIC
        $ENV{CFG_PLATFORM}/pktif.c
    )
    endif()

    if (DEFINED CFG_NET_AMEBA_SDIO)
    add_library(lwip STATIC
        $ENV{CFG_PLATFORM}/ameba_sdio_if.c
    )
    endif()

    if (DEFINED CFG_USB_ECM OR DEFINED CFG_USB_ECM_EX)
    add_library(lwip STATIC
        $ENV{CFG_PLATFORM}/ecmif.c
    )
    endif()

    if (DEFINED CFG_USBH_CD_CDCECM_HCC)
    add_library(lwip STATIC
        $ENV{CFG_PLATFORM}/usbh_hccecmif.c
    )
    endif()

    if (DEFINED CFG_USBD_NCM)
    add_library(lwip STATIC
        $ENV{CFG_PLATFORM}/usbd_ncmif.c
    )
    endif()
else()

    add_library(lwip STATIC
        $ENV{CFG_PLATFORM}/pktdrv.c
        $ENV{CFG_PLATFORM}/pktif.c
    )

endif()

