#ident	"@(#)siserver:include/input.h	1.4"

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/************************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
                        All Rights Reserved
********************************************************/

#ifndef INPUT_H
#define INPUT_H

#include "misc.h"

#define DEVICE_INIT	0
#define DEVICE_ON	1
#define DEVICE_OFF	2
#define DEVICE_CLOSE	3

#define MAP_LENGTH	256
#define DOWN_LENGTH	32	/* 256/8 => number of bytes to hold 256 bits */
#define NullGrab ((GrabPtr)NULL)
#define PointerRootWin ((WindowPtr)PointerRoot)
#define NoneWin ((WindowPtr)None)
#define NullDevice ((DevicePtr)NULL)

#ifndef FollowKeyboard
#define FollowKeyboard 		3
#endif
#ifndef FollowKeyboardWin
#define FollowKeyboardWin  ((WindowPtr) FollowKeyboard)
#endif
#ifndef RevertToFollowKeyboard
#define RevertToFollowKeyboard	3
#endif

typedef unsigned long Leds;
typedef struct _OtherClients *OtherClientsPtr;
typedef struct _InputClients *InputClientsPtr;
typedef struct _GrabRec *GrabPtr;

typedef int (*DeviceProc)();
typedef void (*ProcessInputProc)();

typedef struct _DeviceRec {
    pointer	devicePrivate;
    ProcessInputProc processInputProc;
    ProcessInputProc realInputProc;
    Bool	on;			/* used by DDX to keep state */
} DeviceRec, *DevicePtr;

typedef struct {
    int			click, bell, bell_pitch, bell_duration;
    Bool		autoRepeat;
    unsigned char	autoRepeats[32];
    Leds		leds;
} KeybdCtrl;

typedef struct {
    KeySym  *map;
    KeyCode minKeyCode,
	    maxKeyCode;
    int     mapWidth;
} KeySymsRec, *KeySymsPtr;

typedef struct {
    int		num, den, threshold;
} PtrCtrl;

typedef struct {
    int         resolution, min_value, max_value;
    int         integer_displayed;
} IntegerCtrl;

typedef struct {
    int         max_symbols, num_symbols_supported;
    int         num_symbols_displayed;
    KeySym      *symbols;
} StringCtrl;

typedef struct {
    int         percent, pitch, duration;
} BellCtrl;

typedef struct {
    Leds        led_values;
} LedCtrl;

extern KeybdCtrl	defaultKeyboardControl;
extern PtrCtrl		defaultPointerControl;

extern DevicePtr AddInputDevice();
extern Bool EnableDevice();
extern void RegisterPointerDevice();
extern void RegisterKeyboardDevice();

#if ! SVR4	/* funNotUsedByATT, LookupKeyboardDevice */
extern Bool DisableDevice();
extern DevicePtr LookupKeyboardDevice();
#endif

extern DevicePtr LookupPointerDevice();

extern void ProcessPointerEvent();
extern void ProcessKeyboardEvent();

extern Bool InitKeyClassDeviceStruct();
extern Bool InitButtonClassDeviceStruct();
extern Bool InitFocusClassDeviceStruct();
extern Bool InitKbdFeedbackClassDeviceStruct();
extern Bool InitPtrFeedbackClassDeviceStruct();
extern Bool InitValuatorClassDeviceStruct();
extern Bool InitPointerDeviceStruct();
extern Bool InitKeyboardDeviceStruct();

extern void CloseDownDevices();

extern void WriteEventsToClient();

#endif /* INPUT_H */
