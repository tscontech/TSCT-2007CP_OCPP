#!/bin/bash



# !!! Not support win32 build yet !!!
#pushd ../win32
#export NO_PAUSE=1
#source ./sdk_doorbell_win32.sh
#export NO_PAUSE=
#export COMMON_DEFINED=
#export TARGET=
#popd

export CFG_PROJECT=arm_lite_codec
source ./build.sh

export CFG_PROJECT=sdk_doorbell_sm32
source ./build.sh

export CFG_PROJECT=`basename $0 .sh`
export COMMON_DEFINED=
source ./build.sh

