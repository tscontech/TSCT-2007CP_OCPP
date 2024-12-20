
#include "bta_platform.h"
#include "bte_glue.h"
#include "i2s/i2s.h" 
#include "ite/itp.h"

#define DAC_BUFFER_SIZE (128*1024) //if sampleRate:44KHz*2ch*16bit=172KB/S, can hold stream ~0.28s

uint8_t *dac_buf = NULL;
STRC_I2S_SPEC playback_param = {0};
static uint32_t playback_frame_size = 0;
static uint32_t capture_frame_size = 0;
static bool audio_play = false, bt_link = false;

void audio_mirror_init()
{
	if (bt_link)
	{
		audio_deinit_DA();
		audio_init_DA(&playback_param);

		audio_pause_DA(0);
	}
	audio_play = true;
}

void audio_mirror_stop()
{
	audio_play = false;
	audio_deinit_DA();
	I2S_DA32_SET_WP(I2S_DA32_GET_RP());
}

void AUDIO_Ucodec_2_ite_Params(tUCODEC_CNF_SBC *sbc, STRC_I2S_SPEC * param)
{	
    switch(sbc->ChannelMode)
    {
        /* If the current SBC is Mono */
		case UCODEC_CHN_MONO:
			param->channels = 1;
			break;
		case UCODEC_CHN_DUAL:
		case UCODEC_CHN_STEREO:
		case UCODEC_CHN_JOINT_STEREO:
			param->channels = 2;
			break;
		default:
			param->channels = 0 ;
			//printf("AUDIO_Ucodec_2_ite_Params error bad sbc_channel_mode:%d", sbc->ChannelMode);
			break;
    }
    
    if(dac_buf==NULL)
    {
      dac_buf=GKI_os_malloc(DAC_BUFFER_SIZE);
    }

    param->sample_rate              = ucodec_get_samplerate_value(sbc->SampleFreq);
    param->buffer_size              = DAC_BUFFER_SIZE;
    param->is_big_endian            = 0;
    param->base_i2s                 = (uint8_t *) dac_buf;
    param->sample_size              = 16;
    param->num_hdmi_audio_buffer    = 0;
    param->is_dac_spdif_same_buffer = 1;
    param->enable_Speaker           = 1;
    param->enable_HeadPhone         = 1;
    param->postpone_audio_output    = 1;
    param->base_spdif               = (uint8_t *) dac_buf;

	audio_init_DA(param);
    audio_pause_DA(1);
	bt_link = true;
}

int AUDIO_Playback_Open(void)
{
    APPL_TRACE_DEBUG0("AUDIO_Playback_Open");
    audio_pause_DA(0);
	bt_link = true;
    return 0;
}

int AUDIO_Playback_Close(void)
{
    APPL_TRACE_DEBUG0("AUDIO_Playback_Close");
    audio_pause_DA(1);
	bt_link = false;
    return 0;
}

int AUDIO_Playback_Configure( STRC_I2S_SPEC *param)
{	
    if( memcmp(&playback_param,param,sizeof(playback_param)))
    {
      APPL_TRACE_DEBUG0("init i2s\r\n");
      memcpy(&playback_param, param, sizeof(playback_param));	  
      audio_deinit_DA();
      audio_init_DA(&playback_param);
    }
    audio_pause_DA(0); 
    return 0;
}

void AUDIO_Playback_Play(uint8_t *data, int len)
{
	if (!audio_play)
		return;
    int count, remain = len, wlen, wframe;
    char *pdata = data;
#ifdef AUDIO_USE_FILE
    playback_write_wave_file(data, len);
#else
	
#if 0
    if (remain > 0)
    {
        if (remain > playback_frame_size)
            count = (remain / playback_frame_size) + 1;
        else
            count = 1;


        for (; count; count--) {
            if (remain < playback_frame_size) {
                printf("period_size change ,new period_size:%d,old:%d\n", remain, playback_frame_size);
                playback_frame_size = remain;
            }
            //rt_kprintf("audio_play data_size:%d \n", len);
            wlen = (remain > playback_frame_size) ? playback_frame_size : remain;
            wframe = wlen / playback_param.channels / (playback_param.sample_size >> 3);
			audio_TX_Data_Send(&playback_param, pdata, wframe);
            remain -= wlen;
            pdata = pdata + wlen;
            if (remain == 0)
                break;
        }
    }
#else
    if( GET_DA_RW_GAP>GET_DA_BASE_LEN*3/4) 
    {
         printf("I2S bufer full\n");
         return; 
    }
    i2s_da_data_put(data,len);
#endif
#endif
}

#if 0
int AUDIO_Capture_Open(void )
{
    APPL_TRACE_DEBUG0("AUDIO_Capture_Open");
#if 0
    rt_err_t ret;


#ifdef AUDIO_USE_FILE

#else
    if (capture_audio_device == NULL) {
        capture_audio_device = rt_device_find(RT_USB_AUDIO_C_NAME);
        rt_kprintf("==playback_audio_device %p==\n", capture_audio_device);
        ret = rt_device_open(capture_audio_device, RT_DEVICE_OFLAG_RDONLY);
        RT_ASSERT(ret == RT_EOK);
    }
#endif

#endif
    return 0;
}

int AUDIO_Capture_Close(void)
{
#if 0
    rt_err_t ret = 0;
    printf("==%s playback_audio_device %p==\n", __func__, capture_audio_device);

#ifdef AUDIO_USE_FILE
    capture_close_wave_file();
#else
    if (capture_audio_device != NULL) {
        ret = rt_device_control(capture_audio_device, RK_AUDIO_CTL_STOP, NULL);
        RT_ASSERT(ret == RT_EOK);

        ret = rt_device_control(capture_audio_device, RK_AUDIO_CTL_PCM_RELEASE, NULL);
        RT_ASSERT(ret == RT_EOK);

        ret = rt_device_close(capture_audio_device);
        RT_ASSERT(ret == RT_EOK);

        if (capture_abuf.buf != NULL) {
            rt_free_uncache(capture_abuf.buf);
            capture_abuf.buf = NULL;
        }

        capture_audio_device = NULL;
    }
#endif
#endif
    return 0;
}

int AUDIO_Capture_Configure(struct AUDIO_PARAMS *param)
{
#if 0
    uint32_t size;
    rt_err_t ret;

#ifdef AUDIO_USE_FILE
    capture_wav_format.bits_per_sample = param->sampleBits;
    capture_wav_format.nb_channels = param->channels;
    capture_wav_format.sample_rate = param->sampleRate;

    capture_open_wave_file();
#else
    memcpy(&capture_param, param, sizeof(struct AUDIO_PARAMS));

    if (capture_audio_device != NULL) {
        capture_abuf.period_size = 240; //WBS 16*(15ms)  NBS 8*(30ms)
        capture_abuf.buf_size = capture_abuf.period_size * CAPTURE_AUDIO_PERIOD_COUNT;
        size = capture_abuf.buf_size * param->channels * (param->sampleBits >> 3); /* frames to bytes */
        capture_frame_size = capture_abuf.period_size * param->channels * (param->sampleBits >> 3);

        capture_abuf.buf = rt_malloc_uncache(size);
        RT_ASSERT(capture_abuf.buf != NULL);

        ret = rt_device_control(capture_audio_device, RK_AUDIO_CTL_PCM_PREPARE, &capture_abuf);
        RT_ASSERT(ret == RT_EOK);

        ret = rt_device_control(capture_audio_device, RK_AUDIO_CTL_HW_PARAMS, &capture_param);
        RT_ASSERT(ret == RT_EOK);
    }
#endif

    rt_kprintf("### AUDIO_Capture_Configure rate: %d, channels: %d, bits: %d ###\n",
        param->sampleRate, param->channels, param->sampleBits);
#endif
    return 0;
}

int AUDIO_Capture_Record(uint8_t *ptr, int len)
{
    int rlen = 0;
#if 0
#ifdef AUDIO_USE_FILE
    return capture_read_wave_file(ptr, len);
#else
    RT_ASSERT(capture_abuf.period_size == len);

    rlen = rt_device_read(capture_audio_device, 0, ptr, capture_abuf.period_size);

    if (rlen != capture_abuf.period_size) {
        rt_kprintf("Capturing sample: rlen %d, period_size: %d\n", rlen, capture_abuf.period_size);
    }

#endif
#endif
    return rlen;
}


void AUDIO_SCO_Open(uint16_t codec_type)
{
#if 0
    struct AUDIO_PARAMS sco_param;

    sco_param.channels = 1;
    sco_param.sampleBits = AUDIO_SAMPLEBITS_16;
    if(codec_type == 1) //CVSD
    {
        sco_param.sampleRate = AUDIO_SAMPLERATE_8000;
    }
    else if(codec_type == 2) //MSBC
    {
        sco_param.sampleRate = AUDIO_SAMPLERATE_16000;
    }

    AUDIO_Playback_Open();
    AUDIO_Playback_Configure(&sco_param);

    AUDIO_Capture_Open();
    AUDIO_Capture_Configure(&sco_param);
#endif
}


void AUDIO_SCO_Close(void)
{
    AUDIO_Playback_Close();
    AUDIO_Capture_Close();
}

void AUDIO_SCO_In_Data(uint8_t *data, int len)
{
    AUDIO_Playback_Play(data, len);
}

int AUDIO_SCO_Out_Data(uint8_t *data, int len)
{
    return AUDIO_Capture_Record(data, len);
}

#endif
