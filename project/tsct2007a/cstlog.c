/**
*       @file
*               cstlog.c
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2017.12.20 <br>
*               author: dyhwang <br>
*               description: <br>
*/
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
//#include "ite/audio.h"
//#include "audio_mgr.h"
#include "ctrlboard.h"

//-----------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------
#define RED(X) "\x1b[0;31m" X "\x1b[0m\n"
#define GREEN(X) "\x1b[0;32m" X "\x1b[0m\n"
#define YELLOW(X) "\x1b[0;33m" X "\x1b[0m\n"
#define BLUE(X) "\x1b[0;34m" X "\x1b[0m\n"
#define MAGENTA(X) "\x1b[0;35m" X "\x1b[0m\n"
#define CYAN(X) "\x1b[0;36m" X "\x1b[0m\n"
#define WHITE(X) "\x1b[0;37m" X "\x1b[0m\n"
#define GRAY(X) "\x1b[0;30m" X "\x1b[0m\n"
#define BOLD(X) "\x1b[01m" X "\x1b[0m\n"
#define ULINE(X) "\x1b[04m" X "\x1b[0m\n"


#define MAX_LOG_CHAR 1024

//-----------------------------------------------------------------------
// Local Variable
//-----------------------------------------------------------------------
static int sCstLogLevel = CTLOG_LEVEL_TRACE;


//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------
void CstLogSetLevel(int level)
{
	sCstLogLevel = level;
}


static void outputDebugFString(char* fmt, ...)
{
	char b[1024];

	va_list ap;
   	va_start(ap, fmt);

	memset(b, 0x00, 1024);
	if(vsnprintf(b, 1024, fmt, ap) < 0)
	{
		return;
	}

	va_end(ap);

	printf("%s", b);
}


void CstLogOutput(int type, char* file, char* func, int line, char* fmt, ...)
{
	char b[1024];

	va_list ap;
   	va_start(ap, fmt);

	memset(b, 0x00, 1024);
	if(vsnprintf(b, 1024, fmt, ap) < 0)
	{
		return;
	}

	va_end(ap);

	switch(type)
	{
	case CTLOG_LEVEL_TRACE :
		outputDebugFString(MAGENTA("[T][%s:%04d][%s]%s"), func, line, file, b);
		break;
	case CTLOG_LEVEL_WARN : 
		outputDebugFString(YELLOW("[W][%s:%04d][%s]%s"), func, line, file, b);
		break;
	case CTLOG_LEVEL_INFO :
		outputDebugFString(GREEN("[I][%s:%04d][%s]%s"), func, line, file, b);
		break;
	case CTLOG_LEVEL_DEBUG : 
		outputDebugFString(YELLOW("[D][%s:%04d][%s]%s"), func, line, file, b);
		break;
	case CTLOG_LEVEL_ERROR : 
		outputDebugFString(RED("[E][%s:%04d][%s]%s"), func, line, file, b);
		break;
	case CTLOG_LEVEL_FATAL :
		outputDebugFString(RED("[F][%s:%04d][%s]%s"), func, line, file, b);
		break;
	}

}

void CstLogColorOutput(int type, char* func, char* fmt, ...) {
	char b[1024];

	va_list ap;
   	va_start(ap, fmt);

	memset(b, 0x00, 1024);
	if(vsnprintf(b, 1024, fmt, ap) < 0)	{
		return;
	}

	va_end(ap);

	switch(type)	{
	case CTLOG_GREEN :
		outputDebugFString(GREEN("[%s(O%d)(P%d)]%s"), func, shmDataAppInfo.app_order,shmDataAppInfo.auth_type[0] ,b);
		break;
	case CTLOG_YELLOW : 
		outputDebugFString(YELLOW("[%s(O%d)(P%d)]%s"), func, shmDataAppInfo.app_order,shmDataAppInfo.auth_type[0] ,b);
		break;
	case CTLOG_RED :
		outputDebugFString(RED("[%s(O%d)(P%d)]%s"), func, shmDataAppInfo.app_order,shmDataAppInfo.auth_type[0] ,b);
		break;
	case CTLOG_BLUE :
		outputDebugFString(BLUE("[%s(O%d)(P%d)]%s"), func, shmDataAppInfo.app_order,shmDataAppInfo.auth_type[0] ,b);
		break;		
	case CTLOG_MAGENTA :
		outputDebugFString(MAGENTA("[%s(O%d)(P%d)]%s"), func, shmDataAppInfo.app_order,shmDataAppInfo.auth_type[0] ,b);
		break;		
	case CTLOG_CYAN :
		outputDebugFString(CYAN("[%s(O%d)(P%d)]%s"), func, shmDataAppInfo.app_order,shmDataAppInfo.auth_type[0] ,b);
		break;	
	case CTLOG_WHITE:
		outputDebugFString(WHITE("[%s(O%d)(P%d)]%s"), func, shmDataAppInfo.app_order,shmDataAppInfo.auth_type[0] ,b);
		break;				
	case CTLOG_GRAY:
		outputDebugFString(GRAY("[%s(O%d)(P%d)]%s"), func, shmDataAppInfo.app_order,shmDataAppInfo.auth_type[0] ,b);
		break;						

	}

}

void OcppLogColorOutput(int type, char* fmt, ...)
{
	char b[1024];

	va_list ap;
   	va_start(ap, fmt);

	memset(b, 0x00, 1024);
	if(vsnprintf(b, 1024, fmt, ap) < 0)	{
		return;
	}

	va_end(ap);

	switch(type)	
	{
		case CTLOG_MAGENTA :
			outputDebugFString(MAGENTA("[%s]%s"), "Recv Msg from CSMS", b);
		break;		

		case CTLOG_CYAN :
			outputDebugFString(CYAN("[%s]%s"), "Send Msg to CSMS", b);
		break;				
	}	
}

