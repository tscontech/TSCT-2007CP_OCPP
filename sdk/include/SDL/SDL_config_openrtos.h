﻿/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2012 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef _SDL_config_openrtos_h
#define _SDL_config_openrtos_h

#include "SDL_platform.h"

/* This is a set of defines to configure the SDL features */

#include <stdarg.h>

#define HAVE_ALLOCA_H		1
#define HAVE_SYS_TYPES_H	1
#define HAVE_STDIO_H	1
#define STDC_HEADERS	1
#define HAVE_STRING_H	1
#define HAVE_INTTYPES_H	1
#define HAVE_STDINT_H	1
#define HAVE_CTYPE_H	1
#define HAVE_MATH_H	1

/* C library functions */
#define HAVE_MALLOC	1
#define HAVE_CALLOC	1
#define HAVE_REALLOC	1
#define HAVE_FREE	1
#define HAVE_ALLOCA	1
#define HAVE_GETENV	1
#define HAVE_SETENV	1
#define HAVE_PUTENV	1
#define HAVE_SETENV	1
#define HAVE_UNSETENV	1
#define HAVE_QSORT	1
#define HAVE_ABS	1
#define HAVE_BCOPY	1
#define HAVE_MEMSET	1
#define HAVE_MEMCPY	1
#define HAVE_MEMMOVE	1
#define HAVE_MEMCMP	1
#define HAVE_STRLEN	1
#define HAVE_STRLCPY	1
#define HAVE_STRLCAT	1
#define HAVE_STRDUP	1
#define HAVE_STRCHR	1
#define HAVE_STRRCHR	1
#define HAVE_STRSTR	1
#define HAVE_STRTOL	1
#define HAVE_STRTOUL	1
#define HAVE_STRTOLL	1
#define HAVE_STRTOULL	1
#define HAVE_STRTOD	1
#define HAVE_ATOI	1
#define HAVE_ATOF	1
#define HAVE_STRCMP	1
#define HAVE_STRNCMP	1
#define HAVE_STRCASECMP	1
#define HAVE_STRNCASECMP 1
#define HAVE_SSCANF	1
#define HAVE_SNPRINTF	1
#define HAVE_VSNPRINTF	1
#define HAVE_M_PI	1
#define HAVE_ATAN	1
#define HAVE_ATAN2	1
#define HAVE_CEIL	1
#define HAVE_COPYSIGN	1
#define HAVE_COS	1
#define HAVE_COSF	1
#define HAVE_FABS	1
#define HAVE_FLOOR	1
#define HAVE_LOG	1
#define HAVE_POW	1
#define HAVE_SCALBN	1
#define HAVE_SIN	1
#define HAVE_SINF	1
#define HAVE_SQRT	1
#define HAVE_SETJMP	1
#define HAVE_NANOSLEEP	1

#define SIZEOF_VOIDP 4

/* Enable various audio drivers */
#define SDL_AUDIO_DISABLED 1
//#define SDL_AUDIO_DRIVER_DUMMY	1

/* Enable the stub joystick driver (src/joystick/dummy/\*.c) */
#define SDL_JOYSTICK_DISABLED	1

/* Enable the stub haptic driver (src/haptic/dummy/\*.c) */
#define SDL_HAPTIC_DISABLED	1

/* Enable various threading systems */
#define SDL_THREAD_OPENRTOS	1
#define SDL_ATOMIC_DISABLED 1

/* Enable various timer systems */
#define SDL_TIMER_OPENRTOS	1

/* Enable various video drivers */
#define SDL_VIDEO_DRIVER_CASTOR3	1

/* Enable system power support */
#define SDL_POWER_DISABLED 1
//#define SDL_POWER_CASTOR3 1

//#define SDL_RENDER_DISABLED 1
#ifdef CFG_SDL_M2D_ENABLE
    #define SDL_VIDEO_RENDER_M2D 1
#endif

#ifdef __SM32__
    #define SDL_ATOMIC_DISABLED 1
    #define SDL_BYTEORDER       SDL_BIG_ENDIAN

#elif defined(__NDS32__)
    #define SDL_ATOMIC_DISABLED 1

#endif

/* Enable touch slide gesture */
#ifdef CFG_TOUCH_ENABLE
    #define ENABLE_SLIDE_GESTURE
#endif

#endif /* _SDL_config_minimal_h */
