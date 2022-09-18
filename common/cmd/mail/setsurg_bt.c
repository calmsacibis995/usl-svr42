/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/setsurg_bt.c	1.5.2.2"
#ident "@(#)setsurg_bt.c	1.6 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	setsurg_bw - interpret the option list for postprocessing entries

    SYNOPSIS
	void setsurg_bw(string *st, int *pbatchsize, int *presolved)

    DESCRIPTION
	The string st is of the form
		B=n;W=n;
	where the options can be in any order. Each "n" is of the form
		number

	The value of B= is returned in pbatchsize.
	The value of W= is returned in pwait.
	The value -1 is returned for values not set.
*/

void setsurg_bw(st, pbatchsize, pwait)
string *st;
int *pbatchsize;
int *pwait;
{
    char *pn = "setsurg_bw";
    string *B = 0, *W = 0, *tok;

    Tout(pn, "Looking at status list '%s'\n", s_to_c(st));

    /* split apart at the ;'s */
    while ((tok = s_tok(st, ";")) != 0)
	{
	switch (s_ptr_to_c(tok)[0])
	    {
	    case 'B': case 'b':
		B = tokdef(B, tok, "B");
		break;

	    case 'W': case 'w':
		W = tokdef(W, tok, "W");
		break;

	    default:
		Tout(pn, "Unknown option list field: %s\n",
		    s_to_c(tok));
		s_free(tok);
		break;
	    }
	}

    if (pwait)
	*pwait = W ? atoi(s_to_c(W)+2) : -1;

    if (pbatchsize)
	*pbatchsize = B ? maxbatchsize(B) : BATCH_OFF;

    s_free(W);
    s_free(B);
}
