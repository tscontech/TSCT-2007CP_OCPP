#set (BT_MODULE AP6255)
if (DEFINED CFG_AP6256BLE)
	set (BT_MODULE AP6256)
elseif(DEFINED CFG_AP6236BLE)
	set (BT_MODULE AP6236)
elseif(DEFINED CFG_AP6212BLE)
	set (BT_MODULE AP6212)
endif()

set (BRCM_BT_HCD_SRC
    src/fwpatches/${BT_MODULE}/bt_firmware_image.c
)

include_directories(
    .
    )

file(GLOB SRC
	src/*.c
	)

add_library(bluetooth STATIC
	${SRC}
	${BRCM_BT_HCD_SRC}
    )