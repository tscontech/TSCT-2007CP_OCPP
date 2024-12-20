#!/bin/bash

#export VERBOSE=1
# ==========================================================================
# export AUTOBACKUPCONFIG=1 if you want to backup the .config to the porject dir.
# ==========================================================================
export AUTOBACKUPCONFIG=0

source ./common.sh

if [ ! -d $CFG_PROJECT ]; then
    mkdir $CFG_PROJECT
fi

if [ "$BOOTLOADER" = "" ]; then
    export BOOTLOADER=0
fi

if [ "$ARMLITECODEC" = "" ]; then
    export ARMLITECODEC=0
fi

if [ "$TARGET" = "" ]; then
    export TARGET=$CFG_PROJECT
fi

if [ "$AUTOBUILD" = "" ]; then
    export AUTOBUILD=0
fi

export CFG_DEVELOP=0
export CFG_VER_INTERNATIONAL=0
export BUILD_CMD=./post_build.sh

if [ $BOOTLOADER = 1 ]; then
    export PRESETTINGS="--loadcfg $CMAKE_SOURCE_DIR/build/_presettings/_config_bootloader"
else
    export PRESETTINGS=""
fi

if [ "$PRESETTING" != "" ]; then
    if [ "$PRESETTINGS" != "" ]; then
        export PRESETTINGS="${PRESETTINGS} --loadcfg $CMAKE_SOURCE_DIR/build/_presettings/${PRESETTING}"
    else
        export PRESETTINGS="--loadcfg $CMAKE_SOURCE_DIR/build/_presettings/${PRESETTING}"
    fi
fi

if [ -f $CMAKE_SOURCE_DIR/project/$CFG_PROJECT/config.cmake ]; then
    source ./post_build.sh
elif [ $AUTOBUILD = 1 ]; then
    #if [ $AUTOBACKUPCONFIG = 1 ]; then
        if [ -f $CMAKE_SOURCE_DIR/project/$CFG_PROJECT/.config.$CFG_PLATFORM ]; then
            cmake -E copy $CMAKE_SOURCE_DIR/project/$CFG_PROJECT/.config.$CFG_PLATFORM $CFG_PROJECT/.config
        fi
    #fi
    mconf --autowrite --prefix "CFG_" --cmakefile $CFG_PROJECT/config.cmake --cfgfile $CFG_PROJECT/.config $PRESETTINGS $CMAKE_SOURCE_DIR/project/$CFG_PROJECT/Kconfig
    if [ $AUTOBACKUPCONFIG = 1 ]; then
        cmake -E copy $CFG_PROJECT/.config $CMAKE_SOURCE_DIR/project/$CFG_PROJECT/.config.$CFG_PLATFORM
    fi
    source ./post_build.sh
else
    if [ $AUTOBACKUPCONFIG = 1 ]; then
        if [ -f $CMAKE_SOURCE_DIR/project/$CFG_PROJECT/.config.$CFG_PLATFORM ]; then
            cmake -E copy $CMAKE_SOURCE_DIR/project/$CFG_PROJECT/.config.$CFG_PLATFORM $CFG_PROJECT/.config
        fi
    fi
    qconf --fontsize 11 --prefix "CFG_" --cmakefile $CFG_PROJECT/config.cmake --cfgfile $CFG_PROJECT/.config $PRESETTINGS $CMAKE_SOURCE_DIR/project/$CFG_PROJECT/Kconfig
    if [ $AUTOBACKUPCONFIG = 1 ]; then
        cmake -E copy $CFG_PROJECT/.config $CMAKE_SOURCE_DIR/project/$CFG_PROJECT/.config.$CFG_PLATFORM
    fi
fi

