#include <malloc.h>
#include <string.h>
#include "flower/flower.h"

//=============================================================================
//                              Constant Definition
//=============================================================================
extern IteFilterDes FilterA;
extern IteFilterDes FilterB;
extern IteFilterDes FilterC;
extern IteFilterDes FilterD;
extern IteFilterDes FilterE;
extern IteFilterDes FilterF; 
#if defined(CFG_NET_ENABLE)
extern IteFilterDes FilterUDPsend;
extern IteFilterDes FilterUDPrecv;
extern IteFilterDes FilterTCPsend;
extern IteFilterDes FilterTCPrecv;
#endif
#if defined(CFG_VIDEO_ENABLE)
extern IteFilterDes FilterCapture;
extern IteFilterDes FilterH264DEC;
extern IteFilterDes FilterMJpegDEC;
extern IteFilterDes FilterDisplay;
extern IteFilterDes FilterDisplayCamera;
extern IteFilterDes FilterJpegWriter;
extern IteFilterDes FilterFileWriter;
extern IteFilterDes FilterRecAVI;
extern IteFilterDes FilterMJpegRecAVI;
extern IteFilterDes FilterIPCam;
#endif
#if defined(CFG_UVC_ENABLE)
extern IteFilterDes FilterUVC;
#endif
#ifdef CFG_AUDIO_ENABLE
extern IteFilterDes FilterSndWtire;
extern IteFilterDes FilterSndRead;
extern IteFilterDes FilterFilePlay;
extern IteFilterDes FilterFilePlayMgr;
extern IteFilterDes FilterStreamMgr;
extern IteFilterDes FilterFileRec;
extern IteFilterDes FilterFileMix;
extern IteFilterDes FilterHDT;
extern IteFilterDes FilterCSeparate;
extern IteFilterDes FilterCMerge;
extern IteFilterDes FilterUlawEnc;
extern IteFilterDes FilterUlawDec;
extern IteFilterDes FilterAlawEnc;
extern IteFilterDes FilterAlawDec;
extern IteFilterDes FilterAdpcmEnc;
extern IteFilterDes FilterAdpcmDec;
extern IteFilterDes FilterVoid;
extern IteFilterDes FilterTee;
extern IteFilterDes	FilterVolume;
extern IteFilterDes FilterChadapt;
extern IteFilterDes FilterMix;
extern IteFilterDes FilterMicMix;
extern IteFilterDes FilterQWrite;
extern IteFilterDes FilterQuickPlay;
    #if CFG_BUILD_AUDIO_PREPROCESS
extern IteFilterDes FilterLoopBack;
extern IteFilterDes	FilterDenoise;
extern IteFilterDes FilterAec;
extern IteFilterDes FilterEqualizer;
    #endif
    #if CFG_BUILD_ASR
extern IteFilterDes FilterAsr;
    #endif
    #if CFG_BUILD_SPEEX
    extern IteFilterDes FilterResample;
    #endif
    #if CFG_BUILD_FFMPEG
    extern IteFilterDes FilterPlayM4a;
    //extern IteFilterDes FilterStreamM4a;
    #endif
#endif

// Filter Set
IteFilterDes *gFilterDesSet[] = {
    &FilterA,
    &FilterB,
    &FilterC,
    &FilterD,
    &FilterE,
    &FilterF,
#if defined(CFG_NET_ENABLE)    
    &FilterUDPsend,
    &FilterUDPrecv,
    &FilterTCPsend,
    &FilterTCPrecv,
#endif
#if defined(CFG_VIDEO_ENABLE)
	&FilterCapture,
	&FilterH264DEC,
	&FilterMJpegDEC,
	&FilterDisplay,
	&FilterDisplayCamera,
	&FilterJpegWriter,
	&FilterFileWriter,
	&FilterRecAVI,
	&FilterMJpegRecAVI,
	&FilterIPCam,
#endif
#if defined(CFG_UVC_ENABLE)
	&FilterUVC,
#endif
#ifdef CFG_AUDIO_ENABLE
    &FilterSndWtire,
    &FilterSndRead,
    &FilterFilePlay,
    &FilterFilePlayMgr,
    &FilterStreamMgr,
    &FilterFileRec,
    &FilterFileMix,
    &FilterHDT,
    &FilterCSeparate,
    &FilterCMerge,
    &FilterUlawEnc,
    &FilterUlawDec,
    &FilterAlawEnc,
    &FilterAlawDec,
    &FilterAdpcmEnc,
    &FilterAdpcmDec,
    &FilterVoid,
    &FilterTee, 
    &FilterVolume,
    &FilterChadapt,
	&FilterMix,
    &FilterMicMix,
    &FilterQWrite,
	&FilterQuickPlay,
    #if CFG_BUILD_AUDIO_PREPROCESS
    &FilterLoopBack,
    &FilterAec,
    &FilterDenoise,
    &FilterEqualizer,
    #endif
    #if CFG_BUILD_ASR
    &FilterAsr,
    #endif
    #if CFG_BUILD_SPEEX
    &FilterResample,
    #endif
    #if CFG_BUILD_FFMPEG
    &FilterPlayM4a,
    //&FilterStreamM4a,
    #endif    
    
#endif
    NULL
};



