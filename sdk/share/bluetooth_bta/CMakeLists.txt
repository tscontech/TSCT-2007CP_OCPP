#
# $ Copyright Broadcom Corporation $
#

SET (NAME bluetooth_bta)
SET (BTEDIR BTA_Release)
SET (RTOS openrtos)
SET (PLATFORM_VENDOR ite)


SET (BTEINCLUDES
        ${BTEDIR}/
        ${BTEDIR}/Components/bta/ag
        ${BTEDIR}/Components/bta/ar
        ${BTEDIR}/Components/bta/avk
        ${BTEDIR}/Components/bta/brcm
        ${BTEDIR}/Components/bta/dg
        ${BTEDIR}/Components/bta/dm
        ${BTEDIR}/Components/bta/gatt
        ${BTEDIR}/Components/bta/hd
        ${BTEDIR}/Components/bta/hs
        ${BTEDIR}/Components/bta/include
        ${BTEDIR}/Components/bta/rc
        ${BTEDIR}/Components/bta/sys
        ${BTEDIR}/Components/gki/common
        ${BTEDIR}/Components/gki/${RTOS}/
        ${BTEDIR}/Components/hcis
        ${BTEDIR}/Components/hcis/include
        ${BTEDIR}/Components/stack/include
        ${BTEDIR}/Components/stack/avrc
        ${BTEDIR}/Components/stack/avct
        ${BTEDIR}/Components/stack/avdt
        ${BTEDIR}/Components/stack/a2dp
        ${BTEDIR}/Components/stack/rfcomm
        ${BTEDIR}/Components/stack/btm
        ${BTEDIR}/Components/stack/brcm
        ${BTEDIR}/Components/stack/l2cap
        ${BTEDIR}/Components/stack/sdp
        ${BTEDIR}/Components/stack/gap
        ${BTEDIR}/Components/stack/gatt
        ${BTEDIR}/Components/stack/smp
        ${BTEDIR}/Components/stack/hid
        ${BTEDIR}/Components/stack/hsp2
        ${BTEDIR}/Components/stack/srvc
        ${BTEDIR}/Components/rpc/include
        ${BTEDIR}/Components/udrv/include
        ${BTEDIR}/Components/stack/include
        ${BTEDIR}/Components/embdrv/sbc/decoder/include
        ${BTEDIR}/Components/embdrv/sbc/encoder/include
        ${BTEDIR}/Projects/bte/main
        ${BTEDIR}/Projects/bta/bleapp_inc
        ${BTEDIR}/Projects/bta/btapp_comm_inc
        ${BTEDIR}/Projects/bta/btapp_inc
        ${BTEDIR}/Projects/bta/bleapp_inc
        ${BTEDIR}/Projects/bta/bta_co
        ${BTEDIR}/Projects/proto_disp
        ${BTEDIR}/../port_${PLATFORM_VENDOR}/inc/
        ${BTEDIR}/../btsnoop/
)



SET (OPENRTOS_INCLUDES
    ${PROJECT_SOURCE_DIR}/openrtos/include/openrtos
)


add_definitions(
	-DBUILDCFG
	-DDEBUG_VERSION=1
)

include_directories(
    .
	${BTEINCLUDES}
	${OPENRTOS_INCLUDES}
)

set (PREBUILD_LIB 
    BTA_Release/libs/libbluetooth_bta.a
)

file(COPY ${PREBUILD_LIB}
    DESTINATION ${CMAKE_BINARY_DIR}/lib/${CFG_CPU_NAME}/
)

