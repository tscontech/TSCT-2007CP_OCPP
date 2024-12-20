/**
*       @file
*               layer_password.c
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2018.01.10 <br>
*               author: bmlee <br>
*               description: <br>
*/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lwip/ip.h"
#include "ite/itp.h"
#include "scene.h"
#include "ctrlboard.h"

//-----------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// Local Variable
//-----------------------------------------------------------------------
static PasskeyInputListener sPassInputListener = NULL;

//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------


bool PassKeypadDelOnPress(ITUWidget* widget, char* param)
{
	if (sPassInputListener)
		(*sPassInputListener)('d');

    return true;
}

bool PassKeypad0OnPress(ITUWidget* widget, char* param)
{
	if (sPassInputListener)
		(*sPassInputListener)('0');
    return true;
}

bool PassKeypad9OnPress(ITUWidget* widget, char* param)
{
	if (sPassInputListener)
		(*sPassInputListener)('9');
    return true;
}

bool PassKeypad8OnPress(ITUWidget* widget, char* param)
{
	if (sPassInputListener)
		(*sPassInputListener)('8');
    return true;
}

bool PassKeypad7OnPress(ITUWidget* widget, char* param)
{
	if (sPassInputListener)
		(*sPassInputListener)('7');
    return true;
}

bool PassKeypad6OnPress(ITUWidget* widget, char* param)
{
	if (sPassInputListener)
		(*sPassInputListener)('6');
    return true;
}

bool PassKeypad5OnPress(ITUWidget* widget, char* param)
{
	if (sPassInputListener)
		(*sPassInputListener)('5');
    return true;
}

bool PassKeypad4OnPress(ITUWidget* widget, char* param)
{
	if (sPassInputListener)
		(*sPassInputListener)('4');
    return true;
}

bool PassKeypad3OnPress(ITUWidget* widget, char* param)
{
	if (sPassInputListener)
		(*sPassInputListener)('3');
    return true;
}

bool PassKeypad2OnPress(ITUWidget* widget, char* param)
{
	if (sPassInputListener)
		(*sPassInputListener)('2');
    return true;
}

bool PassKeypad1OnPress(ITUWidget* widget, char* param)
{
	if (sPassInputListener)
		(*sPassInputListener)('1');
    return true;
}

void PassKeypadSetInputListener(PasskeyInputListener listener)
{
	sPassInputListener = listener;
}
