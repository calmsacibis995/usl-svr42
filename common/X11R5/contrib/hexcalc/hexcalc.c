/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4hexcalc:hexcalc.c	1.2"

/*
 *	Copyright (c) 1991, 1992 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyright (c) 1988, 1989, 1990 AT&T
 *	All Rights Reserved 
 */


/* hexcalc - A simple hex calculator for X
 *
 * Copyright 1989 Thomas J. Jarmolowski
 *
 *
 *
 * Author:  Tom Jarmolowski
 *	    jarmolowski%esdsdf.decnet@crd.ge.com
 *
 *	    GESD
 *	    Borton Landing Road
 *	    Moorestown NJ
 *	    MS 108-204
 *	
 *	    (609) 722-4298
 */


#ifndef lint
static char *sccsid = "@(#)hexcalc.c	1.11  11/21/89";
#endif	

#include	<stdio.h>
#include	<ctype.h>
#include	<X11/IntrinsicP.h>
#include	<X11/StringDefs.h>
#include	<X11/Xaw/Form.h>
#include	<X11/Xaw/Label.h>
#include	<X11/Xaw/Command.h>
#include	<X11/Xaw/Paned.h>
#include	<X11/Xaw/Grip.h>

/********************************/
/*	Private Types		*/
/********************************/
typedef	struct	{
	char		*name;
	char		*type;
	Widget		w;
	Widget		*wPtr;
	XtCallbackRec	callback[2];
} ButtonList;

/********************************/
/*	Private Macros		*/
/********************************/
#define	resetArgs()		argCount = 0

#define	setArg(what, value)	XtSetArg(args[argCount], what, value);\
				argCount++

#define	createManagedWidget(name, class, parent)\
			XtCreateManagedWidget(name, class, parent, args, argCount);


#define	Beep()			XBell(XtDisplay(topLevel), 50)

#define	streq(a, b)		(strcmp(a, b) == 0)

#define	PRIVATE	static
#define	PUBLIC

#define	PushArg(n)		argStack[argStackP++] = n
#define	PopArg()		argStack[--argStackP]

#define	initArgs()		argStackP = 0;
#define	emptyArgs()		(argStackP == 0);

#define	PushOp(n)		opStack[opStackP++] = n
#define	PopOp()			opStack[--opStackP]
#define	initOps()		opStackP = 0;
#define	emptyOps()		(opStackP == 0);
#define	TopOp()			opStack[opStackP-1]

/********************************/
/*	Private Routines	*/
/********************************/
static	void	Delete();
static	void	FlipButton(/* w */);
static	void	DoKey(/* w, event, params, numParams */);
static	void	Neg();
static	void	Store();
static	void	Recall();
static	void	ClearM();
static	int	Left(/* token */);
static	int	Right(/* token */);
static	void	Not();
static	void	Op(/* w, data */);
static	void	UpdateDisplay();
static	void	Radix(/* w, data */);
static	void	SetRadixButtons();
static	void	Off();
static	void	SetSize(/* w, data */);
static	void	SetSizeButtons();
static	void	SetSign(/* w, data */);
static	void	SetSignButtons();
static	void	Digit(/* w, data */);
static	void	Clear();
static	void	AllClear();
static	Widget	MakeButtons(/* parent */);
static	Widget	MakeDisplay(/* parent */);
static	void	MakeHexcalc(/* parent */);

/********************************/
/*	Private data		*/
/********************************/
static	int		ac = 0;
static	int		radix	= 10;
static	Widget		display = NULL;
static	Widget		topLevel = NULL;
static	int		memory = 0;
static	Boolean		isSigned = True;
static	int		bitsWide = 32;
static	Boolean		newNumber = True;
static	Boolean		acPushed = False;
static	int		argStackP = 0;
static	int		argStack[100];
static	int		opStackP = 0;
static	int		opStack[100];
static	XtTranslations	translations;

static	Widget	hexW;
static	Widget	binW;
static	Widget	octW;
static	Widget	decW;
static	Widget	shortW;
static	Widget	longW;
static	Widget	signW;
static	Widget	usignW;

static	ButtonList buttonList[] = {

	{"dec",		"baseKey",	NULL,	&decW,	{Radix,	(caddr_t) 10}},
	{"hex",		"baseKey",	NULL,	&hexW,	{Radix,	(caddr_t) 16}},
	{"oct",		"baseKey",	NULL,	&octW,	{Radix,	(caddr_t) 8}},
	{"bin",		"baseKey",	NULL,	&binW,	{Radix,	(caddr_t) 2}},
	{"off",		"controlKey",	NULL,	NULL,	{Off,	NULL}},

	{"16",		"sizeKey",	NULL,	&shortW,{SetSize,  (caddr_t) 16}},
	{"32",		"sizeKey",	NULL,	&longW,	{SetSize,  (caddr_t) 32}},
	{"sgn",		"signKey",	NULL,	&signW,	{SetSign,  (caddr_t) True}},
	{"unsgn",	"signKey",	NULL,	&usignW,{SetSign,  (caddr_t) False}},
	{"C",		"controlKey",	NULL,	NULL,	{AllClear, NULL}},

	{"Sto",		"memKey",	NULL,	NULL,	{Store, NULL}},
	{"Rcl",		"memKey",	NULL,	NULL,	{Recall, NULL}},
	{"Clr",		"memKey",	NULL,	NULL,	{ClearM, NULL}},
	{"(",		"evalKey",	NULL,	NULL,	{Op,	(caddr_t) '('}},
	{")",		"evalKey",	NULL,	NULL,	{Op,	(caddr_t) ')'}},

	{"<<",		"shiftKey",	NULL,	NULL,	{Op,	(caddr_t) '<'}},
	{"d",		"digitKey",	NULL,	NULL,	{Digit,	(caddr_t) 13}},
	{"e",		"digitKey",	NULL,	NULL,	{Digit,	(caddr_t) 14}},
	{"f",		"digitKey",	NULL,	NULL,	{Digit,	(caddr_t) 15}},
	{">>",		"shiftKey",	NULL,	NULL,	{Op,	(caddr_t) '>'}},

	{"~",		"bitKey",	NULL,	NULL,	{Not, NULL}},
	{"a",		"digitKey",	NULL,	NULL,	{Digit,	(caddr_t) 10}},
	{"b",		"digitKey",	NULL,	NULL,	{Digit,	(caddr_t) 11}},
	{"c",		"digitKey",	NULL,	NULL,	{Digit,	(caddr_t) 12}},
	{"/",		"arithKey",	NULL,	NULL,	{Op,	(caddr_t) '/'}},
	
	{"|",		"bitKey",	NULL,	NULL,	{Op,	(caddr_t) '|'}},
	{"7",		"digitKey",	NULL,	NULL,	{Digit,	(caddr_t) 7}},
	{"8",		"digitKey",	NULL,	NULL,	{Digit,	(caddr_t) 8}},
	{"9",		"digitKey",	NULL,	NULL,	{Digit,	(caddr_t) 9}},
	{"*",		"arithKey",	NULL,	NULL,	{Op,	(caddr_t) '*'}},

	{"&",		"bitKey",	NULL,	NULL,	{Op,	(caddr_t) '&'}},
	{"4",		"digitKey",	NULL,	NULL,	{Digit,	(caddr_t) 4}},
	{"5",		"digitKey",	NULL,	NULL,	{Digit,	(caddr_t) 5}},
	{"6",		"digitKey",	NULL,	NULL,	{Digit,	(caddr_t) 6}},
	{"-",		"arithKey",	NULL,	NULL,	{Op,	(caddr_t) '-'}},

	{"^",		"bitKey",	NULL,	NULL,	{Op,	(caddr_t) '^'}},
	{"1",		"digitKey",	NULL,	NULL,	{Digit,	(caddr_t) 1}},
	{"2",		"digitKey",	NULL,	NULL,	{Digit,	(caddr_t) 2}},
	{"3",		"digitKey",	NULL,	NULL,	{Digit,	(caddr_t) 3}},
	{"+",		"arithKey",	NULL,	NULL,	{Op,	(caddr_t) '+'}},

	{"CE/E",	"controlKey",	NULL,	NULL,	{Clear, NULL}},
	{"0",		"digitKey",	NULL,	NULL,	{Digit,	(caddr_t) 0}},
	{"",		"",		NULL,	NULL,	{NULL, NULL}},
	{"+/-",		"",		NULL,	NULL,	{Neg,	(caddr_t) 0}},
	{"=",		"evalKey",	NULL,	NULL,	{Op,	(caddr_t) '='}},
		
};

static	XtActionsRec	actions[] = {
	{"DoKey", 	DoKey},
	{"Delete", 	Delete},
	{"Off", 	Off},
};

static	String		defaultTranslations	=
	":<Key>(:		DoKey(\"(\")\n\
	 :<Key>&:		DoKey(&)\n\
	 :<Key>^:		DoKey(^)\n\
	 :<Key>):		DoKey(\")\")\n\
	 Ctrl<Key>c:		DoKey(off)\n\
	 :<Key>H:		DoKey(hex)\n\
	 :<Key>O:		DoKey(oct)\n\
	 :<Key>D:		DoKey(dec)\n\
	 :<Key>B:		DoKey(bin)\n\
	 :<Key>R:		DoKey(Rcl)\n\
	 :<Key>S:		DoKey(Sto)\n\
	 :<Key>s:		DoKey(16)\n\
	 :<Key>l:		DoKey(32)\n\
	 :<Key>+:		DoKey(+)\n\
	 :<Key>-:		DoKey(-)\n\
	 :<Key>*:		DoKey(*)\n\
	 :<Key>/:		DoKey(/)\n\
	 :<Key>%:		DoKey(%)\n\
	 :<Key>>:		DoKey(>>)\n\
	 :<Key><:		DoKey(<<)\n\
	 :<Key>0:		DoKey(0)\n\
	 :<Key>1:		DoKey(1)\n\
	 :<Key>2:		DoKey(2)\n\
	 :<Key>3:		DoKey(3)\n\
	 :<Key>4:		DoKey(4)\n\
	 :<Key>5:		DoKey(5)\n\
	 :<Key>6:		DoKey(6)\n\
	 :<Key>7:		DoKey(7)\n\
	 :<Key>8:		DoKey(8)\n\
	 :<Key>9:		DoKey(9)\n\
	 :<Key>a:		DoKey(a)\n\
	 :<Key>b:		DoKey(b)\n\
	 :<Key>c:		DoKey(c)\n\
	 :<Key>d:		DoKey(d)\n\
	 :<Key>e:		DoKey(e)\n\
	 :<Key>f:		DoKey(f)\n\
	 Ctrl<Key>h:		Delete()\n\
	 <Key>Delete:		Delete()\n\
	 :<Key>=:		DoKey(=)\n\
	 :<Key>Return:		DoKey(=)"
;


/****************************************************************/
/*								*/
/*   ROUTINE NAME:	Delete					*/
/*								*/
/*   PURPOSE:	Delete a digit					*/
/*								*/
/****************************************************************/
PRIVATE	void	Delete()
{
	if (ac) {
		ac = ((unsigned) ac) / radix;
		UpdateDisplay();
	}
	
}


/****************************************************************/
/*								*/
/*   ROUTINE NAME:	FlipButton				*/
/*								*/
/*   PURPOSE:	Reverse video a button.				*/
/*								*/
/****************************************************************/
PRIVATE	void	FlipButton(w)
Widget		w;
{
	int		argCount;
	Arg		args[10];
	Pixel		foreground;
	Pixel		background;
	
	resetArgs();
	setArg(XtNforeground, &foreground);
	setArg(XtNbackground, &background);
	XtGetValues(w, args, argCount);

	resetArgs();
	setArg(XtNforeground, background);
	setArg(XtNbackground, foreground);
	XtSetValues(w, args, argCount);
}


/****************************************************************/
/*								*/
/*   ROUTINE NAME:	DoKey					*/
/*								*/
/*   PURPOSE:	Execute a key by name				*/
/*								*/
/****************************************************************/
/* ARGSUSED */
PRIVATE	void	DoKey(w, event, params, numParams)
Widget		w;
XEvent		*event;
String		*params;
int		*numParams;
{

	int		i;
	Boolean		match;
	int		args = *numParams;
	
	while (args--) {
		match = False;
		for (i = 0; i < XtNumber(buttonList); i++) {
			if (streq(*params, buttonList[i].name)) {
				FlipButton(buttonList[i].w);
				(*buttonList[i].callback[0].callback)
						(buttonList[i].w, buttonList[i].callback[0].closure, NULL);
				FlipButton(buttonList[i].w);
				match = True;
				break;
			}
		}

		if (!match) {
			(void) fprintf(stderr, "No key named \"%s\".\n", *params);
		}
		params++;
	}
}


/****************************************************************/
/*								*/
/*   ROUTINE NAME:	Neg					*/
/*								*/
/*   PURPOSE:	Change sign of the ac				*/
/*								*/
/****************************************************************/
PRIVATE	void	Neg()
{
	ac = -ac;
	newNumber = True;
	UpdateDisplay();
}


/****************************************************************/
/*								*/
/*   ROUTINE NAME:	store					*/
/*								*/
/*   PURPOSE:	Save the current AC				*/
/*								*/
/****************************************************************/
PRIVATE	void	Store()
{
	newNumber = True;
	memory = ac;
}


/****************************************************************/
/*								*/
/*   ROUTINE NAME:	Recall					*/
/*								*/
/*   PURPOSE:	Load AC from memory.				*/
/*								*/
/****************************************************************/
PRIVATE	void	Recall()
{
	ac = memory;
	newNumber = True;
	UpdateDisplay();
}

/****************************************************************/
/*								*/
/*   ROUTINE NAME:	ClearM					*/
/*								*/
/*   PURPOSE:	Clear memory.				 	*/
/*								*/
/****************************************************************/
PRIVATE	void	ClearM()
{
	memory = 0;
}

/****************************************************************/
/*								*/
/*   ROUTINE NAME:	Left					*/
/*								*/
/*   PURPOSE:	Determine the precedence of this operator 	*/
/* on the left.							*/
/*								*/
/****************************************************************/
PRIVATE	int	Left(token)
char		token;
{
	switch (token) {
	case '*'  : return (16);
	case '/'  : return (16);
	case '%'  : return (16);
	case '+'  : return (8);
	case '-'  : return (8);
	case '<'  : return (4);
	case '>'  : return (4);
	case '&'  : return (2);
	case '^'  : return (2);
	case '|'  : return (2);
	case '('  : return (0);
	case ')'  : return (18);
	case '$'  : return (-1);
	case '='  : return (18);
	default	  :
		(void) fprintf(stderr, "I don't know what to do with %c\n", token);
		exit(1);
	}
/* NOTREACHED */
}



/****************************************************************/
/*								*/
/*   ROUTINE NAME:	Right					*/
/*								*/
/*   PURPOSE:	Determine the precedence of this operator 	*/
/* on the right.						*/
/*								*/
/****************************************************************/
PRIVATE	int	Right(token)
char		token;
{
	switch (token) {
	case '*'  : return (15);
	case '/'  : return (15);
	case '%'  : return (15);
	case '+'  : return (7);
	case '-'  : return (7);
	case '<'  : return (3);
	case '>'  : return (3);
	case '&'  : return (1);
	case '^'  : return (1);
	case '|'  : return (1);
	case '('  : return (17);
	case ')'  : return (-1);
	case '='  : return (-1);
	default	  :
		(void) fprintf(stderr, "I don't know what to do with %c\n", token);
		exit(1);
	}
/* NOTREACHED */
}



/****************************************************************/
/*								*/
/*   ROUTINE NAME: Not						*/
/*								*/
/*   PURPOSE: Bitwise invertions of the accumulator		*/
/*								*/
/****************************************************************/
PRIVATE	void	Not()
{
	ac = ~ac;
	newNumber = True;
	UpdateDisplay();
}


/****************************************************************/
/*								*/
/*   ROUTINE NAME: Op						*/
/*								*/
/*   PURPOSE: Process an operator				*/
/*								*/
/****************************************************************/
/* ARGSUSED */
PRIVATE	void	Op(w, data)
Widget		w;
caddr_t		data;
{
	char	op = (char) data;
	int	temp;
	char	topOp;
		
	/************************************************/
	/* If accumulator has a number, stack it.	*/
	/************************************************/	
	if (!acPushed) {
		newNumber = True;
		acPushed = True;
		PushArg(ac);
	}

	/************************************************/
	/* Process operators according to their		*/
	/* precedence.					*/
	/************************************************/
	while (Left(TopOp()) > Right(op)) {
		topOp = PopOp();
		if ((topOp == '(') && op == ')') break;

		switch(topOp) {
		case '+' :
			ac = PopArg()  + PopArg();
			break;
		case '-' :
			temp = PopArg();
			ac = PopArg() - temp;
			break;
		case '*' :
			ac = PopArg()  * PopArg();
			break;
		case '/' :
			temp = PopArg();
			ac = PopArg()/temp;
			break;
		case '%' :
			temp = PopArg();
			ac = PopArg() % temp;
			break;
			
		case '|' :
			ac = PopArg() | PopArg();
			break;

		case '&' :
			ac = PopArg() & PopArg();
			break;

		case '^' :
			ac = PopArg() ^ PopArg();
			break;

		case '<' :
			temp = PopArg();
			ac = PopArg() << temp;
			break;
			
		case '>' :
			temp = PopArg();
			ac = PopArg() >> temp;
			break;
			
		case '(' :
		case '=' :
			ac = PopArg();
			break;
		}

		UpdateDisplay();
		PushArg(ac);
	}

	if ((op != ')') && (op != '=')) {
		PushOp(op);
	}
}


/****************************************************************/
/*								*/
/*   ROUTINE NAME: UpdateDisplay				*/
/*								*/
/*   PURPOSE:	Display the accumulator				*/
/*								*/
/****************************************************************/
PRIVATE	void	UpdateDisplay()
{
	char		text[50];
	int		argCount;
	Arg		args[10];
	unsigned	i;
	unsigned	ac2	= (bitsWide == 32) ? ac : (ac & 0xffff);
	short		acShort = ac;
	
	switch (radix) {
	case 2:
		text[0] = '\0';
		for (i = (bitsWide == 32) ? 0x80000000 : 0x8000; i; i >>= 1) {
			if (i & ac2) {
				(void) strcat(text, "1");
			} else {
				(void) strcat(text, "0");
			}
		}
		break;
	case 8:
		(void) sprintf(text, (bitsWide == 32) ? "%011o" : "%06o", ac2);
		break;
	case 10:
		(void) sprintf(text, isSigned ? "%d" : "%u", (bitsWide == 32) ? ac : (isSigned ? acShort : ac2));
		break;
	case 16:
		(void) sprintf(text, (bitsWide == 32) ? "%08x" : "%04x", ac2);
		break;
	}


	resetArgs();
	setArg(XtNlabel, text);
	XtSetValues(display, args, argCount);
}


/****************************************************************/
/*								*/
/*   ROUTINE NAME: Radix					*/
/*								*/
/*   PURPOSE:	Change the I/O radix				*/
/*								*/
/****************************************************************/
/* ARGSUSED */
PRIVATE	void	Radix(w, data)
Widget		w;
caddr_t		data;
{
	int	newRadix = (int) data;

	if (radix != newRadix) {
		newNumber = True;
		radix = newRadix;
		UpdateDisplay();
		SetRadixButtons();
	}
}

/****************************************************************/
/*								*/
/*   ROUTINE NAME: SetRadixButtons				*/
/*								*/
/*   PURPOSE:	Make the current radix insensitive		*/
/*								*/
/****************************************************************/
PRIVATE	void	SetRadixButtons()
{

	XtSetSensitive(hexW, True);
	XtSetSensitive(decW, True);
	XtSetSensitive(octW, True);
	XtSetSensitive(binW, True);

	switch(radix) {
	case 16 :
		XtSetSensitive(hexW, False);
		break;
	case 10 :
		XtSetSensitive(decW, False);
		break;
	case 8  :
		XtSetSensitive(octW, False);
		break;
	case 2  :
		XtSetSensitive(binW, False);
		break;
	}
}


/****************************************************************/
/*								*/
/*   ROUTINE NAME:	Off					*/
/*								*/
/*   PURPOSE: Exit.						*/
/*								*/
/****************************************************************/
PRIVATE	void	Off()
{
	exit(0);
}


/****************************************************************/
/*								*/
/*   ROUTINE NAME:	SetSize					*/
/*								*/
/*   PURPOSE:	Set 16 or 32 bit mode				*/
/*								*/
/****************************************************************/
/* ARGSUSED */
PRIVATE	void	SetSize(w, data)
Widget		w;
caddr_t		data;
{
	int	bits = (int) data;

	if (bits != bitsWide) {
		bitsWide = bits;
		UpdateDisplay();
		SetSizeButtons();
	}
}


/****************************************************************/
/*								*/
/*   ROUTINE NAME:	SetSizeButtons				*/
/*								*/
/*   PURPOSE:	Enable/disable size buttons			*/
/*								*/
/****************************************************************/
PRIVATE	void	SetSizeButtons()
{
	if (bitsWide == 16) {
		XtSetSensitive(longW, True);
		XtSetSensitive(shortW, False);
	} else {
		XtSetSensitive(longW, False);
		XtSetSensitive(shortW, True);
	}
}

/****************************************************************/
/*								*/
/*   ROUTINE NAME:	SetSign					*/
/*								*/
/*   PURPOSE:	Set signed or unsigned mode			*/
/*								*/
/****************************************************************/
/* ARGSUSED */
PRIVATE	void	SetSign(w, data)
Widget		w;
caddr_t		data;
{
	Boolean newSign = (Boolean) data;
	if (newSign != isSigned) {
		isSigned = newSign;
		UpdateDisplay();
		SetSignButtons();
	}
}


/****************************************************************/
/*								*/
/*   ROUTINE NAME:	SetSignButtons				*/
/*								*/
/*   PURPOSE:	Enable/disable sign buttons			*/
/*								*/
/****************************************************************/
PRIVATE	void	SetSignButtons()
{
	if (isSigned) {
		XtSetSensitive(usignW, True);
		XtSetSensitive(signW, False);
	} else {
		XtSetSensitive(usignW, False);
		XtSetSensitive(signW, True);
	}
}


/****************************************************************/
/*								*/
/*   ROUTINE NAME:	Digit					*/
/*								*/
/*   PURPOSE:	Process a digit key.				*/
/*								*/
/****************************************************************/
/* ARGSUSED */
PRIVATE	void	Digit(w, data)
Widget		w;
caddr_t		data;
{
	int	digit = (int) data;
	int	oldAc;
	
	if (digit >= radix) {
		Beep();
	} else {
		if (newNumber) {
			ac = 0;
			newNumber = False;
			acPushed = False;
		}
		oldAc = ac;
		ac *= radix;
		ac += digit;
		if  (((radix == 10) && isSigned) ? (oldAc <= ac) : ( (unsigned) oldAc <= (unsigned) ac)) {
			UpdateDisplay();
		} else {
			ac = oldAc;
			Beep();
		}
	}
}


/****************************************************************/
/*								*/
/*   ROUTINE NAME:	Clear					*/
/*								*/
/*   PURPOSE:	Clear the accumulator				*/
/*								*/
/****************************************************************/
PRIVATE	void	Clear()
{
	if (ac) {
		ac = 0;
		UpdateDisplay();
	}
	acPushed = False;
}

/****************************************************************/
/*								*/
/*   ROUTINE NAME:	AllClear				*/
/*								*/
/*   PURPOSE:	Reset the calculator				*/
/*								*/
/****************************************************************/
PRIVATE	void	AllClear()
{
	ac = 0;
	radix = 10;
	memory = 0;
	isSigned = True;
	bitsWide = 32;
	emptyArgs();
	emptyOps();
	PushOp('$');
	newNumber = True;
	acPushed = False;
		
	SetRadixButtons();
	SetSizeButtons();
	SetSignButtons();
	UpdateDisplay();
}


/****************************************************************/
/*								*/
/*   ROUTINE NAME: MakeButtons					*/
/*								*/
/*   PURPOSE: Make a form & all the buttons			*/
/*								*/
/****************************************************************/
PRIVATE	Widget	MakeButtons(parent)
Widget		parent;
{
	int		argCount;
	Arg		args[10];
	Widget		buttonForm;
	Widget		button = NULL;
	Widget		vertButton = NULL;
	int		i;
	Dimension	width;
	Dimension	maxWidth = 0;

	/************************************************/
	/* make a label to hold the display		*/
	/************************************************/
	resetArgs();
	buttonForm = createManagedWidget("buttonForm", formWidgetClass, parent);
	XtAugmentTranslations(buttonForm, translations);
	
	/************************************************/
	/* Make each of the buttons			*/
	/************************************************/
	for (i = 0; i < XtNumber(buttonList); i++) {
		resetArgs();
		setArg(XtNcallback, &buttonList[i].callback[0]);
		setArg(XtNfromHoriz, button);
		setArg(XtNfromVert, vertButton);
		setArg(XtNlabel,	buttonList[i].name);
		if (buttonList[i].name[0] == '\0') {
			setArg(XtNmappedWhenManaged, False);
		}
		button = createManagedWidget(buttonList[i].type, commandWidgetClass, buttonForm);
		buttonList[i].w = button;
		if (buttonList[i].wPtr) *buttonList[i].wPtr = button;
				
		XtAugmentTranslations(button, translations);

		if (((i+1) % 5) == 0) {
			vertButton = button;
			button = NULL;
		}
		
	}

	/************************************************/
	/* Force all buttons to be the same width	*/
	/************************************************/
	for (i = 0; i < XtNumber(buttonList); i++) {
		resetArgs();
		setArg(XtNwidth, &width);
		XtGetValues(buttonList[i].w, args, argCount);
		maxWidth = width > maxWidth ? width : maxWidth;
	}

	for (i = 0; i < XtNumber(buttonList); i++) {		
		resetArgs();
		setArg(XtNwidth, maxWidth);
		XtSetValues(buttonList[i].w, args, argCount);
	}
	
	return (buttonForm);

}



/****************************************************************/
/*								*/
/*   ROUTINE NAME: MakeDisplay					*/
/*								*/
/*   PURPOSE: Make the calculator display			*/
/*								*/
/****************************************************************/
PRIVATE	Widget	MakeDisplay(parent)
Widget		parent;
{
	int	argCount;
	Arg	args[10];

	/************************************************/
	/* make a label to hold the display		*/
	/************************************************/
	resetArgs();
	setArg(XtNjustify, XtJustifyRight);
	setArg(XtNlabel, "-00000000000000000000000000000000");
	display = XtCreateManagedWidget("display", labelWidgetClass, parent, args, argCount);


	XtAugmentTranslations(display, translations);

	return (display);
}


/****************************************************************/
/*								*/
/*   ROUTINE NAME: MakeHexcalc					*/
/*								*/
/*   PURPOSE: Make the calculator				*/
/*								*/
/****************************************************************/
PRIVATE	void	MakeHexcalc(parent)
Widget		parent;
{
	int	argCount;
	Arg	args[10];
	Widget	whole;	
	
	/************************************************/
	/* Build a pane to hold the world		*/
	/************************************************/
	resetArgs();
	setArg(XtNgripIndent, -10);
	whole = XtCreateManagedWidget("wholePane", panedWidgetClass, 
		parent, args, argCount);
	

	XtAugmentTranslations(whole, translations);
	/************************************************/
	/* Make the display and buttons			*/
	/************************************************/	
	(void) MakeDisplay(whole);
	(void) MakeButtons(whole);
}


/****************************************************************/
/*								*/
/*   ROUTINE NAME: main						*/
/*								*/
/*   PURPOSE: Process entry point.				*/
/*								*/
/****************************************************************/
int	main(argc, argv)
int	argc;
char	*argv[];
{
	Atom	wm_delete_window;

	/************************************************/	
	/* Build top level widget			*/
	/************************************************/
	topLevel = XtInitialize("hexcalc", "Hexcalc", NULL, 0, &argc, argv);
	XtAppAddActions(XtWidgetToApplicationContext(topLevel), actions, XtNumber(actions));
	translations = 	XtParseTranslationTable(defaultTranslations);

	/************************************************/
	/* layout and initialize other widgets		*/
	/* Then process events.				*/
	/************************************************/

	MakeHexcalc(topLevel);
	XtRealizeWidget(topLevel);

/*
 *	CHANGE # UNKNOWN
 *  Honouring ICCCM WM_DELETE_WINDOW protocols.
 *	ENDCHANGE # UNKNOWN
 */
	wm_delete_window = XInternAtom(XtDisplay(topLevel), 
			"WM_DELETE_WINDOW", False);
	XSetWMProtocols(XtDisplay(topLevel), XtWindow(topLevel),
		&wm_delete_window, 1);
	XtOverrideTranslations(topLevel, 
		XtParseTranslationTable("<Message>WM_PROTOCOLS:Off()") );
	AllClear();
	XtMainLoop();

	return (0);
}


