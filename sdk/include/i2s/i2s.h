/* sy.chuang, 2012-0423, ITE Tech. */

#ifndef I2S_H
#define I2S_H

#ifdef __cplusplus
extern "C" {
#endif

#define GET_DA_BASE     ithReadRegA(0xD0100070)
#define GET_DA_BASE_LEN (ithReadRegA(0xD0100090)+1)
#define GET_DA_RW_GAP   ithReadRegA(0xD01000C8)
#define GET_AD_BASE     ithReadRegA(0xD0100010)
#define GET_AD_BASE_LEN (ithReadRegA(0xD0100030)+1)
#define GET_AD_RW_GAP   ((ithReadRegA(0xD0100054) >= ithReadRegA(0xD0100050)) ? \
                        (ithReadRegA(0xD0100054) - ithReadRegA(0xD0100050)):\
                        ((GET_AD_BASE_LEN - ithReadRegA(0xD0100050)) + ithReadRegA(0xD0100054)))
#define GET_AD2_RW_GAP   ((ithReadRegA(0xD010005C) >= ithReadRegA(0xD0100058)) ? \
                        (ithReadRegA(0xD010005C) - ithReadRegA(0xD0100058)):\
                        ((GET_AD_BASE_LEN - ithReadRegA(0xD0100058)) + ithReadRegA(0xD010005C)))
/* ************************************************************************** */
typedef struct
{
	/* input/ouput common */
	unsigned channels; /* ex: 1(mono) or 2(stereo) */
	unsigned sample_rate; /* ex: 44100/48000 Hz */
	unsigned buffer_size;
	unsigned is_big_endian;
	unsigned char *base_i2s;
	unsigned sample_size; /* ex: 16/24/32 bits */

	/* for input use */
	unsigned from_LineIN;
	unsigned from_MIC_IN;

	/* for output use */
	unsigned num_hdmi_audio_buffer; /* 0: no hdmi audio output (could save bandwidth) */
	unsigned is_dac_spdif_same_buffer;
	unsigned char *base_hdmi[4];    /*hdmi buffer base (TX RX are same)*/
	unsigned char *base_spdif;      /*spdif   buffer base*/
	unsigned postpone_audio_output; /* manually enable audio output */

	/* for input use */
	unsigned record_mode; /* 0: hardware start via capture hardware, 1: hardware start via software set */
	
	/* for output use */
	unsigned enable_Speaker;	//speaker-out
	unsigned enable_HeadPhone;	//line-out. If "enable_Speaker"=0, "enable_HeadPhone" will be set as true in i2s driver defaut setting.
} STRC_I2S_SPEC;

typedef enum _AudioInOutCase {
	HANDFREE = 0,
	HEADSET,
	TELEPHONETUBE,
    DOUBLE_HEADSET,
    DOUBLE_TELETUBE,
}AudioInOutCase;

/* ************************************************************************** */
/* DA */
unsigned int I2S_DA32_GET_RP(void);
unsigned int I2S_DA32_GET_WP(void);
void I2S_DA32_SET_WP(unsigned int data);
void I2S_DA32_WAIT_RP_EQUAL_WP(int);
unsigned int i2s_is_DAstarvation(void);


int  i2s_get_DA_running(void);
int i2s_get_AD_running(void);
unsigned int I2S_AD32_GET_RP(void);
unsigned int I2S_AD32_GET_WP(void);
void I2S_AD32_SET_RP(unsigned int data);

/*HDMI*/
unsigned int I2S_AD32_GET_HDMI_RP(void);
unsigned int I2S_AD32_GET_HDMI_WP(void);
void I2S_AD32_SET_HDMI_RP(unsigned int data);

/********************** export APIs **********************/
void i2s_CODEC_wake_up(void);
void i2s_CODEC_standby(void);

/* SPDIF */
void i2s_init_spdif(void);
void i2s_init_output_pin(void);
static void _deinit_spdif(void);

/* DA */
void i2s_volume_up(void);
void i2s_volume_down(void);
void i2s_pause_DAC(int pause);
void i2s_deinit_DAC(void);
void i2s_init_DAC(STRC_I2S_SPEC *i2s_spec);
void i2s_mute_DAC(int mute);
void i2s_set_direct_volperc(unsigned volperc);
unsigned i2s_get_current_volperc(void);
void i2s_mute_volume(int mute);
void i2s_enable_fading(int yesno);
uint32_t i2s_da_data_put(unsigned char *ptr,unsigned int size);
void i2s_DAC_channel_switch(int channels,int RL);

/* AD */
void i2s_pause_ADC(int pause);
void i2s_deinit_ADC(void);
void i2s_init_ADC(STRC_I2S_SPEC *i2s_spec);
void i2s_ADC_set_direct_volstep(unsigned recstep);
void i2s_ADC_set_rec_volperc(unsigned recperc);
unsigned i2s_ADC_get_current_volstep(void);
void i2s_ADC_get_volstep_range(unsigned *max, unsigned *normal, unsigned *min);
void i2s_mute_ADC(int mute);
uint32_t i2s_ad_data_get(unsigned char *ptr,unsigned int size);
void i2s_ADC_channel_switch(int channels,int RL);

/* ************************************************************************** */
#ifdef __cplusplus
}
#endif

#endif //I2S_H

