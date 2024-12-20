#include "ite/ith.h"
//#define DBG

void rtkbt_reset(void)
{
	int num = CFG_GPIO_BT_REG_ON, times = 1;

	ithGpioEnable(num);
	ithGpioSetOut(num);
	while (times>0)
	{
		ithGpioClear(num);
		usleep(500000);
		ithGpioSet(num);
		usleep(300000);
		times--;
	}
#ifdef DBG
	printf("BT reset fin pin: %d\n", GPIO_BT_REG_ON);
#endif
}