/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtadmin:print/setup/properties.h	1.14"
#endif

#ifndef PROPERTIES_H
#define PROPERTIES_H

#include <lp.h>
#include <printers.h>

#define APPNAME		"prtsetup"

#define NONE_POSTED	~0

enum {
    Apply_Button, Reset_Button, Cancel_Button,
};

enum {
    Basic_Page, Config_Page, Comm_Page,
};

enum {
    Yes_Button, No_Button,
};

enum {
    No_OS, S5_OS, BSD_OS,
};

typedef struct {
    char	*file;
    char	*icon;
    char	*dfltIcon;
    char	*dfltPrt;
    Boolean	administrator;
} ResourceRec, *ResourcesPtr;

typedef struct {
    XtArgVal	lbl;
    XtArgVal	mnem;
    XtArgVal	sensitive;
    XtArgVal	selectProc;
    XtArgVal	dflt;
    XtArgVal	userData;
    XtArgVal	subMenu;
} MenuItem;

typedef struct {
    char	*title;
    char	*file;
    char	*section;
} HelpText;

typedef enum {
    ParallelPort, SerialPort, RemotePort, NoPort,
} PortType;

typedef struct {
    char		*desc;
    char		**terminfo;
    char		**contentTypes;
    char		*interface; 
    char		**modules;
    char		*stty;
    Cardinal		indx;
} Printer;

typedef struct {
    XtArgVal	userData;
    XtArgVal	lbl;
} ButtonItem;

typedef struct {
    Widget	btn;
    Cardinal	setIndx;
} BtnChoice;

typedef struct {
    Widget	txt;
    char	*setText;
} TxtChoice;

typedef struct {
    TxtChoice	value;
    BtnChoice	units;
} ScaledChoice;

typedef struct {
    BtnChoice	portCtrl;
    TxtChoice	miscDevCtrl;
    BtnChoice	allowRmtCtrl;
} ParallelCntl;

typedef struct {
    Cardinal	port;
    char	*miscDev;
} ParallelData;

typedef struct {
    BtnChoice	portCtrl;
    TxtChoice	miscDevCtrl;
    BtnChoice	allowRmtCtrl;
    BtnChoice	baudRateCtrl;
    BtnChoice	parityCtrl;
    BtnChoice	stopBitsCtrl;
    BtnChoice	charSizeCtrl;
} SerialCntl;

typedef struct {
    Cardinal	port;
    char       	*miscDev;
    Cardinal	baudRate;
    Cardinal	parity;
    Cardinal	stopBits;
    Cardinal	charSize;
} SerialData;

typedef struct {
    TxtChoice	systemCtrl;
    TxtChoice	rmtNameCtrl;
    BtnChoice	osCtrl;
} RemoteCntl;

typedef struct {
    char	*system;
    char	*rmtName;
    Cardinal	os;
} RemoteData;

typedef union {
    ParallelData	parallel;
    SerialData		serial;
    RemoteData		remote;
} PortData;

typedef struct {
    Widget			popupWidget;
    Widget			sheets [3];
    Widget			ports [3];
    Widget			listWidget;
    TxtChoice			prtNameCtrl;
    ParallelCntl		parallelCtrls;
    SerialCntl			serialCtrls;
    RemoteCntl			remoteCtrls;
    BtnChoice			bannerCtrl;
    BtnChoice			alertCtrl;
    ScaledChoice		pgWidCtrl;
    ScaledChoice		pgLenCtrl;
    ScaledChoice		cpiCtrl;
    ScaledChoice		lpiCtrl;
    struct _PropertyData	*owner;
    Cardinal			posted;
    unsigned			pageCreated;
    PortType			mappedPort;
    Boolean			poppedUp;
} PropertyCntl;

typedef struct _PropertyData {
    char		*prtName;
    PortType		kind;
    PortData		device;
    Printer		*printer;
    Printer		*newPrinter;
    Cardinal		allowRmt;	
    Cardinal		newAllowRmt;	
    Cardinal		banner;
    Cardinal		alert;
    SCALED		pgWid;
    SCALED		pgLen;
    SCALED		cpi;
    SCALED		lpi;
    char		*stty;
    PRINTER		*config;
    PropertyCntl	*controls;
    Boolean		accepting;
    Boolean		enabled;
    Boolean		faulted;
    char		*activeJob;
} PropertyData;

typedef struct {
    Widget		popupWindow;
    Widget		whenPopupWindow;
    Widget		footer;
    Widget		state;
    BtnChoice		acceptCtrl;
    BtnChoice		enableCtrl;
    PropertyData	*owner;
    int			pendingAccept;
    Boolean		poppedUp;
} PrinterStatus;

typedef struct {
    char	*lbl;
    void	(*createProc)(Widget, PropertyData *);
    void	(*initProc)(Widget, PropertyData *, PRINTER *);
    void	(*updateProc)(Widget, PropertyData *, PRINTER *);
    char	*(*checkProc)(Widget, PropertyData *);
    void	(*applyProc)(Widget, PropertyData *);
    void	(*resetProc)(Widget, PropertyData *);
    void	(*factoryProc)(Widget, PropertyData *);
} CategoryPage;

extern XtAppContext	AppContext;
extern ResourceRec	AppResources;
extern char		*AppName;
extern char		*AppTitle;
extern String		MenuFields [];
extern int		NumMenuFields;
extern String		ButtonFields [];
extern int		NumButtonFields;

extern PrinterStatus	PrtStatus;

extern void	Properties (Widget, PropertyData *, Cardinal);
extern void	InitPropertySheets (Widget);
extern void	InitProperties (Widget, PropertyData *, PRINTER *);
extern char	*UpdatePrinter (PropertyData *);
extern void	PrtControl (Widget, PropertyData *);
extern void	SaveDefault (char *name);
extern void	ButtonSelectCB (Widget, XtPointer, XtPointer);
extern void	NoneSetSelectCB (Widget, XtPointer, XtPointer);
extern void	NoneSetUnselectCB (Widget, XtPointer, XtPointer);
extern void	SetLabels (MenuItem *, int);
extern void	SetButtonLbls (ButtonItem *, int);
extern void	SetHelpLabels (HelpText *);
extern void	MakeButtons (Widget, char *, ButtonItem *, Cardinal,
			     BtnChoice *);
extern void	MakeNoneSetButtons (Widget, char *, ButtonItem *, Cardinal,
				    BtnChoice *);
extern void	ResetNoneSetButton (BtnChoice *, Cardinal);
extern void	MakeText (Widget, char *, TxtChoice *, int);
extern void	ApplyText (TxtChoice *, char **);
extern char	*GetText (TxtChoice *);
extern void	MakeScaled (Widget, char *, ScaledChoice *);
extern void	ApplyScaled (ScaledChoice *, float *, char *);
extern void	ResetScaled (ScaledChoice *, float, char);
extern Boolean	CheckScaled (ScaledChoice *);
extern void	CopyConfig (PRINTER *dst, PRINTER *src);
extern void	FreeConfig (PRINTER *config, Boolean freeall);
extern void	ChangePrinter (PropertyData *, PRINTER *);
extern Boolean	AddRemote (char *, int);
extern Boolean	IsAdmin (void);
extern void	DisplayHelp (Widget, HelpText *);
extern void	VerifyCB (Widget, XtPointer, XtPointer);
extern void	CancelCB (Widget, XtPointer, XtPointer);
extern void	HelpCB (Widget, XtPointer, XtPointer);
extern void	FooterMsg (Widget, char *);
extern void	BringDownPopup (Widget popup);

#endif /* PROPERTIES_H */
