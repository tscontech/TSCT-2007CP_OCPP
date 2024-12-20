#!/bin/bash

export PROJECT_NAME=$CFG_PROJECT

if [ "${MAKEJOBS}" = "" ]; then
    export MAKEJOBS=$(getconf _NPROCESSORS_ONLN)
fi

function buildProject {
    export CFG_PROJECT=$1

    if [ ! -d $CFG_PROJECT ]; then
        mkdir $CFG_PROJECT
    fi

    pushd $CFG_PROJECT
    if [ -f CMakeCache.txt ]; then
        cmake -G"Unix Makefiles" $CMAKE_SOURCE_DIR
    else
        cmake -G"Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=$CFG_TOOLCHAIN_FILE $CMAKE_SOURCE_DIR
    fi

    if [ $? != 0 ]; then
        echo "execute cmake error."
        exit
    fi

    if [ "${MAKECLEAN}" = "1" ]; then
        echo "Clean build..."
        make clean
    fi

    make -j $MAKEJOBS VERBOSE=$VERBOSE
    popd
}

if [ -f $CFG_PROJECT/project/$TARGET/$TARGET ]; then
    rm $CFG_PROJECT/project/$TARGET/$TARGET
fi

if [ "$ARMLITECODEC" = "1" ]; then
    buildProject "arm_lite_codec"
fi

buildProject "$PROJECT_NAME"

