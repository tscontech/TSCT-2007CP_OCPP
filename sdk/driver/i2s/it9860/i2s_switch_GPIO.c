#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ite/ith.h"
#include "ite/itp.h"
#include "i2s/i2s.h"
#include "i2s_reg_9860.h"
void GPIO_switch_set(AudioInOutCase enable){
    int CHANEL_SEL = CFG_CHANEL_SEL;
    int CHANEL_ENB = CFG_CHANEL_ENB;
    ithGpioSetOut(CHANEL_ENB);
    ithGpioSetOut(CHANEL_SEL);
    switch (enable)
    {
    case HANDFREE:
        {/* hand free */
            itp_codec_playback_init(0);//Faraday DAC ONLY right
            ithWriteRegMaskA(I2S_REG_OUT_CTRL, 1<<6|0<<4, 1<<6|1<<4);//l channels ,right high
            itp_codec_rec_init(0);
            ithWriteRegMaskA(I2S_REG_IN_CTRL , 1<<6|0<<4, 1<<6|1<<4);//l channels ,right high
            ithGpioClear(CHANEL_ENB);
            ithGpioClear(CHANEL_SEL);
            break;
        }
    case HEADSET:
        {/* head set */
            itp_codec_playback_init(1);//Faraday DAC ONLY left
            ithWriteRegMaskA(I2S_REG_OUT_CTRL, 0<<6|0<<4, 1<<6|1<<4);//l channels ,left high
            itp_codec_rec_init(1);
            ithWriteRegMaskA(I2S_REG_IN_CTRL, 0<<6|0<<4, 1<<6|1<<4);//l channels ,left high
            ithGpioSetOut(CHANEL_ENB);
            ithGpioClear(CHANEL_SEL);
            break;
        }
    case TELEPHONETUBE:
        {/* telephone tube */
            itp_codec_playback_init(1);//Faraday DAC ONLY left
            ithWriteRegMaskA(I2S_REG_OUT_CTRL, 0<<6|0<<4, 1<<6|1<<4);//l channels ,left high
            itp_codec_rec_init(1);
            ithWriteRegMaskA(I2S_REG_IN_CTRL , 0<<6|0<<4, 1<<6|1<<4);//l channels ,left high
            ithGpioSetOut(CHANEL_ENB);
            ithGpioSet(CHANEL_SEL);
            break;
        }
    case DOUBLE_HEADSET:
        {
            itp_codec_playback_init(2);//Faraday DAC LEFT/RIGHT
            ithWriteRegMaskA(I2S_REG_OUT_CTRL, 0<<6|0<<4, 1<<6|1<<4);//l channels ,left high
            itp_codec_rec_init(1);
            ithWriteRegMaskA(I2S_REG_IN_CTRL, 0<<6|0<<4, 1<<6|1<<4);//1 channel ,left high
            ithGpioSetOut(CHANEL_ENB);
            ithGpioSet(CHANEL_SEL);
            break;
        }
    case DOUBLE_TELETUBE:
        {
            itp_codec_playback_init(2);//Faraday DAC LEFT/RIGHT
            ithWriteRegMaskA(I2S_REG_OUT_CTRL, 0<<6|0<<4, 1<<6|1<<4);//l channels ,left high
            itp_codec_rec_init(1);
            //ithWriteRegMaskA(I2S_REG_OUT_CTRL, 0<<6||1<<4, 1<<6||1<<4);//2 channel ,left high
            ithWriteRegMaskA(I2S_REG_IN_CTRL, 0<<6|0<<4, 1<<6|1<<4);
            ithGpioSetOut(CHANEL_ENB);
            ithGpioSetOut(CHANEL_SEL);
            break;
        }
    default:
        {
            itp_codec_playback_init(1);
            itp_codec_rec_init(1);
            //ithWriteRegMaskA(I2S_REG_OUT_CTRL, 0<<6, 0<<6);
            ithGpioClear(CHANEL_ENB);
            ithGpioClear(CHANEL_SEL);
            break;
        }
    }
    printf("Gpio%d:%d Gpio%d:%d\n",CHANEL_SEL,ithGpioGet(CHANEL_SEL),CHANEL_ENB,ithGpioGet(CHANEL_ENB));

}

void i2s_DAC_channel_switch(int channels,int RL){
    int  Creg=channels-1;
    bool reg=!RL;
    if(channels!=1 && channels!=2) {
        printf("channels(%d) must be 1 or 2 :set channel as 2\n",channels);
        reg=2;
    }
    if(channels==2)
        itp_codec_playback_init(channels);
    else
        itp_codec_playback_init(RL);
    ithWriteRegMaskA(I2S_REG_OUT_CTRL, reg<<6|Creg<<4, 1<<6|1<<4);
    //printf("channels %d RL %d Creg=%d reg=%d\n",channels,RL,Creg,reg);
}

void i2s_ADC_channel_switch(int channels,int RL){
    int  Creg=channels-1;
    bool reg=!RL;
    if(channels!=1 && channels!=2) {
        printf("channels(%d) must be 1 or 2 :set channel as 2\n",channels);
        Creg=2;
    }

    if(channels==2)
        itp_codec_rec_init(channels);
    else
        itp_codec_rec_init(RL);
    ithWriteRegMaskA(I2S_REG_IN_CTRL, reg<<6|Creg<<4, 1<<6|1<<4);   
    //printf("channels %d RL %d Creg=%d reg=%d\n",channels,RL,Creg,reg);    
}

void i2s_loopback_set(unsigned char *ptr,int number){
    /*loopbac DA to HDMI IN 0*/
    if(ptr!=NULL){
    
        switch(number)
        {
            case 0: ithWriteRegA(I2S_REG_IN2_BASE1, ptr);break;
            case 1: ithWriteRegA(I2S_REG_IN2_BASE2, ptr);break;
            case 2: ithWriteRegA(I2S_REG_IN2_BASE3, ptr);break;
            case 3: ithWriteRegA(I2S_REG_IN2_BASE4, ptr);break;
            default:ithWriteRegA(I2S_REG_IN2_BASE1, ptr);break;
        }
        ithWriteRegMaskA(I2S_REG_IN_CTRL,1<<(12+number),1<<(12+number));//enable base[number]
        ithWriteRegMaskA(I2S_REG_DATA_LOOPBACK, 1<<(5+number), 1<<(5+number));
    }else{
        ithWriteRegMaskA(I2S_REG_IN_CTRL,0<<(12+number),0<<(12+number));//enable base[number]
        ithWriteRegMaskA(I2S_REG_DATA_LOOPBACK, 0<<(5+number), 0<<(5+number));        
    }
}

uint32_t i2s_ad2_data_get(unsigned char *ptr,unsigned int size,int number)
{//get data from AD2 base[number]
	uint32_t AD_r = I2S_AD32_GET_HDMI_RP();
    uint8_t *base_i2s;
    uint32_t buffer_size = GET_AD_BASE_LEN;
    switch(number)
    {
        case 0: base_i2s = ithReadRegA(I2S_REG_IN2_BASE1);break;
        case 1: base_i2s = ithReadRegA(I2S_REG_IN2_BASE2);break;
        case 2: base_i2s = ithReadRegA(I2S_REG_IN2_BASE3);break;
        case 3: base_i2s = ithReadRegA(I2S_REG_IN2_BASE4);break;
        default:base_i2s = ithReadRegA(I2S_REG_IN2_BASE1);break;
    }

    if (AD_r + size <= buffer_size)
    {
        //bsize = AD_w - AD_r;
        ithInvalidateDCacheRange(base_i2s + AD_r, size);
        memcpy(ptr, base_i2s + AD_r, size);
        AD_r += size;    
    }
    else
    { // AD_r > AD_w
        uint32_t szsec0 = buffer_size - AD_r;
        uint32_t szsec1 = size - szsec0;
        if (szsec0)
        {
            ithInvalidateDCacheRange(base_i2s + AD_r, szsec0);
            memcpy(ptr, base_i2s + AD_r, szsec0);
        }
        ithInvalidateDCacheRange(base_i2s, szsec1);
        memcpy(ptr + szsec0, base_i2s, szsec1);
        AD_r = szsec1;
    }
    
    I2S_AD32_SET_HDMI_RP(AD_r);

    if(AD_r == buffer_size) AD_r=0;

    return 1;
}