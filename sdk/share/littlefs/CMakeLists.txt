add_definitions(
    -DLFS_YES_TRACE0=1
    )

include_directories(
    ${PROJECT_SOURCE_DIR}/sdk/include/littlefs
    )
    
add_library(littlefs STATIC
    lfs.c
    lfs_util.c
    )
