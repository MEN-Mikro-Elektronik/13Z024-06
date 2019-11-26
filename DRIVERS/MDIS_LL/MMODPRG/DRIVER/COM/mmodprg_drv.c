/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: mmodprg_drv.c
 *      Project: M-module driver (MDIS5)
 *
 *       Author: kp
 *
 *  Description: Description: Low-level driver to perform 8/16/32-bit read/write
 *               access to various HW devices (e.g. M-Modules, 16Z024_SRAM SRAM
 *               Controller)
 *
 *     Required: OSS, DESC, DBG, ID libraries
 *     Switches: _ONE_NAMESPACE_PER_DRIVER_, MAC_BYTESWAP,
 *               MMODPROG_ADDRSPACE_SIZE
 *
 *---------------------------------------------------------------------------
 * Copyright 2010-2019, MEN Mikro Elektronik GmbH
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


#include <MEN/men_typs.h>   /* system dependent definitions   */
#include <MEN/maccess.h>    /* hw access macros and types     */
#include <MEN/dbg.h>        /* debug functions                */
#include <MEN/oss.h>        /* oss functions                  */
#include <MEN/desc.h>       /* descriptor functions           */
#include <MEN/modcom.h>     /* ID PROM functions              */
#include <MEN/mdis_api.h>   /* MDIS global defs               */
#include <MEN/mdis_com.h>   /* MDIS common defs               */
#include <MEN/mdis_err.h>   /* MDIS error codes               */
#include <MEN/ll_defs.h>    /* low-level driver definitions   */
#include <MEN/ll_entry.h>   /* low-level driver jump table  */
#include <MEN/mmodprg_drv.h>   /* MMODPRG driver header file */

static const char IdentString[]=MENT_XSTR(MAK_REVISION);

/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/
/* general */
#define CH_NUMBER			1			/* number of device channels */
#define USE_IRQ				FALSE		/* interrupt required  */
#define ADDRSPACE_COUNT		1			/* nr of required address spaces */
#define MOD_ID_SIZE			128			/* ID PROM size [bytes] */

/* debug settings */
#define DBG_MYLEVEL			h->dbgLevel
#define DBH					h->dbgHdl

/* register offsets */
/* ... */

/*-----------------------------------------+
|  TYPEDEFS                                |
+-----------------------------------------*/
/* low-level handle */
typedef struct {
	/* general */
    int32           memAlloc;		/* size allocated for the handle */
    OSS_HANDLE      *osHdl;         /* oss handle */
    OSS_IRQ_HANDLE  *irqHdl;        /* irq handle */
    DESC_HANDLE     *descHdl;       /* desc handle */
    MACCESS         ma;             /* hw access handle */
	MDIS_IDENT_FUNCT_TBL idFuncTbl;	/* id function table */
	/* debug */
    u_int32         dbgLevel;		/* debug level */
	DBG_HANDLE      *dbgHdl;        /* debug handle */
	/* misc */
    u_int32         irqCount;       /* interrupt counter */
    u_int32         idCheck;		/* id check enabled */
} MMODPRG_HANDLE;

/* include files which need LL_HANDLE */

/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
static int32 MMODPRG_Init(DESC_SPEC *descSpec, OSS_HANDLE *osHdl,
					   MACCESS *ma, OSS_SEM_HANDLE *devSemHdl,
					   OSS_IRQ_HANDLE *irqHdl, LL_HANDLE **llHdlP);
static int32 MMODPRG_Exit(LL_HANDLE **llHdlP );
static int32 MMODPRG_Read(LL_HANDLE *llHdl, int32 ch, int32 *value);
static int32 MMODPRG_Write(LL_HANDLE *llHdl, int32 ch, int32 value);
static int32 MMODPRG_SetStat(LL_HANDLE *llHdl,int32 ch, int32 code, INT32_OR_64 value32_or_64);
static int32 MMODPRG_GetStat(LL_HANDLE *llHdl, int32 ch, int32 code, INT32_OR_64 *value32_or_64P);
static int32 MMODPRG_BlockRead(LL_HANDLE *llHdl, int32 ch, void *buf, int32 size,
							int32 *nbrRdBytesP);
static int32 MMODPRG_BlockWrite(LL_HANDLE *llHdl, int32 ch, void *buf, int32 size,
							 int32 *nbrWrBytesP);
static int32 MMODPRG_Irq(LL_HANDLE *llHdl );
static int32 MMODPRG_Info(int32 infoType, ... );

static char* Ident( void );
static int32 Cleanup(MMODPRG_HANDLE *llHdl, int32 retCode);

/**************************** MMODPRG_GetEntry *********************************
 *
 *  Description:  Initialize driver's jump table
 *
 *---------------------------------------------------------------------------
 *  Input......:  ---
 *  Output.....:  drvP  pointer to the initialized jump table structure
 *  Globals....:  ---
 ****************************************************************************/
extern void __MMODPRG_GetEntry( LL_ENTRY* drvP )
{
    drvP->init        = MMODPRG_Init;
    drvP->exit        = MMODPRG_Exit;
    drvP->read        = MMODPRG_Read;
    drvP->write       = MMODPRG_Write;
    drvP->blockRead   = MMODPRG_BlockRead;
    drvP->blockWrite  = MMODPRG_BlockWrite;
    drvP->setStat     = MMODPRG_SetStat;
    drvP->getStat     = MMODPRG_GetStat;
    drvP->irq         = MMODPRG_Irq;
    drvP->info        = MMODPRG_Info;
}

/******************************** MMODPRG_Init *******************************
 *
 *  Description:  Allocate and return low-level handle, initialize hardware
 *
 *                The function initializes all channels with the
 *                definitions made in the descriptor. The interrupt
 *                is disabled.
 *
 *                The following descriptor keys are used:
 *
 *                Descriptor key        Default          Range
 *                --------------------  ---------------  -------------
 *                DEBUG_LEVEL_DESC      OSS_DBG_DEFAULT  see dbg.h
 *                DEBUG_LEVEL           OSS_DBG_DEFAULT  see dbg.h
 *                ID_CHECK              1                0..1
 *
 *---------------------------------------------------------------------------
 *  Input......:  descSpec   pointer to descriptor data
 *                osHdl      oss handle
 *                ma         hw access handle
 *                devSemHdl  device semaphore handle
 *                irqHdl     irq handle
 *  Output.....:  llHdlP     pointer to low-level driver handle
 *                return     success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 MMODPRG_Init(
    DESC_SPEC       *descP,
    OSS_HANDLE      *osHdl,
    MACCESS         *ma,
    OSS_SEM_HANDLE  *devSemHdl,
    OSS_IRQ_HANDLE  *irqHdl,
    LL_HANDLE       **_hP
)
{
    MMODPRG_HANDLE *h = NULL;
    u_int32 gotsize;
    int32 error;
    u_int32 value;

    /*------------------------------+
    |  prepare the handle           |
    +------------------------------*/
	*_hP = NULL;		/* set low-level driver handle to NULL */

	/* alloc */
    if ((h = (MMODPRG_HANDLE*)OSS_MemGet(
    				osHdl, sizeof(MMODPRG_HANDLE), &gotsize)) == NULL)
       return(ERR_OSS_MEM_ALLOC);

	/* clear */
    OSS_MemFill(osHdl, gotsize, (char*)h, 0x00);

	/* init */
    h->memAlloc   = gotsize;
    h->osHdl      = osHdl;
    h->irqHdl     = irqHdl;
    h->ma		  = *ma;

    /*------------------------------+
    |  init id function table       |
    +------------------------------*/
	/* driver's ident function */
	h->idFuncTbl.idCall[0].identCall = Ident;
	/* library's ident functions */
	h->idFuncTbl.idCall[1].identCall = DESC_Ident;
	h->idFuncTbl.idCall[2].identCall = OSS_Ident;
	/* terminator */
	h->idFuncTbl.idCall[3].identCall = NULL;

    /*------------------------------+
    |  prepare debugging            |
    +------------------------------*/
	DBG_MYLEVEL = OSS_DBG_DEFAULT;	/* set OS specific debug level */
	DBGINIT((NULL,&DBH));

    /*------------------------------+
    |  scan descriptor              |
    +------------------------------*/
	/* prepare access */
    if ((error = DESC_Init(descP, osHdl, &h->descHdl)))
		return( Cleanup(h,error) );

    /* DEBUG_LEVEL_DESC */
    if ((error = DESC_GetUInt32(h->descHdl, OSS_DBG_DEFAULT,
								&value, "DEBUG_LEVEL_DESC")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(h,error) );

	DESC_DbgLevelSet(h->descHdl, value);	/* set level */

    /* DEBUG_LEVEL */
    if ((error = DESC_GetUInt32(h->descHdl, OSS_DBG_DEFAULT,
								&h->dbgLevel, "DEBUG_LEVEL")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(h,error) );

    DBGWRT_1((DBH, "LL - MMODPRG_Init:  base address=0x%x\n", h->ma));

    /* ID_CHECK */
    if ((error = DESC_GetUInt32(h->descHdl, TRUE,
								&h->idCheck, "ID_CHECK")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(h,error) );

    /*------------------------------+
    |  init hardware                |
    +------------------------------*/
	/* ... */

	*_hP = (LL_HANDLE *)h;	/* set low-level driver handle */

	return(ERR_SUCCESS);
}

/****************************** MMODPRG_Exit *************************************
 *
 *  Description:  De-initialize hardware and clean up memory
 *
 *                The function deinitializes all channels by setting them
 *                to ???. The interrupt is disabled.
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdlP  	pointer to low-level driver handle
 *  Output.....:  return    success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 MMODPRG_Exit(
   LL_HANDLE    **_hP
)
{
    MMODPRG_HANDLE *h = (MMODPRG_HANDLE *)*_hP;
	int32 error = 0;

    DBGWRT_1((DBH, "LL - MMODPRG_Exit\n"));

    /*------------------------------+
    |  de-init hardware             |
    +------------------------------*/
	/* ... */

    /*------------------------------+
    |  clean up memory               |
    +------------------------------*/
	error = Cleanup(h,error);
	*_hP = NULL;		/* set low-level driver handle to NULL */

	return(error);
}

/****************************** MMODPRG_Read *************************************
 *
 *  Description:  Read a value from the device
 *
 *                The function reads the ??? state of the current channel.
 *
 *                If the channel's direction is not configured as input
 *                the function returns an ERR_LL_ILL_DIR error.
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl    low-level handle
 *                ch       current channel
 *  Output.....:  valueP   read value
 *                return   success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 MMODPRG_Read(
    LL_HANDLE *llHdl,
    int32 ch,
    int32 *valueP
)
{
	return ERR_LL_ILL_FUNC;
}

/****************************** MMODPRG_Write ********************************
 *
 *  Description:  Write a value to the device
 *
 *                The function writes a value to the current channel.
 *
 *                If the channel's direction is not configured as output
 *                the function returns an ERR_LL_ILL_DIR error.
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl    low-level handle
 *                ch       current channel
 *                value    value to write
 *  Output.....:  return   success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 MMODPRG_Write(
    LL_HANDLE *llHdl,
    int32 ch,
    int32 value
)
{
	return ERR_LL_ILL_FUNC;
}

/****************************** MMODPRG_SetStat *******************************
 *
 *  Description:  Set the driver status
 *
 *                The following status codes are supported:
 *
 *                Code                 Description                 Values
 *                -------------------  --------------------------  ----------
 *                M_LL_BLK_ID_DATA     program IDPROM data         -
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl             low-level handle
 *                code              status code
 *                ch                current channel
 *                value32_or_64     data or
 *                           pointer to block data structure (M_SG_BLOCK)  (*)
 *                (*) = for block status codes
 *  Output.....:  return     success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 MMODPRG_SetStat(
    LL_HANDLE *llHdl,
    int32  code,
    int32  ch,
    INT32_OR_64 value32_or_64
)
{
    int32 value = (int32)value32_or_64;	    /* 32bit value */
    INT32_OR_64 valueP = value32_or_64;     /* stores 32/64bit pointer */
	int32 error = ERR_SUCCESS;
	M_SG_BLOCK *blk = (M_SG_BLOCK*)valueP;
	MMODPRG_HANDLE *h = (MMODPRG_HANDLE *)llHdl;
    MACCESS ma = h->ma;

    DBGWRT_1((DBH, "LL - MMODPRG_SetStat: ch=%d code=0x%04x value=0x%x\n",
			  ch,code,value));

    switch(code) {
        /*--------------------------+
        |  program M-module ID		|
        +--------------------------*/
	    case M_LL_BLK_ID_DATA:
		{
			int32 n;
			u_int16 *dataP = (u_int16*)blk->data;

			for (n=0; n<blk->size/2; n++){		/* write MOD_ID_SIZE/2 words */
				if( m_write((u_int8 *)h->ma, (u_int8)n, *dataP++)){
					error = ERR_LL_WRITE;
					break;
				}
			}

			break;
		}

        /*--------------------------+
        |  write 8 bit value        |
        +--------------------------*/
        case MMODPRG_BLK_D8:
        {
            MMODPRG_DX_PB *pb = (MMODPRG_DX_PB*)blk->data;
            DBGWRT_3((DBH, "write 8 bit value 0x%x to offset 0x%x\n",
                      pb->value, pb->offset ));

            MWRITE_D8( ma, pb->offset, pb->value );
            break;
        }

        /*--------------------------+
        |  write 16 bit value       |
        +--------------------------*/
        case MMODPRG_BLK_D16:
        {
            MMODPRG_DX_PB *pb = (MMODPRG_DX_PB*)blk->data;
            DBGWRT_3((DBH, "write 16 bit value 0x%x to offset 0x%x\n",
                      pb->value, pb->offset ));

            MWRITE_D16( ma, pb->offset, pb->value );
            break;
        }

        /*--------------------------+
        |  write 32 bit value       |
        +--------------------------*/
        case MMODPRG_BLK_D32:
        {
            MMODPRG_DX_PB *pb = (MMODPRG_DX_PB*)blk->data;
            DBGWRT_3((DBH, "write 32 bit value 0x%x to offset 0x%x\n",
                      pb->value, pb->offset ));

            MWRITE_D32( ma, pb->offset, pb->value );
            break;
        }

        /*--------------------------+
        |  debug level              |
        +--------------------------*/
        case M_LL_DEBUG_LEVEL:
            h->dbgLevel = value;
            break;
        /*--------------------------+
        |  (unknown)                |
        +--------------------------*/
        default:
            error = ERR_LL_UNK_CODE;
    }

	return(error);
}

/****************************** MMODPRG_GetStat *******************************
 *
 *  Description:  Get the driver status
 *
 *                The following status codes are supported:
 *
 *                Code                 Description                 Values
 *                -------------------  --------------------------  ----------
 *                M_LL_DEBUG_LEVEL     driver debug level          see dbg.h
 *                M_LL_CH_NUMBER       number of channels          ???
 *                M_LL_CH_DIR          direction of curr. chan.    M_CH_???
 *                M_LL_CH_LEN          length of curr. ch. [bits]  1..max
 *                M_LL_CH_TYP          description of curr. chan.  M_CH_???
 *                M_LL_IRQ_COUNT       interrupt counter           0..max
 *                M_LL_ID_CHECK        EEPROM is checked           0..1
 *                M_LL_ID_SIZE         EEPROM size [bytes]         128
 *                M_LL_BLK_ID_DATA     EEPROM raw data             -
 *                M_MK_BLK_REV_ID      ident function table ptr    -
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl             low-level handle
 *                code              status code
 *                ch                current channel
 *                value32_or_64P    pointer to block data structure (M_SG_BLOCK)  (*)
 *                (*) = for block status codes
 *  Output.....:  value32_or_64P    data ptr or
 *                                  pointer to block data structure (M_SG_BLOCK)  (*)
 *                return     success (0) or error code
 *                (*) = for block status codes
 *  Globals....:  ---
 ****************************************************************************/
static int32 MMODPRG_GetStat(
    LL_HANDLE *llHdl,
    int32  code,
    int32  ch,
    INT32_OR_64 *value32_or_64P
)
{
	MMODPRG_HANDLE *h = (MMODPRG_HANDLE *)llHdl;
    MACCESS ma = h->ma;
    int32 *valueP = (int32*)value32_or_64P;	            /* pointer to 32bit value  */
    INT32_OR_64	*value64P = value32_or_64P;		 		/* stores 32/64bit pointer  */
    M_SG_BLOCK *blk = (M_SG_BLOCK*)value32_or_64P; 	    /* stores block struct pointer */

	int32 error = ERR_SUCCESS;

    DBGWRT_1((DBH, "LL - MMODPRG_GetStat: ch=%d code=0x%04x\n",
			  ch,code));

    switch(code)
    {
        /*--------------------------+
        |  debug level              |
        +--------------------------*/
        case M_LL_DEBUG_LEVEL:
            *valueP = h->dbgLevel;
            break;
        /*--------------------------+
        |  number of channels       |
        +--------------------------*/
        case M_LL_CH_NUMBER:
            *valueP = CH_NUMBER;
            break;
        /*--------------------------+
        |  channel direction        |
        +--------------------------*/
        case M_LL_CH_DIR:
            *valueP = M_CH_INOUT;
            break;
        /*--------------------------+
        |  channel length [bits]    |
        +--------------------------*/
        case M_LL_CH_LEN:
            *valueP = 32;
            break;
        /*--------------------------+
        |  channel type info        |
        +--------------------------*/
        case M_LL_CH_TYP:
            *valueP = M_CH_BINARY;
            break;
        /*--------------------------+
        |  irq counter              |
        +--------------------------*/
        case M_LL_IRQ_COUNT:
            *valueP = h->irqCount;
            break;
        /*--------------------------+
        |  ID PROM check enabled    |
        +--------------------------*/
        case M_LL_ID_CHECK:
            *valueP = h->idCheck;
            break;
        /*--------------------------+
        |   ID PROM size            |
        +--------------------------*/
        case M_LL_ID_SIZE:
            *valueP = MOD_ID_SIZE;
            break;
        /*--------------------------+
        |   ID PROM data            |
        +--------------------------*/
        case M_LL_BLK_ID_DATA:
		{
			int32 n;
			u_int16 *dataP = (u_int16*)blk->data;

			if (blk->size < MOD_ID_SIZE)		/* check buf size */
				return(ERR_LL_USERBUF);

			for (n=0; n<MOD_ID_SIZE/2; n++)		/* read MOD_ID_SIZE/2 words */
				*dataP++ = (u_int16)m_read((U_INT32_OR_64)h->ma, (u_int8)n);

			break;
		}
        /*--------------------------+
        |   ident table pointer     |
        |   (treat as non-block!)   |
        +--------------------------*/
        case M_MK_BLK_REV_ID:
           *value64P = (INT32_OR_64)&h->idFuncTbl;
           break;

        /*--------------------------+
        |  read 8 bit value         |
        +--------------------------*/
        case MMODPRG_BLK_D8:
        {
            MMODPRG_DX_PB *pb = (MMODPRG_DX_PB*)blk->data;

            pb->value = MREAD_D8( ma, pb->offset );
            DBGWRT_3((DBH, "8 bit value 0x%x read from offset 0x%x\n",
                      pb->value, pb->offset ));
            break;
        }

        /*--------------------------+
        |  read 16 bit value        |
        +--------------------------*/
        case MMODPRG_BLK_D16:
        {
            MMODPRG_DX_PB *pb = (MMODPRG_DX_PB*)blk->data;

            pb->value = MREAD_D16( ma, pb->offset );
            DBGWRT_3((DBH, "16 bit value 0x%x read from offset 0x%x\n",
                      pb->value, pb->offset ));
            break;
        }

        /*--------------------------+
        |  read 32 bit value        |
        +--------------------------*/
        case MMODPRG_BLK_D32:
        {
            MMODPRG_DX_PB *pb = (MMODPRG_DX_PB*)blk->data;

            pb->value = MREAD_D32( ma, pb->offset );
            DBGWRT_3((DBH, "32 bit value 0x%x read from offset 0x%x\n",
                      pb->value, pb->offset ));
            break;
        }

        /*--------------------------+
        |  (unknown)                |
        +--------------------------*/
        default:
            error = ERR_LL_UNK_CODE;
    }

	return(error);
}

/******************************* MMODPRG_BlockRead ****************************
 *
 *  Description:  Read a data block from the device
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl        low-level handle
 *                ch           current channel
 *                buf          data buffer
 *                size         data buffer size
 *  Output.....:  nbrRdBytesP  number of read bytes
 *                return       success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 MMODPRG_BlockRead(
     LL_HANDLE *llHdl,
     int32     ch,
     void      *buf,
     int32     size,
     int32     *nbrRdBytesP
)
{
	return ERR_LL_ILL_FUNC;
}

/****************************** MMODPRG_BlockWrite *******************************
 *
 *  Description:  Write a data block to the device
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl        low-level handle
 *                ch           current channel
 *                buf          data buffer
 *                size         data buffer size
 *  Output.....:  nbrWrBytesP  number of written bytes
 *                return       success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 MMODPRG_BlockWrite(
     LL_HANDLE *llHdl,
     int32     ch,
     void      *buf,
     int32     size,
     int32     *nbrWrBytesP
)
{
	return ERR_LL_ILL_FUNC;
}


/****************************** MMODPRG_Irq *************************************
 *
 *  Description:  Interrupt service routine
 *
 *                The interrupt is triggered when ??? occurs.
 *
 *                If the driver can detect the interrupt's cause it returns
 *                LL_IRQ_DEVICE or LL_IRQ_DEV_NOT, otherwise LL_IRQ_UNKNOWN.
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl    low-level handle
 *  Output.....:  return   LL_IRQ_DEVICE	irq caused by device
 *                         LL_IRQ_DEV_NOT   irq not caused by device
 *                         LL_IRQ_UNKNOWN   unknown
 *  Globals....:  ---
 ****************************************************************************/
static int32 MMODPRG_Irq(
   LL_HANDLE *llHdl
)
{
	return(LL_IRQ_UNKNOWN);		/* say: unknown */
}

/****************************** MMODPRG_Info ************************************
 *
 *  Description:  Get information about hardware and driver requirements
 *
 *                The following info codes are supported:
 *
 *                Code                      Description
 *                ------------------------  -----------------------------
 *                LL_INFO_HW_CHARACTER      hardware characteristics
 *                LL_INFO_ADDRSPACE_COUNT   nr of required address spaces
 *                LL_INFO_ADDRSPACE         address space information
 *                LL_INFO_IRQ               interrupt required
 *                LL_INFO_LOCKMODE          process lock mode required
 *
 *                The LL_INFO_HW_CHARACTER code returns all address and
 *                data modes (ORed) which are supported by the hardware
 *                (MDIS_MAxx, MDIS_MDxx).
 *
 *                The LL_INFO_ADDRSPACE_COUNT code returns the number
 *                of address spaces used by the driver.
 *
 *                The LL_INFO_ADDRSPACE code returns information about one
 *                specific address space (MDIS_MAxx, MDIS_MDxx). The returned
 *                data mode represents the widest hardware access used by
 *                the driver.
 *
 *                The LL_INFO_IRQ code returns whether the driver supports an
 *                interrupt routine (TRUE or FALSE).
 *
 *                The LL_INFO_LOCKMODE code returns which process locking
 *                mode the driver needs (LL_LOCK_xxx).
 *
 *---------------------------------------------------------------------------
 *  Input......:  infoType	   info code
 *                ...          argument(s)
 *  Output.....:  return       success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 MMODPRG_Info(
   int32  infoType,
   ...
)
{
    int32   error = ERR_SUCCESS;
    va_list argptr;

    va_start(argptr, infoType );

    switch(infoType) {
		/*-------------------------------+
        |  hardware characteristics      |
        |  (all addr/data modes ORed)   |
        +-------------------------------*/
        case LL_INFO_HW_CHARACTER:
		{
			u_int32 *addrModeP = va_arg(argptr, u_int32*);
			u_int32 *dataModeP = va_arg(argptr, u_int32*);

			*addrModeP = MDIS_MA08;
			*dataModeP = MDIS_MD08 | MDIS_MD16;
			break;
	    }
		/*-------------------------------+
        |  nr of required address spaces |
        |  (total spaces used)           |
        +-------------------------------*/
        case LL_INFO_ADDRSPACE_COUNT:
		{
			u_int32 *nbrOfAddrSpaceP = va_arg(argptr, u_int32*);

			*nbrOfAddrSpaceP = ADDRSPACE_COUNT;
			break;
	    }
		/*-------------------------------+
        |  address space type            |
        |  (widest used data mode)       |
        +-------------------------------*/
        case LL_INFO_ADDRSPACE:
		{
			u_int32 addrSpaceIndex = va_arg(argptr, u_int32);
			u_int32 *addrModeP = va_arg(argptr, u_int32*);
			u_int32 *dataModeP = va_arg(argptr, u_int32*);
			u_int32 *addrSizeP = va_arg(argptr, u_int32*);

			if (addrSpaceIndex >= ADDRSPACE_COUNT)
				error = ERR_LL_ILL_PARAM;
			else {
				*addrModeP = MDIS_MA08;
				*dataModeP = MDIS_MD16;
				*addrSizeP = MMODPRG_ADDRSPACE_SIZE;
			}

			break;
	    }
		/*-------------------------------+
        |   interrupt required           |
        +-------------------------------*/
        case LL_INFO_IRQ:
		{
			u_int32 *useIrqP = va_arg(argptr, u_int32*);

			*useIrqP = USE_IRQ;
			break;
	    }
		/*-------------------------------+
        |   process lock mode            |
        +-------------------------------*/
        case LL_INFO_LOCKMODE:
		{
			u_int32 *lockModeP = va_arg(argptr, u_int32*);

			*lockModeP = LL_LOCK_CALL;
			break;
	    }
		/*-------------------------------+
        |   (unknown)                    |
        +-------------------------------*/
        default:
          error = ERR_LL_ILL_PARAM;
    }

    va_end(argptr);
    return(error);
}

/*******************************  Ident  ************************************
 *
 *  Description:  Return ident string
 *
 *---------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  return  pointer to ident string
 *  Globals....:  -
 ****************************************************************************/
static char* Ident( void )	/* nodoc */
{
    return( (char*) IdentString );
}

/********************************* Cleanup **********************************
 *
 *  Description: Close all handles, free memory and return error code
 *		         NOTE: The low-level handle is invalid after this function is
 *                     called.
 *
 *---------------------------------------------------------------------------
 *  Input......: llHdl		low-level handle
 *               retCode    return value
 *  Output.....: return	    retCode
 *  Globals....: -
 ****************************************************************************/
static int32 Cleanup(
   MMODPRG_HANDLE    *h,
   int32        retCode		/* nodoc */
)
{
    /*------------------------------+
    |  close handles                |
    +------------------------------*/
	/* clean up desc */
	if (h->descHdl)
		DESC_Exit(&h->descHdl);

	/* clean up debug */
	DBGEXIT((&DBH));

    /*------------------------------+
    |  free memory                  |
    +------------------------------*/
    /* free my handle */
    OSS_MemFree(h->osHdl, (int8*)h, h->memAlloc);

    /*------------------------------+
    |  return error code            |
    +------------------------------*/
	return(retCode);
}

