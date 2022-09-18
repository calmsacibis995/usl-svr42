/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/io/init.c	1.13"

/*
 *	Copyright (c) 1991 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyright (c) 1988, 1989, 1990 AT&T
 *	All Rights Reserved 
 */

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved
******************************************************************/
/* $Header: init.c,v 1.27 87/09/09 17:09:06 rws Exp $ */

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#ifdef SVR4
#include <sys/stream.h>
#include <sys/fcntl.h>
#else
#include <fcntl.h>
#endif

#include <signal.h>
#include <sys/param.h>

#define EVC
#include <sys/vt.h>
#include <sys/at_ansi.h>
#include <sys/kd.h>
#include <sys/xque.h>
#include <termio.h>

#include "X.h"
#include "Xproto.h"
#include "screenint.h"
#include "input.h"
#include "cursor.h"
#include "misc.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "gc.h"
#include "gcstruct.h"

#include "xwin_io.h"
#include "siconfig.h"

#include "../si/mipointer.h"

#define XSIG    SIGTRAP
int	condev = 0;		/* fd of the console */
int	indev;			/* fd of the mouse   */
unchar  orgleds;                /* LED's that were down
                                 * when we started or after
                                 * we switched back VT */
static 	int	vtnum = 0;

#define VENDOR_STRING_BASE      "USL's XWIN WGS"
#define VENDOR_STRING_MAXLENGTH 50

char 	*vendor_string; 	/* it's a global dynamic vendor string */
Bool 	defaultVendorString = TRUE; /* default vendor string flag */

extern int	xsig;
extern unchar 	ledsdown;	/* defined in i386_io.c */
extern xqEventQueue    *queue;
extern int	qLimit;
extern void	GetKDKeyMap ();
extern void	NoopDDA ();

#ifdef FLUSH_IN_BH
extern void	xwinBlockHandler ();
extern void	xwinWakeupHandler ();
#endif

static Bool	ioCloseScreen ();
static Bool	ioSaveScreen ();

/*
 * Frame Buffer specifics
 */
struct fbtype {
	int fb_type;
};

/*
 * MAXEVENTS is the maximum number of events the mouse and keyboard functions
 * will read on a given call to their GetEvents vectors.
 */
#define MAXEVENTS 	256

/*
 * Frame-buffer-private info.
 *	fd  	  	file opened to the frame buffer device.
 *	info	  	description of the frame buffer -- type, height, depth,
 *	    	  	width, etc.
 *	pGC 	  	A GC for realizing cursors.
 *	GetImage  	Original GetImage function for this screen.
 *	CreateGC  	Original CreateGC function
 *	CreateWindow	Original CreateWindow function
 *	ChangeWindowAttributes	Original function
 *	GetSpans  	GC function which needs to be here b/c GetSpans isn't
 *	    	  	called with the GC as an argument...
 *	fbPriv	  	Data private to the frame buffer type.
 */
typedef struct {
    GCPtr   	  	pGC;	    /* GC for realizing cursors */

    void    	  	(*GetImage)();
    Bool	      	(*CreateGC)();/* GC Creation function previously in the
				       * Screen structure */
    Bool	      	(*CreateWindow)();
    Bool		(*ChangeWindowAttributes)();
    unsigned int  	*(*GetSpans)();
    void		(*EnterLeave)();
    int			fd; 	    /* Descriptor open to frame buffer */
    struct fbtype 	info;	    /* Frame buffer characteristics */
    pointer		fbPriv;	    /* Frame-buffer-dependent data */
    int			fb_width, fb_height;
    int			fb_dpix, fb_dpiy;
} fbFd;

extern fbFd 	  ioFbs[];
extern Bool	  screenSaved;		/* True is screen is being saved */
extern void	  ErrorF();

extern Bool i386ScreenInit();		/* i386 specific code */
extern int i386MouseProc();
extern int i386KeybdProc();

#define MOTION_BUFFER_SIZE 0
#define NUMSCREENS 1

int displayType;

/* SI: start */
extern  void    CloseVirtTerm ();
Bool          screenSaved = xFalse;
extern int condev;

static int	ioScreenIndex;


/* need to define NUMSCREENS here to something correct */
#define	NUMSCREENS	1	/*XXX - is this what we want? Certainly not! */
fbFd	ioFbs[NUMSCREENS];  /* Space for descriptors of open frame buffers */

static PixmapFormatRec	formats[] = {
    1, 1, BITMAP_SCANLINE_PAD,	/* 1-bit deep */
    2, 2, BITMAP_SCANLINE_PAD,	/* 2-bit deep */
    4, 4, BITMAP_SCANLINE_PAD,	/* 4-bit deep */
    8, 8, BITMAP_SCANLINE_PAD,	/* 8-bit deep */
};
#define NUMFORMATS	(sizeof formats)/(sizeof formats[0])

extern	char	*display;		 /* in server/os/sysV/connection.c */
siCmap		si_colormap;		/* user specified colormap */

Bool	screen_inited = 0; 	/* if 1, display has been initialized */

#define MOTION_BUFFER_SIZE 0	 /* What should this *really* be? */
/* SI: end */

/* ARGSUSED */
void
InitInput(argc, argv)
    int argc;
    char *argv[];
{
    DevicePtr p, k;
    
    p = AddInputDevice(i386MouseProc, TRUE);
    k = AddInputDevice(i386KeybdProc, TRUE);

    RegisterPointerDevice(p, MOTION_BUFFER_SIZE);
    RegisterKeyboardDevice(k);
    miRegisterPointerDevice(screenInfo.screens[0], p);
}

/*-----------------------------------------------------------------------
 * InitOutput --
 *	Initialize screenInfo for all framebuffers configured for the
 *	current display.
 * Results:
 *	screenInfo init proc field set
 * Side Effects:
 *	None
 *----------------------------------------------------------------------
 *
 * NUMSCREENS is the number of supported frame buffers (i.e. the number of
 * structures in odFbData which have an actual probeProc).
 *---------------------------------------------------------------------*/

InitOutput(pScreenInfo, argc, argv)
    ScreenInfo 	  *pScreenInfo;
    int     	  argc;
    char    	  **argv;
{
	int		i,
			index,
			displaylen;
	SIConfigP	cf;

	index = 0;
	displaylen = strlen (display);

	/*
	 *	Due to the inner workings of vts, need to find and open an
	 *	available vt before actually opening the output device.
	 *	This requires that we find the correct keyboard device here,
	 *	instead of dealing with it in InitInput().  (Note that only
	 *	the first keyboard for this display will actually be used.)
	 */

	pScreenInfo->imageByteOrder = IMAGE_BYTE_ORDER;
	pScreenInfo->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
	pScreenInfo->bitmapScanlinePad = BITMAP_SCANLINE_PAD;
	pScreenInfo->bitmapBitOrder = BITMAP_BIT_ORDER;

	/*SI:  */
	pScreenInfo->numPixmapFormats = NUMFORMATS;
	for (i=0; i< NUMFORMATS; i++)
		pScreenInfo->formats[i] = formats[i];

	/*
	 *	Read the configuration file.
	 */
	if ((cf = config_getresource ("display", display)) != (SIConfigP)0) {

		/* If no screen was specified, then "screen" will be -1.
		 * It needs to be changed to 0.  This may not be correct
		 * behaviour when support for multiple screens is added.
		 */
		if (cf->screen == -1)
			cf->screen = 0;

		/*
		 * See if there's a user specified colormap to be used
		 * for this screen.  
		 */
		si_colormap.display = cf->displaynum;
		si_colormap.screen = cf->screen;
		si_colormap.visual = cf->deflt;
		if (io_readcmapdata(&si_colormap) < 0) {
		   si_colormap.sz = 0;
		   ErrorF ("Cannot read colormap data. Using system generated colormap.\n");
		}
	
		/* this entry interests us. Try initializing the device */
		if (ioDisplayDeviceInit (pScreenInfo, index, cf, argc, argv) == TRUE)
		{
			/* This display initialized properly */
			index++;

			/* remember that we need to de-init screen later */
			screen_inited++;
		}
	} 

	if (index == 0) {
	    FatalError("Cannot initialize display device. Check if all the shared libraries are installed properly.\n");
	}
	else {
	    if (defaultVendorString) {
	    	sprintf (vendor_string, "%s with %s display. [(using vt%02d)]",
			VENDOR_STRING_BASE, cf->type, vtnum);
		/*
		 * VPiX wierdity: VPiX depends on a pattern "(using" in 
		 * vendor_string; so if you do not have this the 'zoom' 
		 * feature in VPiX will not work
		 */
	     }
	}

	pScreenInfo->numScreens = index;
}

/*-----------------------------------------------------------------------
 * ioDisplayDeviceInit --
 *	Attempt to find and initialize a particular framebuffer.
 *
 * Results:
 *	Returns TRUE if successful, FALSE if not.
 *
 * Side Effects:
 *	The device driver init routine is called, passed the open VT file
 *	descriptor, a pointer to the configuration information, and the
 *	hardware information pointer.
 *
 *----------------------------------------------------------------------*/
Bool
ioDisplayDeviceInit(pScreenInfo, index, configp, argc, argv)
    ScreenInfo	  *pScreenInfo;	/* The screenInfo struct */
    int	    	  index;    	/* The index of pScreen in the ScreenInfo */
    SIConfigP     configp;	/* device configuration information */
    int	    	  argc;	    	/* The number of the Server's arguments. */
    char    	  **argv;   	/* The arguments themselves. Don't change! */
{
    int         i, oldNumScreens;
    int         fd;
    char        *name;
    extern	char *display_lib;

    if ((ioScreenIndex = AllocateScreenPrivateIndex()) < 0)
	return (FALSE);

    if (!screen_inited) {
	/*
	 * SVR4: display library loaded at run time.
	 * SVR32: initializes the pointer to DisplayFuncs data str
	 */
	if(LoadDisplayLibrary(display_lib) <0 )
		return(FALSE);
    	/*
	 * call the device-specific initialization routine 
	 */
    	if (si_Init (condev, configp, HWinfo, &HWroutines) == SI_FALSE) {
	   FatalError ("Error detected at display initialization time.\n\
	   display class: %s , device: %s\n", configp->type, configp->device);
    	}
    }

    ioFbs[index].fd = condev;
    ioFbs[index].fb_width = si_getscanlinelen;
    ioFbs[index].fb_height = si_getscanlinecnt;
    ioFbs[index].fb_dpix = HWinfo->SIxppin ? HWinfo->SIxppin : 90;
    ioFbs[index].fb_dpiy = HWinfo->SIyppin ? HWinfo->SIyppin : 90;

    return (AddScreen(i386ScreenInit, argc, argv) >= 0);
}

/*------------------------------------------------------------------------
 * ioFrameBufferInit --
 *	Attempt to initialize a framebuffer
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	Most of the elements of the ScreenRec are filled in.  The
 *	video is enabled for the frame buffer...
 *
 *	The graphics context for the screen is created. The CreateGC,
 *	CreateWindow and ChangeWindowAttributes vectors are changed in
 *	the screen structure.
 *
 *	Both a BlockHandler and a WakeupHandler are installed for the
 *	first screen.
 *----------------------------------------------------------------------*/
/*ARGSUSED*/
Bool
ioFrameBufferInit (index, pScreen, argc, argv)
    int	    	  index;    	/* The index of pScreen in the ScreenInfo */
    ScreenPtr	  pScreen;  	/* The Screen to initialize */
    int	    	  argc;	    	/* The number of the Server's arguments. */
    char    	  **argv;   	/* The arguments themselves. Don't change! */
{
    ColormapPtr 	pColormap;
    fbFd          	*fb;
    DrawablePtr   	pDrawable;
    extern miPointerCursorFuncRec       siPointerCursorFuncs;   /* SI (rjk) */

    if (!siScreenInit(pScreen,
			   (unsigned char *)-1,
			   ioFbs[index].fb_width,
			   ioFbs[index].fb_height,
			   ioFbs[index].fb_dpix,
			   ioFbs[index].fb_dpiy,
			   ioFbs[index].fb_width))
				
	return (FALSE);

    pScreen->SaveScreen = ioSaveScreen;
    pScreen->CloseScreen = ioCloseScreen;
    pScreen->devPrivates[ioScreenIndex].ptr = (pointer) pScreen->CloseScreen;
    pScreen->blockData = (pointer)0;
    pScreen->wakeupData = (pointer)0;

#ifdef FLUSH_IN_BH
    pScreen->WakeupHandler = xwinHandler;
    pScreen->BlockHandler = xwinHandler;
#else
    pScreen->BlockHandler = NoopDDA;
    pScreen->WakeupHandler = NoopDDA;
#endif

    (void) ioSaveScreen(pScreen, SCREEN_SAVER_FORCER);
    miDCInitialize(pScreen, &siPointerCursorFuncs);

    return (siCreateDefColormap(pScreen));
}

#ifdef FLUSH_IN_BH
void
xwinHandler(nscreen, pbdata, pptv, pReadmask)
    int nscreen;
    pointer pbdata;
    struct timeval **pptv;
    pointer pReadmask;
{
    si_Flushcache();
}
#endif

/*------------------------------------------------------------------------
 * ioSaveScreen --
 *	Disable the video on the frame buffer to save the screen.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Video enable state changes.
 *
 *----------------------------------------------------------------------- */
static Bool
ioSaveScreen (pScreen, on)
    ScreenPtr	  pScreen;
    Bool    	  on;
{
    SIint32         state;

    switch (on) {
    case SCREEN_SAVER_FORCER:
/*
	SetTimeSinceLastInputEvent();
*/
	TimeSinceLastInputEvent ();
	screenSaved = FALSE;
	state = SI_TRUE;
	break;
    case SCREEN_SAVER_OFF:
	screenSaved = FALSE;
	state = SI_TRUE;
	break;
    case SCREEN_SAVER_ON:
    default:
	screenSaved = TRUE;
	state = SI_FALSE;
	break;
    }

    si_setvideo(state);
}

/*------------------------------------------------------------------------
 * ioCloseScreen --
 *	called to ensure video is enabled when server exits.
 *
 * Results:
 *	Screen is unsaved.
 *
 * Side Effects:
 *	None
 *
 *----------------------------------------------------------------------- */
/*ARGSUSED*/
static Bool
ioCloseScreen(i, pScreen)
    int		i;
    ScreenPtr	pScreen;
{
    Bool ret;

    ret = pScreen->SaveScreen(pScreen, SCREEN_SAVER_OFF);

    if (pScreen->allowedDepths && pScreen->allowedDepths->vids)
		xfree(pScreen->allowedDepths->vids);
 
    /*  pScreen->visuals does not need to be freed here, since it is added as
     *  a resource in ScreenInit and is freed with the rest of the 
     *  resources. 
     */

    if(pScreen->devPrivate)
	xfree(pScreen->devPrivate);

    return(TRUE);
}

static
i386PixelError(index)
    int index;
{
    ErrorF("Only 0 or 1 are acceptable pixels for device %d\n", index);
}

/*
 * NOTE: ScreenInit is called when the server first starts up and also
 *	 _again_ whenever the server resets (iff all clients gone).
 */

Bool
i386ScreenInit(index, pScreen, argc, argv)
    int index;
    ScreenPtr pScreen;
    int argc;		/* these two may NOT be changed */
    char **argv;
{
    Bool		retval;
    int			i;
    extern char		*blackValue, *whiteValue;
    extern xqCursorPtr  mouse;
    static Bool		doit = TRUE;

    if (doit) {
	struct kd_quemode	qmode;
	extern int		waiting_for_acquire;
	extern void		sigusr1 ();

	/* The code to open the VT has been moved to a separate routine,
	 * openvt(), and is called later by CreateWellKnownSocket().
	 * This is to prevent switching VT's before making sure that this
	 * is the first server that is running.
	 */
	doit = FALSE;

	if ((indev = open ("/dev/mouse", O_RDONLY)) < 0) {
		Error("Cannot open /dev/mouse");

#undef  MOUSELESS
#define MOUSELESS
#ifndef MOUSELESS
		attexit(1);
#endif
	}
	if (xsig == 0)
		xsig = XSIG;
	qmode.qsize = 500;
	qmode.signo = xsig;
	if (ioctl(condev, KDQUEMODE, &qmode) == -1) {
		Error("KDQUEMODE failed");
		attexit(1);
	}
	queue = (xqEventQueue *)qmode.qaddr;
	qLimit = queue->xq_size;


	/* allocate the mouse cursor here and never free it */
	if (!(mouse = (xqCursorPtr)Xalloc(sizeof(xqCursorRec)))) {
	    ErrorF ("i386ScreenInit: can't allocate mouse cursor\n");
	    return (FALSE);
	}

	/*
	 * if the user didn't pass any vendor string, then allocate enough
	 * space here and initialize * the default vendor string later.
	 */
	if (defaultVendorString) {
	   /* allocate the vendor_string here and never free it */
	   if (!(vendor_string =
		(char *)Xalloc(VENDOR_STRING_MAXLENGTH * sizeof(char)))) {
	    	ErrorF ("i386ScreenInit: can't allocate vendor string\n");
	    	return (FALSE);
	    }
	}

	orgleds = GetKbdLeds();		/* Save the current kbd LED state */
	SetKbdLeds(ledsdown);		/* Turn 'em all off for starters */

	waiting_for_acquire = 0;	/* "Turn it on", set to not-waiting. */
	sigset(SIGUSR1, sigusr1); 	/* Trap the VT switch signal forever. */

    }	/* end of: if (doit) */

    retval = ioFrameBufferInit (index, pScreen, argc, argv);

    /* blackValue, whiteValue may have been set in ddxProcessArgument */

    if(blackValue)
    {
	if((i = atoi(blackValue)) == 0 || i == 1)
	    pScreen->blackPixel = i;
	else  
	    i386PixelError(index);
    }
    if(whiteValue)
    {
	if((i = atoi(whiteValue)) == 0 || i == 1)
	    pScreen->whitePixel = i;
	else  
	    i386PixelError(index);
    }
    return(retval);
}


int
openvt()
{		
    char		vtdev[20];	/* allow up to 20 chars in tty name */
    int			fd;
#ifdef SVR4
    int        		devvt;
#endif
    struct vt_mode	vtmode;
    struct termio	ttyparms;

    /* Sun River work */

    char		tty[20];	/* allow up to 20 chars in tty name */
    char		suntty[20];	/* allow up to 20 chars in tty name */
    char		errmsg[40];
    extern char		*display;
    int			dispno = atoi( display );
    int			devno;
    int			srchan;
    extern Bool		SunRiverStation;

    devno = dispno / 10;

    switch ( devno )
    {
        case 0:		/* console */

            sprintf( tty, "/dev/console" );
            *suntty = '\0';
            sprintf( errmsg, "Cannot open /dev/console" );
            break;

        case 10:	/* one of the Sun River Stations */
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:

            SunRiverStation = TRUE;
            srchan = devno - 10;	/* there are 4 channels per board */
					/* and you can use up to 4 boards */

            sprintf( tty, "/dev/s%dvt00",  srchan );
            sprintf( suntty, "s%d", srchan );
            sprintf( errmsg, "Cannot open /dev/s%dvt00", srchan );
            break;

        default:

            Error("Illegal display number specified");
            return ( -1 );
            break;
    }

    /* Sun River work: add timeout if monitor NOT hooked up to board */

    alarm( 20 );	/* 20 seconds of real time */

    if ((fd = open(tty, O_WRONLY)) < 0) {
	alarm( 0 );		/* turn off alarm */
	Error(errmsg);
	return(-1);
    }
    alarm( 0 );		/* turn off alarm */
    GetKDKeyMap (fd);		/* Get default keyboard mappings */
    if (ioctl(fd, VT_OPENQRY, &vtnum) < 0 || vtnum < 0) {
	Error("Cannot find a free VT");
	return(-1);
    }

    /* Sun River work: add suntty below */

    sprintf(vtdev, "/dev/%svt%02d", suntty, vtnum);

    if (ioctl (fd, TCGETA, &ttyparms) < 0)
    {
	Error ("openvt:	 TCGETA failed");
	return (-1);
    }

    if (ttyparms.c_lflag & (ICANON | ISIG) == 0)
    {
	ErrorF ("openvt:  /dev/console is in raw mode\n");
	return (-1);
    }

    close(fd);
    setpgrp();

    condev = open(vtdev, O_RDWR);
    if (condev < 0) {
        ErrorF("Cannot open device %s", vtdev);
        Error(" ");
        return(-1);
    }

    if (ioctl (condev, TCSETAW, &ttyparms) < 0)
    {
        Error ("openvt:  TCSETAW failed");
        return (-1);
    }

    sigset(SIGUSR1, SIG_HOLD);	/* Hold till we are set up to service it. */
    signal(SIGUSR2, SIG_IGN);	/* Ignore until we need it later. */

    if (ioctl(condev, VT_ACTIVATE, vtnum) == -1)
    {
	Error("VT_ACTIVATE failed");
    }
    else 
    {
	if (ioctl(condev, VT_WAITACTIVE, 0) == -1)
	    Error("VT_WAITACTIVE failed");
    }
    vtmode.mode = VT_PROCESS;
    vtmode.relsig = SIGUSR1;
    vtmode.acqsig = SIGUSR2;
    vtmode.frsig = SIGUSR2;

    if (ioctl(condev, VT_SETMODE, &vtmode) == -1)
    {
	Error("VT_SETMODE VT_PROCESS failed");
	/* attexit (1); */
    }

    if (ioctl(condev, KDSETMODE, KD_GRAPHICS) == -1)
    {
	Error("KDSETMODE KD_GRAPHICS failed");
	attexit (-1);
    }

    return(condev);
}

/*
 * DDX - specific abort routine.  Called by AbortServer().
 */
void
AbortDDX()
{
	RestoreOutput ();
}

/* Called by GiveUp(). */
void
ddxGiveUp()
{
	resetmodes ();
}

int
ddxProcessArgument (argc, argv, i)
    int argc;
    char *argv[];
    int i;
{
    int num_consumed = 0;
    extern char *blackValue, *whiteValue;

    if(strcmp(argv[i], "-bp") == 0)
    {
	if(++i < argc)
	{
	    blackValue = (char *)argv[i];
	    num_consumed = 2;
	}
	else
	    UseMsgExit1();
    }
    else if(strcmp(argv[i], "-wp") == 0)
    {
	if(++i < argc)
	{
	    whiteValue = (char *)argv[i];
	    num_consumed = 2;
	}
	else
	    UseMsgExit1();
    }
    return (num_consumed);
}

void
ddxUseMsg()
{
    ErrorF("-bp color            BlackPixel for screen\n");
    ErrorF("-wp color            WhitePixel for screen\n");
}
