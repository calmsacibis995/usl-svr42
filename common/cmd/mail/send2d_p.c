/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/send2d_p.c	1.5.2.3"
#ident "@(#)send2d_p.c	1.10 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	send2d_p - send to the surrogate delivery and postdelivery entries

    SYNOPSIS
	void send2d_p(Msg *pmsg, int surr_num)

    DESCRIPTION
	send2d_p() will traverse the given recipient list
	and deliver it or run a postdelivery command.
*/

static void savesurrinfo ARGS((Msg *,int));

void send2d_p(pmsg, surr_num)
Msg	*pmsg;
int	surr_num;
{
    static const char pn[] = "send2d_p";
    t_surrtype surr_type = surrfile[surr_num].surr_type;
    Recip *r, *curmsg;
    char *lbraslist[2 * RE_NBRAK], *lbraelist[2 * RE_NBRAK];
    int onbra = surrfile[surr_num].orig_nbra;
    int origmatch = matchsurr(pmsg->orig, surrfile[surr_num].orig_regex, lbraslist, lbraelist, onbra);
    int batchsize = surrfile[surr_num].batchsize;
    int temp, wherefrom, wheretemp, whereexec;
    int nowait4process = (surr_type == t_transport) ? 0 : surrfile[surr_num].nowait4postprocess;
    SendSurgRet rc, execcmd;

    Tout(pn, "%s Command='%s' '%s'\n",
	((surr_type == t_transport) ? "Delivery" :
				 "Post Delivery"),
	s_to_c(surrfile[surr_num].cmd_left),
	(surrfile[surr_num].cmd_right ? s_to_c(surrfile[surr_num].cmd_right) : "NULL"));

    /* Can't do anything else if we don't match here! */
    if (!origmatch)
	{
	Tout(pn, "No match on originator '%s':'%s'!\n", s_to_c(surrfile[surr_num].orig_pattern), s_to_c(pmsg->orig));
	send2move(pmsg, surr_num, surr_num + 1);
	return;
	}
    Tout(pn, "Matched originator '%s':'%s'!\n", s_to_c(surrfile[surr_num].orig_pattern), s_to_c(pmsg->orig));

    /* print a single message now for all names passed through here */
    if (flgT)
	{
	Tout(pn, "Suppressing execution phase (-T); assuming CONTINUE\n");
	execcmd = CONTINUE;
	}
    else if (flglb)
	{
	Dout(pn, 0, "Suppressing execution phase (-#); assuming SUCCESS\n");
	execcmd = SUCCESS;
	}
    else
	execcmd = FAILURE; /* the name FAILURE is stretching it, but it works just fine */

    /* This loop shuffles the top message plus all matching messages to one temp list, */
    /* while moving the rest of the messages to another temp list. It then executes the */
    /* commands for the one list. For non-batching, only one list is maintained. */
    wherefrom = surr_num;
    wheretemp = surr_len + RECIPS_TEMP;
    whereexec = surr_len + RECIPS_TEMP2;
    while (recips_exist(pmsg, wherefrom) &&
	   (curmsg = send2findleft(pmsg, wherefrom, whereexec, surr_num, lbraslist, lbraelist, onbra)) != (Recip*)NULL)
	{
	/* Expand the delivery command, such as uux */
	/* Fill up the list of matched left parts. */
	/* Move everything else to the temp area. */
	rc = send2findright(pmsg, batchsize, curmsg, wherefrom, whereexec, wheretemp, surr_num,
	    lbraslist+onbra, lbraelist+onbra, ((flgT || flglb) ? "\t" : "Delivery command: "), pn,
	    execcmd, (surr_type == t_transport), nowait4process);

	/* free any saved surrogate info */
	for (r = recips_head(pmsg, whereexec)->next; r != (Recip*)NULL; r = r->next)
	    {
	    s_delete(r->SURRcmd);
	    s_delete(r->SURRoutput);
	    r->SURRrc = SURG_RC_DEF;
	    }

	/* check exit codes */
	switch (rc)
	    {
	    case SUCCESS:
		/* move recip to list->succ_recips */
		send2clean(pmsg, whereexec, surr_len + RECIPS_SUCCESS);
		cleansurrinfo(pmsg);
		break;

	    case CONTINUE:
		/* save the surrogate information for later use */
		savesurrinfo(pmsg, whereexec);
		/* move recip to list->surr_recips[i+1] */
		send2clean(pmsg, whereexec, surr_num + 1);
		break;

	    case FAILURE:
		error = E_SURG;
		Dout(pn, 0, "surrogate command failed, error set to %d\n", error);
		retmail(pmsg, whereexec, E_SURG, ":345:Surrogate command failed\n");

		/* move recip to list->fail_recips */
		send2clean(pmsg, whereexec, surr_len + RECIPS_FAILURE);
		cleansurrinfo(pmsg);
		break;
	    }

	/* the wherefrom queue is now empty, so swap the lists */
	if (batchsize >= 0)
	    {
	    temp = wherefrom;
	    wherefrom = wheretemp;
	    wheretemp = temp;
	    }
	}
}

/*
    NAME
	savesurrinfo - save surrogate information

    SYNOPSIS
	void savesurrinfo(Msg *pmsg, int whereexec)

    DESCRIPTION
	Save the surrogate information stored in pmsg->SURRcmd,
	pmsg->SURRoutfile and pmsg->SURRerrfile into strings associated
	with each recipient. Only save the first ten lines of any output file.
*/
static void savesurrinfo(pmsg, whereexec)
Msg *pmsg;
int whereexec;
{
    string *SURRoutput = s_new();
    Recip *r;

    /* save the stdout */
    if (pmsg->SURRoutfile)
	{
	char buf[1024+1];
	int i = 0;
	FILE *ofp = pmsg->SURRoutfile;
	SURRoutput = s_append(SURRoutput, "     ==== Start of stdout ===\n");
	rewind(ofp);
	buf[0] = buf[1024] = '\0';
	while ((fgets(buf, sizeof(buf)-1, ofp) != (char *)NULL) && (i++ < 10))
	    SURRoutput = s_xappend(SURRoutput, "     :", buf, (char*)0);
	if (fgets(buf, sizeof(buf)-1, ofp) != (char *)NULL)
	    SURRoutput = s_xappend(SURRoutput, "     :...\n", (char*)0);
	}

    /* save the stderr */
    if (pmsg->SURRerrfile)
	{
	char buf[1024+1];
	int i = 0;
	FILE *ofp = pmsg->SURRerrfile;
	SURRoutput = s_append(SURRoutput, "     ==== Start of stderr ===\n");
	rewind(ofp);
	buf[0] = buf[1024] = '\0';
	while ((fgets(buf, sizeof(buf)-1, ofp) != (char *)NULL) && (i++ < 10))
	    SURRoutput = s_xappend(SURRoutput, "     :", buf, (char*)0);
	if (fgets(buf, sizeof(buf)-1, ofp) != (char *)NULL)
	    SURRoutput = s_xappend(SURRoutput, "     :...\n", (char*)0);
	}

    /* graft the saved information into the recipients */
    for (r = recips_head(pmsg, whereexec)->next; r != (Recip*)NULL; r = r->next)
	{
	r->SURRcmd = pmsg->SURRcmd ? s_dup(pmsg->SURRcmd) : 0;
	r->SURRoutput = s_dup(SURRoutput);
	r->SURRrc = pmsg->surg_rc;
	}

    s_free(SURRoutput);
}

/*
    NAME
	cleansurrinfo - clean surrogate information

    SYNOPSIS
	void cleansurrinfo(Msg *pmsg)

    DESCRIPTION
	Clean up the surrogate information stored in pmsg->SURRcmd,
	pmsg->SURRoutfile, pmsg->SURRerrfile and pmsg->surg_rc.
*/
void cleansurrinfo(pmsg)
Msg *pmsg;
{
    if (pmsg->SURRoutfile)
	{
	fclose(pmsg->SURRoutfile);
	pmsg->SURRoutfile = 0;
	}

    if (pmsg->SURRerrfile)
	{
	fclose(pmsg->SURRerrfile);
	pmsg->SURRerrfile = 0;
	}

    s_delete(pmsg->SURRcmd);
    pmsg->surg_rc = SURG_RC_DEF;
}
