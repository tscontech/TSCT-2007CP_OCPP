#include "ite/audio.h"
#include "ite/main_processor_message_queue.h"
#include "i2s/i2s.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>

#define TOINT(n)   n
#define TOSHORT(n) n
    
int ParsingAudioPluginCmd(unsigned short nRegisterStatus);

//static FILE *fout = NULL;
//static unsigned char ptWaveHeader[44];
//static ITE_WmaInfo gWmaInfo;
//static unsigned long gnDataSize;
//static int *ptThread;
static char cSpecTrum[100];

static int PackWaveHeader(int nChannels,int nSampleRate,int nDataSize)
{
    return 0;
}

int ParsingAudioPluginCmd(unsigned short nRegisterStatus)
{
    unsigned int nTemp;

    // parsing audio cpu id
    nTemp = (nRegisterStatus & 0xc000)>>14;
    //printf("main processor parsing cpu id %d \n",nTemp);
    if (nTemp!=SMTK_AUDIO_PROCESSOR_ID) {
        //dbg_msg(DBG_MSG_TYPE_ERROR, "[MainProcessorExecuteAudioPlugin] Parsing error SMTK_MAIN_PROCESSOR_MESSAGE_QUEUE_NOT_SUPPORT_CPU_ID %d #line %d \n",nTemp,__LINE__);
        return SMTK_MAIN_PROCESSOR_MESSAGE_QUEUE_NOT_SUPPORT_CPU_ID;
    }

    // parsing audio plugin cmd
    nTemp = nRegisterStatus & 0x3fff;
    if (nTemp ==0 || nTemp > SMTK_AUDIO_PLUGIN_CMD_ID_PRINTF) {
        //dbg_msg(DBG_MSG_TYPE_ERROR, "[MainProcessorExecuteAudioPlugin] Parsing error SMTK_MAIN_PROCESSOR_MESSAGE_QUEUE_NOT_SUPPORT_CMD_ID %d #line %d \n",nTemp,__LINE__);
        return SMTK_MAIN_PROCESSOR_MESSAGE_QUEUE_NOT_SUPPORT_CMD_ID;
    }

    return SMTK_MAIN_PROCESSOR_MESSAGE_QUEUE_NO_ERROR;

}

int
ExcuteFileOpen()
{
    return 0;
}

int ExcuteFileWrite()
{
    return 0;
}

int ExcuteFileRead()
{
    return 0;
}

int ExcuteFileClose()
{
    return 0;
}

int ExcuteI2SInitDac()
{
    unsigned char* I2SBuf;
    int nch;
    int rate;
    int bufflen;
    int nTemp = 1;
    unsigned int* pBuf;
    uint32_t nLength;
    STRC_I2S_SPEC spec;
#if defined(__OPENRTOS__)

    pBuf = (unsigned int*)iteAudioGetAudioCodecAPIBuffer(&nLength);
    I2SBuf = (unsigned char*)(TOINT(pBuf[0])+iteAudioGetAudioCodecBufferBaseAddress());
    nch = TOINT(pBuf[1]);
    rate = TOINT(pBuf[2]);
    bufflen = TOINT(pBuf[3]);
    iteAudioSetAttrib(ITE_AUDIO_CODEC_SET_SAMPLE_RATE, &rate);
    iteAudioSetAttrib(ITE_AUDIO_CODEC_SET_CHANNEL, &nch);
    iteAudioSetAttrib(ITE_AUDIO_CODEC_SET_BUFFER_LENGTH, &bufflen);
    iteAudioSetAttrib(ITE_AUDIO_I2S_PTR, I2SBuf);
    iteAudioSetAttrib(ITE_AUDIO_I2S_INIT, &nTemp);
    
    printf("[Main Processor Message Queue]sdl  I2SInitDac 0x%x  %d %d %d \n",I2SBuf,nch,rate,bufflen);
    if (nch>2 || nch<=0) 
        return 0;
    
    if (rate>48000 || rate<8000) 
        return 0;
    
    if(iteAudioGetMusicCodecDump())
        return 0;
    
    /* init I2S */
    if(i2s_get_DA_running()) i2s_deinit_DAC();
    
    memset(&spec,0,sizeof(spec));
    spec.sample_rate              = rate;
    spec.channels                 = nch;
    spec.sample_size              = 16;
    spec.base_i2s                 = I2SBuf;
    spec.buffer_size              = bufflen;
    spec.is_big_endian            = 0;
    spec.enable_Speaker           = 1;
    
    /*hdmi*/
    spec.num_hdmi_audio_buffer    = 1;
    spec.base_hdmi[0]             = I2SBuf;
    spec.base_hdmi[1]             = I2SBuf;
    spec.base_hdmi[2]             = I2SBuf;
    spec.base_hdmi[3]             = I2SBuf;

    i2s_init_DAC(&spec);
    
#endif
    return 0;
}

int ExcuteI2SInitAdc()
{
    return 0;
}

int
ExcutePauseDAC()
{
    return 0;
}

int ExcuteDeactiveDAC()
{
#if defined(__OPENRTOS__)
    i2s_deinit_DAC();
#endif
    printf("[Main Processor Message Queue] deactiveDAC \n");
    return 0;
}

int ExcuteDeactiveADC()
{
    return 0;
}

int ExcutePrintf()
{
    unsigned char* pBuf;
    uint32_t nLength;

#if defined(__OPENRTOS__)
    pBuf = (unsigned char*)iteAudioGetAudioCodecAPIBuffer(&nLength);
    puts(pBuf);
    memset(pBuf, 0, nLength);

    ithFlushDCacheRange((void*)pBuf, nLength);
    ithFlushMemBuffer();
#endif
    return 0;
}

int ExcuteWmaInfo()
{
    return 0;
}

int ExcuteSpectrum()
{
#if defined(__OPENRTOS__)
    unsigned int* pBuf;
    uint32_t nLength;
    int nTemp;
    
    pBuf = (unsigned int*)iteAudioGetAudioCodecAPIBuffer(&nLength);
    nTemp = TOINT(pBuf[0]);

    memcpy(cSpecTrum,&pBuf[4],sizeof(cSpecTrum));
    //printf("[Main Processor Message Queue] spectrum %d [0x%x] 0x%x, 0x%x, 0x%x, 0x%x,  0x%x, 0x%x, 0x%x, 0x%x",nTemp,cSpecTrum,cSpecTrum[0],cSpecTrum[1],cSpecTrum[2],cSpecTrum[3],cSpecTrum[4],cSpecTrum[5],cSpecTrum[6],cSpecTrum[7],cSpecTrum[8]);
    //printf("0x%x, 0x%x, 0x%x, 0x%x,  0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x \n",cSpecTrum[9],cSpecTrum[10],cSpecTrum[11],cSpecTrum[12],cSpecTrum[13],cSpecTrum[14],cSpecTrum[15],cSpecTrum[16],cSpecTrum[17],cSpecTrum[18],cSpecTrum[19]);
    // set spectrum to audio mgr
    #if BUILD_AUDIO_MGR
    nTemp=smtkAudioMgrSetSpectrum(cSpecTrum);
    #endif

#endif
    return 0;
}

int ExcutePcmIdx()
{
    unsigned int* pBuf;
    uint32_t nLength;
    int nTemp;

#if defined(__OPENRTOS__)

    pBuf = (unsigned int*)iteAudioGetAudioCodecAPIBuffer(&nLength);
    nTemp = TOINT(pBuf[0]);
    iteAudioCodecSetPcmIdx(nTemp);
    //printf("nTemp=%d\n",nTemp);
#endif
    return 0;
}

int ExcuteAudioPluginCmd( unsigned short RegStatus)
{
    unsigned int nTemp;
    int nResult=0;

    //ithInvalidateDCache();

    // execute audio plugin cmd
    nTemp = RegStatus & 0x3fff;
    //printf("[Main Processor] cmd  %d\n",nTemp);
    switch (nTemp)
    {
        case SMTK_AUDIO_PLUGIN_CMD_ID_I2S_INIT_DAC:
        {
            nResult = ExcuteI2SInitDac();
            break;
        }

        case SMTK_AUDIO_PLUGIN_CMD_ID_I2S_PAUSE_DAC:
        {
            nResult = ExcutePauseDAC();
            break;
        }

        case SMTK_AUDIO_PLUGIN_CMD_ID_I2S_DEACTIVE_DAC:
        {
            nResult = ExcuteDeactiveDAC();
            break;
        }

        case SMTK_AUDIO_PLUGIN_CMD_ID_PRINTF:
        {
            nResult = ExcutePrintf();
            break;
        }

        case SMTK_AUDIO_PLUGIN_CMD_ID_SPECTRUM:
        {
            nResult = ExcuteSpectrum();
            break;
        }

        case SMTK_AUDIO_PLUGIN_CMD_ID_PCMIDX:
        {
            nResult = ExcutePcmIdx();//dump to filter (flower OR mediastreamer filter)
            break;
        }

        default:
            break;
    }
    return nResult;

}


int smtkMainProcessorExecuteAudioPluginCmd( unsigned short nRegStatus)
{
    int nResult=0;
    unsigned short reg;

    nResult =  ExcuteAudioPluginCmd(nRegStatus);
    reg = SMTK_MAIN_PROCESSOR_ID << 14 | nResult;

    setAudioPluginMessageStatus(reg);

    return nResult;
}
