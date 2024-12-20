/**
*       @file
*               layer_numkeypad.c
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2017.12.08 <br>
*               author: dyhwang <br>
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
static NumkeyInputListener sInputListener = NULL;

//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------
bool NumKeypad1OnPress(ITUWidget* widget, char* param)
{
	if (sInputListener)
		(*sInputListener)('1');

    return true;
}


bool NumKeypad2OnPress(ITUWidget* widget, char* param)
{
	if (sInputListener)
		(*sInputListener)('2');
	

    return true;
}


bool NumKeypad3OnPress(ITUWidget* widget, char* param)
{
	if (sInputListener)
		(*sInputListener)('3');
	
    return true;
}


bool NumKeypad4OnPress(ITUWidget* widget, char* param)
{
	if (sInputListener)
		(*sInputListener)('4');


    return true;
}


bool NumKeypad5OnPress(ITUWidget* widget, char* param)
{
	if (sInputListener)
		(*sInputListener)('5');


    return true;
}


bool NumKeypad6OnPress(ITUWidget* widget, char* param)
{
	if (sInputListener)
		(*sInputListener)('6');

    return true;
}


bool NumKeypad7OnPress(ITUWidget* widget, char* param)
{
	if (sInputListener)
		(*sInputListener)('7');


    return true;
}


bool NumKeypad8OnPress(ITUWidget* widget, char* param)
{
	if (sInputListener)
		(*sInputListener)('8');

	
    return true;
}


bool NumKeypad9OnPress(ITUWidget* widget, char* param)
{
	if (sInputListener)
		(*sInputListener)('9');


    return true;
}


bool NumKeypad0OnPress(ITUWidget* widget, char* param)
{
	if (sInputListener)
		(*sInputListener)('0');

    return true;
}


bool NumKeypadDelOnPress(ITUWidget* widget, char* param)
{
	if (sInputListener)
		(*sInputListener)('d');

    return true;
}

bool NumKeypadResetOnPress(ITUWidget* widget, char* param)
{
	if (sInputListener)
		(*sInputListener)('r');

    return true;
}

void NumKeypadSetInputListener(NumkeyInputListener listener)
{
	sInputListener = listener;
}

