/****************************************************************************
 ************                                                    ************
 ************                     Z24_RAMTEST                    ************
 ************                                                    ************
 ****************************************************************************/
/*!
 *         \file z24_ramtest.c
 *       \author ulrich.bogensperger@men.de/cs
 *        $Date: 2010/04/15 13:30:52 $
 *    $Revision: 1.3 $
 *
 *        \brief Test program for the Z24 SRAM controller chameleon FPGA
 *
 *     Required: libraries: mdis_api, usr_oss, usr_utl
 *               drivers:   mmodprg
 *     \switches see usage()
 */
 /*-------------------------------[ History ]--------------------------------
 *
 * $Log: z24_ramtest.c,v $
 * Revision 1.3  2010/04/15 13:30:52  amorbach
 * R:1. Porting to MDIS5
 * 2. warning: *** sscanf() not allowed in portable MDIS examples/tools
 * 3. No error message if end addr < start addr
 * 4. Warnings: signed/unsigned
 * M:1. changed according to MDIS Porting Guide 0.8
 * 2. Use strtol() instaed of sscanf()
 * 3. Plausibility check added for end addr and start addr
 * 4. Variables type changed to u_int32
 *
 * Revision 1.2  2006/03/01 12:12:09  cs
 * adapted for Z24_SRAM
 *
 * Revision 1.1  2006/02/24 17:27:19  cs
 * Initial Revision
 *
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2010 by MEN Mikro Elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>
#include <MEN/mdis_api.h>
#include <MEN/mmodprg_drv.h>

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define REV         "1.0"               /* program revision */


/* some error handling macros */

#define FAIL_UNLESS(expression) \
 if( !(expression)) {\
     printf("\n*** Error during: %s\nfile %s\nline %d\n", \
      #expression,__FILE__,__LINE__);\
      printf("%s\n",M_errstring(UOS_ErrnoGet()));\
     goto ABORT;\
 }

#define FAIL_UNLESS_(expression) \
 if( !(expression)) {\
     printf("\n*** Error during: %s\nfile %s\nline %d\n", \
      #expression,__FILE__,__LINE__);\
     goto ABORT;\
 }


/*
 * hardware definitions
 */

/* misc */
#define SRAM_MAX         0x800          /* 2 kByte */

/* access macros */
#define SRAM_SET_D8( offs, val ) \
        FAIL_UNLESS( MMODPRG_SetD8( d->path, offs, val ) == 0 )

#define SRAM_SET_D16( offs, val ) \
        FAIL_UNLESS( MMODPRG_SetD16( d->path, offs, val ) == 0 )

#define SRAM_SET_D32( offs, val ) \
        FAIL_UNLESS( MMODPRG_SetD32( d->path, offs, val ) == 0 )


#define SRAM_GET_D8( offs, val ) \
        FAIL_UNLESS( MMODPRG_GetD8( d->path, offs, val ) == 0 )

#define SRAM_GET_D16( offs, val ) \
        FAIL_UNLESS( MMODPRG_GetD16( d->path, offs, val ) == 0 )

#define SRAM_GET_D32( offs, val ) \
        FAIL_UNLESS( MMODPRG_GetD32( d->path, offs, val ) == 0 )


/*--------------------------------------+
|   TYPDEFS                             |
+--------------------------------------*/
typedef struct {
    MDIS_PATH path;
    char *name;
} DEVICE;

/* test list description */
typedef struct {
    char code;
    char *descr;
    int (*func)( DEVICE *, u_int32 startAddr, u_int32 endAddr );
} TEST_ELEM;


/*--------------------------------------+
|   EXTERNALS                           |
+--------------------------------------*/
/* none */

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static void    printmsg( int level, char *fmt, ... );
static void    waitKey( char *msg, char key );

static int     Init( DEVICE *d );
static int     Deinit( DEVICE *d );
static int     dumpSram( DEVICE *d, u_int32 adr, int numBytes );



/* test functions */
static int     TestA( DEVICE *d, u_int32 startAddr, u_int32 endAddr );
static int     TestB( DEVICE *d, u_int32 startAddr, u_int32 endAddr );
static int     TestC( DEVICE *d, u_int32 startAddr, u_int32 endAddr );
static int     TestD( DEVICE *d, u_int32 startAddr, u_int32 endAddr );

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static TEST_ELEM G_testList[] = {
    { 'a', "Simple read/write",                        TestA },
    { 'b', "Autoincrement",                            TestB },
    { 'c', "Linear read/write",                        TestC },
    { 'd', "Random read/write",                        TestD },
    { 0, NULL, NULL }
};

static int G_verbose = 0;

/********************************* usage ************************************
 *
 *  Description: Print program usage
 *
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void usage(void)
{
    TEST_ELEM *te=G_testList;

    printf("Usage: mmodprog_ramtest [<opts>] <device> [<opts>]\n");
    printf("Function: Verification program for SRAM controller\n");
    printf("Options:\n");
	printf("  -b=<offs>    start addr (relative to base addr).... [0]\n");
	printf("  -e=<offs>    end addr (relative to base addr (+1)). [0x800]\n");
    printf("  -v=<num>     verbosity level (0-2)................. [0]\n");
    printf("  -n           number of runs for each test.......... [1]\n");
    printf("  -s           stop on first error .................. [no]\n");
    printf("  -t=<list>    perform onlys those tests listed:..... [all]\n");

    while( te->func ){
        printf("    %c: %s\n", te->code, te->descr );
        te++;
    }

    printf("\n");
    printf("(c) 2006 by MEN Mikro Elektronik GmbH. Revision %s\n\n", REV);
}

/********************************* main *************************************
 *
 *  Description: Program main function
 *
 *---------------------------------------------------------------------------
 *  Input......: argc,argv  argument counter, data ..
 *  Output.....: return     success (0) or error (1)
 *  Globals....: -
 ****************************************************************************/
int main(int argc, char *argv[])
{
    int32   n;
    DEVICE  d;
    char    buf[80];
    char    *str, *errstr, *testlist;
    char    *tCode;
    int     errCount=1, err, stopOnFirst, runs, wait=0;
    u_int32 startAddr = 0, endAddr = SRAM_MAX;

    TEST_ELEM *te;

    /*--------------------+
    |  check arguments    |
    +--------------------*/
    if ((errstr = UTL_ILLIOPT("b=e=v=n=t=s?", buf))) {   /* check args */
        printf("*** %s\n", errstr);
        return(1);
    }

    if (UTL_TSTOPT("?")) {                      /* help requested ? */
        usage();
        return(1);
    }

    /*--------------------+
    |  get arguments      |
    +--------------------*/
    d.name = NULL;

    for (n=1; n<argc; n++){
        if (*argv[n] != '-') {
            d.name = argv[n];
        }
    }

    if (d.name == NULL) {
        usage();
        return(1);
    }

    G_verbose     = ((str = UTL_TSTOPT("v=")) ? atoi(str) : 0);
    testlist      = ((str = UTL_TSTOPT("t=")) ? str : "abcd"/*efghijklmnopqrstuvxyz"*/);
    stopOnFirst   = !!UTL_TSTOPT("s");
    runs          = ((str = UTL_TSTOPT("n=")) ? atoi(str) : 1);

	if( (str = UTL_TSTOPT("b=")) )
		startAddr = strtol(str, NULL, 16);

	startAddr &= 0xfffffffc;     /* addresses have to be multiple of 4 */

	if( (str = UTL_TSTOPT("e=")) )
		endAddr = strtol(str, NULL, 16);
	if( 0 == endAddr )
		endAddr = SRAM_MAX;

	if( endAddr > SRAM_MAX )
		endAddr = SRAM_MAX;

	FAIL_UNLESS(endAddr > startAddr);

    /*--------------------+
    |  open device        |
    +--------------------*/
    d.path = -1;
    FAIL_UNLESS((d.path = M_open(d.name)) >= 0);


    /*-----------------+
    |  init device     |
    +-----------------*/

    FAIL_UNLESS( Init( &d ) == 0 );
    errCount = 0;

    /*-----------------+
    |  perform tests   |
    +-----------------*/
    for( tCode=testlist; *tCode; tCode++ ){

        for( te=G_testList; te->func; te++ )
            if( *tCode == te->code )
                break;

        if( te->func == NULL ){
            printf("Unknown test: %c\n", *tCode );
            return( 1 );
        }

        for( n=0; n<runs; ++n ) {

            if( wait ) {
                printf( "Hit <Return> for test %c.\n", te->code );
                getchar();
            }

            printf("=== Performing test %c: %-43s ", te->code, te->descr );
            printmsg( 1, "===\n");
            fflush(stdout);

            err = te->func( &d, startAddr, endAddr );
            errCount += err;

            printmsg( 1, "Test %c: ", te->code);
            printf( "%s\n", err ? "FAILED" : "ok" );

            if( err && stopOnFirst )
                goto ABORT;
        }
    }

 ABORT:
    printf("------------------------------------------------\n");
    printf("TEST RESULT: %d errors\n", errCount );

    if( wait ) {
        waitKey( "Enter 'x' to finish program.\n", 'x');
    }

    if( d.path != -1 ) {
        Deinit( &d );
        M_close( d.path );
    }

    return errCount ? 1 : 0 ;
}

/***************************************************************************/
static void printmsg( int level, char *fmt, ... )
{
    va_list ap;

    if( level <= G_verbose ){
        va_start(ap,fmt);
        vprintf( fmt, ap );
        va_end(ap);
    }
}


/***************************************************************************/
static void
waitKey( char *msg, char key )
{
    do {
        printf( "%s\n", msg );
    } while( getchar() != key );
}

/***************************************************************************/
static int
dumpSram( DEVICE *d, u_int32 adr, int numBytes )
{
    int i;
    u_int32 val;

    printmsg( 2, "--- SRAM at Address 0x%x ---", adr );

    for( i=0; i<numBytes/4; ++i ) {
        if( (i & 3) == 0 )
            printmsg(2, "\n0x%04x: ", adr+i*4 );

        if( (i & 1) == 0 )
            printmsg(2, " " );

        SRAM_GET_D32( adr+(i<<2), &val );
#ifdef _BIG_ENDIAN_
        printmsg( 2, "%02x  %02x  %02x  %02x  ",
                  val&0xff, (val>>8) & 0xff,
                  (val>>16)&0xff, (val>>24) & 0xff );
#else
        printmsg( 2, "%02x  %02x  %02x  %02x  ",
                  (val>>24) & 0xff, (val>>16)&0xff,
                  (val>>8) & 0xff, val&0xff );
#endif
    }

    printmsg( 2, "\n" );

    return( 0 );
 ABORT:
    return( 1 );
}

/*--------------------------------------------------------------------------*/
/* init device for test */
/*--------------------------------------------------------------------------*/
static int
Init( DEVICE *d )
{
    return( 0 );
}

/*--------------------------------------------------------------------------*/
static int
Deinit( DEVICE *d )
{
    return( 0 );
}


static u_int32 G_testval_d32[] = { 0x5f5f5f5f, 0xa0a0a0a0 };

static int
TestA( DEVICE *d, u_int32 startAddr, u_int32 endAddr )
{
    u_int32 i;
    u_int32 val;


    /* test simple access */
    printmsg( 1, "32 bit access...\n" );

    for( i=startAddr; i<startAddr+32; i++ ) {
        SRAM_SET_D32( i<<2, 1<<i );
        SRAM_GET_D32( i<<2, &val );
        FAIL_UNLESS_( val != (u_int32)(1<<i) );
    }

    printmsg( 1, "16 bit access...\n" );

    for( i=startAddr; i<startAddr+32; i++ ) {
        SRAM_SET_D16( i<<1, 1<<i );
        SRAM_GET_D16( i<<1, &val );
        FAIL_UNLESS_( val != (u_int32)(1<<i) );
    }

    printmsg( 1, "8 bit access...\n" );

    for( i=startAddr; i<startAddr+32; i++ ) {
        SRAM_SET_D8( i, 1<<i );
        SRAM_GET_D8( i, &val );
        FAIL_UNLESS_( val != (u_int32)(1<<i) );
    }

    SRAM_SET_D32( startAddr, 0x12345678 );
    SRAM_GET_D8( startAddr, &val );

    printmsg( 3, "8bit: val=0x%x\n", val );

#ifdef _LITTLE_ENDIAN_
    FAIL_UNLESS_( (val & 0xff) == 0x78 );
#else
    FAIL_UNLESS_( (val & 0xff) == 0x12 );
#endif

    SRAM_GET_D16( startAddr, &val );
    printmsg( 3, "16bit: val=0x%x\n", val );

#ifdef _LITTLE_ENDIAN_
    FAIL_UNLESS_( (val & 0xffff) == 0x5678 );
#else
    FAIL_UNLESS_( (val & 0xffff) == 0x1234 );
#endif

    SRAM_GET_D32( startAddr, &val );
    printmsg( 3, "32bit: val=0x%x\n", val );
    FAIL_UNLESS_( val == 0x12345678 );

    return( 0 );
 ABORT:
    return( 1 );
}


static int
TestB( DEVICE *d, u_int32 startAddr, u_int32 endAddr )
{
    u_int32 adr;
    u_int32 val, failed = 0;

    /*--- fill RAM with test pattern ---*/
    printmsg( 2, "setting test values...\n" );

    for( adr=startAddr; adr<endAddr; adr+=4 ) {
        SRAM_SET_D32( adr, adr );
    }

    /*--- verify test pattern ---*/
    printmsg( 2, "verifying ...\n" );

    for( adr=startAddr; adr<endAddr; adr+=4 ) {
        SRAM_GET_D32( adr, &val );
        if( val != adr ) {
            printf( "Failure: address=0x%x  val=0x%x  sb=0x%x\n",
                    adr, val, adr );
            failed++;
            FAIL_UNLESS_( val != adr );
        }
    }

    dumpSram( d, startAddr, 32 );

    return( failed );
 ABORT:
    return( 1 );
}


static int
TestC( DEVICE *d, u_int32 startAddr, u_int32 endAddr )
{
    u_int32 i, adr;
    u_int32 val, failed = 0;

    printmsg( 1, "writing/checking test pattern...\n" );

    /*--- for all RAM cells ---*/
    for( adr = startAddr; adr < endAddr; adr+=4 ) {
         if( (adr & 0x3ff) == 0 ) {
            printmsg( 1, "." );
            fflush( stdout );
        }

        /*--- for all test patterns ---*/
        for( i=0; i<2; ++i ) {
            SRAM_SET_D32( adr, G_testval_d32[i] );
            SRAM_GET_D32( adr, &val );
            if( val != G_testval_d32[i] ) {
                printmsg( 3, "Adr 0x%x: got value 0x%x (should be 0x%x)\n",
                          adr, val, G_testval_d32[i] );
            	failed++;
                FAIL_UNLESS_( 0 );
            }
        }
    }

    dumpSram( d, startAddr, 32 );

    return( failed );
 ABORT:
    return( 1 );
}

static int
TestD( DEVICE *d, u_int32 startAddr, u_int32 endAddr )
{
    u_int32 i, adr;
    u_int32 val, rndVal, failed = 0;

    printmsg( 1, "writing/checking random test pattern...\n" );

    rndVal = UOS_Random( 0 );

    /*--- check 100000 random accesses ---*/
    for( i = 0; i < 100000; ++i ) {
        /* address register ignores lowest two bits */
        adr =  rndVal & 0x7fffc;

        if( (adr < startAddr) || (adr >= (endAddr - 3)) ) {
        	i--;
        	continue;
        }

        SRAM_SET_D32( adr, rndVal );
        SRAM_GET_D32( adr, &val );

        if( val != rndVal ) {
            printmsg( 1, "Adr 0x%x: got value 0x%x (should be 0x%x)\n",
                      adr, val, rndVal );
            failed++;
            FAIL_UNLESS_( 0 );
        }

        rndVal = UOS_Random( rndVal );
    }

    dumpSram( d, startAddr, 32 );

    return( failed );
 ABORT:
    return( 1 );
}


#if 0
/* template */
static int
TestX( DEVICE *d, u_int32 startAddr, u_int32 endAddr )
{
    return( 0 );
 ABORT:
    return( 1 );
}
#endif


