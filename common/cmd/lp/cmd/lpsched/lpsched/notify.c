/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:cmd/lpsched/lpsched/notify.c	1.2.8.4"
#ident  "$Header: notify.c 1.2 91/06/27 $"

#include "lpsched.h"

static char		*N_Msg[] = {
	"Subject: Status of lp request %s\n\nYour request %s destined for %s%s\n",
	"has completed successfully on printer %s.\n",
	"was canceled by the administrator%s\n",
	"encountered an error during filtering.\n",
	"encountered an error while printing on printer %s.\n",
	"Filtering stopped with an exit code of %d.\n",
	"Printing stopped with an exit code of %d.\n",
	"Filtering was interrupted with a signal %d.\n",
	"Printing was interrupted with a signal %d.\n",
	"\nReason for failure:\n\n%s\n",
	"\nReason for being canceled:\n\n%s\n",
	"was canceled%s\n",
};

static struct reason {
	short			reason;
	char			*msg;
}			N_Reason[] = {
    {
	MNODEST,
	"The requested print destination has been removed."
    }, {
	MERRDEST,
	"All candidate destinations are rejecting further requests."
    }, {
	MDENYDEST,
	"You are no longer allowed to use any printer suitable for\nthe request."
    }, {
	MDENYDEST,
	"No candidate printer can handle these characteristics:"
    }, {
	MNOMEDIA,
	"The form you requested no longer exists."
    }, {
	MDENYMEDIA,
	"You are no longer allowed to use the form you requested."
    }, {
	MDENYMEDIA,
	"The form you wanted now requires a different character set."
    }, {
	MNOFILTER,
	"There is no longer a filter that will convert your file for printing."
    }, {
	MNOMOUNT,
	"The form or print wheel you requested is not allowed on any\nprinter otherwise suitable for the request."
    }, {
	MNOSPACE,
	"Memory allocation problem."
    }, {
	MNOOPEN,
	"One or more files to be printed is not accessible"
    }, {
	-1,
	""
    }
};
	
#ifdef	__STDC__
static void		print_reason ( FILE * , int );
#else
static void             print_reason();
#endif

/*
 * Procedure:     notify
 *
 * Restrictions:
 *               open_lpfile: None
 *               close_lpfile: None
 * Notes - NOTIFY USER OF FINISHED REQUEST
 */
	
void
#ifdef	__STDC__
notify (
	register RSTATUS *	prs,
	char *			errbuf,
	int			k,
	int			e,
	int			slow
)
#else
notify (prs, errbuf, k, e, slow)
	register RSTATUS        *prs;
	char                    *errbuf;
	int                     k,
				e,
				slow;
#endif
{
	DEFINE_FNNAME (notify)

	register char		*cp;

	char			*file;

	FILE                    *fp;


	ENTRYP
	/*
	 * Screen out cases where no notification is needed.
	 */
	if (!(prs->request->outcome & RS_NOTIFY))
		return;
	if (
		!(prs->request->actions & (ACT_MAIL|ACT_WRITE|ACT_NOTIFY))
	     && !prs->request->alert
	     && !(prs->request->outcome & RS_CANCELLED)
	     && !e && !k && !errbuf       /* exited normally */
	)
		return;

	/*
	 * Create the notification message to the user.
	 */
	file = makereqerr(prs);
	if ((fp = open_lpfile(file, "w", MODE_NOREAD))) {
		(void) fprintf (
			fp,
			N_Msg[0],
			prs->secure->req_id,
			prs->secure->req_id,
			prs->request->destination,
			STREQU(prs->request->destination, NAME_ANY)?
				  " printer"
				: ""
		);
	
		if (prs->request->outcome & RS_PRINTED)
			(void) fprintf (
				fp,
				N_Msg[1],
				prs->printer->printer->name
			);

		if (prs->request->outcome & RS_CANCELLED)
			if (!(prs->request->actions & 
			     (ACT_MAIL|ACT_WRITE|ACT_NOTIFY)))
			(void) fprintf (
				fp,
				N_Msg[2],
				(prs->request->outcome & RS_FAILED)?
					  ", and"
					: "."
			);
			else
			(void) fprintf (
				fp,
				N_Msg[11],
				(prs->request->outcome & RS_FAILED)?
					  ", and"
					: "."
			);
		if (prs->request->outcome & RS_FAILED) {
			if (slow)
				(void) fputs (N_Msg[3], fp);
			else
				(void) fprintf (
					fp,
					N_Msg[4],
					prs->printer->printer->name
				);
	
			if (e > 0)
				(void) fprintf (fp, N_Msg[slow? 5 : 6], e);
			else if (k)
				(void) fprintf (fp, N_Msg[slow? 7 : 8], k);
		}
	
		if (errbuf) {
			for (cp = errbuf; *cp && *cp == '\n'; cp++)
				;
			(void) fprintf (fp, N_Msg[9], cp);
			if (prs->request->outcome & RS_CANCELLED)
				(void) fputs ("\n", fp);
		}

		if (prs->request->outcome & RS_CANCELLED)
			print_reason (fp, prs->reason);

		(void) close_lpfile (fp);
		schedule (EV_NOTIFY, prs);

	}
	if (file)
		Free (file);

	EXITP
	return;
}

/*
 * Procedure:     print_reason
 *
 * Restrictions:
 *               fprintf: None
 *               fputs: None
 * Notes - PRINT REASON FOR AUTOMATIC CANCEL
 */

static void
#ifdef	__STDC__
print_reason (
	FILE *			fp,
	int			reason
)
#else
print_reason (fp, reason)
	FILE			*fp;
	register int		reason;
#endif
{
	DEFINE_FNNAME (print_reason)

	register int		i;


#define P(BIT,MSG)	if (chkprinter_result & BIT) (void) fputs (MSG, fp)

	for (i = 0; N_Reason[i].reason != -1; i++)
		if (N_Reason[i].reason == reason) {
			if (reason == MDENYDEST && chkprinter_result)
				i++;
			if (reason == MDENYMEDIA && chkprinter_result)
				i++;
			(void) fprintf (fp, N_Msg[10], N_Reason[i].msg);
			if (reason == MDENYDEST && chkprinter_result) {
				P (PCK_TYPE,	"\tprinter type\n");
				P (PCK_CHARSET,	"\tcharacter set\n");
				P (PCK_CPI,	"\tcharacter pitch\n");
				P (PCK_LPI,	"\tline pitch\n");
				P (PCK_WIDTH,	"\tpage width\n");
				P (PCK_LENGTH,	"\tpage length\n");
				P (PCK_BANNER,	"\tno banner\n");
			}
			break;
		}

	return;
}
