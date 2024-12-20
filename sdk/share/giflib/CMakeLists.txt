add_definitions(
    -DHAVE_CONFIG_H
    )

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    add_definitions(
        -DDEBUG
        )
endif()

include_directories(
	.
 ${PROJECT_SOURCE_DIR}/sdk/include
    )
        
add_library(giflib STATIC
    dgif_lib.c
    egif_lib.c
    getarg.c
    gif_err.c 
    gif_font.c
    gif_hash.c
    gif2rgb.c
    gifalloc.c
    gifbg.c
    gifbuild.c
    gifclrmp.c
    gifcolor.c
    gifecho.c
    giffilter.c
    giffix.c
    gifhisto.c
    gifinto.c
    gifsponge.c
    giftext.c
    giftool.c
    gifwedge.c
    openbsd-reallocarray.c
    qprintf.c 
    quantize.c
    )
