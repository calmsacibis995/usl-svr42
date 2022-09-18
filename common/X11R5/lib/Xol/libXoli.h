/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)shlib:libXoli.h	1.8"
#endif

#ifndef __Ol_libXoli_h__
#define __Ol_libXoli_h__

#ifdef SHARELIB

/* put all imported data */
#define applicationShellWidgetClass      (*_libXol_applicationShellWidgetClass)
#define compositeWidgetClass      (*_libXol_compositeWidgetClass)
#define daylight                  (*_libXol_daylight)
#define errno                     (*_libXol_errno)
#define _iob		  	  (*_libXol__iob)
#define _ctype  		  (*_libXol__ctype)
#define overrideShellWidgetClass  (*_libXol_overrideShellWidgetClass)
#define shellWidgetClass          (*_libXol_shellWidgetClass)
#define timezone                  (*_libXol_timezone)
#define topLevelShellWidgetClass  (*_libXol_topLevelShellWidgetClass)
#define transientShellWidgetClass (*_libXol_transientShellWidgetClass)
#define wmShellWidgetClass        (*_libXol_wmShellWidgetClass) 
#define _XtperDisplayList	  (*_libXol__XtperDisplayList)

/*
   These are external variables that are used to initialize internal
   variables and, at run time, fixed up with the real values.
*/
#ifndef USE_EXT_VARS
#define applicationShellClassRec  (_libXol_applicationShellClassRec)
#define rectObjClassRec           (_libXol_rectObjClassRec)
#define constraintClassRec        (_libXol_constraintClassRec)
#define shellClassRec             (_libXol_shellClassRec)
#define topLevelShellClassRec     (_libXol_topLevelShellClassRec)
#define transientShellClassRec    (_libXol_transientShellClassRec)
#define compositeClassRec         (_libXol_compositeClassRec)
#define widgetClassRec            (_libXol_widgetClassRec)
#define wmShellClassRec           (_libXol_wmShellClassRec)
#define _XtInherit                (_libXol__XtInherit)
#define _XtInheritTranslations    (_libXol__XtInheritTranslations)
#else
#define applicationShellClassRec  (*_libXol_applicationShellClassRec)
#define rectObjClassRec           (*_libXol_rectObjClassRec)
#define constraintClassRec        (*_libXol_constraintClassRec)
#define shellClassRec             (*_libXol_shellClassRec)
#define topLevelShellClassRec     (*_libXol_topLevelShellClassRec)
#define transientShellClassRec    (*_libXol_transientShellClassRec)
#define compositeClassRec         (*_libXol_compositeClassRec)
#define widgetClassRec            (*_libXol_widgetClassRec)
#define wmShellClassRec           (*_libXol_wmShellClassRec)
#define _XtInherit                (*_libXol__XtInherit)
#define _XtInheritTranslations    (*_libXol__XtInheritTranslations)
#endif

/* put all imported functions here */
#define XBell                          (*_libXol_XBell)
#define XChangeProperty                (*_libXol_XChangeProperty)
#define XChangeWindowAttributes        (*_libXol_XChangeWindowAttributes)
#define XCheckWindowEvent              (*_libXol_XCheckWindowEvent)
#define XClearArea                     (*_libXol_XClearArea)
#define XClearWindow                   (*_libXol_XClearWindow)
#define XClipBox                       (*_libXol_XClipBox)
#define XConvertSelection              (*_libXol_XConvertSelection)
#define XCopyArea                      (*_libXol_XCopyArea)
#define XCopyPlane                     (*_libXol_XCopyPlane)
#define XCreateBitmapFromData          (*_libXol_XCreateBitmapFromData)
#define XCreateFontCursor              (*_libXol_XCreateFontCursor)
#define XCreateGC                      (*_libXol_XCreateGC)
#define XCreatePixmap                  (*_libXol_XCreatePixmap)
#define XCreatePixmapFromBitmapData    (*_libXol_XCreatePixmapFromBitmapData)
#define XCreateRegion                  (*_libXol_XCreateRegion)
#define XDefaultDepth                  (*_libXol_XDefaultDepth)
#define XDefaultScreen                 (*_libXol_XDefaultScreen)
#define XDefaultScreenOfDisplay        (*_libXol_XDefaultScreenOfDisplay)
#define XDefineCursor                  (*_libXol_XDefineCursor)
#define XDeleteProperty                (*_libXol_XDeleteProperty)
#define XDisplayOfScreen               (*_libXol_XDisplayOfScreen)
#define XDrawImageString               (*_libXol_XDrawImageString)
#define XDrawLine                      (*_libXol_XDrawLine)
#define XDrawLines                     (*_libXol_XDrawLines)
#define XDrawRectangle                 (*_libXol_XDrawRectangle)
#define XDrawSegments                  (*_libXol_XDrawSegments)
#define XDrawString                    (*_libXol_XDrawString)
#define XFillPolygon                   (*_libXol_XFillPolygon)
#define XFillRectangle                 (*_libXol_XFillRectangle)
#define XFillRectangles                (*_libXol_XFillRectangles)
#define XFlush                         (*_libXol_XFlush)
#define XFree                          (*_libXol_XFree)
#define XFreeFont                      (*_libXol_XFreeFont)
#define XFreeGC                        (*_libXol_XFreeGC)
#define XFreePixmap                    (*_libXol_XFreePixmap)
#define XGetDefault                    (*_libXol_XGetDefault)
#define XGetFontProperty               (*_libXol_XGetFontProperty)
#define XGetImage                      (*_libXol_XGetImage)
#define XGetSelectionOwner             (*_libXol_XGetSelectionOwner)
#define XGetWMHints                    (*_libXol_XGetWMHints)
#define XGetWindowAttributes           (*_libXol_XGetWindowAttributes)
#define XGetWindowProperty             (*_libXol_XGetWindowProperty)
#define XGrabPointer                   (*_libXol_XGrabPointer)
#define XGrabServer                    (*_libXol_XGrabServer)
#define XIfEvent                       (*_libXol_XIfEvent)
#define XInternAtom                    (*_libXol_XInternAtom)
#define XIntersectRegion               (*_libXol_XIntersectRegion)
#define XKeysymToKeycode               (*_libXol_XKeysymToKeycode)
#define XKeysymToString                (*_libXol_XKeysymToString)
#define XLoadQueryFont                 (*_libXol_XLoadQueryFont)
#define XLookupString                  (*_libXol_XLookupString)
#define XMapRaised                     (*_libXol_XMapRaised)
#define XMapWindow                     (*_libXol_XMapWindow)
#define XMoveWindow                    (*_libXol_XMoveWindow)
#define XPutBackEvent                  (*_libXol_XPutBackEvent)
#define XPutImage                      (*_libXol_XPutImage)
#define XQueryPointer                  (*_libXol_XQueryPointer)
#define XRaiseWindow                   (*_libXol_XRaiseWindow)
#define XRectInRegion                  (*_libXol_XRectInRegion)
#define XRootWindow                    (*_libXol_XRootWindow)
#define XSelectInput                   (*_libXol_XSelectInput)
#define XSendEvent                     (*_libXol_XSendEvent)
#define XSetClipRectangles             (*_libXol_XSetClipRectangles)
#define XSetFillStyle                  (*_libXol_XSetFillStyle)
#define XSetInputFocus                 (*_libXol_XSetInputFocus)
#define XSetNormalHints                (*_libXol_XSetNormalHints)
#define XSetSelectionOwner             (*_libXol_XSetSelectionOwner)
#define XSetStipple                    (*_libXol_XSetStipple)
#define XSetWMHints                    (*_libXol_XSetWMHints)
#define XSetWindowBackground           (*_libXol_XSetWindowBackground)
#define XSetWindowBackgroundPixmap     (*_libXol_XSetWindowBackgroundPixmap)
#define XSetWindowBorder               (*_libXol_XSetWindowBorder)
#define XSetWindowBorderWidth          (*_libXol_XSetWindowBorderWidth)
#define XStoreBuffer                   (*_libXol_XStoreBuffer)
#define XStringToKeysym                (*_libXol_XStringToKeysym)
#define XSync                          (*_libXol_XSync)
#define XTextWidth                     (*_libXol_XTextWidth)
#define XTranslateCoordinates          (*_libXol_XTranslateCoordinates)
#define XUngrabPointer                 (*_libXol_XUngrabPointer)
#define XUngrabServer                  (*_libXol_XUngrabServer)
#define XUnionRectWithRegion           (*_libXol_XUnionRectWithRegion)
#define XUnmapWindow                   (*_libXol_XUnmapWindow)
#define XWarpPointer                   (*_libXol_XWarpPointer)
#define XrmQuarkToString               (*_libXol_XrmQuarkToString)
#define XrmStringToQuark               (*_libXol_XrmStringToQuark)
#define XtAddCallback                  (*_libXol_XtAddCallback)
#define XtAddCallbacks                 (*_libXol_XtAddCallbacks)
#define XtAddConverter                 (*_libXol_XtAddConverter)
#define XtAddEventHandler              (*_libXol_XtAddEventHandler)
#define XtAddExposureToRegion          (*_libXol_XtAddExposureToRegion)
#define XtAddGrab                      (*_libXol_XtAddGrab)
#define XtAddRawEventHandler           (*_libXol_XtAddRawEventHandler)
#define XtAddTimeOut                   (*_libXol_XtAddTimeOut)
#define XtAppAddConverter              (*_libXol_XtAppAddConverter)
#define XtAppCreateShell               (*_libXol_XtAppCreateShell)
#define XtAppErrorMsg                  (*_libXol_XtAppErrorMsg)
#define XtAppWarningMsg                (*_libXol_XtAppWarningMsg)
#define XtCallCallbacks                (*_libXol_XtCallCallbacks)
#define XtCallConverter                (*_libXol_XtCallConverter)
#define XtCallbackPopdown              (*_libXol_XtCallbackPopdown)
#define XtCalloc                       (*_libXol_XtCalloc)
#define XtConfigureWidget              (*_libXol_XtConfigureWidget)
#define XtConvert                      (*_libXol_XtConvert)
#define XtConvertCase                  (*_libXol_XtConvertCase)
#define XtCreateApplicationContext     (*_libXol_XtCreateApplicationContext)
#define XtCreateApplicationShell       (*_libXol_XtCreateApplicationShell)
#define XtCreateManagedWidget          (*_libXol_XtCreateManagedWidget)
#define XtCreatePopupShell             (*_libXol_XtCreatePopupShell)
#define XtCreateWidget                 (*_libXol_XtCreateWidget)
#define XtCreateWindow                 (*_libXol_XtCreateWindow)
#define _XtDefaultAppContext           (*_libXol__XtDefaultAppContext)
#define XtDestroyGC                    (*_libXol_XtDestroyGC)
#define XtDestroyWidget                (*_libXol_XtDestroyWidget)
#ifndef XtDisplayOfObject
#define XtDisplayOfObject              (*_libXol_XtDisplayOfObject)
#endif
#define XtDisplayToApplicationContext  (*_libXol_XtDisplayToApplicationContext)
#define XtError                        (*_libXol_XtError)
#define XtErrorMsg                     (*_libXol_XtErrorMsg)
#define XtFree                         (*_libXol_XtFree)
#define _XtFreeEventTable              (*_libXol__XtFreeEventTable)
#define XtGetApplicationResources      (*_libXol_XtGetApplicationResources)
#define XtGetGC                        (*_libXol_XtGetGC)
#define XtGetKeysymTable               (*_libXol_XtGetKeysymTable)
#define _XtGetProcessContext           (*_libXol__XtGetProcessContext)
#define XtGetSelectionValue            (*_libXol_XtGetSelectionValue)
#define XtGetSubresources              (*_libXol_XtGetSubresources)
#define XtGetSubvalues                 (*_libXol_XtGetSubvalues)
#define XtGetValues                    (*_libXol_XtGetValues)
#define XtGrabKey                      (*_libXol_XtGrabKey)
#define XtGrabKeyboard                 (*_libXol_XtGrabKeyboard)
#define XtHasCallbacks                 (*_libXol_XtHasCallbacks)
#define XtInitialize                   (*_libXol_XtInitialize)
#ifndef XtIsSensitive
#define XtIsSensitive                  (*_libXol_XtIsSensitive)
#endif
#define XtIsSubclass                   (*_libXol_XtIsSubclass)
#define _XtIsSubclassOf                 (*_libXol__XtIsSubclassOf)
#ifndef XtIsManaged
#define XtIsManaged                    (*_libXol_XtIsManaged)
#endif
#define XtMakeGeometryRequest          (*_libXol_XtMakeGeometryRequest)
#define XtMakeResizeRequest            (*_libXol_XtMakeResizeRequest)
#define XtMalloc                       (*_libXol_XtMalloc)
#define XtManageChild                  (*_libXol_XtManageChild)
#define XtManageChildren               (*_libXol_XtManageChildren)
#define XtMergeArgLists                (*_libXol_XtMergeArgLists)
#define XtMoveWidget                   (*_libXol_XtMoveWidget)
#define XtName                         (*_libXol_XtName)
#define XtOpenDisplay                  (*_libXol_XtOpenDisplay)
#define XtOverrideTranslations         (*_libXol_XtOverrideTranslations)
#define XtOwnSelection                 (*_libXol_XtOwnSelection)
#define XtParseTranslationTable        (*_libXol_XtParseTranslationTable)
#define XtPopdown                      (*_libXol_XtPopdown)
#define XtPopup                        (*_libXol_XtPopup)
#define XtQueryGeometry                (*_libXol_XtQueryGeometry)
#define XtRealizeWidget                (*_libXol_XtRealizeWidget)
#define XtRealloc                      (*_libXol_XtRealloc)
#define XtReleaseGC                    (*_libXol_XtReleaseGC)
#define XtRemoveAllCallbacks           (*_libXol_XtRemoveAllCallbacks)
#define XtRemoveCallback               (*_libXol_XtRemoveCallback)
#define XtRemoveEventHandler           (*_libXol_XtRemoveEventHandler)
#define XtRemoveGrab                   (*_libXol_XtRemoveGrab)
#define XtRemoveRawEventHandler        (*_libXol_XtRemoveRawEventHandler)
#define XtRemoveTimeOut                (*_libXol_XtRemoveTimeOut)
#define XtResizeWidget                 (*_libXol_XtResizeWidget)
#ifndef XtScreenOfObject
#define XtScreenOfObject               (*_libXol_XtScreenOfObject)
#endif
#define XtSetKeyboardFocus             (*_libXol_XtSetKeyboardFocus)
#define XtSetMappedWhenManaged         (*_libXol_XtSetMappedWhenManaged)
#define XtSetSensitive                 (*_libXol_XtSetSensitive)
#define XtSetSubvalues                 (*_libXol_XtSetSubvalues)
#define XtSetTypeConverter             (*_libXol_XtSetTypeConverter)
#define XtSetValues                    (*_libXol_XtSetValues)
#define _XtSortPerDisplayList          (*_libXol__XtSortPerDisplayList)
#define XtToolkitInitialize            (*_libXol_XtToolkitInitialize)
#define XtUngrabKey                    (*_libXol_XtUngrabKey)
#define XtUngrabKeyboard               (*_libXol_XtUngrabKeyboard)
#define XtUnmanageChildren             (*_libXol_XtUnmanageChildren)
#ifndef XtWindowOfObject
#define XtWindowOfObject               (*_libXol_XtWindowOfObject)
#endif
#define XtWindowToWidget               (*_libXol_XtWindowToWidget)
#define access                         (*_libXol_access)
#define bcopy                          (*_libXol_bcopy)
#define exit                           (*_libXol_exit)
#define fclose                         (*_libXol_fclose)
#define fgets                          (*_libXol_fgets)
#define fopen                          (*_libXol_fopen)
#define fprintf                        (*_libXol_fprintf)
#define fread                          (*_libXol_fread)
#define free                           (*_libXol_free)
#define fscanf                         (*_libXol_fscanf)
#define fseek                          (*_libXol_fseek)
#define ftell                          (*_libXol_ftell)
#define fwrite                         (*_libXol_fwrite)
#define getuid                         (*_libXol_getuid)
#define getgid                         (*_libXol_getgid)
#define malloc                         (*_libXol_malloc)
#define printf                         (*_libXol_printf)
#define realloc                        (*_libXol_realloc)
#define sprintf                        (*_libXol_sprintf)
#define sscanf                         (*_libXol_sscanf)
#define strcat                         (*_libXol_strcat)
#define strchr                         (*_libXol_strchr)
#define strcmp                         (*_libXol_strcmp)
#define strcpy                         (*_libXol_strcpy)
#define strlen                         (*_libXol_strlen)
#define strncpy                        (*_libXol_strncpy)
#define strrchr                        (*_libXol_strrchr)
#define strtok                         (*_libXol_strtok)
#define strtod                         (*_libXol_strtod)
#define strtol                         (*_libXol_strtol)
#define tempnam                        (*_libXol_tempnam)
/*
#define time                           (*_libXol_time)
#define times                          (*_libXol_times)
*/
#define tolower                        (*_libXol_tolower)
#define toupper                        (*_libXol_toupper)
#define vfprintf                       (*_libXol_vfprintf)
#define vsprintf                       (*_libXol_vsprintf)
#define ctermid				(*_libXol_ctermid)
#define cuserid				(*_libXol_cuserid)
#define fflush				(*_libXol_fflush)
#define fgetc				(*_libXol_fgetc)
#define fputc				(*_libXol_fputc)
#define fputs				(*_libXol_fputs)
#define gets				(*_libXol_gets)
#define getw				(*_libXol_getw)
#define memccpy				(*_libXol_memccpy)
#define memchr				(*_libXol_memchr)
#define memcmp				(*_libXol_memcmp)
#define memcpy				(*_libXol_memcpy)
#define memset				(*_libXol_memset)
#define puts				(*_libXol_puts)
#define putw				(*_libXol_putw)
#define rewind				(*_libXol_rewind)
#define scanf				(*_libXol_scanf)
#define setbuf				(*_libXol_setbuf)
#define setvbuf				(*_libXol_setvbuf)
#define strcspn				(*_libXol_strcspn)
#define strdup				(*_libXol_strdup)
#define strncat				(*_libXol_strncat)
#define strncmp				(*_libXol_strncmp)
#define strpbrk				(*_libXol_strpbrk)
#define strspn				(*_libXol_strspn)
#define system				(*_libXol_system)
#define tmpfile				(*_libXol_tmpfile)
#define tmpnam				(*_libXol_tmpnam)
#define tzset				(*_libXol_tzset)
#define ungetc				(*_libXol_ungetc)
#define abort				(*_libXol_abort)

extern int                   XBell();
extern int                   XChangeProperty();
extern int                   XChangeWindowAttributes();
extern int                   XClearArea();
extern int                   XClearWindow();
extern int                   XClipBox();
extern int                   XConvertSelection();
extern int                   XCopyArea();
extern int                   XCopyPlane();
extern int                   XDefaultDepth();
extern int                   XDefaultScreen();
extern int                   XDefineCursor();
extern int                   XDrawImageString();
extern int                   XDrawRectangle();
extern int                   XDrawSegments();
extern int                   XDrawString();
extern int                   XFillPolygon();
extern int                   XFillRectangles();
extern int                   XFlush();
extern int                   XDrawLine();
extern int                   XDrawLines();
extern int                   XFillRectangle();
extern int                   XFree();
extern int                   XFreeFont();
extern int                   XFreeGC();
extern int                   XFreePixmap();
extern int                   XGetWindowProperty();
extern int                   XGrabPointer();
extern int                   XGrabServer();
extern int                   XIfEvent();
extern int                   XIntersectRegion();
extern int                   XLookupString();
extern int                   XMapRaised();
extern int                   XMapWindow();
extern int                   XMoveWindow();
extern int                   XPutBackEvent();
extern int                   XPutImage();
extern int                   XRaiseWindow();
extern int                   XRectInRegion();
extern int                   XSelectInput();
extern int                   XSetClipRectangles();
extern int                   XSetFillStyle();
extern int                   XSetInputFocus();
extern int                  XSetNormalHints();
extern int                   XSetSelectionOwner();
extern int                   XSetStipple();
extern int                   XSetWMHints();
extern int                   XSetWindowBackground();
extern int                   XSetWindowBackgroundPixmap();
extern int                   XSetWindowBorder();
extern int                   XSetWindowBorderWidth();
extern int                   XStoreBuffer();
extern int                   XSync();
extern int                   XTextWidth();
extern int                   XUngrabPointer();
extern int                   XUngrabServer();
extern int                   XUnionRectWithRegion();
extern int                   XUnmapWindow();
extern int                   XWarpPointer();
extern void                  XtAddCallback();
extern void                  XtAddCallbacks();
extern void                  XtAddConverter();
extern void                  XtAddEventHandler();
extern void                  XtAddExposureToRegion();
extern void                  XtAddGrab();
extern void                  XtAddRawEventHandler();
extern void                  XtAppAddConverter();
extern void                  XtAppErrorMsg();
extern void                  XtAppWarningMsg();
extern void                  _XtFreeEventTable();
extern void                  XtGetApplicationResources();
extern void                  XtGrabKey();
extern int                   XtGrabKeyboard();
extern void		     XtReleaseGC();
extern int                   access();
extern void                  bcopy();
extern void                  exit();
extern void                  free();
extern char *                malloc();
extern int                   printf();
extern char *                realloc();
extern char *                strcat();
extern char *                strchr();
extern char *                strcpy();
extern char *                strncpy();
extern char *                strrchr();
extern char *                strtok();
extern double                strtod();
extern long                  strtol();
extern int                   sprintf();
extern int                   sscanf();
extern int                   strcmp();
extern int                   strlen();
extern int                   tolower();
extern int                   toupper();
extern int                  strncmp();
extern int                  setvbuf();
extern char *               strncat();
extern int                  fputs();
extern int                  fputc();
extern int                  fflush();
extern char *               strdup();
extern int                  puts();
extern int                  putw();
extern void                 rewind();
extern int                  ungetc();
extern int                  abort();
extern char *               gets();
extern int                  getw();
extern char *               cuserid();
extern int                  scanf();
extern char *               memccpy();
extern char *               memset();
extern char *               ctermid();
extern int                  fgetc();
extern void                 tzset();
extern int                  strspn();
extern char *               strpbrk();
extern void                 setbuf();
extern int                  strcspn();
extern char *               tmpnam();
extern int                  system();
extern char *               memchr();
extern int                  memcmp();
extern char *               memcpy();
extern char *               memccpy();
extern char *               memset();
extern char *               memchr();
extern int                  memcmp();
extern char *               memcpy();

#ifdef NOTUSED
/* put imported data here */
extern WidgetClass             applicationShellWidgetClass;
extern CompositeClassRec       compositeClassRec;
extern WidgetClass             compositeWidgetClass;
extern ConstraintClass         constraintClassRec;
extern RectObjClassRec         rectObjClassRec;
extern int                     daylight;
extern int                     errno;
extern FILE *		       _iob[];
extern unsigned char  		_ctype[];
extern WidgetClass             overrideShellWidgetClass;
extern ShellClassRec           shellClassRec;
extern WidgetClass             shellWidgetClass;
extern long                    timezone;
extern TopLevelShellClassRec   topLevelShellClassRec;
extern WidgetClass             topLevelShellWidgetClass;
extern WidgetClass             widgetClass;
extern WidgetClassRec          widgetClassRec;
extern WMShellClassRec         wmShellClassRec;
extern int		       _XtInheritTranslations;

/* put imported functions here */
extern Bool                  XCheckWindowEvent();
extern Pixmap                XCreateBitmapFromData();
extern Cursor                XCreateFontCursor();
extern GC                    XCreateGC();
extern Pixmap                XCreatePixmap();
extern Pixmap                XCreatePixmapFromBitmapData();
extern Region                XCreateRegion();
extern Screen *              XDefaultScreenOfDisplay();
extern Display *             XDisplayOfScreen();
extern char *                XGetDefault();
extern Bool                  XGetFontProperty();
extern XImage *              XGetImage();
extern Window                XGetSelectionOwner();
extern XWMHints              XGetWMHints();
extern Status                XGetWindowAttributes();
extern Atom                  XInternAtom();
extern KeyCode               XKeysymToKeycode();
extern char *                XKeysymToString();
extern XFontStruct *         XLoadQueryFont();
extern Bool                  XQueryPointer();
extern Window                XRootWindow();
extern Status                XSendEvent();
extern KeySym                XStringToKeysym();
extern Bool                  XTranslateCoordinates();
extern XrmString             XrmQuarkToString();
extern XrmQuark              XrmStringToQuark();
extern XtIntervalId          XtAddTimeOut();
extern void                  XtCallCallbacks();
extern Boolean               XtCallConverter();
extern char *                XtCalloc();
extern void                  XtConfigureWidget();
extern void                  XtConvert();
extern void                  XtConvertcase();
extern XtAppContext          XtCreateApplicationContext();
extern Widget                XtCreateApplicationShell();
extern Widget                XtCreateManagedWidget();
extern Widget                XtCreatePopupShell();
extern Widget                XtCreateWidget();
extern void                  XtCreateWindow();
extern XtAppContext          _XtDefaultAppContext();
extern void                  XtDestroyGC();
extern void                  XtDestroyWidget();
extern XtAppContext          XtDisplayToApplicationContext();
extern void                  XtError();
extern void                  XtErrorMsg();
extern void                  XtFree();
extern GC                    XtGetGC();
extern KeySym *              XtGetKeysymTable();
extern void                  XtGetSelectionValue();
extern void                  XtGetSubresources();
extern void                  XtGetSubvalues();
extern void                  XtGetValues();
extern XtCallbackStatus      XtHasCallbacks();
extern Widget                XtInitialize();
extern Boolean               XtIsSubclass();
extern Boolean               _XtIsSubclassOf();
extern XtGeometryResult      XtMakeGeometryRequest();
extern XtGeometryResult      XtMakeResizeRequest();
extern char *                XtMalloc();
extern void                  XtManageChild();
extern void                  XtManageChildren();
extern ArgList               XtMergeArgLists();
extern void                  XtMoveWidget();
extern String                XtName();
extern void                  XtOverrideTranslations();
extern Display               XtOpenDisplay();
extern Boolean               XtOwnSelection();
extern XtTranslations        XtParseTranslationTable();
extern void                  XtPopdown();
extern void                  XtPopup();
extern XtGeometryResult      XtQueryGeometry();
extern void                  XtRealizeWidget();
extern char *                XtRealloc();
extern void		     XtReleaseGC();
extern void                  XtRemoveAllCallbacks();
extern void                  XtRemoveCallback();
extern void                  XtRemoveEventHandler();
extern void                  XtRemoveGrab();
extern void                  XtRemoveRawEventHandler();
extern void                  XtRemoveTimeOut();
extern void                  XtResizeWidget();
extern void                  XtSetKeyboardFocus();
extern void                  XtSetMappedWhenManaged();
extern void                  XtSetSensitive();
extern void                  XtSetSubvalues();
extern void                  XtSetTypeConverter();
extern void                  XtSetValues();
extern void                  XtToolkitInitialize();
extern void                  XtUngrabKey();
extern void                  XtUngrabKeyboard();
extern void                  XtUnmanageChildren();
extern Widget                XtWindowToWidget();
extern void                  _XtInherit();
extern int                   close();
extern int                   fclose();
extern char *                fgets();
extern FILE *                fopen();
extern int                   fprintf();
extern int                   fread();
extern int                   fscanf();
extern int                   fseek();
extern long                  ftell();
extern int                   fwrite();
extern int                   open();
extern int                   read();
extern char *                tempnam();
/*extern long                  time();*/
/*extern long                  times();*/
extern int                   vfprintf();
extern int                   vsprintf();
#endif /* NOTUSED */
#endif /* SHARELIB */

#endif /* __Ol_libXoli_h__ */
