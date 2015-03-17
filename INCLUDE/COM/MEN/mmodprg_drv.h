/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: mmodprg_drv.h
 *
 *       Author: kp
 *        $Date: 2010/04/15 13:31:07 $
 *    $Revision: 2.6 $
 *
 *  Description: Header file for MMODPRG driver
 *               - MMODPRG specific status codes
 *               - MMODPRG function prototypes
 *
 *     Switches: _ONE_NAMESPACE_PER_DRIVER_
 *               _LL_DRV_
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: mmodprg_drv.h,v $
 * Revision 2.6  2010/04/15 13:31:07  amorbach
 * R: driver ported to MDIS5, new MDIS_API and men_typs
 * M: for backward compatibility to MDIS4 optionally define new types
 *
 * Revision 2.5  2006/03/01 18:59:01  cs
 * added support for various driver variants (different addr space sizes)
 *
 * Revision 2.4  2005/11/17 16:03:36  ub
 * fixed: GNU-C compiler not recognized
 *
 * Revision 2.3  2005/08/23 14:45:00  ub
 * inline declaration disabled for non-GCC compilers
 *
 * Revision 2.2  2004/09/09 14:18:36  ub
 * - added get/setstat codes for hardware access
 * - added some macros for easier use of get/setstats
 *
 * Revision 2.1  2002/06/06 15:35:00  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2010 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

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

    return( M_setstat( path, code, (int32)&blk ) );
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

