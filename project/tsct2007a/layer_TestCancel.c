#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "scene.h"
#include "ctrlboard.h"

static ITUBackground* stestCancelBackground;
static ITUTextBox* stestCanceltxt;

static unsigned char numarr[12];
static int codenum;


static void TestCodeNumber()
{
	int i;
	
	for (i = 0; i < 11; i++)
		numarr[i] = ' ';
	codenum = 0;
	
	ituTextBoxSetString(stestCanceltxt, "");
}


static void UpdatecdNumber()
{	
	char buf[12];
	sprintf(buf, "%c%c%c%c%c%c%c%c%c%c%c", numarr[0], numarr[1], numarr[2], numarr[3], numarr[4], numarr[5], numarr[6], numarr[7], numarr[8], numarr[9], numarr[10]);

	ituTextBoxSetString(stestCanceltxt, buf);
}


static void TestCancelNumInputListener(char c)
{
	if (c != 'd') // number
	{
		if (codenum < 4)
		{
			numarr[codenum++] = c;
		}
	}
	else // delete (back space)
	{
		if (codenum > 0)
		{			
			numarr[--codenum] = ' ';
		}
	}

	UpdatecdNumber();	
	
}


bool TestCancelEnter(ITUWidget* widget, char* param)
{

	if (!stestCancelBackground)
	{		
        stestCancelBackground = ituSceneFindWidget(&theScene, "testCancelBackground");
        assert(stestCancelBackground);

		stestCanceltxt = ituSceneFindWidget(&theScene, "testCanceltxt");
		assert(stestCanceltxt);
		
		
	}
	
	TestCodeNumber();	
	NumKeypadSetInputListener(TestCancelNumInputListener);

    return true;
}


bool TestCancelLeave(ITUWidget* widget, char* param)
{

    return true;
}

