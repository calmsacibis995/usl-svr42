/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/send2tran.c	1.6.2.4"
#ident "@(#)send2tran.c	1.12 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	send2tran - send to the translation surrogate entry

    SYNOPSIS
	int send2tran(Msg *pmsg, int surr_num)

    DESCRIPTION
	send2tran() will traverse the given recipient list and run the
	translation command on it. The translations are placed back onto the
	recipient list. A name will either be moved directly to the
	RECIPS_DONE queue if it's translated, or kept (moved to the next
	queue) if that name is found within the translation output. The mail
	will be returned if an error occurs during translation.

	For a string replacement:
	    if string == NOTHING
		keep origname
	    else if string == origname
		keep origname
	    else
		add string
		origname is done

	For a nonbatched translation, the output will look like a list of
	names, as in
	    name1 name2 name3
	    name4 name5
	Note that multiple lines may be used to express the translation.

	    if translation command succeeded
		if no output (no translations occurred)
		    keep origname
		else
		    if any of the names == origname
			keep origname
		    for each other name
			add new name
	    else translation command failed
		return the stderr in a message

	For a batched translation, the output will look like a list of names,
	each line prefixed with the original name whose translation follows,
	as in
	    orignameA nameA1 nameA2 nameA3
	    orignameB nameB1 nameB2
	    orignameA nameA4 nameA5
	Note that a name may have multiple translation lines.

	    if translation command succeeded
		if no output (no translations occurred)
		    keep orignames
		else
		    for each line of output
			if any of the names == the origname
			    keep that origname
			for each other name
			    add new name
	    else translation command failed
		for each line of stdout output
		    if any of the names == the origname
			keep that origname
		    for each other name
			add new name
		return the stderr in a message listing all orignames not so translated

	The "fullytranslated" number means that T=1 was specified in the
	surrogate file. It is an indication that if a name which was
	translated by this surrogate command ever makes it back here to be
	translated again, it needn't be passed through the command again as
	it has been "fully translated by that command." Instead, it will be
	moved to the next queue.
*/

int send2tran(pmsg, surr_num)
Msg	*pmsg;
int	surr_num;
{
    static const char pn[] = "send2tran";
    Recip *l, *r, *curmsg;
    char *lbraslist[2 * RE_NBRAK], *lbraelist[2 * RE_NBRAK];
    int onbra = surrfile[surr_num].orig_nbra;
    int origmatch = matchsurr(pmsg->orig, surrfile[surr_num].orig_regex, lbraslist, lbraelist, onbra);
    int batchsize = surrfile[surr_num].batchsize;
    int fullyresolved = surrfile[surr_num].fullyresolved; /* from T=1; in the surrogate */
    int temp, wherefrom, wheretemp, whereexec, wherefail;
    SendSurgRet rc;
    int ret = 0; /* set to TRUE if any translations occurred */
    int whereto = (surrfile[surr_num].quit_translate & 1) ? (surr_len + RECIPS_LOCAL) : 0;
    int stripbangs = (surrfile[surr_num].quit_translate & 2);

    Tout(pn, "Translation Command='%s' '%s'\n",
	s_to_c(surrfile[surr_num].cmd_left),
	(surrfile[surr_num].cmd_right ? s_to_c(surrfile[surr_num].cmd_right) : "NULL"));

    /* Can't do anything else if we don't match here! */
    if (!origmatch)
	{
	Tout(pn, "No match on originator '%s':'%s'!\n", s_to_c(surrfile[surr_num].orig_pattern), s_to_c(pmsg->orig));
	send2move(pmsg, surr_num, surr_num + 1);
	return 0;
	}
    Tout(pn, "Matched originator '%s':'%s'!\n", s_to_c(surrfile[surr_num].orig_pattern), s_to_c(pmsg->orig));

    wherefrom = surr_num;
    wheretemp = surr_len + RECIPS_TEMP;
    whereexec = surr_len + RECIPS_TEMP2;
    wherefail = surr_len + RECIPS_TEMP3;

    /* Look for names already fully resolved at this level */
    if (fullyresolved > 0)
	{
	Dout(pn, 40, "Looking for fully resolved names\n");
	for (l = recips_head(pmsg, wherefrom); (r = l->next) != (Recip*)NULL; )
	    {
	    if (r->fullyresolved == fullyresolved)
		send2mvrecip(pmsg, wherefrom, surr_num + 1);
	    else
		send2mvrecip(pmsg, wherefrom, wheretemp);
	    }
	/* now the messages are in RECIPS_TEMP */
	temp = wheretemp;
	wheretemp = wherefrom;
	wherefrom = temp;
	}

    /* This loop shuffles the top message plus all matching messages to one temp list, */
    /* while moving the rest of the messages to another temp list. It then executes the */
    /* commands for the one list. For non-batching, only one list is maintained. */
    while (recips_exist(pmsg, wherefrom) &&
	   (curmsg = send2findleft(pmsg, wherefrom, whereexec, surr_num, lbraslist, lbraelist, onbra)) != (Recip*)NULL)
	{
	/* Expand the translation command, such as mailalias */
	if (batchsize == BATCH_STRING)
	    {
	    Tout(pn, "straight replacement...\n");
	    s_restart(curmsg->cmdl);
	    s_skipwhite(curmsg->cmdl);
	    if (strcmp(s_to_c(curmsg->name), s_ptr_to_c(curmsg->cmdl)) == 0)
		{
		/* Translation returned original name. Continue... */
		Tout("", "\nof same name!\n");
		send2mvrecip(pmsg, whereexec, surr_num + 1);
		}

	    else if (s_ptr_to_c(curmsg->cmdl)[0] == '\0')
		{
		/* Translation into nothing? */
		Tout("", "found end of string, no replacement!\n");
		send2mvrecip(pmsg, whereexec, surr_num + 1);
		}

	    else
		{
		/* Straight replacement */
		Tout("", "adding new name '%s'\n", s_ptr_to_c(curmsg->cmdl));
		add_recip(pmsg, s_ptr_to_c(curmsg->cmdl), TRUE, curmsg, fullyresolved, FALSE, -1, -1, whereto);
		ret = 1;
		curmsg->lastsurrogate = surr_num;
		send2mvrecip(pmsg, whereexec, surr_len + RECIPS_DONE);
		}

	    if (curmsg->cmdl) s_delete(curmsg->cmdl);
	    if (curmsg->cmdr) s_delete(curmsg->cmdr);
	    continue;
	    }

	/* Fill up the list of matched left parts. */
	/* Move everything else to the temp area. */
	rc = send2findright(pmsg, batchsize, curmsg, wherefrom, whereexec, wheretemp, surr_num,
	    lbraslist+onbra, lbraelist+onbra, "Translate Command: ", pn, 1, 1, 0);

	/* Handle translations. */
	/* Move successful translations to RECIPS_DONE, new names to 0, leave failures alone. */
	rewind(pmsg->SURRoutfile);

	/* Unbatched command */
	if (batchsize == BATCH_OFF)
	    {
	    int num_added = 0, found_self = 0;
	    string *buf = s_new();
	    /* Read the translation output. It looks like:
		translation1 translation2 ...
	    */
	    while ((s_restart(buf), s_read_line(pmsg->SURRoutfile, buf)) != (char*)NULL)
		{
		trimnl(s_to_c(buf));
		Dout(pn, 9, "Translation '%s'\n", s_to_c(buf));
		found_self |= madd_recip(pmsg, s_to_c(buf), TRUE, curmsg, fullyresolved, stripbangs, -1, -1, whereto);
		num_added++;
		}
	    s_delete(buf);

	    if (num_added == 0)
		{
		if (rc == SUCCESS)
		    send2clean(pmsg, whereexec, surr_num + 1);
		else
		    send2move(pmsg, whereexec, wherefail);
		}

	    else
		{
		ret = 1;
		if (found_self)
		    send2clean(pmsg, whereexec, surr_num + 1);
		else if (rc == SUCCESS)
		    {
		    for (r = recips_head(pmsg, whereexec)->next; r != (Recip*)NULL; r = r->next)
			r->lastsurrogate = surr_num;
		    send2clean(pmsg, whereexec, surr_len + RECIPS_DONE);
		    }
		else
		    send2move(pmsg, whereexec, wherefail);
		}
	    }

	/* batched command */
	else
	    {
	    /* Save the left part of the command so that it */
	    /* can be put into return mail on failure. */
	    Recip *svr = recips_head(pmsg, whereexec)->next;
	    string *svcmdl = svr->cmdl;
	    string *buf = s_new();
	    svr->cmdl = 0;

	    for (r = recips_head(pmsg, whereexec)->next; r != (Recip*)NULL; r = r->next)
		r->translated = 0;

	    /* Read the translation output. It looks like:
		user translation1 translation2 ...
	    */
	    while ((s_restart(buf), s_read_line(pmsg->SURRoutfile, buf)) != (char*)NULL)
		{
		char *namefrom = (char*)skipspace(s_to_c(buf));	/* beginning of 1st name */
		char *tmp = (char*)skiptospace(namefrom);	/* beginning of whitespace after 1st name */
		char *nameto = (char*)skipspace(tmp);		/* the translations */
		*tmp = '\0';
		trimnl(nameto);

		Dout(pn, 9, "Translation '%s' -> '%s'\n", namefrom, nameto);
		for (r = recips_head(pmsg, whereexec)->next; r != (Recip*)NULL; r = r->next)
		    if (strcmp(namefrom, s_to_c(r->name)) == 0)
			break;

		if (r != (Recip*)0)
		    {
		    r->translated = 2 | madd_recip(pmsg, nameto, TRUE, r, fullyresolved, stripbangs, wherefrom, wheretemp, whereto);
		    ret = 1;
		    }
		}
	    s_delete(buf);

	    /* Check the exit codes. */
	    for (l = recips_head(pmsg, whereexec); (r = l->next) != (Recip*)NULL; )
		{
		int whereto;
		/* A translation-to-self occurred. */
		if (r->translated & 1)
		    {
		    r->translated &= ~1;
		    whereto = surr_num + 1;
		    }
		/* This name was translated. */
		else if (r->translated)
		    {
		    r->lastsurrogate = surr_num;
		    whereto = surr_len + RECIPS_DONE;
		    }
		/* This name was not translated, but successful. */
		else if (rc == SUCCESS)
		    whereto = surr_num + 1;
		/* This name was not translated and an error occurred. */
		else
		    whereto = wherefail;
		if (whereto != wherefail)
		    {
		    if (r->cmdl) s_delete(r->cmdl);
		    if (r->cmdr) s_delete(r->cmdr);
		    }
		send2mvrecip(pmsg, whereexec, whereto);
		}

	    /* Restore the left part of the command onto the head */
	    /* of the failed list so that retmail can return it. */
	    svr = recips_head(pmsg, wherefail);
	    if (svr && !svr->cmdl) /* checking for !svr->cmdl is defensive programming */
		svr->cmdl = svcmdl;
	    else
		s_free(svcmdl);
	    }

	if (recips_exist(pmsg, wherefail))
	    {
	    error = E_TRAN;
	    Dout(pn, 0, "translation command failed, error set to %d\n", error);
	    retmail(pmsg, wherefail, E_TRAN, ":342:Translation command failed\n");

	    /* move recip to list->fail_recips */
	    send2clean(pmsg, wherefail, surr_len + RECIPS_FAILURE);
	    }

	/* swap the lists */
	if (batchsize >= 0)
	    {
	    temp = wherefrom;
	    wherefrom = wheretemp;
	    wheretemp = temp;
	    }
	}

    cleansurrinfo(pmsg);
    return ret;
}
