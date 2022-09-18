/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtm:olwsm/fun.h	1.5"
{
	static String		original;
	static Cardinal		fun_state	= 0;
	static Cardinal		fun_states[]	= {
		1,1,2,2,3,3,1,2,3
	};
	static Dimension tablist[] = { 10, 0 };
	static String		list		= "\
\n\n\n\n\n\n\n\n\n\n\
	Don Alecci,\n\
	Lloyd Altman,\n\
	Bruce Barnett,\n\
	Jim Bash,\n\
	Alberto Benjamin,\n\
	Mrinal Bhaumik,\n\
	Stan Brener,\n\
	David Bryant,\n\
	Sam Chang,\n\
	Joe Chao,\n\
	Ping Chen,\n\
	Lew Church,\n\
	Betty Dall,\n\
	Lee Davenport,\n\
	Dave Francis,\n\
	Bill Franken,\n\
	Keisuke Fukui,\n\
	Jan Gryck,\n\
	Ali Haddara,\n\
	Ross Hilbert,\n\
	Fred Horman,\n\
	Steve Humphrey,\n\
	Paul Jayne,\n\
	Karen Jones,\n\
	Norikazu Kaiya,\n\
	Anne Kane,\n\
	Karen Kendler,\n\
	Bob Kirby,\n\
	Ruth Klein,\n\
	Becky Meacham,\n\
	Steve Mershon,\n\
	Marcel Meth,\n\
	John Miller,\n\
	Mark Miller,\n\
	Val Mitchell-Stevens,\n\
	Jeff Moore,\n\
	Carol Nelson,\n\
	Joanne Newbauer,\n\
	Scott Novack,\n\
	Andy Oakland,\n\
	Marcia Paisner,\n\
	Mark Pochtar,\n\
	Jeff Prem,\n\
	Ernie Rice,\n\
	Chris Schoettle,\n\
	Bill Sherman,\n\
	Manish Sheth,\n\
	Sam Shteingart,\n\
	Sarah Siegel,\n\
	Michael Siemon,\n\
	Rich Smolucha,\n\
	Phil Stern,\n\
	Bill Stoll,\n\
	Kumar Talluri,\n\
	Les Temple,\n\
	Rick Thomas,\n\
	Charles Tsai,\n\
	Ed Whelan,\n\
	Joanne Woo,\n\
	Henry Yen,\n\
	Kai Young,\n\
	Mike Zanchelli,\n\
	...";

	if (fun_state > XtNumber(fun_states))
		/*EMPTY*/;	/* no more fun */

	else if (fun_state == XtNumber(fun_states)) {
		fun_state++;
		XtVaSetValues (
			example_text,
			XtNsource,          (XtArgVal)original,
			XtNdisplayPosition, (XtArgVal)0,
			XtNsourceType,      (XtArgVal)OL_STRING_SOURCE,
			(String)0
		);
		XtFree (original);

	} else if (fun_states[fun_state] == button) {
		fun_state++;
		if (fun_state >= XtNumber(fun_states)) {
			TextBuffer *		text;
			static TextLocation	from = { 0, 0, 0 };
			TextLocation		to;

			XtVaGetValues (
				example_text,
				XtNsource, (XtArgVal)&text,
				(String)0
			);
			to = LastTextBufferLocation(text);
			original = GetTextBufferBlock(text, from, to);

			XtVaSetValues (
				example_text,
				XtNsource,     (XtArgVal)list,
				XtNtabTable,     (XtArgVal)&tablist[0],
				XtNsourceType, (XtArgVal)OL_STRING_SOURCE,
				(String)0
			);
		}

	} else
		fun_state = 0;	/* wrong combination, start again */
}
