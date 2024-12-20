include_directories(
    .
    ${PROJECT_SOURCE_DIR}/sdk/include/bluetooth/rtl8723ble
    )

file(GLOB rtk_SRC
	src/*.c
	)
	
get_filename_component(rm_file ${CMAKE_CURRENT_SOURCE_DIR}/src/mini_upper_stack.c ABSOLUTE)

list(REMOVE_ITEM rtk_SRC "${rm_file}")

add_library(bluetooth STATIC
	${rtk_SRC}
    )