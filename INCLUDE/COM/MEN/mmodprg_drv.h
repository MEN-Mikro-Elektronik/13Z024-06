/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: mmodprg_drv.h
 *
 *       Author: kp
 *
 *  Description: Header file for MMODPRG driver
 *               - MMODPRG specific status codes
 *               - MMODPRG function prototypes
 *
 *     Switches: _ONE_NAMESPACE_PER_DRIVER_
 *               _LL_DRV_
 *
 *---------------------------------------------------------------------------
 * Copyright (c) 2010-2019, MEN Mikro Elektronik GmbH
 ****************************************************************************/
/*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _MMODPRG_DRV_H
#define _MMODPRG_DRV_H

#ifdef __cplusplus
      extern "C" {
#endif


/*-----------------------------------------+
|  TYPEDEFS                                |
+-----------------------------------------*/
typedef struct {
    int      offset;      /**< offset relative to hardware start address */
    u_int32  value;       /**< value read from / write to hardware register */
} MMODPRG_DX_PB;

/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/
/* MMODPRG specific status codes (STD) */			/* S,G: S=setstat, G=getstat */
/*#define MMODPRG_XXX           M_DEV_OF+0x00	*/	/* G,S: xxx */

/* MMODPRG specific status codes (BLK)	*/	   /* S,G: S=setstat, G=getstat */
#define MMODPRG_BLK_D8       M_DEV_BLK_OF+0x00 /* G,S: Read/write 8bit value */
#define MMODPRG_BLK_D16      M_DEV_BLK_OF+0x01 /* G,S: Read/write 16bit value*/
#define MMODPRG_BLK_D32      M_DEV_BLK_OF+0x02 /* G,S: Read/write 32bit value*/


/* some useful defines... */

#ifndef __GNUC__
#define inline
#endif

static inline int
MMODPRG_SetValue( MDIS_PATH path, int code, int offset, u_int32 value )
{
    MMODPRG_DX_PB   pb;
    M_SG_BLOCK      blk;

    pb.offset = offset;
    pb.value  = value;

    blk.size = sizeof( pb );
    blk.data = (void*)&pb;

    return( M_setstat( path, code, (INT32_OR_64)&blk ) );
}

#define MMODPRG_SetD8( path, offset, val )   \
        MMODPRG_SetValue( path, MMODPRG_BLK_D8, offset, val )

#define MMODPRG_SetD16( path, offset, val )   \
        MMODPRG_SetValue( path, MMODPRG_BLK_D16, offset, val )

#define MMODPRG_SetD32( path, offset, val )   \
        MMODPRG_SetValue( path, MMODPRG_BLK_D32, offset, val )


static inline int
MMODPRG_GetValue( MDIS_PATH path, int code, int offset, u_int32 *value )
{
    MMODPRG_DX_PB   pb;
    M_SG_BLOCK      blk;
    int32           rc;

    pb.offset = offset;
    blk.size = sizeof( pb );
    blk.data = (void*)&pb;

    rc = M_getstat( path, code, (int32*)&blk );

    if( rc == 0 )
        *value = pb.value;

    return( rc );
}

#define MMODPRG_GetD8( path, offset, val )   \
        MMODPRG_GetValue( path, MMODPRG_BLK_D8, offset, val )

#define MMODPRG_GetD16( path, offset, val )   \
        MMODPRG_GetValue( path, MMODPRG_BLK_D16, offset, val )

#define MMODPRG_GetD32( path, offset, val )   \
        MMODPRG_GetValue( path, MMODPRG_BLK_D32, offset, val )

/*--- macros to make unique names for global symbols ---*/
#ifndef  MMODPRG_VARIANT
# define MMODPRG_VARIANT MMODPRG
#endif

#define _MMODPRG_GLOBNAME(var,name) var##_##name
#ifndef _ONE_NAMESPACE_PER_DRIVER_
# define MMODPRG_GLOBNAME(var,name) _MMODPRG_GLOBNAME(var,name)
#else
# define MMODPRG_GLOBNAME(var,name) _MMODPRG_GLOBNAME(LL,name)
#endif

#define __MMODPRG_GetEntry			MMODPRG_GLOBNAME(MMODPRG_VARIANT,GetEntry)

/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
#ifdef _LL_DRV_
# ifndef _ONE_NAMESPACE_PER_DRIVER_
	extern void MMODPRG_GetEntry(LL_ENTRY* drvP);
# endif
#endif /* _LL_DRV_ */

/*-----------------------------------------+
|  BACKWARD COMPATIBILITY TO MDIS4         |
+-----------------------------------------*/
#ifndef U_INT32_OR_64
    /* we have an MDIS4 men_types.h and mdis_api.h included */
    /* only 32bit compatibility needed!                     */
    #define INT32_OR_64  int32
        #define U_INT32_OR_64 u_int32
    typedef INT32_OR_64  MDIS_PATH;
#endif /* U_INT32_OR_64 */

#ifdef __cplusplus
      }
#endif

#endif /* _MMODPRG_DRV_H */

