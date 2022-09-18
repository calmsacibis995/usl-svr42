/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:include/inputstr.h	1.3"

/************************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
                        All Rights Reserved
********************************************************/

/* $XConsortium: inputstr.h,v 1.23 89/10/03 19:50:03 rws Exp $ */

#ifndef INPUTSTRUCT_H
#define INPUTSTRUCT_H

#include "input.h"
#include "window.h"
#include "dixstruct.h"
#include "screenint.h"

#define BitIsOn(ptr, bit) (((BYTE *) (ptr))[(bit)>>3] & (1 << ((bit) & 7)))

#define SameClient(obj,client) \
	(CLIENT_BITS((obj)->resource) == (client)->clientAsMask)

#define MAX_DEVICES	7

#define EMASKSIZE	MAX_DEVICES

/* Kludge: OtherClients and InputClients must be compatible, see code */

typedef struct _OtherClients {
    OtherClientsPtr	next;
    XID			resource; /* id for putting into resource manager */
    Mask		mask;
} OtherClients;

typedef struct _InputClients {
    InputClientsPtr	next;
    XID			resource; /* id for putting into resource manager */
    Mask		mask[EMASKSIZE];
} InputClients;

typedef struct _OtherInputMasks {
    Mask		deliverableEvents[EMASKSIZE];
    Mask		inputEvents[EMASKSIZE];
    Mask		dontPropagateMask[EMASKSIZE];
    InputClientsPtr	inputClients;
} OtherInputMasks;

typedef struct _DeviceIntRec *DeviceIntPtr;

/*
 * The following structure gets used for both active and passive grabs. For
 * active grabs some of the fields (e.g. modifiers) are not used. However,
 * that is not much waste since there aren't many active grabs (one per
 * keyboard/pointer device) going at once in the server.
 */

#define MasksPerDetailMask 8		/* 256 keycodes and 256 possible
						modifier combinations, but only	
						3 buttons. */

  typedef struct _DetailRec {		/* Grab details may be bit masks */
	unsigned short exact;
	Mask *pMask;
  } DetailRec;

  typedef struct _GrabRec {
    GrabPtr		next;		/* for chain of passive grabs */
    XID			resource;
    DeviceIntPtr	device;
    WindowPtr		window;
    unsigned		ownerEvents:1;
    unsigned		keyboardMode:1;
    unsigned		pointerMode:1;
    unsigned		coreGrab:1;	/* grab is on core device */
    unsigned		coreMods:1;	/* modifiers are on core keyboard */
    CARD8		type;		/* event type */
    DetailRec		modifiersDetail;
    DeviceIntPtr	modifierDevice;
    DetailRec		detail;		/* key or button */
    WindowPtr		confineTo;	/* always NULL for keyboards */
    CursorPtr		cursor;		/* always NULL for keyboards */
    Mask		eventMask;
} GrabRec;

typedef struct _KeyClassRec {
    CARD8		down[DOWN_LENGTH];
    KeyCode 		*modifierKeyMap;
    KeySymsRec		curKeySyms;
    int			modifierKeyCount[8];
    CARD8		modifierMap[MAP_LENGTH];
    CARD8		maxKeysPerModifier;
    unsigned short	state;
} KeyClassRec, *KeyClassPtr;

typedef struct _XAxisInfo {
    int		resolution;
    int		min_value;
    int		max_value;
} XAxisInfo, *XAxisInfoPtr;

typedef struct _ValuatorClassRec {
    int		 	(*GetMotionProc) ();
    int		 	numMotionEvents;
    WindowPtr    	motionHintWindow;
    XAxisInfoPtr 	axes;
    unsigned short	numAxes;
    unsigned short	*axisVal;
    CARD8	 	mode;
} ValuatorClassRec, *ValuatorClassPtr;

typedef struct _ButtonClassRec {
    CARD8		numButtons;
    CARD8		buttonsDown;	/* number of buttons currently down */
    unsigned short	state;
    Mask		motionMask;
    CARD8		down[DOWN_LENGTH];
    CARD8		map[MAP_LENGTH];
} ButtonClassRec, *ButtonClassPtr;

typedef struct _FocusClassRec {
    WindowPtr	win;
    int		revert;
    TimeStamp	time;
    WindowPtr	*trace;
    int		traceSize;
    int		traceGood;
} FocusClassRec, *FocusClassPtr;

typedef struct _ProximityClassRec {
    char	pad;
} ProximityClassRec, *ProximityClassPtr;

typedef struct _KbdFeedbackClassRec {
    void		(*BellProc) ();
    void		(*CtrlProc) ();
    KeybdCtrl	 	ctrl;
} KbdFeedbackClassRec, *KbdFeedbackPtr;

typedef struct _PtrFeedbackClassRec {
    void	(*CtrlProc) ();
    PtrCtrl	 ctrl;
} PtrFeedbackClassRec, *PtrFeedbackPtr;

typedef struct _IntegerFeedbackClassRec {
    void	(*CtrlProc) ();
    IntegerCtrl	 ctrl;
} IntegerFeedbackClassRec, *IntegerFeedbackPtr;

typedef struct _StringFeedbackClassRec {
    void	(*CtrlProc) ();
    StringCtrl	 ctrl;
} StringFeedbackClassRec, *StringFeedbackPtr;

typedef struct _BellFeedbackClassRec {
    void	(*BellProc) ();
    void	(*CtrlProc) ();
    BellCtrl	 ctrl;
} BellFeedbackClassRec, *BellFeedbackPtr;

typedef struct _LedFeedbackClassRec {
    void	(*CtrlProc) ();
    LedCtrl	 ctrl;
} LedFeedbackClassRec, *LedFeedbackPtr;

/* states for devices */

#define NOT_GRABBED		0
#define THAWED			1
#define THAWED_BOTH		2	/* not a real state */
#define FREEZE_NEXT_EVENT	3
#define FREEZE_BOTH_NEXT_EVENT	4
#define FROZEN			5	/* any state >= has device frozen */
#define FROZEN_NO_EVENT		5
#define FROZEN_WITH_EVENT	6
#define THAW_OTHERS		7

typedef struct _DeviceIntRec {
    DeviceRec	public;
    DeviceIntPtr next;
    TimeStamp	grabTime;
    Bool	startup;		/* true if needs to be turned on at
				          server intialization time */
    DeviceProc	deviceProc;		/* proc(DevicePtr, DEVICE_xx). It is
					  used to initialize, turn on, or
					  turn off the device */
    Bool	inited;			/* TRUE if INIT returns Success */
    GrabPtr	grab;			/* the grabber - used by DIX */
    struct {
	Bool		frozen;
	int		state;
	GrabPtr		other;		/* if other grab has this frozen */
	xEvent		*event;		/* saved to be replayed */
	int		evcount;
    } sync;
    Atom		type;
    char		*name;
    CARD8		id;
    CARD8		activatingKey;
    Bool		fromPassiveGrab;
    GrabRec		activeGrab;
    void		(*ActivateGrab)();
    void		(*DeactivateGrab)();
    KeyClassPtr		key;
    ValuatorClassPtr	valuator;
    ButtonClassPtr	button;
    FocusClassPtr	focus;
    ProximityClassPtr	proximity;
    KbdFeedbackPtr	kbdfeed;
    PtrFeedbackPtr	ptrfeed;
    IntegerFeedbackPtr	intfeed;
    StringFeedbackPtr	stringfeed;
    BellFeedbackPtr	bell;
    LedFeedbackPtr	leds;
} DeviceIntRec;

typedef struct {
    int			numDevices;	/* total number of devices */
    DeviceIntPtr	devices;	/* all devices turned on */
    DeviceIntPtr	off_devices;	/* all devices turned off */
    DeviceIntPtr	keyboard;	/* the main one for the server */
    DeviceIntPtr	pointer;
} InputInfo;

/* for keeping the events for devices grabbed synchronously */
typedef struct _QdEvent *QdEventPtr;
typedef struct _QdEvent {
    QdEventPtr		next;
    DeviceIntPtr	device;
    ScreenPtr		pScreen;	/* what screen the pointer was on */
    unsigned long	months;		/* milliseconds is in the event */
    xEvent		*event;
    int			evcount;
} QdEventRec;    

#endif /* INPUTSTRUCT_H */
