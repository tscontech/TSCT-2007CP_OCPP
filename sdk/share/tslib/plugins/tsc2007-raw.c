/**
*               tsc2007-raw.c
*   		Copyright (c) TSCT All rights reserved. <br>
*               data: 2021.12.05 <br>
*               author: dhy <br>
*               description: <br>
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <alloca.h>
#include <pthread.h>	
#include "ite/ith.h"   // for 
#include "ite/itp.h"
#include "config.h"
#include "tslib-private.h"

//-----------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------
#define TP_GPIO_PIN	    CFG_GPIO_TOUCH_INT

#if (TP_GPIO_PIN<32)
#define TP_GPIO_MASK    (1<<TP_GPIO_PIN)
#else
#define TP_GPIO_MASK    (1<<(TP_GPIO_PIN-32))
#endif

#define TSC2007_SLAVE_ADDR 	(0x90 >> 1)

#define CTRL_CMD_X_AXIS_ACC		(0x80)
#define CTRL_CMD_Y_AXIS_ACC		(0x90)

#define CTRL_CMD_X_AXIS		    (0xC0)
#define CTRL_CMD_Y_AXIS		    (0xD0)

#define CTRL_CMD_POWER_OFF_IRQ_EN	(0x0 << 2)  // Touch ADC off.. touch interrupt ON 
#define CTRL_CMD_ADC_ON_IRQ_DIS0	(0x1 << 2)	// Touch ADC On, touch interrupt Off

#define ADC_READ_DATA_CNT 3
#define TP_SAMPLE_DIFF_THRES 160

#define X_POS_MIN 80
#define X_POS_MAX 3975
#define Y_POS_MIN 105
#define Y_POS_MAX 3760

#define UI_OSD_PANE_W 800
#define UI_OSD_PANE_H 480


//-----------------------------------------------------------------------
// Local Variable
//-----------------------------------------------------------------------
static bool initialized = false;


//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------
static bool tsc2007_write_i2c(int fd, unsigned int cmdlen, unsigned char *cmd)
{
	ITPI2cInfo evt;
	int ret;

	if((cmd != NULL) && (cmdlen>0)) // test for i2c error... ktlee. 20190827
	{
		evt.slaveAddress   = TSC2007_SLAVE_ADDR;
		evt.cmdBuffer      = cmd;
		evt.cmdBufferSize  = cmdlen;
		evt.dataBuffer     = 0;
		evt.dataBufferSize = 0;
		
		ret = write(fd, &evt, 1);

		if (ret < 0)
		{
			printf("[TOUCH] i2c write failed\n");	
			return false;
		}
		else
		{
			return true;
		}
	}		

	return false;
}

static bool tsc2007_read_i2c(int fd, unsigned int cmdlen, unsigned char *cmd, unsigned int datalen, unsigned char *data)
{
	ITPI2cInfo evt;
	int ret;

	if((cmd != NULL) && (cmdlen>0) && (data != NULL) && (datalen>0)) // test for i2c error... ktlee. 20190827
	{
		evt.slaveAddress   = TSC2007_SLAVE_ADDR;
		evt.cmdBuffer      = cmd;
		evt.cmdBufferSize  = cmdlen;	
		evt.dataBuffer     = data;
		evt.dataBufferSize = datalen;	
		
		ret = read(fd, &evt, 1);
		
		if (ret < 0)
		{
			printf("[TOUCH] i2c read failed\n");
			return false;		
		}
		else
		{
			return true;
		}
	}

	return false;
}

static bool tsc2007_initialize(int fd)
{
	printf("[TOUCH] initialize tsc2007 for i2c\n");
	
	ithGpioSetMode(TP_GPIO_PIN, ITH_GPIO_MODE0);
	ithGpioSetIn(TP_GPIO_PIN);
	ithGpioCtrlEnable(TP_GPIO_PIN, ITH_GPIO_PULL_ENABLE);
	ithGpioCtrlEnable(TP_GPIO_PIN, ITH_GPIO_PULL_UP);
	ithGpioEnable(TP_GPIO_PIN);

	initialized = true;

	return true;
}

static bool tsc2007_read_coord(int fd, int *px, int *py)
{
	char buf[2];

	// read x
	unsigned char cmd = CTRL_CMD_X_AXIS_ACC|CTRL_CMD_ADC_ON_IRQ_DIS0;
	if (tsc2007_write_i2c(fd, 1, &cmd) == false)
	{
		printf("[TOUCH] touch activate x failed\n");
		return false;
	}
	
	usleep(1);

	cmd = CTRL_CMD_X_AXIS|CTRL_CMD_ADC_ON_IRQ_DIS0;
	if (tsc2007_read_i2c(fd, 1, &cmd, 2, buf) == false)
	{
		printf("[TOUCH] read x failed\n");
		return false;
	}
	*px = ((buf[0] << 8) | buf[1]) >> 4;

	// read y
	cmd = CTRL_CMD_Y_AXIS_ACC|CTRL_CMD_ADC_ON_IRQ_DIS0;
	if (tsc2007_write_i2c(fd, 1, &cmd) == false)
	{
		printf("[TOUCH] touch activate y failed\n");
		return false;
	}
	
	usleep(1);

	cmd = CTRL_CMD_Y_AXIS|CTRL_CMD_ADC_ON_IRQ_DIS0;
    if(tsc2007_read_i2c(fd, 1, &cmd, 2, buf) == false)
    {
		printf("[TOUCH] read y failed\n");
		return false;
	}	
	*py = ((buf[0] << 8) | buf[1]) >> 4;
	
	cmd = CTRL_CMD_POWER_OFF_IRQ_EN;
	if (tsc2007_write_i2c(fd, 1, &cmd) == false)
	{
		printf("[TOUCH] touch ADC power down and Pen Int enable. \n");
		return false;
	}
	return true;
}

static bool tsc2007_filter(unsigned int *pos, unsigned int *average)
{
	unsigned int diffab, diffbc, diffac;
	unsigned int group_a, group_b, group_c;

    if ((pos == NULL) || (average == NULL))
        return false;

    group_a = pos[0];
    group_b = pos[1];
    group_c = pos[2];

    diffab = abs(group_a - group_b);
    diffac = abs(group_a - group_c);
    diffbc = abs(group_b - group_c);

    if ((diffab > TP_SAMPLE_DIFF_THRES) ||
        (diffbc > TP_SAMPLE_DIFF_THRES))
        return false;

    if ((diffab <= diffac) && (diffab <= diffbc))
    {
        *average = (group_a + group_b) / 2;
    }
    else if ((diffac <= diffab) && (diffac <= diffbc))
    {
        *average = (group_a + group_c) / 2;
    }
    else
    {
        *average = (group_b + group_c) / 2;
    }

    return true;
}

static int tsc2007_read(struct tslib_module_info *inf, struct ts_sample *samp, int nr)
{
	struct tsdev *ts = inf->dev;
	
	if (initialized == false)
		tsc2007_initialize(ts->fd);

	samp->pressure = !(ithGpioGet(TP_GPIO_PIN) & TP_GPIO_MASK);

	if (samp->pressure) {
		int i = 0, avr_x, avr_y;
		unsigned int xpos[ADC_READ_DATA_CNT] = {0,};
		unsigned int ypos[ADC_READ_DATA_CNT] = {0,};

		while (i != ADC_READ_DATA_CNT) 
		{
			if (tsc2007_read_coord(ts->fd, &xpos[i], &ypos[i]) == false) 
			{
				printf("[TOUCH] get coord failed(i=%d)\n", i);
					return false;
			}

			if (xpos[i] == 4095 || xpos[i] == 0 || ypos[i] == 4095 || ypos[i] == 0) 
			{
				printf("[TOUCH] invalid coord (x=%d, y=%d)\n", xpos[i], ypos[i]);
					return false;
			}
			i++;
		}

		if (tsc2007_filter(xpos, &avr_x) == false) return false;
		if (tsc2007_filter(ypos, &avr_y) == false) return false;

		if ((avr_x < X_POS_MIN) || (avr_x > X_POS_MAX) || (avr_y < Y_POS_MIN) || (avr_y > Y_POS_MAX)){
			printf("[TOUCH] coord exceeded!(avr_x=%d, avr_y=%d, [minx:%ld, maxx=%ld, miny=%ld, maxy=%ld])",avr_x, avr_y, X_POS_MIN, X_POS_MAX, Y_POS_MIN, Y_POS_MAX);
			return false;
		}

		samp->x = (UI_OSD_PANE_W * (avr_x - X_POS_MIN + 1)) / (X_POS_MAX - X_POS_MIN + 1);
		samp->y = (UI_OSD_PANE_H * (Y_POS_MAX + 1 - avr_y)) / (Y_POS_MAX - Y_POS_MIN + 1);

	//	printf("[TOUCH] x:%d, y:%d\n", samp->x, samp->y);
		usleep(1000);  // fixed repeate key error.. 20200416...by ktlee 
	}
	
	return nr;
}

static const struct tslib_ops tsc2007_ops =
{
	.read	= tsc2007_read,
};

TSAPI struct tslib_module_info *tsc2007_mod_init(struct tsdev *dev, const char *params)
{
	struct tslib_module_info *m;

	m = malloc(sizeof(struct tslib_module_info));
	if (m == NULL)
		return NULL;

	m->ops = &tsc2007_ops;
	return m;
}

#ifndef TSLIB_STATIC_CASTOR3_MODULE
	TSLIB_MODULE_INIT(tsc2007_mod_init);
#endif


