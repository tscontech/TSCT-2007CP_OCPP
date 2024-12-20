/***************************************************************************
 *
 *            Copyright (c) 2010-2019 by HCC Embedded
 *
 * This software is copyrighted by and is the sole property of
 * HCC.  All rights, title, ownership, or other interests
 * in the software remain the property of HCC.  This
 * software may only be used in accordance with the corresponding
 * license agreement.  Any unauthorized use, duplication, transmission,
 * distribution, or disclosure of this software is expressly forbidden.
 *
 * This Copyright notice may not be removed or modified without prior
 * written consent of HCC.
 *
 * HCC reserves the right to modify this software without notice.
 *
 * HCC Embedded
 * Budapest 1133
 * Vaci ut 76
 * Hungary
 *
 * Tel:  +36 (1) 450 1302
 * Fax:  +36 (1) 450 1303
 * http: www.hcc-embedded.com
 * email: info@hcc-embedded.com
 *
 ***************************************************************************/
#ifndef PSP_STRING_H_
#define PSP_STRING_H_

#include <string.h>
#include "psp_types.h"

#include "../../version/ver_psp_string.h"
#if VER_PSP_STRING_MAJOR != 1 || VER_PSP_STRING_MINOR != 7
 #error Incompatible PSP_STRING version number!
#endif

/* 35 S : MISRA-C:2012/AMD1/TC1 R.2.1: Static procedure is not explicitly called in code analysed. */
/* 77 S : MISRA-C:2004 19.4: Macro replacement list needs parentheses. */
/*LDRA_INSPECTED 35 S*/
/*LDRA_INSPECTED 77 S*/
#define psp_memcpy( d, s, l )    memcpy( ( d ), ( s ), (size_t)( l ) )

/*LDRA_INSPECTED 35 S*/
/*LDRA_INSPECTED 77 S*/
#define psp_memmove( d, s, l )   memmove( ( d ), ( s ), (size_t)( l ) )

/*LDRA_INSPECTED 35 S*/
/*LDRA_INSPECTED 77 S*/
#define psp_memset( d, c, l )    memset( ( d ), ( c ), (size_t)( l ) )

/*LDRA_INSPECTED 35 S*/
/*LDRA_INSPECTED 77 S*/
#define psp_memcmp( s1, s2, l )  memcmp( ( s1 ), ( s2 ), (size_t)( l ) )

/*LDRA_INSPECTED 35 S*/
/*LDRA_INSPECTED 77 S*/
#define psp_strnlen( s, l )      strnlen( ( s ), (size_t)( l ) )

/*LDRA_INSPECTED 35 S*/
/*LDRA_INSPECTED 77 S*/
#define psp_strncat( d, s, l )   strncat( ( d ), ( s ), (size_t)( l ) )

/*LDRA_INSPECTED 35 S*/
/*LDRA_INSPECTED 77 S*/
#define psp_strncpy( d, s, l )   strncpy( ( d ), ( s ), (size_t)( l ) )

/*LDRA_INSPECTED 35 S*/
/*LDRA_INSPECTED 77 S*/
#define psp_strncmp( s1, s2, l ) strncmp( ( s1 ), ( s2 ), (size_t)( l ) )

uint16_t psp_w16csnlen ( wchar16_t const * const p_src, uint16_t size );
wchar16_t * psp_w16csncat ( wchar16_t * const p_dst, wchar16_t const * const p_src, uint16_t size );
wchar16_t * psp_w16csncpy ( wchar16_t * const p_dst, wchar16_t const * const p_src, uint16_t size );
int psp_w16csncmp ( wchar16_t const * const p_src1, const wchar16_t * const p_src2, uint16_t size );
const wchar16_t * psp_w16csnchr ( wchar16_t const * const p_src, uint16_t size, wchar16_t ch );

#endif /* ifndef PSP_STRING_H_ */

