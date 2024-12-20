if (DEFINED CFG_GCC_LTO)
    add_definitions("-fno-lto")
endif ()

include_directories(
	${PROJECT_SOURCE_DIR}/sdk/share/
    ${PROJECT_SOURCE_DIR}/sdk/share/onvif/
    ${PROJECT_SOURCE_DIR}/sdk/include/
)

add_library(onvif STATIC
    onvif.c
)


