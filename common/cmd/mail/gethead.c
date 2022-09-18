/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/gethead.c	1.13.2.2"
#ident "@(#)gethead.c	2.18 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	gethead - display headers, indicating current and status

    SYNOPSIS
	int gethead(Letinfo *pletinfo, int current, int all)

    DESCRIPTION
	Gethead() displays a given range of a letter's headers.

	"current" is the displacement into the mailfile of the 
	current letter.

	"all" indicates how many headers should be shown.
		0	->	show window +/-6 around current
		1	->	show all messages
		2	->	show deleted messages

    RETURNS
	0 - a successful display
	1 - an error occurred
*/

int gethead(pletinfo, current, all)
Letinfo	*pletinfo;
int	current;
int	all;
{
	int	displayed = 0;
	FILE	*file;
	char	holdval[LSIZE];
	char	*hold;
	char	wline[LSIZE];
	int	ln;
	char	mark;
	int	size, start, stop;
	string	*userval;
	int	newmail = pletinfo->nlet - pletinfo->onlet;
	int	ret = 0;

	if (pletinfo->nlet == 1)
		if (pletinfo->changed == 1)
			if (newmail == 1)
				pfmt(stdout, MM_NOSTD,
					":48:1 letter found in %s, 1 scheduled for deletion, 1 newly arrived\n", mailfile);
			else
				pfmt(stdout, MM_NOSTD,
					":49:1 letter found in %s, 1 scheduled for deletion, %d newly arrived\n", mailfile, newmail);
		else
			if (newmail == 1)
				pfmt(stdout, MM_NOSTD,
					":50:1 letter found in %s, %d scheduled for deletion, 1 newly arrived\n", mailfile, pletinfo->changed);
			else
				pfmt(stdout, MM_NOSTD,
					":51:1 letter found in %s, %d scheduled for deletion, %d newly arrived\n", mailfile, pletinfo->changed, newmail);
	else
		if (pletinfo->changed == 1)
			if (newmail == 1)
				pfmt(stdout, MM_NOSTD,
					":52:%d letters found in %s, 1 scheduled for deletion, 1 newly arrived\n", pletinfo->nlet, mailfile);
			else
				pfmt(stdout, MM_NOSTD,
					":53:%d letters found in %s, 1 scheduled for deletion, %d newly arrived\n", pletinfo->nlet, mailfile, newmail);
		else
			if (newmail == 1)
				pfmt(stdout, MM_NOSTD,
					":54:%d letters found in %s, %d scheduled for deletion, 1 newly arrived\n", pletinfo->nlet, mailfile, pletinfo->changed);
			else
				pfmt(stdout, MM_NOSTD,
					":55:%d letters found in %s, %d scheduled for deletion, %d newly arrived\n", pletinfo->nlet, mailfile, pletinfo->changed, newmail);

	if (all==2 && !pletinfo->changed) return(0);

	userval = s_new();
	file = doopen(pletinfo->tmpfile.lettmp,"r",E_TMP);
	if (!flgr) {
		stop = current - 6;
		if (stop < -1) stop = -1;
		start = current + 5;
		if (start > pletinfo->nlet - 1) start = pletinfo->nlet - 1;
		if (all) {
			start = pletinfo->nlet -1;
			stop = -1;
		}
	}
	else {
		stop = current + 6;
		if (stop > pletinfo->nlet) stop = pletinfo->nlet;
		start = current - 5;
		if (start < 0) start = 0;
		if (all) {
			start = 0;
			stop = pletinfo->nlet;
		}
	}

	for (ln = start; ln != stop; ln = flgr ? ln + 1 : ln - 1) {
		size = pletinfo->let[ln+1].adr - pletinfo->let[ln].adr;
		if (fseek(file, pletinfo->let[ln].adr, 0) != 0) {
			errmsg(E_FILE,":361:Cannot seek header\n");
			ret = 1;
			break;
		}
		if (fgets(wline, sizeof(wline), file) == NULL) {
			errmsg(E_FILE,":362:Cannot read header\n");
			ret = 1;
			break;
		}
		if (strncmp(wline, header[H_FROM].tag, 5) != SAME) {
			errmsg(E_FILE,":363:Invalid header encountered\n");
			ret = 1;
			break;
		}
		/* Find the LAST ">From...remote from" line and copy it into "hold" */
		hold = holdval;
		strcpy(hold, wline + 5);
		while (fgets(wline, sizeof(wline), file) &&
		       (strncmp(wline, header[H_FROM1].tag, 6) == SAME))
			if (substr(wline,"remote from ") != -1)
				strcpy(hold, wline + 6);
	
		/* Copy user name into "userval". If find "!", start over. */
		s_restart(userval);
		for ( ; !isspace(*hold); hold++) {
			s_putc(userval, *hold);
			if (*hold == '!') s_restart(userval);
		}
		s_terminate(userval);

		/* Trim the rest of the line */
		hold = (char*)skipspace(hold);
		trimnl(hold);
	
		if (!flgh && current == ln) mark = '>';
		else mark = ' ';
	
		if (all == 2) {
			if (displayed >= pletinfo->changed) {
				ret = 0;
				break;
			}
			if (pletinfo->let[ln].change == ' ')
				continue;
		}
		printf("%c %3d  %c  %-5d  %-10s  %s\n", mark, ln + 1, pletinfo->let[ln].change,
		    size, s_to_c(userval), hold);
		displayed++;
	}

	fclose(file);
	s_free(userval);
	return ret;
}
