/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)shlib:libXoli.c	1.9"
#endif

#define _USHORT_H               /* prevent conflicts between BSD
                                   sys/types.h and interlan/il_types.h */

#ifdef SHARELIB
/* kludge to force X11/Xos.h to get the correct sys/time.h */
#define SAVE_SHARELIB
#undef SHARELIB
#endif

#include <stdio.h>
#include "X11/Xlib.h"
#include "X11/X.h"
#include "X11/Xutil.h"
#include "../Xt/IntrinsicI.h"
#include "X11/IntrinsicP.h"
#include "X11/RectObjP.h"
#include "X11/CoreP.h"
#include "X11/CompositeP.h"
#include "X11/ConstrainP.h"
#include "X11/ShellP.h"
 
#ifdef SAVE_SHARELIB
#define SHARELIB
#undef SAVE_SHARELIB
#endif

typedef int  (*PFI)();
typedef void (*PFV)();

#ifdef SHARELIB
/* put imported functions here */
int             (*_libXol_XBell                             )()       = 0;
int             (*_libXol_XChangeProperty                   )()       = 0;
int             (*_libXol_XChangeWindowAttributes           )()       = 0;
Bool            (*_libXol_XCheckWindowEvent                 )()       = 0;
int             (*_libXol_XClearArea                        )()       = 0;
int             (*_libXol_XClearWindow                      )()       = 0;
int             (*_libXol_XClipBox                          )()       = 0;
int             (*_libXol_XConvertSelection                 )()       = 0;
int             (*_libXol_XCopyArea                         )()       = 0;
int             (*_libXol_XCopyPlane                        )()       = 0;
Pixmap          (*_libXol_XCreateBitmapFromData             )()       = 0;
Cursor          (*_libXol_XCreateFontCursor                 )()       = 0;
GC              (*_libXol_XCreateGC                         )()       = 0;
Pixmap          (*_libXol_XCreatePixmap                     )()       = 0;
Pixmap          (*_libXol_XCreatePixmapFromBitmapData       )()       = 0;
Region          (*_libXol_XCreateRegion                     )()       = 0;
int             (*_libXol_XDefaultDepth                     )()       = 0;
int             (*_libXol_XDefaultScreen                    )()       = 0;
Screen *        (*_libXol_XDefaultScreenOfDisplay           )()       = 0;
int             (*_libXol_XDefineCursor                     )()       = 0;
int             (*_libXol_XDeleteProperty                   )()       = 0;
Display *       (*_libXol_XDisplayOfScreen                  )()       = 0;
int             (*_libXol_XDrawImageString                  )()       = 0;
int             (*_libXol_XDrawLine                         )()       = 0;
int             (*_libXol_XDrawLines                        )()       = 0;
int             (*_libXol_XDrawRectangle                    )()       = 0;
int             (*_libXol_XDrawSegments                     )()       = 0;
int             (*_libXol_XDrawString                       )()       = 0;
int             (*_libXol_XFillPolygon                      )()       = 0;
int             (*_libXol_XFillRectangle                    )()       = 0;
int             (*_libXol_XFillRectangles                   )()       = 0;
int             (*_libXol_XFlush                            )()       = 0;
int             (*_libXol_XFree                             )()       = 0;
int             (*_libXol_XFreeFont                         )()       = 0;
int             (*_libXol_XFreeGC                           )()       = 0;
int             (*_libXol_XFreePixmap                       )()       = 0;
char *          (*_libXol_XGetDefault                       )()       = 0;
Bool            (*_libXol_XGetFontProperty                  )()       = 0;
XImage *        (*_libXol_XGetImage                         )()       = 0;
Window          (*_libXol_XGetSelectionOwner                )()       = 0;
XWMHints        (*_libXol_XGetWMHints                       )()       = 0;
Status          (*_libXol_XGetWindowAttributes              )()       = 0;
int             (*_libXol_XGetWindowProperty                )()       = 0;
int             (*_libXol_XGrabPointer                      )()       = 0;
int             (*_libXol_XGrabServer                       )()       = 0;
int             (*_libXol_XIfEvent                          )()       = 0;
Atom            (*_libXol_XInternAtom                       )()       = 0;
int             (*_libXol_XIntersectRegion                  )()       = 0;
KeyCode         (*_libXol_XKeysymToKeycode                  )()       = 0;
char *          (*_libXol_XKeysymToString                   )()       = 0;
XFontStruct *   (*_libXol_XLoadQueryFont                    )()       = 0;
int             (*_libXol_XLookupString                     )()       = 0;
int             (*_libXol_XMapRaised                        )()       = 0;
int             (*_libXol_XMapWindow                        )()       = 0;
int             (*_libXol_XMoveWindow                       )()       = 0;
int             (*_libXol_XPutBackEvent                     )()       = 0;
int             (*_libXol_XPutImage                         )()       = 0;
Bool            (*_libXol_XQueryPointer                     )()       = 0;
int             (*_libXol_XRaiseWindow                      )()       = 0;
int             (*_libXol_XRectInRegion                     )()       = 0;
Window          (*_libXol_XRootWindow                       )()       = 0;
int             (*_libXol_XSelectInput                      )()       = 0;
Status          (*_libXol_XSendEvent                        )()       = 0;
int             (*_libXol_XSetClipRectangles                )()       = 0;
int             (*_libXol_XSetFillStyle                     )()       = 0;
int             (*_libXol_XSetInputFocus                    )()       = 0;
void            (*_libXol_XSetNormalHints                   )()       = 0;
int             (*_libXol_XSetSelectionOwner                )()       = 0;
int             (*_libXol_XSetStipple                       )()       = 0;
int             (*_libXol_XSetWMHints                       )()       = 0;
int             (*_libXol_XSetWindowBackground              )()       = 0;
int             (*_libXol_XSetWindowBackgroundPixmap        )()       = 0;
int             (*_libXol_XSetWindowBorder                  )()       = 0;
int             (*_libXol_XSetWindowBorderWidth             )()       = 0;
int             (*_libXol_XStoreBuffer                      )()       = 0;
KeySym          (*_libXol_XStringToKeysym                   )()       = 0;
int             (*_libXol_XSync                             )()       = 0;
int             (*_libXol_XTextWidth                        )()       = 0;
Bool            (*_libXol_XTranslateCoordinates             )()       = 0;
int             (*_libXol_XUngrabPointer                    )()       = 0;
int             (*_libXol_XUngrabServer                     )()       = 0;
int             (*_libXol_XUnionRectWithRegion              )()       = 0;
int             (*_libXol_XUnmapWindow                      )()       = 0;
int             (*_libXol_XWarpPointer                      )()       = 0;
XrmString       (*_libXol_XrmQuarkToString                  )()       = 0;
XrmQuark        (*_libXol_XrmStringToQuark                  )()       = 0;
void            (*_libXol_XtAddCallback                     )()       = 0;
void            (*_libXol_XtAddCallbacks                    )()       = 0;
void            (*_libXol_XtAddConverter                    )()       = 0;
void            (*_libXol_XtAddEventHandler                 )()       = 0;
int             (*_libXol_XtAddExposureToRegion             )()       = 0;
void            (*_libXol_XtAddGrab                         )()       = 0;
void            (*_libXol_XtAddRawEventHandler              )()       = 0;
XtIntervalId    (*_libXol_XtAddTimeOut                      )()       = 0;
void            (*_libXol_XtAppAddConverter                 )()       = 0;
Widget          (*_libXol_XtAppCreateShell                  )()       = 0;
void            (*_libXol_XtAppErrorMsg                     )()       = 0;
void            (*_libXol_XtAppWarningMsg                   )()       = 0;
void            (*_libXol_XtCallCallbacks                   )()       = 0;
Boolean         (*_libXol_XtCallConverter                   )()       = 0;
void            (*_libXol_XtCallbackPopdown                 )()       = 0;
char *          (*_libXol_XtCalloc                          )()       = 0;
void            (*_libXol_XtConfigureWidget                 )()       = 0;
void            (*_libXol_XtConvert                         )()       = 0;
void            (*_libXol_XtConvertCase                     )()       = 0;
XtAppContext    (*_libXol_XtCreateApplicationContext        )()       = 0;
Widget          (*_libXol_XtCreateApplicationShell          )()       = 0;
Widget          (*_libXol_XtCreateManagedWidget             )()       = 0;
Widget          (*_libXol_XtCreatePopupShell                )()       = 0;
Widget          (*_libXol_XtCreateWidget                    )()       = 0;
void            (*_libXol_XtCreateWindow                    )()       = 0;
XtAppContext    (*_libXol__XtDefaultAppContext              )()       = 0;
void            (*_libXol_XtDestroyGC                       )()       = 0;
void            (*_libXol_XtDestroyWidget                   )()       = 0;
Display *       (*_libXol_XtDisplay                         )()       = 0;
Display *       (*_libXol_XtDisplayOfObject                 )()       = 0;
XtAppContext    (*_libXol_XtDisplayToApplicationContext     )()       = 0;
void            (*_libXol_XtError                           )()       = 0;
void            (*_libXol_XtErrorMsg                        )()       = 0;
void            (*_libXol_XtFree                            )()       = 0;
void            (*_libXol__XtFreeEventTable                 )()       = 0;
void            (*_libXol_XtGetApplicationResources         )()       = 0;
GC              (*_libXol_XtGetGC                           )()       = 0;
KeySym *        (*_libXol_XtGetKeysymTable                  )()       = 0;
ProcessContext  (*_libXol__XtGetProcessContext              )()       = 0;
void            (*_libXol_XtGetSelectionValue               )()       = 0;
void            (*_libXol_XtGetSubresources                 )()       = 0;
void            (*_libXol_XtGetSubvalues                    )()       = 0;
void            (*_libXol_XtGetValues                       )()       = 0;
void            (*_libXol_XtGrabKey                         )()       = 0;
int             (*_libXol_XtGrabKeyboard                    )()       = 0;
XtCallbackStatus(*_libXol_XtHasCallbacks                    )()       = 0;
Widget          (*_libXol_XtInitialize                      )()       = 0;
Boolean         (*_libXol_XtIsRealized                      )()       = 0;
Boolean         (*_libXol_XtIsManaged                       )()       = 0;
Boolean         (*_libXol_XtIsSubclass                      )()       = 0;
Boolean         (*_libXol__XtIsSubclassOf                   )()       = 0;
Boolean         (*_libXol_XtIsSensitive                     )()       = 0;
XtGeometryResult(*_libXol_XtMakeGeometryRequest             )()       = 0;
XtGeometryResult(*_libXol_XtMakeResizeRequest               )()       = 0;
char *          (*_libXol_XtMalloc                          )()       = 0;
void            (*_libXol_XtManageChild                     )()       = 0;
void            (*_libXol_XtManageChildren                  )()       = 0;
ArgList         (*_libXol_XtMergeArgLists                   )()       = 0;
void            (*_libXol_XtMoveWidget                      )()       = 0;
String          (*_libXol_XtName                            )()       = 0;
Display         (*_libXol_XtOpenDisplay                     )()       = 0;
void            (*_libXol_XtOverrideTranslations            )()       = 0;
Boolean         (*_libXol_XtOwnSelection                    )()       = 0;
XtTranslations  (*_libXol_XtParseTranslationTable           )()       = 0;
void            (*_libXol_XtPopdown                         )()       = 0;
void            (*_libXol_XtPopup                           )()       = 0;
XtGeometryResult(*_libXol_XtQueryGeometry                   )()       = 0;
void            (*_libXol_XtRealizeWidget                   )()       = 0;
char *          (*_libXol_XtRealloc                         )()       = 0;
void            (*_libXol_XtReleaseGC                       )()       = 0;
void            (*_libXol_XtRemoveAllCallbacks              )()       = 0;
void            (*_libXol_XtRemoveCallback                  )()       = 0;
void            (*_libXol_XtRemoveEventHandler              )()       = 0;
void            (*_libXol_XtRemoveGrab                      )()       = 0;
void            (*_libXol_XtRemoveRawEventHandler           )()       = 0;
void            (*_libXol_XtRemoveTimeOut                   )()       = 0;
void            (*_libXol_XtResizeWidget                    )()       = 0;
Screen *        (*_libXol_XtScreen                          )()       = 0;
Screen *        (*_libXol_XtScreenOfObject                  )()       = 0;
void            (*_libXol_XtSetKeyboardFocus                )()       = 0;
void            (*_libXol_XtSetMappedWhenManaged            )()       = 0;
void            (*_libXol_XtSetSensitive                    )()       = 0;
void            (*_libXol_XtSetSubvalues                    )()       = 0;
void            (*_libXol_XtSetTypeConverter                )()       = 0;
void            (*_libXol_XtSetValues                       )()       = 0;
XtPerDisplay    (*_libXol__XtSortPerDisplayList             )()       = 0;
void            (*_libXol_XtToolkitInitialize               )()       = 0;
void            (*_libXol_XtUngrabKey                       )()       = 0;
void            (*_libXol_XtUngrabKeyboard                  )()       = 0;
void            (*_libXol_XtUnmanageChildren                )()       = 0;
Window          (*_libXol_XtWindow                          )()       = 0;
Window          (*_libXol_XtWindowOfObject                  )()       = 0;
Widget          (*_libXol_XtWindowToWidget                  )()       = 0;
void            (*_libXol__XtInherit                        )()       = &_libXol__XtInherit;
int             (*_libXol_access                            )()       = 0;
void            (*_libXol_bcopy                             )()       = 0;
int             (*_libXol_close                             )()       = 0;
void            (*_libXol_exit                              )()       = 0;
int             (*_libXol_fclose                            )()       = 0;
char *          (*_libXol_fgets                             )()       = 0;
FILE *          (*_libXol_fopen                             )()       = 0;
int             (*_libXol_fprintf                           )()       = 0;
int             (*_libXol_fread                             )()       = 0;
void            (*_libXol_free                              )()       = 0;
int             (*_libXol_fscanf                            )()       = 0;
int             (*_libXol_fseek                             )()       = 0;
long            (*_libXol_ftell                             )()       = 0;
int             (*_libXol_fwrite                            )()       = 0;
unsigned int    (*_libXol_getuid                            )()       = 0;
unsigned int    (*_libXol_getgid                            )()       = 0;
char *          (*_libXol_malloc                            )()       = 0;
int             (*_libXol_open                              )()       = 0;
int             (*_libXol_printf                            )()       = 0;
int             (*_libXol_read                              )()       = 0;
char *          (*_libXol_realloc                           )()       = 0;
int             (*_libXol_sprintf                           )()       = 0;
int             (*_libXol_sscanf                            )()       = 0;
char *          (*_libXol_strcat                            )()       = 0;
char *          (*_libXol_strchr                            )()       = 0;
int             (*_libXol_strcmp                            )()       = 0;
char *          (*_libXol_strcpy                            )()       = 0;
int             (*_libXol_strlen                            )()       = 0;
char *          (*_libXol_strncpy                           )()       = 0;
char *          (*_libXol_strrchr                           )()       = 0;
double          (*_libXol_strtod                            )()       = 0;
char *          (*_libXol_strtok                            )()       = 0;
long            (*_libXol_strtol                            )()       = 0;
char *          (*_libXol_tempnam                           )()       = 0;
long            (*_libXol_time                              )()       = 0;
long            (*_libXol_times                             )()       = 0;
int             (*_libXol_tolower                           )()       = 0;
int             (*_libXol_toupper                           )()       = 0;
int             (*_libXol_vfprintf                          )()       = 0;
int             (*_libXol_vsprintf                          )()       = 0;
int                  (*_libXol_strncmp			)()	= 0;
int                  (*_libXol_setvbuf			)()	= 0;
char *               (*_libXol_strncat			)()	= 0;
int                  (*_libXol_fputs			)()	= 0;
int                  (*_libXol_fputc			)()	= 0;
int                  (*_libXol_fflush			)()	= 0;
char *               (*_libXol_strdup			)()	= 0;
int                  (*_libXol_puts			)()	= 0;
int                  (*_libXol_putw			)()	= 0;
void                 (*_libXol_rewind			)()	= 0;
int                  (*_libXol_ungetc			)()	= 0;
int                  (*_libXol_abort 			)()	= 0;
char *               (*_libXol_gets			)()	= 0;
int                  (*_libXol_getw			)()	= 0;
char *               (*_libXol_cuserid			)()	= 0;
int                  (*_libXol_scanf			)()	= 0;
char *               (*_libXol_memccpy			)()	= 0;
char *               (*_libXol_memset			)()	= 0;
char *               (*_libXol_ctermid			)()	= 0;
int                  (*_libXol_fgetc			)()	= 0;
void                 (*_libXol_tzset			)()	= 0;
int                  (*_libXol_strspn			)()	= 0;
char *               (*_libXol_strpbrk			)()	= 0;
void                 (*_libXol_setbuf			)()	= 0;
int                  (*_libXol_strcspn			)()	= 0;
FILE *               (*_libXol_tmpfile			)()	= 0;
char *               (*_libXol_tmpnam			)()	= 0;
int                  (*_libXol_system			)()	= 0;
char *               (*_libXol_memchr			)()	= 0;
int                  (*_libXol_memcmp			)()	= 0;
char *               (*_libXol_memcpy			)()	= 0;

/* imported data */
ApplicationShellClassRec(*_libXol_applicationShellClassRec      )     = 0;
WidgetClass             (*_libXol_applicationShellWidgetClass   )     = 0;
CompositeClassRec       (*_libXol_compositeClassRec             )     = 0;
WidgetClass             (*_libXol_compositeWidgetClass          )     = 0;
ConstraintClassRec      (*_libXol_constraintClassRec            )     = 0;
int                     (*_libXol_daylight                      )     = 0;
int                     (*_libXol_errno                         )     = 0;
FILE *			(*_libXol__iob				)[]   = 0;
unsigned char  		(*_libXol__ctype			)[]   = 0;
WidgetClass             (*_libXol_overrideShellWidgetClass      )     = 0;
RectObjClassRec         (*_libXol_rectObjClassRec               )     = 0;
ShellClassRec           (*_libXol_shellClassRec                 )     = 0;
WidgetClass             (*_libXol_shellWidgetClass              )     = 0;
long                    (*_libXol_timezone                      )     = 0;
TopLevelShellClassRec   (*_libXol_topLevelShellClassRec         )     = 0;
WidgetClass             (*_libXol_topLevelShellWidgetClass      )     = 0;
TransientShellClassRec  (*_libXol_transientShellClassRec        )     = 0;
WidgetClass             (*_libXol_transientShellWidgetClass     )     = 0;
WidgetClass             (*_libXol_widgetClass                   )     = 0;
WidgetClassRec          (*_libXol_widgetClassRec                )     = 0;
WMShellClassRec         (*_libXol_wmShellClassRec               )     = 0;
WidgetClass             (*_libXol_wmShellWidgetClass            )     = 0;
int			(*_libXol__XtInheritTranslations        )     = &_libXol__XtInheritTranslations;
PerDisplayTablePtr	(*_libXol__XtperDisplayList             )     = 0;
#endif
