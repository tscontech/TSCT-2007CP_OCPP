﻿#include <sys/ioctl.h>
//#include <assert.h>
//#include <stdio.h>
//#include <pthread.h>
//#include <unistd.h>
#include "itp_cfg.h"
#include "ite/itp.h"
#include "ssp/mmp_spi.h"

#include <errno.h>
//#include <malloc.h>
#include <string.h>

#include "ite/ith.h"
//#include "itp_cfg.h"
//=============================================================================
//                Constant Definition
//============================================================================= 
#define RESET_PORT      CFG_GPIO_MIPI_RESET

#define DEVIE_ID (0x70 << 16)

#define RESET_SET()     ithGpioSet(RESET_PORT)
#define RESET_CLR()      ithGpioClear(RESET_PORT)

#define PalSleep(x)		ithDelay(x*1000)

#define REGFLAG_DELAY                     0XFFE
#define REGFLAG_END_OF_TABLE   				    0xFFF

#ifdef	CFG_RGBTOMIPI_SPI0
#define MIPI_SPI_PORT    SPI_0
#endif

#ifdef	CFG_RGBTOMIPI_SPI1
#define MIPI_SPI_PORT    SPI_1
#endif

#ifndef	MIPI_SPI_PORT
#error "It must select SPI0 or SPI1 for SSD2828"
#endif

//=============================================================================
//                Macro Definition
//=============================================================================
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

//=============================================================================
//                Structure Definition
//=============================================================================
static unsigned int ssd_pre_initialize[] = {
    0x00B10404,
    0x00B20408,
    0x00B30826,
    0x00B40140,
    0x00B501E0,
    0x00B60003,
    0x00B80000,
    0x00B90000,
    0x00BA811F,
    0x00BB0005,
    0x00D51860,
    0x00C91301,
    0x00CA1402,
    0x00CB0513,
    0x00CC0709,
    0x00DE0000, //1-lane
    0x00B90001,
    0x00D60004,
    0x00B80000,
};

static unsigned int ssd_post_initialize[] = {
    0x00B70342,
    0x00B80000,
    0x00BC0001,
    0x00BF0011,
    0x00BF0029,
    0x00B7034B,  
};

static unsigned int ssd_post_initialize_bist[] = {
    0x00EE0600,  
    0x00B7034B,  
};

struct LCM_setting_table
{
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] =
{
    { 0xf0, 1, { 0xc3 } },
    { 0xf0, 1, { 0x96 } },

    { 0xB6, 3, { 0xa0, 0x02, 0x3b } },
    { 0xB4, 1, { 0x01 } },

    { 0xe8, 8, { 0x40, 0x8a, 0x00, 0x00, 0x29, 0x19, 0xa5, 0x33 } },

    { 0xc1, 1, { 0x06 } },
    { 0xc2, 1, { 0xa7 } },
    { 0xc5, 1, { 0x18 } },

    { 0xe0, 14, { 0xf0, 0x09, 0x0b, 0x06, 0x04, 0x15, 0x2f, 0x54, 0x42, 0x3c, 0x17, 0x14, 0x18, 0x1b } },//Positive Voltage Gamma Control
    { 0xe1, 14, { 0xf0, 0x09, 0x0b, 0x06, 0x04, 0x03, 0x2d, 0x43, 0x42, 0x3b, 0x16, 0x14, 0x17, 0x1b } },//Negative Voltage Gamma Control

    { 0xf0, 1, { 0x3c } },
    { 0xf0, 1, { 0x69 } },
    { REGFLAG_DELAY, 120, {} },

    { 0x36, 1, { 0x48 } },
    { 0x3a, 1, { 0x77 } },

    { 0x11, 1, { 0x00 } },                // Sleep-Out                                                          
    { REGFLAG_DELAY, 150, {} },
    { 0x29, 1, { 0x00 } },
    { REGFLAG_DELAY, 50, {} },
    { REGFLAG_END_OF_TABLE, 0x00, {}}
};

//=============================================================================
//                Global Data Definition
//=============================================================================
static unsigned char dcs_exit_sleep_mode[] = {0x11};
static unsigned char dcs_set_diaplay_on[] = {0x29};
static unsigned char dcs_enter_sleep_mode[] = {0x10};
static unsigned char dcs_set_diaplay_off[] = {0x28};

//=============================================================================
//                Private Function Definition
//=============================================================================

/*******************************************************************************

*******************************************************************************/
void ssd2828_gpio_init(void)
{
    uint32_t result = 0;
    
    result = mmpSpiInitialize(MIPI_SPI_PORT, SPI_OP_MASTR, CPO_0_CPH_0, SPI_CLK_20M);
    //if(result == 0)
    //{
    //    //LOG_ERR "MIPI SPI port initial FAIL!! \n", LOG_END
    //    return;
    //}

	ithGpioSet(RESET_PORT);
	ithGpioSetMode(RESET_PORT, ITH_GPIO_MODE0);
	ithGpioSetOut(RESET_PORT);
}
void ssd2828_reset(void) 
{
    RESET_SET();
    PalSleep(20);
    RESET_CLR();
    PalSleep(30);
    RESET_SET();
    PalSleep(200);	   
}

static void spi_send_data(unsigned int data)
{
    uint8_t cBuf[4];
    uint32_t rst;
    
    cBuf[0] = (uint8_t)((data>>16)&0xFF);
    cBuf[1] = (uint8_t)((data>>8)&0xFF);
    cBuf[2] = (uint8_t)(data&0xFF);
    
    rst = mmpSpiPioWrite(MIPI_SPI_PORT, cBuf, 3, 0, 0, 8);
    
    return;
}

static void spi_recv_data(unsigned int* data)
{
    unsigned int i = 0, temp = 0x73;   //read data
    uint8_t cBuf[2];
    uint8_t dBuf[2];
    uint32_t rst;
    
    cBuf[0] = 0x73;
    rst = mmpSpiPioRead(MIPI_SPI_PORT, cBuf, 1, dBuf, 2, 8);
    //rst = mmpSpiPioRead(MIPI_SPI_PORT, cBuf, 1, data, 2, 8);
    memcpy(data, dBuf, 2);
    //printf(" RdData = %04x, %02x, %02x\n",*data, dBuf[0], dBuf[1] );
}

void send_ctrl_cmd(unsigned int cmd)
{
    unsigned int out = (DEVIE_ID | cmd );
    spi_send_data(out);
}

static void send_data_cmd(unsigned int data)
{
    unsigned int out = (DEVIE_ID | (0x2 << 16) | data );
    spi_send_data(out);
}

unsigned int ssd_read_register(unsigned int reg) 
{
	unsigned int data = 0;
	send_ctrl_cmd(reg);
	spi_recv_data(&data);
	return data;
}

void ssd_set_register(unsigned int reg_and_value)
{
    send_ctrl_cmd(reg_and_value >> 16);
    send_data_cmd(reg_and_value & 0x0000ffff);
}

int ssd_set_registers(unsigned int reg_array[], int n) 
{

	int i = 0;
	for(i = 0; i < n; i++) {
		if(reg_array[i] < 0x00b00000) {      //the lowest address is 0xb0 of ssd2828
		    if(reg_array[i] < 20000)
		    	ithDelay(reg_array[i]);
		    else {
		    	ithDelay(reg_array[i]);
		    }
		} else {
			ssd_set_register(reg_array[i]);
		}
	}
	return 0;
}

static int ssd2828_read_from_lcd(unsigned char *buf, int reg, int size)
{
    unsigned int i=0;
    unsigned int tmp;
    unsigned int data;
    
    printf("reg=%x, len=%d\n",reg,size);
    
    //do unluck and page command
    
    //0xB70382 (set LOW power generic read )
    ssd_set_register(0x00B70382);

    //0xBB00XX (set LP freq, RXinternal clock freq, )
    ssd_set_register(0x00BB0028);
    
    //set read count (0xC1 00 xx)
    ssd_set_register(0x00C10000 | size);   
        
    //STOP mipi transport(0xC0 00 01)
    ssd_set_register(0x00C00100);     
    
    //0xBC 00 01 (set read length of register that wanna be read)
    ssd_set_register(0x00BC0001);
    
    //0xBF 00 2B (set reg addr 0x2B)    
    ssd_set_register(0x00BF0000| reg&0xFF);    
    
    //0xC4 00 01 (enable BTA)
    ssd_set_register(0x00C40001);  
      
    //0xC6 read read (read and verify RDR bit0=0 0xC6 read read)
    /*
    tmp = ssd_read_register(0xC6);
    printf("GOT error1, tmp=%x\n",tmp);
    if( !(tmp&0x0001) )	goto ErrEnd;
    */
    
    //wait for 16 ms
    ithDelay(200*1000);

    //0xC6 Read read (check RDR bit0=1)
    tmp = ssd_read_register(0xC6);
    while(tmp&0x01)
    {
        data = ssd_read_register(0xFF);
        printf("data=%04x\n",data);
        
        buf[i*2+0] = data&0xFF;
        if( (i!=(size/2)) || !(size&0x1) )
        {
            buf[i*2+1] = (data>>8)&0xFF;
        }
        
        tmp = ssd_read_register(0xC6);    
        i++;
        if( i>=(size/2) )	break;
    }
    
    return i*2;
    
ErrEnd:
    //error and return 0 byte
    tmp = ssd_read_register(0xB0);
    printf("GOT error, tmp=%x\n",tmp);
    return 0;
}

static void ssd2828_write_to_lcd(unsigned short *buf, int size)
{
	unsigned int i = 0;
	int cnt = size/2;
	
	if(size&0x01)  cnt++; 
	
	ssd_set_register(0x00B70342);
	ssd_set_register(0x00Bc0000 | size);
	
	send_ctrl_cmd(0x000000BF);
	for(i=0; i<cnt; i++)    send_data_cmd(buf[i]);
	
	ithDelay(1000);
}

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;
	unsigned int size;
	unsigned char dcs[64] = {0};
	memset((char*)dcs,0,sizeof(dcs));
	
	for(i = 0; i < count; i++)
	{
        int j;
        
        if(table[i].cmd==REGFLAG_DELAY )
        {
            ithDelay(table[i].count*1000);
            continue;
        }
        else if(table[i].cmd==REGFLAG_END_OF_TABLE)  break;
        else
        {
    		    dcs[0] = table[i].cmd;
    		
            for(j=0; j<table[i].count; j++)
            {
              	if(j>=sizeof(table[i].para_list))  break;
              	
              	dcs[1+j]=table[i].para_list[j];
              	//printf("dcs=[%02x, %02x],",dcs[1+j],table[i].para_list[j]);
            }
            size=1+table[i].count;
            dcs[1+table[i].count] = 0;
            	
            ssd2828_write_to_lcd((unsigned short *)&dcs[0], size);
            //printf("\n  show table::\n");
            //if(size&0x01)   ithPrintVram8(dcs, size+1);
            //else    ithPrintVram8(dcs, size);  
            //ssd2828_read_from_lcd((unsigned short *)&dcs[0], table[i].cmd, size);
       }          
	}
   // printf("LCM_setting_table count is:%d,init count is %d\n",count,i); 
}

static int ssd2828_init(void) 
{
    //printf("lcd_init\n");
	  
    PalSleep(20); //sleep 20 ms 
	  
    ssd_set_registers(ssd_pre_initialize, ARRAY_SIZE(ssd_pre_initialize));	
	  
    PalSleep(20); //sleep 20 ms

    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
    
    ssd_set_registers(ssd_post_initialize, ARRAY_SIZE(ssd_post_initialize));

    return 0;
}

static void SSD2828Initialize(void)
{
    unsigned int temp,i;
    temp = 0xB0-1;
	
    printf("ssd2828_gpio_init::\n");
    ssd2828_gpio_init();
    
    printf("ssd2828_reset::\n");
    ssd2828_reset();
    
    printf("ssd2828_mipi_init::\n");
    ssd2828_init();       


    for(i=0;i<63;i++)
    {
        temp++;
        printf("read %02x=%04x\r\n",temp,ssd_read_register(temp));
    }

}
//=============================================================================
//                Public Function Definition
//=============================================================================


void itpRGBToMIPIInit(void)
{
    SSD2828Initialize();
}

