if (DEFINED CFG_GCC_LTO)
    add_definitions("-fno-lto")
endif ()

include_directories(
	${PROJECT_SOURCE_DIR}/sdk/share/
    ${PROJECT_SOURCE_DIR}/sdk/share/flower/
    ${PROJECT_SOURCE_DIR}/sdk/include/
    ${PROJECT_SOURCE_DIR}/sdk/share/ffmpeg
    if(DEFINED CFG_BUILD_AUDIO_PREPROCESS)
    ${PROJECT_SOURCE_DIR}/sdk/share/audio_preprocess
    endif()
)

add_library(flower STATIC
    filter_chain.c
    example/filterA.c
    example/filterB.c
    example/filterC.c
    example/filterD.c
    example/filterE.c
    example/filterF.c
    filter_des.c
    ite_queue.c
    ite_buffer.c
    flower_stream.c
)

if(DEFINED CFG_NET_ENABLE)
add_library(flower STATIC
    filter_udp.c
    filter_tcp.c
	)
endif()
if (DEFINED CFG_VIDEO_ENABLE)
add_library(flower STATIC
	video/filter_h264dec.c
	video/filter_mjpegdec.c
	video/filter_display.c
	video/filter_jpegwriter.c
	video/filter_filewriter.c
	video/filter_rec_avi.c
	video/filter_mjpeg_rec_avi.c
	video/ite_avienc.h
	video/ite_avienc.c
	video/filter_ipcam.h
	video/filter_ipcam.c
	video/filter_capture.c
	video/filter_displaycamera.c
	)
endif()
if (DEFINED CFG_UVC_ENABLE)
    add_library(flower STATIC
        video/filter_uvc.c
    )
endif()
if (DEFINED CFG_AUDIO_ENABLE)
    add_library(flower STATIC
    audio/audiofilterflow.c
    audio/amusefilterflow.c
    audio/amixfilterflow.c
    audio/audioqueue.c
    audio/volumeapi.c
    audio/Ffileplay.c
    audio/Ffilerec.c
    audio/Fcastor3snd.c
    audio/Ffilemix.c
    audio/Fulaw.c
    audio/Falaw.c
	audio/Fadpcm.c
    audio/Fcmerge.c
    audio/Fcseparate.c
	audio/Fvoid.c
	audio/Ftee.c
    audio/Fvolume.c
    audio/Fhalfduplex.c
    audio/Fchanadapt.c
        audio/Fmixer.c
        #audio/Fmix.c
 	audio/Floopback.c
    audio/Ffileplaymgr.c
    audio/Fstreammgr.c
    audio/Fmicmix.c
	audio/Fqwrite.c
	#audio/Fquickplay.c
        audio/Ffastplay.c
        audio/audioparsing.c
    )
endif()

if(DEFINED CFG_BUILD_SPEEX)
    add_library(flower STATIC
    audio/Fresample.c
    )
endif()

if(DEFINED CFG_BUILD_AUDIO_PREPROCESS)
    add_library(flower STATIC
    audio/Faec.c
    audio/Fdenoise.c
    audio/Fequalizer.c
    )
endif()

if(DEFINED CFG_BUILD_ASR)
    if(DEFINED CFG_BUILD_WFSTASR)
        add_library(flower STATIC
        audio/asrfilterflow.c
        audio/Fasr.c
        )
    else()
        add_library(flower STATIC
        audio/asrfilterflow.c
        audio/Fasr1.c
        )
    endif()
endif()

if (DEFINED CFG_BUILD_FFMPEG)
        add_library(flower STATIC
        audio/Ffileplaym4a.c
        #audio/Fstreamm4a.c
        #audio/avpacketQ.c
        )
endif()



