#ident	"@(#)siserver:os/utils.c	1.23"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
                        All Rights Reserved
******************************************************************/
/* $XConsortium: utils.c,v 1.89 89/12/13 09:10:53 rws Exp $ */

#ifdef SVR4
#include <sys/types.h>
#include <sys/procset.h>
#include <sys/priocntl.h>
#include <sys/rtpriocntl.h>
#include <ieeefp.h>
#endif /* SVR4 */

#include <stdio.h>
#include "Xos.h"
#include "misc.h"
#include "X.h"
#include "input.h"
#include "opaque.h"
#include <sys/signal.h>

#ifdef SVR4
#include <sys/time.h>
#endif /* SVR4 */

#include <string.h>		/* Needed for Sun River work */
#include <sys/at_ansi.h>	/* Needed for Sun River work */
#include <sys/kd.h>		/* Needed for Sun River work */

/*
 * TEMP: figure out how to differentiate between V4 and ES 
 * default : SVR_ES
 * until then, if you are building on non ES machines comment the next line
 */
#define SVR4_ES	1

#ifdef SVR4_ES
#include <sys/fc.h>
#include <sys/fcpriocntl.h>
#endif


/*
 * New fixed class added : Jan 1992 
 * 0 - 49  : user class
 * 50 - 59 : fixed class
 * 60 - 100 : kernel mode
 * > 101    : real time mode
 *
 * The server by default runs in fixed class ie: 54 (mid range for fixed class)
 * but the user has different options like to run in normal mode, fixed class
 * or real time
 * 
 * The fixedupri of 25 adds this number to default thus giving
 * user a higher priority that all other time share user.
 */
short	fixedupri = 25;

#define TIMESHARE	0
#define FIXED		1
#define REALTIME	2

static short SchedulerClass = FIXED;

extern char display[];
int     xsig = 0;
extern int  AccessEnabled;

extern char *vendor_string;
extern Bool defaultVendorString;

extern long defaultScreenSaverTime;	/* for parsing command line */
extern long defaultScreenSaverInterval;
extern int defaultScreenSaverBlanking;
extern int defaultBackingStore;
extern Bool disableBackingStore;
extern Bool disableSaveUnders;
#ifndef NOLOGOHACK
extern int logoScreenSaver;
#endif
extern int defaultColorVisualClass;
extern long ScreenSaverTime;		/* for forcing reset */
extern Bool permitOldBugs;
extern int monitorResolution;

void ddxUseMsg();

/* -Xa flag to the compiler doesn't like this; since we don't need it here
 * we can delete this
 */
#ifdef DELETE
extern char *sbrk();
#endif


extern char *config_file;
extern char *color_file;
extern char *display_lib;

#ifdef DEBUG
#ifndef SPECIAL_MALLOC
#define MEMBUG
#endif
#endif

#undef MEMBUG		/* LES */

#ifdef MEMBUG
#define MEM_FAIL_SCALE 100000
long Memory_fail = 0;
static pointer minfree = NULL;
static void CheckNode();
#endif

#ifndef MAXRENDER
#define MAXRENDER  3
#endif
short fntopts=0;
char *renderer_options[MAXRENDER]={0,0,0};
char *fontconfig_file = NULL;
char *cmdline_fontpath = NULL;
Bool Must_have_memory = FALSE;

char *dev_tty_from_init = NULL;		/* since we need to parse it anyway */

#ifdef NEED_ALLOCSAVESCREEN
/* AllocSaveScreen and FreeSaveScreen functions are moved to Display Library
 * layer, ie: vga; since these are used only for allocating and freeing mem
 * at sbrk for vt switch.
 * Since the display library (ex: libvga.so) should be totally independent 
 * of the remaining server, these functions have been moved to the display
 * library layer, ex: to libvga.so or libvga.a.   11/20/90 tmk
 */

/* Allocate screen saver memory "on-the-fly" */

char *
AllocSaveScreen (size)
long size;
{
	char *sbrk();
	char *save_screen;

	/* Save old end of data space */
	/* and add in screen size */
	save_screen = sbrk (size);
	if (!save_screen)
	{
		FatalError ("Can not get memory for screen save area");
	}
	return (save_screen);
}

/* Free up memory allocated for screen saver. */
/* This memory is being returned to the os */

void
FreeSaveScreen (save_screen)
char *save_screen;
{
	int brk();

	if (save_screen != (char *)0) {
		if (brk (save_screen) == -1)
		{
			FatalError ("Can not free memory alloc'ed for screen save area");
		}
	}
}

#endif /* NEED_ALLOCSAVESCREEN  */
/* Force connections to close on SIGHUP from init */

AutoResetServer ()
{
    dispatchException |= DE_RESET;
    isItTimeToYield = TRUE;
#ifdef GPROF
    chdir ("/tmp");
    attexit (0);
#endif
    signal (SIGHUP, AutoResetServer);
}

/* Force connections to close and then exit on SIGTERM, SIGINT */

GiveUp()
{
    dispatchException |= DE_TERMINATE;
    isItTimeToYield = TRUE;
}


void
AbortServer()
{
    extern void AbortDDX();

    AbortDDX();
    fflush(stderr);
    resetmodes();
    abort();
}

void
Error(str)
    char *str;
{
    perror(str);
}
/* defined in ddxi/i386 
long
GetTimeInMillis()
{
    struct timeval  tp;

    gettimeofday(&tp);
    return(tp.tv_sec * 1000) + (tp.tv_usec / 1000);
}
*/

void
UseMsg()
{
static Bool been_here = FALSE;

    if (been_here)
        return;

    been_here = TRUE;

#if !defined(AIXrt) && !defined(AIX386)
    ErrorF("use: X [:<display>] [option]\n");
#ifdef NOT_US
    ErrorF("-a #                   mouse acceleration (pixels)\n");
#endif
#ifdef MEMBUG
    ErrorF("-alloc int             chance alloc should fail\n");
#endif
    ErrorF("-auth string           select authorization file\n");	
    ErrorF("bc                     enable bug compatibility\n");
    ErrorF("-bs                    disable any backing store support\n");
#ifdef NOT_US
    ErrorF("-c                     turns off key-click\n");
    ErrorF("c #                    key-click volume (0-100)\n");
    ErrorF("-cc int                default color visual class\n");
#endif
    ErrorF("-renderer string       font renderer\n");
    ErrorF("-fontconfig string           font configuration file\n");
    ErrorF("-config string         configuration file\n");
    ErrorF("-cmap string           colormap file\n");
    ErrorF("-co string             color database file\n");
#ifdef NOT_US
    ErrorF("-dpi int               screen resolution in dots per inch\n");
#endif
    ErrorF("-fc string             cursor font\n");
    ErrorF("-fn string             default font name\n");
    ErrorF("-fp string             default font path\n");
#ifndef NOLOGOHACK
    ErrorF("-logo                  enable logo in screen saver\n");
    ErrorF("nologo                 disable logo in screen saver\n");
#endif
    ErrorF("-xnetaccess on|off     turn network access on/off\n");
    ErrorF("-p #                   screen-saver pattern duration (seconds)\n");
#ifdef NOT_US
    ErrorF("-r                     turns off auto-repeat\n");
    ErrorF("r                      turns on auto-repeat \n");
    ErrorF("-f #                   bell base (0-100)\n");
    ErrorF("-x string              loads named extension at init time \n");
#endif
    ErrorF("-help                  read $DEST/README file for more info\n");
    ErrorF("-s #                   screen-saver timeout (minutes)\n");
    ErrorF("-su                    disable any save under support\n");
#ifdef NOT_US
    ErrorF("-t #                   mouse threshold (pixels)\n");
#endif
#ifdef USE_TIMEOUT
    ErrorF("-to #                  connection time out\n");
#endif
    ErrorF("v                      video blanking for screen-saver\n");
    ErrorF("-v                     screen-saver without video blanking\n");
    ErrorF("-I                     ignore all remaining arguments\n");
#ifdef NOT_US
    ErrorF("ttyxx                  server started from init on /dev/ttyxx\n");
#endif
    ErrorF("-wm                    WhenMapped default backing-store\n");
    ErrorF("-class <fixed|timeshare|realtime>    run server in fixed or timeshare or realtime mode.\n");

#ifdef XDMCP
    XdmcpUseMsg();
#endif
#endif /* !AIXrt && ! AIX386 */
    ddxUseMsg();
}

void
UseMsgExit1()
{
    UseMsg();
    attexit(1);
}


/*
 * This function parses the command line. Handles device-independent fields
 * and allows ddx to handle additional fields.  It is not allowed to modify
 * argc or any of the strings pointed to by argv.
 */
Bool	sdbMouse = FALSE;

void
ProcessCommandLine ( argc, argv )
int	argc;
char	*argv[];

{
    int i, skip;
    int r=0;
   
#ifdef MEMBUG
    if (!minfree)
	minfree = (pointer)sbrk(0);
#endif
    defaultKeyboardControl.autoRepeat = TRUE;

    for ( i = 1; i < argc; i++ )
    {
	/* call ddx first, so it can peek/override if it wants */
        if(skip = ddxProcessArgument(argc, argv, i))
	{
	    i += (skip - 1);
	}
	else if(argv[i][0] ==  ':')  
	{
	    /* initialize display */
/* Sun River work:

   Old code: display used to char *
            display = argv[i];
            display++;
*/
	    if ( strlen( argv[i] + 1 ) > (unsigned)3 )
	    {
		Error("Illegal display number specified");
	        attexit( -1 );
	    }

            strcpy( display, argv[i] + 1 );
	}
#ifdef NOT_US
	else if ( strcmp( argv[i], "-a") == 0)
	{
	    if(++i < argc)
	        defaultPointerControl.num = atoi(argv[i]);
	    else
		UseMsg();
	}
#endif
#ifdef MEMBUG
	else if ( strcmp( argv[i], "-alloc") == 0)
	{
	    if(++i < argc)
	        Memory_fail = atoi(argv[i]);
	    else
		UseMsg();
	}
#endif
	else if ( strcmp( argv[i], "-auth") == 0)
	{
	    if(++i < argc)
	        InitAuthorization (argv[i]);
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "bc") == 0)
	    permitOldBugs = TRUE;
	else if ( strcmp( argv[i], "-bs") == 0)
	    disableBackingStore = TRUE;
#ifdef NOT_US
	else if ( strcmp( argv[i], "c") == 0)
	{
	    if(++i < argc)
	        defaultKeyboardControl.click = atoi(argv[i]);
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-c") == 0)
	{
	    defaultKeyboardControl.click = 0;
	}
#endif
#ifdef NOT_US
	else if ( strcmp( argv[i], "-cc") == 0)
	{
	    if(++i < argc)
	        defaultColorVisualClass = atoi(argv[i]);
	    else
		UseMsg();
	}
#endif
	else if ( strcmp( argv[i], "-renderer") == 0)
	{
	    if(++i < argc) 
	        if (fntopts < MAXRENDER) renderer_options[fntopts++] = argv[i];
	else
		UseMsg();
        }
	else if ( strcmp( argv[i], "-fontconfig") == 0)
	{
	    if(++i < argc)
	        fontconfig_file = argv[i];
	    else
		UseMsg();
        }
	else if ( strcmp( argv[i], "-config") == 0)
	{
	    if(++i < argc)
	        config_file = argv[i];
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-cmap") == 0)
	{
	    if(++i < argc)
	        color_file = argv[i];
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-co") == 0)
	{
	    if(++i < argc)
	        CLrgbPath = argv[i];
	    else
		UseMsg();
	}
        else if ( strcmp( argv[i], "-d") == 0) /* Debugging signal */
        {
            if(++i < argc)
                xsig = atoi(argv[i]);
            else
                UseMsg();
            if(xsig < 1 || xsig == 9 ||  xsig >= NSIG){
                ErrorF("Only signals between the ranges 1-8 and 10-%d are allowed\n", NSIG-1);
                attexit(1);
            }
        }
	else if ( strcmp( argv[i], "-vs") == 0)
	{
	    /*
	     * -vs <vendor_string>
	     *
	     * this will be an unofficial flag. Some older
	     * clients like VPiX, USL's xterm depend on some key words in 
	     * vendor string. By providing this flag, a user can override
	     * default Vendor String
	     */	
	    if(++i < argc) {
	        vendor_string = argv[i];
		defaultVendorString = FALSE;
	    }
	    else
		UseMsg();
	}
#ifdef NOT_US
	else if ( strcmp( argv[i], "-dpi") == 0)
	{
	    if(++i < argc)
	        monitorResolution = atoi(argv[i]);
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-f") == 0)
	{
	    if(++i < argc)
	        defaultKeyboardControl.bell = atoi(argv[i]);
	    else
		UseMsg();
	}
#endif
	else if ( strcmp( argv[i], "-fc") == 0)
	{
	    if(++i < argc)
	        defaultCursorFont = argv[i];
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-fn") == 0)
	{
	    if(++i < argc)
	        defaultTextFont = argv[i];
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-fp") == 0)
	{
	    if(++i < argc)
	        cmdline_fontpath = argv[i];
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-help") == 0)
	{
	    UseMsg();
	    attexit(0);
	}
#ifndef NOLOGOHACK
	else if ( strcmp( argv[i], "-logo") == 0)
	{
	    logoScreenSaver = 1;
	}
	else if ( strcmp( argv[i], "nologo") == 0)
	{
	    logoScreenSaver = 0;
	}
#endif
       else if ( strcmp( argv[i], "-xnetaccess") == 0)
        {
            if(++i < argc){
                if ( strcmp( argv[i], "on") == 0)
                    AccessEnabled = EnableAccess;
                else if ( strcmp( argv[i], "off") == 0)
                    AccessEnabled = DisableAccess;
                else
                    UseMsg();
            }
            else
                UseMsg();
        }
	else if ( strcmp( argv[i], "-p") == 0)
	{
	    if(++i < argc)
	        defaultScreenSaverInterval = ((long)atoi(argv[i])) *
					     MILLI_PER_MIN;
	    else
		UseMsg();
	}
#ifdef NOT_US
	else if ( strcmp( argv[i], "r") == 0)
	    defaultKeyboardControl.autoRepeat = TRUE;
	else if ( strcmp( argv[i], "-r") == 0)
	    defaultKeyboardControl.autoRepeat = FALSE;
#endif
	else if ( strcmp( argv[i], "-s") == 0)
	{
	    if(++i < argc)
	        defaultScreenSaverTime = ((long)atoi(argv[i])) * MILLI_PER_MIN;
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-su") == 0)
	    disableSaveUnders = TRUE;
#ifdef NOT_US
	else if ( strcmp( argv[i], "-t") == 0)
	{
	    if(++i < argc)
	        defaultPointerControl.threshold = atoi(argv[i]);
	    else
		UseMsg();
	}
#endif
#ifdef USE_TIMEOUT
	else if ( strcmp( argv[i], "-to") == 0)
	{
	    if(++i < argc)
		TimeOutValue = ((long)atoi(argv[i])) * MILLI_PER_SECOND;
	    else
		UseMsg();
	}
#endif
	else if ( strcmp( argv[i], "v") == 0)
	    defaultScreenSaverBlanking = PreferBlanking;
	else if ( strcmp( argv[i], "-v") == 0)
	    defaultScreenSaverBlanking = DontPreferBlanking;
	else if ( strcmp( argv[i], "-wm") == 0)
	    defaultBackingStore = WhenMapped;
#ifdef NOT_US
	else if ( strcmp( argv[i], "-x") == 0)
	{
	    if(++i >= argc)
		UseMsg();
	    /* For U**x, which doesn't support dynamic loading, there's nothing
	     * to do when we see a -x.  Either the extension is linked in or
	     * it isn't */
	}
#endif
	else if ( strcmp( argv[i], "-I") == 0)
	{
	    /* ignore all remaining arguments */
	    break;
	}
#ifdef NOT_US
	else if (strncmp (argv[i], "tty", 3) == 0)
	{
	    /* just in case any body is interested */
	    dev_tty_from_init = argv[i];
	}
#endif
#ifdef XDMCP
	else if ((skip = XdmcpOptions(argc, argv, i)) != i)
	{
	    i = skip - 1;
	}
#endif
#ifdef SDB_MOUSE
	else if ( strcmp( argv[i], "-sdbmse") == 0)
	{
	    /*
	     * mouse interrupts are a pain while using sdb, so to get around
	     * the mouse interrupts, make queue->sigenable = 0 and set
	     * wait time = 0; The side effect is that server continuously
	     * loops, but this is only for debugging ......
	     *
 	     * sdbMouse is set to TRUE while running the server in sdb.
	     * Also look in dix/dispatch.c, os/sysV/WaitFor.c,
	     * ddx/io/init.c and ddx/io/xwin_io.c
	     */
	     sdbMouse = TRUE;
	}
#endif

	else if ( strcmp( argv[i], "-class") == 0)
	{
		/*
		 * if a class is specified on the command line, save it.
		 * default is fixed class
		 */
	        if(++i < argc) {
			if ( (strcmp(argv[i],"fixed")) == 0 )
				SchedulerClass = FIXED;
			else if ( (strcmp(argv[i],"timeshare")) == 0 )
				SchedulerClass = TIMESHARE;
			else if ( (strcmp(argv[i],"realtime")) == 0 )
				SchedulerClass = REALTIME;
			else
				UseMsg();
		} else
			UseMsg();
	}
	else if ( strcmp( argv[i], "-upri") == 0)
	{
		/*
		 * run the server in fixed class scheduler using the 
		 * specified priority.
		 */
	        if(++i < argc) {
			fixedupri = (short)atoi(argv[i]);
		} else
			UseMsg();
	}
 	else
 	{
	    UseMsg();
	    ErrorF ("\nFor more info, read /usr/X/defaults/README\n");
	    attexit (1);
        }
    }
}

#ifndef SPECIAL_MALLOC

#ifdef MEMBUG
#define FIRSTMAGIC 0x11aaaa11
#define SECONDMAGIC 0x22aaaa22
#define FREEDMAGIC  0x33aaaa33

typedef struct _MallocHeader	*MallocHeaderPtr;

typedef struct _MallocHeader {
	unsigned long	amount;
	unsigned long	time;
	MallocHeaderPtr	prev;
	MallocHeaderPtr	next;
	unsigned long	magic;
} MallocHeaderRec;

typedef struct _MallocTrailer {
	unsigned long	magic;
} MallocTrailerRec, *MallocTrailerPtr;

unsigned long	MemoryAllocTime;
unsigned long	MemoryAllocBreakpoint = ~0;
unsigned long	MemoryActive = 0;
unsigned long	MemoryValidate;

MallocHeaderPtr	MemoryInUse;

#define request(amount)	((amount) + sizeof (MallocHeaderRec) + sizeof (MallocTrailerRec))
#define Header(ptr)	((MallocHeaderPtr) (((char *) ptr) - sizeof (MallocHeaderRec)))
#define Trailer(ptr)	((MallocTrailerPtr) (((char *) ptr) + Header(ptr)->amount))

static unsigned long *
SetupBlock(ptr, amount)
    unsigned long   *ptr;
{
    MallocHeaderPtr	head = (MallocHeaderPtr) ptr;
    MallocTrailerPtr	tail = (MallocTrailerPtr) (((char *) ptr) + amount + sizeof (MallocHeaderRec));

    MemoryActive += amount;
    head->magic = FIRSTMAGIC;
    head->amount = amount;
    if (MemoryAllocTime == MemoryAllocBreakpoint)
	head->amount = amount;
    head->time = MemoryAllocTime++;
    head->next = MemoryInUse;
    head->prev = 0;
    if (MemoryInUse)
	MemoryInUse->prev = head;
    MemoryInUse = head;

    tail->magic = SECONDMAGIC;
    
    return (unsigned long *)(((char *) ptr) + sizeof (MallocHeaderRec));
}

ValidateAllActiveMemory ()
{
    MallocHeaderPtr	head;
    MallocTrailerPtr	tail;

    for (head = MemoryInUse; head; head = head->next)
    {
	tail = (MallocTrailerPtr) (((char *) (head + 1)) + head->amount);
    	if (head->magic == FREEDMAGIC)
	    FatalError("Free data on active list");
    	if(head->magic != FIRSTMAGIC || tail->magic != SECONDMAGIC)
	    FatalError("Garbage object on active list");
    }
}

#endif

/* XALLOC -- X's internal memory allocator.  Why does it return unsigned
 * int * instead of the more common char *?  Well, if you read K&R you'll
 * see they say that alloc must return a pointer "suitable for conversion"
 * to whatever type you really want.  In a full-blown generic allocator
 * there's no way to solve the alignment problems without potentially
 * wasting lots of space.  But we have a more limited problem. We know
 * we're only ever returning pointers to structures which will have to
 * be long word aligned.  So we are making a stronger guarantee.  It might
 * have made sense to make Xalloc return char * to conform with people's
 * expectations of malloc, but this makes lint happier.
 */

unsigned long * 
Xalloc (amount)
    unsigned long amount;
{
    char		*malloc();
    register pointer  ptr;
	
    if(!amount)
	return (unsigned long *)NULL;
    /* aligned extra on long word boundary */
    amount = (amount + 3) & ~3;
#ifdef MEMBUG
    if (MemoryValidate)
	ValidateAllActiveMemory ();
    if (!Must_have_memory && Memory_fail &&
	((random() % MEM_FAIL_SCALE) < Memory_fail))
	return (unsigned long *)NULL;
    if (ptr = (pointer)malloc(request(amount)))
	return SetupBlock (ptr, amount);
#else
    if (ptr = (pointer)malloc(amount))
	return (unsigned long *)ptr;
#endif
    if (Must_have_memory)
	FatalError("Out of memory");
    return (unsigned long *)NULL;
}

#ifndef i386	/* funNotUsedByATT, Xcalloc */

/*****************
 * Xcalloc
 *****************/

unsigned long *
Xcalloc (amount)
    unsigned long   amount;
{
    unsigned long   *ret;

    ret = Xalloc (amount);
    if (ret)
	bzero ((char *) ret, (int) amount);
    return ret;
}

#endif	/* i386, funNotUsedByATT */

/*****************
 * Xrealloc
 *****************/

unsigned long *
Xrealloc (ptr, amount)
    register pointer ptr;
    unsigned long amount;
{
    char *malloc();
    char *realloc();

#ifdef MEMBUG
    if (MemoryValidate)
	ValidateAllActiveMemory ();
    if (!amount)
    {
	Xfree(ptr);
	return (unsigned long *)NULL;
    }
    if (!Must_have_memory && Memory_fail &&
	((random() % MEM_FAIL_SCALE) < Memory_fail))
	return (unsigned long *)NULL;
    amount = (amount + 3) & ~3;
    if (ptr)
    {
	CheckNode(ptr);
	ptr = (pointer)realloc((char *) Header (ptr), request(amount));
    }
    else
	ptr = (pointer)malloc(request(amount));
    if (ptr)
	return SetupBlock (ptr, amount);
#else
    if (!amount)
    {
	if (ptr)
	    free(ptr);
	return (unsigned long *)NULL;
    }
    amount = (amount + 3) & ~3;
    if (ptr)
        ptr = (pointer)realloc((char *)ptr, amount);
    else
	ptr = (pointer)malloc(amount);
    if (ptr)
        return (unsigned long *)ptr;
#endif
    if (Must_have_memory)
	FatalError("Out of memory");
    return (unsigned long *)NULL;
}
                    
/*****************
 *  Xfree
 *    calls free 
 *****************/    

void
Xfree(ptr)
    register pointer ptr;
{
#ifdef MEMBUG
    if (MemoryValidate)
	ValidateAllActiveMemory ();
    if (ptr)
    {
	MallocHeaderPtr	head;

	CheckNode(ptr);
	head = Header(ptr);
	head->magic = FREEDMAGIC;
	free ((char *) head);
    }
#else
    if (ptr)
	free((char *)ptr); 
#endif
}

#ifdef MEMBUG
static void
CheckNode(ptr)
    pointer ptr;
{
    MallocHeaderPtr	head;
    MallocHeaderPtr	f, prev;

    if (ptr < minfree)
	FatalError("Trying to free static storage");
    head = Header(ptr);
    if (((pointer) head) < minfree)
	FatalError("Trying to free static storage");
    if (head->magic == FREEDMAGIC)
	FatalError("Freeing something already freed");
    if(head->magic != FIRSTMAGIC || Trailer(ptr)->magic != SECONDMAGIC)
	FatalError("Freeing a garbage object");
    if(head->prev)
	head->prev->next = head->next;
    else
	MemoryInUse = head->next;
    if (head->next)
	head->next->prev = head->prev;
    MemoryActive -= head->amount;
}

DumpMemoryInUse (time)
    unsigned long   time;
{
    MallocHeaderPtr	head;

    for (head = MemoryInUse; head; head = head->next)
	if (head->time >= time)
	    printf ("0x%08x %5d %6d\n", head,
					head->amount,
					head->time);
}

#ifndef i386	/* funNotUsedByATT, MarkMemoryTime, DumpMemorySince */

static unsigned long	MarkedTime;

MarkMemoryTime ()
{
    MarkedTime = MemoryAllocTime;
}

DumpMemorySince ()
{
    DumpMemoryInUse (MarkedTime);
}

#endif	/* i386, funNotUsedByATT */

#endif
#endif /* SPECIAL_MALLOC */

/*VARARGS1*/
void
FatalError(f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9) /* limit of ten args */
    char *f;
    char *s0, *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8, *s9;
{
    ErrorF("\nFatal server bug!\n");
    ErrorF(f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9);
    ErrorF("\n");
    AbortServer();
    /*NOTREACHED*/
}

/*VARARGS1*/
void
ErrorF( f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9) /* limit of ten args */
    char *f;
    char *s0, *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8, *s9;
{
    fprintf( stderr, f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9);
}

/* Sun River work */

#include <errno.h>

void
attclean()
{
 	char	wellknownsocket[ 20 ];
	char	pid_file[ 30 ];
#ifdef SVR4
 	char	namedpipe[ 20 ];
#endif

        sprintf( wellknownsocket, "/dev/X/server.%s", display );
	sprintf( pid_file, "/dev/X/server.%s.pid", display );
#ifdef SVR4
        sprintf( namedpipe, "/dev/X/Nserver.%s", display );
#endif

        if ( unlink( wellknownsocket ) < 0 && errno == EINTR )
		unlink( wellknownsocket );	/* try again */

        if ( unlink( pid_file ) < 0 && errno == EINTR )
		unlink( pid_file );	/* try again */

#ifdef SVR4
        if ( unlink( namedpipe ) < 0 && errno == EINTR )
		unlink( namedpipe );	/* try again */
#endif
}

void
attcleanup( signo )
int	signo;
{
	attclean();
	sigset( signo, SIG_DFL );
	kill( getpid(), signo );
}

int
attexit( ret )
int	ret;
{
	resetmodes();
	exit( ret );
}

/*
    This ioctl info is from the kd driver folks:

       -1 => a tty like your 630
       0x6B64 => your console
       otherwise the lower 8 bits are Sun River term #,

       I map the Sun River channel number using (d * 10) + 100 to
       get a display number to use when creating /dev/X/server.n.
       This is an arbitrary decision for a display number scheme.
       We choose it over several alternatives.
*/

void
attGetDisplay( display )
char	display[];
{
    int	   fd;
    int	   ret;
    int	   dispno;

    fd = open( "/dev/tty", O_RDWR );
    ret = ioctl( fd, KIOCINFO );
    close( fd );

    if ( ret == -1 || ret == 0x6B64 )
    {
	display[ 0 ] = '0';
	display[ 1 ] = '\0';
    }
    else if (( ret & 0xFF00 ) == 0x7300 )	/* Sun River Station */
    {
	dispno = ((ret & 0xFF) * 10) + 100;
	sprintf( display, "%d", dispno );
    }
    else
    {
	FatalError( "I can't determine the display with KIOCINFO ioctl" );
    }
}

/*
    Check if server is already running, if it's not, setup signal
    handlers to clean up before exiting.
*/

attStartUp()
{
    FILE	*pid_fp;
    char	pid_file[ 30 ];
    int		server_pid;
    int		fd;
    char	buf[ 20 ];

    sprintf( pid_file, "/dev/X/server.%s.pid", display );

    if (( pid_fp = fopen( pid_file, "r+" )) != NULL )
    {
	fscanf( pid_fp, "%d", &server_pid );

	if ( kill( server_pid, 0 ) == 0 )	/* server_pid is legit */
	{
            /* Just because the pid is legit, does NOT mean it belongs
               to the server, so double check running server.
            */

            sprintf( buf, "/dev/X/server.%s", display );

            if (( fd = open( buf, O_RDWR )) >= 0)
            {
		fprintf( stderr, "Server on display %s is already running\n", display );
        	close( fd );
        	_exit( 1 );
            }
	}
    }

    /* The server is NOT running on the specified display */

    attclean();		/* just in case server was killed with -9 */
    fclose( pid_fp );

    if (( pid_fp = fopen( pid_file, "w" )) == NULL )
    {
        fprintf( stderr, "Cannot open %s\n", pid_file );
    }
    else
    {
        fprintf( pid_fp, "%d", getpid());
        fclose( pid_fp );
    }

/* The following signals are reset elsewhere in the server.
   I'm doing this now just in case a signal that would kill
   the server comes in before it gets to "elsewhere."
*/

    fpsetround(FP_RN);
    sigset (SIGPIPE, SIG_IGN);
    sigset (SIGFPE, SIG_IGN);	
    sigset( SIGHUP, attcleanup );
    sigset( SIGINT, attcleanup );
    sigset( SIGQUIT, attcleanup );
    sigset( SIGILL, attcleanup );
    sigset( SIGTERM, attcleanup );
    sigset( SIGUSR1, attcleanup );
    sigset( SIGUSR2, attcleanup );


/* The following signals are NEVER dealt with in the server */

    sigset( SIGABRT, attcleanup );
    sigset( SIGEMT, attcleanup );
    sigset( SIGBUS, attcleanup );
    sigset( SIGSEGV, attcleanup );
    sigset( SIGSYS, attcleanup );
    sigset( SIGALRM, attcleanup );

#ifdef SVR4
    sigset( SIGVTALRM, attcleanup );
    sigset( SIGPROF, attcleanup );
    sigset( SIGXCPU, attcleanup );
    sigset( SIGXFSZ, attcleanup );
#endif
}


InitMiscellaneous ( argc, argv )
int	argc;
char	*argv[];
{

    /*  Sun River work:
    I need to determine what terminal I'm on.  The user can override the
    display using the ":n" command line arg to the server that is processed
    by ProcessCommandLine(argc, argv) below.

    The old hard coded way:

       display = "0";
    */

    attGetDisplay( display );
    /*defaultFontPath  = (char *)GetFontDefaults(defaultFontPath ); jhg - r5*/
		    /* rjk */
    ProcessCommandLine(argc, argv);

    /* Sun River work */
    attStartUp();
}

/* Set the server's scheduler class to realtime */
void
SetSchedulerClass (file)
char *file;
{
#ifdef SVR4
	pcinfo_t  info;
	pcparms_t args;

	/*
	 * default mode : fixed class
	 *
	 * the user can override the default mode by specifying on 
	 * command line:
	 * 		ex: X -class timeshare
	 */
	switch (SchedulerClass) {
	 case REALTIME:
	    strcpy (info.pc_clname, "RT");
	    if (priocntl (0, 0, PC_GETCID, &info) >= 0) {
		args.pc_cid = info.pc_cid;
		((rtparms_t *)args.pc_clparms)->rt_tqnsecs = RT_TQDEF;
		((rtparms_t *)args.pc_clparms)->rt_pri = RT_NOCHANGE;
		if (priocntl (P_PID, P_MYID, PC_SETPARMS, &args) >= 0) {
			ErrorF ("Server running in RealTime mode.\n");
			return;
		}
	    }

	 case FIXED:
	 default:
#ifdef SVR4_ES
	    strcpy (info.pc_clname, "FC");
	    if (priocntl (0, 0, PC_GETCID, &info) >= 0) {
		args.pc_cid = info.pc_cid;
		((fcparms_t *)args.pc_clparms)->fc_uprilim = fixedupri;
		((fcparms_t *)args.pc_clparms)->fc_upri = fixedupri;
		if (priocntl (P_PID, P_MYID, PC_SETPARMS, &args) >= 0) {
			ErrorF ("Server running in Fixed Class Mode.\n");
			return;
		}
	    }
#endif

	 case TIMESHARE:
		ErrorF ("Server running in Time Share Mode.\n");
	    break;
	}
#endif /* SVR4 */
}
