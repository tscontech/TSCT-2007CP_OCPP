if (DEFINED CFG_HDMI_TX_ENABLE)
	add_library(i2s STATIC
	i2s_9860_HDMITX.c
    i2s_switch_GPIO.c
    i2s_reg_9860.h
	)
else()
ADD_LIBRARY(i2s STATIC
    i2s_9860.c
    i2s_switch_GPIO.c
    i2s_reg_9860.h
)	
endif()