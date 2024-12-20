#!/bin/bash

if [ "$COMMON_DEFINED" == "" ]; then
    export COMMON_DEFINED=1
    export GCC_COLORS=1

    pushd ../..
    export CMAKE_SOURCE_DIR=`pwd`
    popd

    export CFG_BUILDPLATFORM=linux
    export CFG_PLATFORM=openrtos
    export CFG_TOOLCHAIN_FILE=$CMAKE_SOURCE_DIR/$CFG_PLATFORM/toolchain.cmake
    export PATH=$CMAKE_SOURCE_DIR/tool/bin/linux:$CMAKE_SOURCE_DIR/tool/bin/linux/cmake/bin:/opt/ITEGCC/bin:$PATH
    export LD_LIBRARY_PATH=$CMAKE_SOURCE_DIR/tool/bin/linux:$LD_LIBRARY_PATH
fi

