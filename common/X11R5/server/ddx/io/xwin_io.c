#ident	"@(#)siserver:ddx/io/xwin_io.c	1.8"
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

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#ifdef SVR4
#include <sys/stream.h>
#include <sys/fcntl.h>
#endif /* SVR4 */
#ifndef SVR4
#include <fcntl.h>
#endif
#include <signal.h>
#include <sys/param.h>

#include "X.h"
#define	 NEED_EVENTS
#include "Xproto.h"
#include "scrnintstr.h"
#include "cursorstr.h"
#include "pixmap.h"
#include "input.h"
#include "windowstr.h"
#include "regionstr.h"
#include "resource.h"

/* #include "att.h" */

#include "xwin_io.h"
#include "keynames.h"
#include "osdep.h"

#define EVC
#include <sys/mouse.h>
#include <sys/vt.h>
#include <sys/at_ansi.h>
#include <sys/ascii.h>
#include <sys/kd.h>
#include <sys/xque.h>
#include <setjmp.h>
#include <termio.h>

#ifdef XTESTEXT1
#define XTestSERVER_SIDE
#include "xtestqueue.h"
#include "xtestext1.h"
extern xqEvent		*XTestDeQueue();
extern int		on_steal_input;
extern Bool		XTestStealKeyData();
extern void		XTestStealMotionData();
static Bool		XTestKernelQueueFlag;

XTestQueue	xTestQ = { (XTestQueueNode *)0,
			   (XTestQueueNode *)0 };
#endif

#define E0      0xe0
#define E1      0xe1
#define MOTION_BUFFER_SIZE	0	
/*
 *  Following four lines declare the corresponding functions as static
 *  to avoid -Xc warning message
 */
static int  	i386GetMotionEvents();
static void 	i386ChangePointerControl(),
		i386ChangeKeyboardControl(),
		i386Bell();

extern void		UseMsg(),
				UseMsgExit1();

unchar	ledsdown = 0,				/* LED keys that are down */
	ledsup = LED_NUM|LED_CAP|LED_SCR;	/* Initially all up */

extern int		xsig;
extern int		condev;			/* fd of the console */
xqEventQueue		*queue;
long			lastEventTime = 0;
Bool			input_init_time = 1;
int			qLimit; 
static DevicePtr	i386Keyboard;
static DevicePtr	i386Pointer;

/* XTESTEXT1 Macros **/
#ifdef XTESTEXT1
#define XTEST_STEALKEYDATA(Action) { \
	if (on_steal_input)				\
		XTestStealKeyData( xtest_map_key( key ), \
		Action, 0, mouse->x, mouse->y); \
}

#define XTEST_MOTIONDATA1() \
	if ( !on_steal_input || XTestStealKeyData( x.u.u.detail, \
		x.u.u.type, 0, mouse->x, mouse->y ))

#define XTEST_STEALMOTIONDATA() { \
	if ( on_steal_input ) \
	    XTestStealMotionData( mouse->x - lastmouse.x, \
		mouse->y - lastmouse.y, 0, lastmouse.x, lastmouse.y ); \
}
#else
#define XTEST_STEALKEYDATA(Action)
#define XTEST_MOTIONDATA1()
#define XTEST_STEALMOTIONDATA()
#endif

#define SETBUTTONINFO(button_bit, value) \
	if (changes&button_bit) { \
	    if (current_state&button_bit) \
		x.u.u.type = ButtonRelease; \
	    else \
		x.u.u.type = ButtonPress; \
					  \
	    x.u.u.detail = value; \
	    XTEST_MOTIONDATA1() \
	        (*i386Pointer->processInputProc)(&x, i386Pointer, 1); \
	}
/* XTESTEXT1 Macros **/

xqCursorPtr	mouse = (xqCursorPtr)NULL;  /* current mouse position */
static ushort	length, tone;		/* For kd bell (initialized via DIX) */
char 		*blackValue = NULL,
		*whiteValue = NULL;

/*
 * This table is used to translate keyboard scan codes to ASCII character
 * sequences for the AT386 keyboard/display driver.  It is the default table,
 * and may be changed with system calls.
 */

/* Define key remappings using KEY to index into new mapping */

#define KEY(i) KeyMap.key[i].map[1]

keymap_t KeyMap;	/* This keymap will contain possible changed */
			/* keyboard mapping to be compared with */
			/* the default keyboard map (below). */
#include "keytable.h"

void
SetKbdLeds(ledson)
unchar	ledson;
{
	if(ioctl(condev, KDSETLED, ledson) < 0)
		ErrorF("ioctl(KDSETLED) failed\n");
}

unchar
GetKbdLeds()
{
	unchar	c;
	if(ioctl(condev, KDGETLED, &c) < 0){
		ErrorF("ioctl(KDGETLED) failed\n");
		return(0);
	}
	return(c);
}

/* ARGSUSED */
int
i386MouseProc(pDev, onoff, argc, argv)
    DevicePtr pDev;
    int onoff, argc;
    char *argv[];
{
    BYTE map[4];

    i386Pointer = pDev;

    switch (onoff)
    {
	case DEVICE_INIT: 
	    /* Make sure there is only one pointer device at this time */
	    if (pDev != LookupPointerDevice()) {
		ErrorF ("Cannot open non-system mouse\n");
		return (!Success);
	    }

	    /*
	     * The only private data will be the screen ptr
	     * for now (use screens[0]?).
	     */
	    pDev->devicePrivate = (pointer) screenInfo.screens[0]; 
	    map[1] = 1;
	    map[2] = 2;
	    map[3] = 3;
	    InitPointerDeviceStruct(
		pDev, map, 3, i386GetMotionEvents,
		i386ChangePointerControl, MOTION_BUFFER_SIZE);

	    SetInputCheck(&queue->xq_head, &queue->xq_tail);
	    break;
	case DEVICE_ON: 
	    pDev->on = TRUE;
	    /*
	     * We use shared memory for our mouse and keyboard devices,
	     * not selectable file descriptors...
	     * AddEnabledDevice(indev);
	     */
	    break;
	case DEVICE_OFF: 
	    pDev->on = FALSE;
	    break;
	case DEVICE_CLOSE: 
	    break;
    }
    return Success;
}

#include "setjmp.h"

char			PendingInput = 0;
extern jmp_buf		startagain;
extern char		ServerAsleep;

void
catch_xsig(sig)
int	sig;
{
	extern long LastSelectMask[];

	/* ErrorF("signal %d received\n", sig); */

	if (sig != SIGTRAP && sig != SIGILL)
		signal(sig, catch_xsig);

	/*
	 * If we were in WaitForSomething(), select()ing for a non-zero
	 * amount of time, then abort the sleeping by jumping back to the
	 * top of Dispatch().
	 */
	if (ServerAsleep) {
		ServerAsleep = 0;
		longjmp(startagain, /*arbitrary*/1);
	}

	/*
	 * Set the global flag to signify that there is kbd/mouse input
	 * waiting to be processed.
	 */
	PendingInput = 1;
}

/* ARGSUSED */
int
i386KeybdProc(pDev, onoff, argc, argv)
    DevicePtr pDev;
    int onoff, argc;
    char *argv[];
{
    KeySymsRec keySyms;
    CARD8 modMap[MAP_LENGTH];
    switch (onoff)
    {
	case DEVICE_INIT: 
	    i386Keyboard = pDev;
	    GetI386Mappings( &keySyms, modMap);
	    InitKeyboardDeviceStruct(
		    i386Keyboard, &keySyms, modMap, i386Bell,
		    i386ChangeKeyboardControl);
	    Xfree((pointer)keySyms.map);
	    signal(xsig, catch_xsig);
	    break;
	case DEVICE_ON: 
	    pDev->on = TRUE;
	    /*
	     * We use shared memory for our mouse and keyboard devices,
	     * not selectable file descriptors...
	     * AddEnabledDevice(condev);
	     */
	    break;
	case DEVICE_OFF: 
	    /* NOTE: This case doesn't seem to be used anywhere in the server*/
	    pDev->on = FALSE;
	    break;
	case DEVICE_CLOSE: 
	    break;
    }
    return Success;
}

/* static void */
void
GetKDKeyMap (fd)
int fd;			/* fd for open ("console") */
{
    int i, j, k;

    KeyMap.n_keys = NUM_KEYS;
    if (ioctl (fd, GIO_KEYMAP, &KeyMap) == -1) {
	/* ioctl failed, use default map */
	for (i=0; i<KeyMap.n_keys; i++) {
	    KeyMap.key[i].map[0] = KDKeyMap.key[i].map[0];
	    /* Use second element of map to store key index value */
	    KeyMap.key[i].map[1] = i;
	}
	return;
    }
    /* Look to see if the map the kernal gives us is the same as */
    /* the default keyboard map.  For every entry that is the same */
    /* save the index to this location in the second map element. */
    /* If values are different search for the new value in the */
    /* map and store the index to this location in the new map. */

    for (i=0; i<KeyMap.n_keys; i++) {
	/* Check to see if key in new key map is same as */
	/* the one in the default map. */
	if (KeyMap.key[i].spcl == KDKeyMap.key[i].spcl) {
	    for (k=0; k<NUM_STATES; k++) {
		if (KeyMap.key[i].map[k] != KDKeyMap.key[i].map[k]) {
		    break;
		}
	    }
	    if (k == NUM_STATES) {
		/* Keys are the same */
	        KeyMap.key[i].map[1] = i;
		continue;
	    }
	}
	/* Key has been remapped */
	for (j=0; j<KeyMap.n_keys; j++) {
	    if (KeyMap.key[i].spcl == KDKeyMap.key[j].spcl) {
		for (k=0; k<NUM_STATES; k++) {
		    if (KeyMap.key[i].map[k] != KDKeyMap.key[j].map[k]) {
			break;
		    }
		}
		if (k == NUM_STATES) {
		    /* This key matches the new key. */
		    KeyMap.key[i].map[1] = j;
		    break;
		}
	    }
	}
	if (j == KeyMap.n_keys) {	/* No match, don't remap */
	    KeyMap.key[i].map[1] = i;
        }
    }
}

static int
TranslateE0Keycodes (key, x)
BYTE *key;
xEvent *x;
{

    switch (*key) {
	case K_Shift_L:
	case K_Shift_R: {
            /* Ignore E0 {left,right} shift */
            return FALSE;
	}
	case K_Enter: {
	    *key = x->u.u.detail = KEY(K_KP_Enter);
	    break;
	}
	case K_Control_L: {
	    *key = x->u.u.detail = KEY(K_Control_R);
	    break;
	}
	case K_Alt_L: {
	    *key = x->u.u.detail = KEY(K_Alt_R);
	    break;
	}
        case K_Scroll_Lock: {
            *key = x->u.u.detail = KEY(K_Pause);
            break;
	}
        case K_slash: {
            *key = x->u.u.detail = KEY(K_KP_Divide);
            break;
	}
        case K_Down: {
            *key = x->u.u.detail = KEY(K_ExDown);
            break;
	}
        case K_Right: {
            *key = x->u.u.detail = KEY(K_ExRight);
            break;
	}
        case K_Left: {
            *key = x->u.u.detail = KEY(K_ExLeft);
            break;
	}
        case K_Up: {
            *key = x->u.u.detail = KEY(K_ExUp);
            break;
	}
        case K_Insert: {
            *key = x->u.u.detail = KEY(K_Ex_Insert);
            break;
	}
        case K_Home: {
            *key = x->u.u.detail = KEY(K_Ex_Home);
            break;
	}
        case K_Prior: {
            *key = x->u.u.detail = KEY(K_Ex_Prior);
	    break;
        }
        case K_Delete: {
            *key = x->u.u.detail = KEY(K_Ex_Delete);
            break;
	}
        case K_End: {
            *key = x->u.u.detail = KEY(K_Ex_End);
            break;
	}
        case K_Next: {
            *key = x->u.u.detail = KEY(K_Ex_Next);
            break;
	}
        case K_KP_Multiply: {
            *key = x->u.u.detail = KEY(K_Ex_Print);
            break;
	}
    }
    return TRUE;
}

static int
TranslateKeycodes (key, x, pE)
BYTE *key;
xEvent *x;
xqEvent *pE;
{
    static BYTE lastKeycode = 0;	/* last key encountered */

    if (pE->xq_code == E0) {
	lastKeycode = E0;		/* Indicate E0 encountered */
	return FALSE;
    }
    else if (pE->xq_code == E1) {
	lastKeycode = E1;		/* Indicate E1 encountered */
	return FALSE;
    }
    switch (lastKeycode) {
	case E0: {
	    lastKeycode = 0;
	    if (TranslateE0Keycodes (key, x) == FALSE) {
		return FALSE;
	    }
	    return TRUE;
	}
	case E1: {		/* Process E1 */
	    lastKeycode = pE->xq_code&0177;
	    return FALSE;
	}
	case K_Control_L: {
	    lastKeycode = 0;
	    if (*key != K_Num_Lock) {
		return FALSE;
	    }
	    *key = x->u.u.detail = KEY(K_Pause);
	    return TRUE;
	}
	default: {
	    *key = x->u.u.detail = KEY(*key);
	    lastKeycode = 0;
	    return TRUE;
	}
    }
}

/*
 *	Assuming all mouse buttons were up when we started 
 */

#define LEFT_BUTTON_BIT		4	
#define MIDDLE_BUTTON_BIT	2	
#define RIGHT_BUTTON_BIT	1

long
GetTimeInMillis()
{
    return(queue->xq_curtime);
}

/*****************
 * ProcessInputEvents:
 *    processes all the pending input events
 *****************/

extern int screenIsSaved;

#ifdef XTESTEXT1
static ushort	mouse_state = 07;
#endif

void
ProcessInputEvents(flag)
    char	flag;
{
#ifndef XTESTEXT1
    static ushort	mouse_state = 07;
#endif

    register ushort	current_state, changes;
    xqEvent	* pE;
    xEvent		x;
    Bool		mouseMoved;
    extern long		ClientsBlocked[];
    DevicePtr		pDev;
    extern int xwinDisplayCursor();
    extern Bool		sdbMouse;

    pDev = LookupPointerDevice();

    for (;;)
    {
	xqCursorRec	lastmouse;	/* the previous mouse position */

/*
	if (ANYSET(ClientsBlocked))
		return;
*/
    {
#ifdef XTESTEXT1
	if(queue->xq_head == queue->xq_tail
	   && XTestQueueEmpty( xTestQ ))
#else
	if(queue->xq_head == queue->xq_tail)
#endif
	    break;
	queue->xq_sigenable = 0;
	if (screenIsSaved == SCREEN_SAVER_ON)
	    SaveScreens(SCREEN_SAVER_OFF, ScreenSaverReset);

#ifdef	XTESTEXT1
	if ( XTestQueueEmpty( xTestQ )
	    || queue->xq_head != queue->xq_tail
	    && queue->xq_events[queue->xq_head].xq_time
	    < xTestQ.head->xqevent.xq_time )
	{
	    XTestKernelQueueFlag = TRUE;
	    pE = &queue->xq_events[queue->xq_head];
	}
	else
	    pE = XTestDeQueue();
#else
	pE = &queue->xq_events[queue->xq_head];
#endif
    }
	if(queue->xq_curtime <= pE->xq_time)
		queue->xq_curtime = pE->xq_time+1;

	x.u.keyButtonPointer.time=lastEventTime=(long) pE->xq_time;


	switch (pE->xq_type) {

	  case XQ_KEY:
	    {			/* better be a key */
		BYTE key;

		x.u.keyButtonPointer.rootX = mouse->x; /* necessary? */
		x.u.keyButtonPointer.rootY = mouse->y; /* necessary? */
		key = x.u.u.detail = pE->xq_code&0177;

		if (pE->xq_code&0200) {
		    x.u.u.type = KeyRelease;
                    if (TranslateKeycodes (&key, &x, pE) == FALSE) {
			XTEST_STEALKEYDATA(KeyRelease);
                        goto EndOfLoop;
                    }
		    if (key == K_Num_Lock) {
			if ((ledsup&LED_NUM) == 0) {
			    ledsup	 |= LED_NUM;
			    ledsdown &= ~LED_NUM;
			}
			else {
			    ledsup &= ~LED_NUM;
			    XTEST_STEALKEYDATA(KeyRelease);
			    goto EndOfLoop;
			}
		    }
		    else if(key == K_Caps_Lock){
			if((ledsup&LED_CAP) == 0){
			    ledsup	 |= LED_CAP;
			    ledsdown &= ~LED_CAP;
			}
			else {
			    ledsup &= ~LED_CAP;
			    XTEST_STEALKEYDATA(KeyRelease);
			    goto EndOfLoop;
			}
		    }
		      else if(key == K_Scroll_Lock){
		          if((ledsup&LED_SCR) == 0){
		              ledsup	 |= LED_SCR;
		              ledsdown &= ~LED_SCR;
		          }
			  else {
			      ledsup &= ~LED_SCR;
			      XTEST_STEALKEYDATA(KeyRelease);
			      goto EndOfLoop;
			  }
		      }
	
#ifdef XTESTEXT1
		    if ( !on_steal_input
			|| XTestStealKeyData( xtest_map_key( key ),
					     KeyRelease, 0,
					     mouse->x, mouse->y ))
#endif
			ProcessI386Input(&x, i386Keyboard);
		}
		else {
		    x.u.u.type = KeyPress;
                    if (TranslateKeycodes (&key, &x, pE) == FALSE) {
			XTEST_STEALKEYDATA(KeyRelease);
                        goto EndOfLoop;
                    }
		    if(key == K_Num_Lock){
			if((ledsdown&LED_NUM) == 0)
			    ledsdown |= LED_NUM;
			else
			{
			    XTEST_STEALKEYDATA(KeyPress);
			    goto EndOfLoop;
			}
		    }
		    else if(key == K_Caps_Lock){
			if((ledsdown&LED_CAP) == 0)
			    ledsdown |= LED_CAP;
			else
			{
			    XTEST_STEALKEYDATA(KeyPress);
			    goto EndOfLoop;
			}
		    }
		      else if(key == K_Scroll_Lock){
			  if((ledsdown&LED_SCR) == 0)
			      ledsdown |= LED_SCR;
			  else {
			      XTEST_STEALKEYDATA(KeyPress);
			      goto EndOfLoop;
			  }
		      }
	
#ifdef XTESTEXT1
		    if ( !on_steal_input
			|| XTestStealKeyData( xtest_map_key( key ),
					     KeyPress, 0,
					     mouse->x, mouse->y ))
#endif
			ProcessI386Input(&x, i386Keyboard);
		}
	    }
	    break;

	  case XQ_MOTION:
	    /*
             * rodent deltas (and maybe button state change)
             */
	    lastmouse = *mouse;
	    mouse->x += (short) pE->xq_x; /* update the mouse position */
	    mouse->y += (short) pE->xq_y;


	    if (mouse->x != lastmouse.x || mouse->y != lastmouse.y) {
		XTEST_STEALMOTIONDATA();
	    	miPointerDeltaCursor(screenInfo.screens[0],
			mouse->x - lastmouse.x , mouse->y - lastmouse.y, TRUE);
	    } 

	    /* (fall through to check button state) */
	  case XQ_BUTTON:
	    /* button state change (no rodent deltas) */
	    /* Do we need to generate a button event ? */
	    current_state = pE->xq_code&07;
	    changes	  = current_state^mouse_state;
	    if (changes)
	    {
		x.u.keyButtonPointer.rootX = mouse->x; /* necessary? */
		x.u.keyButtonPointer.rootY = mouse->y; /* necessary? */
		SETBUTTONINFO(LEFT_BUTTON_BIT, 1);
		SETBUTTONINFO(MIDDLE_BUTTON_BIT, 2);
		SETBUTTONINFO(RIGHT_BUTTON_BIT, 3);
		mouse_state = current_state; 
	    }
	    break;

	  default:
	    ErrorF("Bad kbd/mouse queue event type: %d\n", pE->xq_type);
	    break;
	}
	    
      EndOfLoop:
	PendingInput = 0;
	if (flag) {
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
#ifdef SDB_MOUSE
	    if (sdbMouse)
	    	queue->xq_sigenable = 0;
	    else
#endif
	    	queue->xq_sigenable = 1;
	}
#ifdef XTESTEXT1
	if ( XTestKernelQueueFlag )
#endif
	{
	    queue->xq_head++;
	    queue->xq_head %= qLimit;
	}
    }
}


#ifdef DEBUG
prdebug(x)
    xEvent	x;
{
    char	*what, *sense;
    char	temp[16];

    switch (x.u.u.type) {
	case KeyPress:
	    sense = "KeyPress";
	    break;
	case KeyRelease:
	    sense = "KeyRelease";
	    break;
	case ButtonPress:
	    sense = "ButtonPress";
	    break;
	case ButtonRelease:
	    sense = "ButtonRelease";
	    break;
	default:
	    sense = "MotionNotify";
	    break;
    }

    switch (x.u.u.detail) {
	case 3:
	    what = "RIGHT_BUTTON";
	    break;
	case 2:
	    what = "MIDDLE_BUTTON";
	    break;
	case 1:
	    what = "LEFT_BUTTON";
	    break;
	case 0:
	    what = "MOVEMENT";
	    break;
	default:
	    sprintf(temp, "KEY_%d", x.u.u.detail);
	    what = temp;
	    break;
    }

    ErrorF("%s: %s at (%d, %d)\n", what, sense,
	x.u.keyButtonPointer.rootX, x.u.keyButtonPointer.rootY);
}
#endif

long
TimeSinceLastInputEvent()
{
    time_t elapsed;

    /*
     * NOTE: queue->xq_curtime is being used here instead of the system call
     * GetTimeInMillis().  Some precision in the timing is being sacrificed
     * as the resolution of queue->xq_curtime can be pretty crude.  Because
     * of this, it's quite possible for xq_curtime to lag behind lastEventTime.
     */
    if (lastEventTime == 0) {
	lastEventTime = (long) queue->xq_curtime;
    }
    if ( (elapsed = queue->xq_curtime - (time_t)lastEventTime) > 0)
	return (elapsed);
    else
	return(0);
}
long
KKTimeSinceLastInputEvent()
{
    time_t elapsed;

    /*
     * NOTE: queue->xq_curtime is being used here instead of the system call
     * GetTimeInMillis().  Some precision in the timing is being sacrificed
     * as the resolution of queue->xq_curtime can be pretty crude.  Because
     * of this, it's quite possible for xq_curtime to lag behind lastEventTime.
     */
    if (lastEventTime == 0) {
	lastEventTime = (long) queue->xq_curtime;
	return (0);
    }
    else if ((elapsed = queue->xq_curtime - (time_t)lastEventTime) < 0)
	return (0);
    else
	return ((long) elapsed);
}

/* ARGSUSED */
static void
i386Bell(loud, pDevice)
    int loud;
    DevicePtr pDevice;
{
    if (loud > 0) {
	/* Ring the kd bell - no software volume control;
	 * bell parameters: length 0x____0000, tone 0x0000____.
	 * Our old fixed setting was length: 150, tone: 500.
	 */

	ioctl (condev, KDMKTONE, (long)(length << 16 | tone));
    }
}

/* ARGSUSED */
static void
i386ChangeKeyboardControl(pDevice, ctrl)
    DevicePtr pDevice;
    KeybdCtrl *ctrl;
{
    /* We don't support changeable bell volume, keyclick, autorepeat, or LEDs */

    length = (ushort) ctrl->bell_duration;
    tone = (ushort) ctrl->bell_pitch;
}

/* ARGSUSED */
static void
i386ChangePointerControl(pDevice, ctrl)
    DevicePtr pDevice;
    PtrCtrl   *ctrl;
{
    /* We don't support changeable mouse parameters (i.e., acceleration) */
}

/* ARGSUSED */
static int
i386GetMotionEvents(buff, start, stop)
    CARD32 start, stop;
    xTimecoord *buff;
{
    /* We don't support a motion buffer */
    return 0;
}

#ifdef	XTESTEXT1

/*
** Very Hardware Dependent: assuming 8-bit, 2's comp signed chars
*/

#define MAX_SIGNED_CHAR		 127
#define MIN_SIGNED_CHAR		-128

static xqEvent *
XTestDeQueue()
{
	extern XTestQueueNode	*XTestQueueNodeFreeList;
	XTestQueueNode		*node;
	xqEvent			*event;

	XTestKernelQueueFlag = FALSE;
	node = xTestQ.head;
	event = &node->xqevent;

	if ( event->xq_type == XQ_MOTION )
	{
		Bool	skip_DeQ = FALSE;
		int	delta_x;
		int	delta_y;

		event->xq_code = mouse_state;

		delta_x = node->variant1 - mouse->x;
		delta_y = node->variant2 - mouse->y;

		if ( delta_x > MAX_SIGNED_CHAR )
		{
			event->xq_x = MAX_SIGNED_CHAR;
			skip_DeQ = TRUE;
		}
		else if ( delta_x < MIN_SIGNED_CHAR )
		{
			event->xq_x = MIN_SIGNED_CHAR;
			skip_DeQ = TRUE;
		}
		else
			event->xq_x = delta_x;

		if ( delta_y > MAX_SIGNED_CHAR )
		{
			event->xq_y = MAX_SIGNED_CHAR;
			skip_DeQ = TRUE;
		}
		else if ( delta_y < MIN_SIGNED_CHAR )
		{
			event->xq_y = MIN_SIGNED_CHAR;
			skip_DeQ = TRUE;
		}
		else
			event->xq_y = delta_y;

		if ( skip_DeQ )
			return event;
	}
	else if ( event->xq_type == XQ_BUTTON )
	{
		if ( node->variant1 == XTestKEY_UP )
			event->xq_code = mouse_state | node->variant2;
		else
			event->xq_code = mouse_state & ~node->variant2;
	}
	else	/* xq_type == XQ_KEY */
	{
		switch( event->xq_code )
		{
			case K_Num_Lock:

				if ( ledsdown & LED_NUM )
					SetKbdLeds( ledsdown & ~LED_NUM );
				else
					SetKbdLeds( ledsdown | LED_NUM );

				break;

			case K_Caps_Lock:

				if ( ledsdown & LED_CAP )
					SetKbdLeds( ledsdown & ~LED_CAP );
				else
					SetKbdLeds( ledsdown | LED_CAP );

				break;

/*
			case K_Scroll_Lock:

				if ( ledsdown & LED_SCR )
					SetKbdLeds( ledsdown & ~LED_SCR );
				else
					SetKbdLeds( ledsdown | LED_SCR );

				break;
*/
			default:

				break;
		}
	}

	/*
	** Remove head of xTestQ, place on free list
	** and return pointer to xqEvent.
	*/

	xTestQ.head = node->next;
	node->next = XTestQueueNodeFreeList;
	XTestQueueNodeFreeList = node;
	return event;
}

static int
xtest_map_key( key )
{
#ifdef INGNORE_UNMAPPED_KEYS	/* Don't Blame me for the misspelling */

	/*
	** The following codes are not mapped:
	**
	**	70: ScrollLock
	**	84: Alt_SysRq
	**	85: ??
	**	86: ??
	*/

	if ( keycode == 70 || ( keycode >= 84 && keycode <= 86 ))
		return key;
#endif

	return key + TABLE_OFFSET;
}

#endif

void
constrainXY2Scr (px, py)
   short           *px;
   short           *py;
{
#ifdef XTESTEXT1
            if ( *px < 0 )
                 *px = 0;
            else if ( *px >= (short) screenInfo.screens[0]->width)
                *px = (short) screenInfo.screens[0]->width -1;

            if ( *py < 0 )
                 *py = 0;
            else if ( *py >= (short) screenInfo.screens[0]->height)
                 *py = (short) screenInfo.screens[0]->height -1;
#endif
}

