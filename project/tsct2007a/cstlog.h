/**
*       @file
*               cstlog.h
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2017.12.20 <br>
*               author: dyhwang <br>
*               description: <br>
*/
#ifndef __CSTLOG_H__
#define __CSTLOG_H__

#define CTLOG_LEVEL_FATAL 5
#define CTLOG_LEVEL_ERROR 4
#define CTLOG_LEVEL_DEBUG 3
#define CTLOG_LEVEL_INFO  2
#define CTLOG_LEVEL_WARN  1
#define CTLOG_LEVEL_TRACE 0
#define CTLOG_LEVEL_OFF   0xff

#define CtLogFatal(...) CstLogOutput(CTLOG_LEVEL_FATAL,__FILE__, __func__, __LINE__, __VA_ARGS__)
#define CtLogError(...) CstLogOutput(CTLOG_LEVEL_ERROR,__FILE__, __func__, __LINE__, __VA_ARGS__)
#define CtLogDebug(...) CstLogOutput(CTLOG_LEVEL_DEBUG,__FILE__, __func__, __LINE__, __VA_ARGS__)
#define CtLogInfo(...) CstLogOutput(CTLOG_LEVEL_INFO,__FILE__, __func__, __LINE__, __VA_ARGS__)
#define CtLogWarn(...) CstLogOutput(CTLOG_LEVEL_WARN,__FILE__, __func__, __LINE__, __VA_ARGS__)
#define CtLogTrace(...) CstLogOutput(CTLOG_LEVEL_TRACE,__FILE__, __func__, __LINE__,  __VA_ARGS__)


#define CTLOG_RED 		5
#define CTLOG_YELLOW 	4
#define CTLOG_GREEN 	3
#define CTLOG_BLUE      2
#define CTLOG_MAGENTA   1
#define CTLOG_CYAN      6
#define CTLOG_WHITE     7
#define CTLOG_GRAY      8



#define CtLogRed(...) CstLogColorOutput(CTLOG_RED,__func__,__VA_ARGS__)
#define CtLogYellow(...) CstLogColorOutput(CTLOG_YELLOW,__func__,__VA_ARGS__)
#define CtLogGreen(...) CstLogColorOutput(CTLOG_GREEN,__func__,__VA_ARGS__)
#define CtLogBlue(...) CstLogColorOutput(CTLOG_BLUE,__func__,__VA_ARGS__)
#define CtLogMagenta(...) CstLogColorOutput(CTLOG_MAGENTA,__func__,__VA_ARGS__)
#define CtLogCyan(...) CstLogColorOutput(CTLOG_CYAN,__func__,__VA_ARGS__)
#define CtLogWhite(...) CstLogColorOutput(CTLOG_WHITE,__func__,__VA_ARGS__)
#define CtLogGray(...) CstLogColorOutput(CTLOG_GRAY,__func__,__VA_ARGS__)

#define OcppTxMsgLog(...) OcppLogColorOutput(CTLOG_CYAN,__VA_ARGS__)
#define OcppRxMsgLog(...) OcppLogColorOutput(CTLOG_MAGENTA,__VA_ARGS__)


#endif

