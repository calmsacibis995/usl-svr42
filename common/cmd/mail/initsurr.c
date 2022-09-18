/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/initsurr.c	1.11.2.5"
#ident "@(#)initsurr.c	1.21 'attmail mail(1) command'"
#include "mail.h"
#ifdef SVR4_1
# include <mac.h>
#endif
/*
    NAME
	initsurrfile - initialize the surrogate file array

    SYNOPSIS
	void initsurrfile()

    DESCRIPTION
	initsurrfile() reads in the precompiled surrogate file
	and stores everything into the surrfile array. If the
	precompiled surrogate file is out of date, or is corrupted,
	the information will be read from the uncompiled surrogate
	file and the precompiled surrogate file will be recreated.
*/

#define SURRSIZE 50
static t_surrfile _surrfile[SURRSIZE];
t_surrfile *surrfile = &_surrfile[0];
static int surrsize = SURRSIZE;

static char Cmailsurr[] = CMAILSURR;
static char Tmailsurr[] = TMAILSURR;
static char mailcnfg[] = MAILCNFG;

static int a_file_is_newer ARGS((void));
static void compile_surrfile ARGS((int));
static int read_check_string ARGS((FILE *infp, char *cmpstr, int cmplen, int rdlen));
static int read_compiled_surrfile ARGS((void));
static int read_int ARGS((FILE *infp, int *pint));
static int read_regex ARGS((FILE *infp, re_re **pre));
static int read_statlist ARGS((FILE *infp, char **pstatlist));
static int read_string ARGS((FILE *infp, string **pstr));
static void read_surrfile ARGS((void));
static void write_compiled_surrfile ARGS((void));
static int write_cstr ARGS((char *str, FILE *outfp));
static int write_int ARGS((int i, FILE *outfp));
static int write_regex ARGS((re_re *re, FILE *outfp));
static int write_string ARGS((string *str, FILE *outfp));

void initsurrfile()
{
    static const char pn[] = "initsurrfile";
    static int alreadyseen = 0;
    Dout(pn, 2, "Entered\n");

    if (alreadyseen) return;
    else alreadyseen = 1;

    if (flgT || xgetenv("NOCOMPILEDSURRFILE"))
	{
	read_surrfile();
	return;
	}

    if (a_file_is_newer())
	{
	compile_surrfile(1);
	return;
	}

    if (!read_compiled_surrfile())
	{
	compile_surrfile(0);
	return;
	}
}

/*
    NAME
	a_file_is_newer()

    SYNOPSIS
	int a_file_is_newer()

    DESCRIPTION
	Check the timestamp on a couple of
	files against the uncompiled surrogate
	file to see if it is out of date.
*/
static int a_file_is_newer()
{
    static const char pn[] = "a_file_is_newer";
    Dout(pn, 3, "Entered\n");
    if (newer(mailsurr, Cmailsurr))
	{
	Dout(pn, 3, "%s is newer than %s\n", mailsurr, Cmailsurr);
	return 1;
	}

    if (newer(mailcnfg, Cmailsurr))
	{
	Dout(pn, 3, "%s is newer than %s\n", mailsurr, mailcnfg);
	return 1;
	}

    Dout(pn, 3, "returning 0\n");
    return 0;
}

/*
    NAME
	compile_surrfile - create a newly compiled version

    SYNOPSIS
	void compile_surrfile(int check)

    DESCRIPTION
	compile_surrfile() reads in the uncompiled surrogate
	file and writes out the compiled version. Note that there
	is a window between when we decided that the file needed
	rewriting and we get a lock on the file. Se we double
	check to make certain that the surrofate file is still
	out of date.
*/
static void compile_surrfile(check)
int check;
{
    static const char pn[] = "compile_surrfile";
    int ret;

    /* read the slow version */
    read_surrfile();

    /* lock the surrogate file while we rewrite it */
    if ((ret = maildlock("mailsurr", 1, "/etc/mail", 0)) != L_SUCCESS)
	{
	Dout(pn, 2, "Cannot lock, ret = %d\n", ret);
	return;
	}

    /* Make certain the window wasn't caught */
    if (!check || a_file_is_newer())
	write_compiled_surrfile();

    mailunlock();
}

#define MAXBUF 1024
static char MAGIC[] = "<compmailsurr>\n";
#define MAGICLEN (sizeof(MAGIC) - 1)

static int read_compiled_surrfile()
{
    static const char pn[] = "read_compiled_surrfile";
    FILE *infp = fopen(Cmailsurr, "r");
    int nsurr, tmpint, i, j;

    if (!infp)
	{
	Dout(pn, 2, "Cannot open compiled surrogate file\n");
	return 0;
	}

    if (!read_check_string(infp, MAGIC, MAGICLEN, 0))
	{
	Dout(pn, 2, "Bad magic number\n");
    err1:
	(void) fclose(infp);
	return 0;
	}

    if (!read_check_string(infp, mailsystem(1), 0, 1))
	{
	Dout(pn, 2, "Uname does not match\n");
	goto err1;
	}

    if (!read_check_string(infp, maildomain(), 0, 1))
	{
	Dout(pn, 2, "Domain does not match\n");
	goto err1;
	}

    if (fread((char*)&nsurr, sizeof(nsurr), 1, infp) != 1)
	{
	Dout(pn, 2, "Problems reading # surrogate entries\n");
	goto err1;
	}

    if (nsurr < SURRSIZE)
	surrfile = &_surrfile[0];

    else
	surrfile = (t_surrfile*) malloc(nsurr * sizeof(t_surrfile));

    if (!surrfile)
	{
	Dout(pn, 2, "Problems allocating memory for surrogate file\n");
	goto err1;
	}

    surr_len = nsurr;

    for (i = 0; i < surr_len; i++)
	{
	surrfile[i].orig_pattern = 0;
	surrfile[i].orig_regex = 0;
	surrfile[i].recip_pattern = 0;
	surrfile[i].recip_regex = 0;
	surrfile[i].statlist = 0;
	surrfile[i].cmd_left = 0;	/* also takes care of deny_msg */
	surrfile[i].cmd_right = 0;

	if (!read_string(infp, &surrfile[i].orig_pattern)) goto err2;
	if (!read_regex(infp, &surrfile[i].orig_regex)) goto err2;
	if (!read_int(infp, &surrfile[i].orig_nbra)) goto err2;

	if (!read_string(infp, &surrfile[i].recip_pattern)) goto err2;
	if (!read_regex(infp, &surrfile[i].recip_regex)) goto err2;
	if (!read_int(infp, &surrfile[i].recip_nbra)) goto err2;

	if (!read_int(infp, &tmpint)) goto err2;
	surrfile[i].surr_type = (t_surrtype)tmpint;

	if (!read_int(infp, &tmpint)) goto err2;
	if (tmpint)
	    if (!read_statlist(infp, &surrfile[i].statlist)) goto err2;

	if (!read_int(infp, &tmpint)) goto err2;
	if (tmpint)
	    if (!read_string(infp, &surrfile[i].cmd_left)) goto err2;

	if (!read_int(infp, &tmpint)) goto err2;
	if (tmpint)
	    if (!read_string(infp, &surrfile[i].cmd_right)) goto err2;
	if (!read_int(infp, &surrfile[i].batchsize)) goto err2;
	if (!read_int(infp, &surrfile[i].nowait4postprocess)) goto err2;
	if (!read_int(infp, &surrfile[i].fullyresolved)) goto err2;
	}

    (void) fclose(infp);
    return 1;

err2:
    /* Free up all of the space allocated so far */
    for (j = 0; j <= i; j++)
	{
	if (surrfile[i].orig_pattern) s_free(surrfile[i].orig_pattern);
	if (surrfile[i].orig_regex) re_refree(surrfile[i].orig_regex);
	if (surrfile[i].recip_pattern) s_free(surrfile[i].recip_pattern);
	if (surrfile[i].recip_regex) re_refree(surrfile[i].recip_regex);
	if (surrfile[i].statlist) free(surrfile[i].statlist);
	if (surrfile[i].cmd_left) s_free(surrfile[i].cmd_left);
	if (surrfile[i].cmd_right) s_free(surrfile[i].cmd_right);
	}

    if (surrfile != &_surrfile[0])
	{
	free((char*)surrfile);
	surrfile = &_surrfile[0];
	}

    (void) fclose(infp);
    return 0;
}

/*
    NAME
	read_check_string - read and check a string

    DESCRIPTION
	read a string with an optional length before it,
	and compare it with the passed comparison string.
*/
static int read_check_string(infp, cmpstr, cmplen, rdlen)
FILE *infp;
char *cmpstr;
int cmplen;
int rdlen;
{
    char buf[MAXBUF+1];
    int len;

    if (rdlen)
	{
	if (fread((char*)&len, sizeof(len), 1, infp) != 1)
	    return 0;
	if ((len < 0) || (len >= MAXBUF))
	    return 0;
	}

    else
	len = cmplen;

    if (len > 0 && fread(buf, sizeof(char), len, infp) != len)
	return 0;

    buf[len] = '\0';
    if (strcmp(buf, cmpstr) != 0)
	return 0;
    return 1;
}

/* read in a string */
static int read_string(infp, pstr)
FILE *infp;
string **pstr;
{
    int tmpint;
    char buf[MAXBUF+1];

    if (!read_int(infp, &tmpint))
	return 0;

    if (tmpint > MAXBUF)
	{
	if (fread(buf, sizeof(char), MAXBUF, infp) != MAXBUF) return 0;
	buf[MAXBUF] = 0;
	*pstr = s_copy(buf);
	tmpint -= MAXBUF;
	while (tmpint-- > 0)
	    {
	    int c = getc(infp);
	    if (c == EOF) return 0;
	    s_putc(*pstr, c);
	    }
	s_terminate(*pstr);
	}

    else
	{
	if (fread(buf, sizeof(char), tmpint, infp) != tmpint) return 0;
	buf[tmpint] = '\0';
	*pstr = s_copy(buf);
	}

    s_restart(*pstr);
    return 1;
}

/* read in a regular expression */
static int read_regex(infp, pre)
FILE *infp;
re_re **pre;
{
    *pre = re_filere(infp);
    return (*pre != 0);
}

/* read in a string */
static int read_int(infp, pint)
FILE *infp;
int *pint;
{
    return (fread((char*)pint, sizeof(int), 1, infp) == 1);
}

/* read in a stat list */
static int read_statlist(infp, pstatlist)
FILE *infp;
char **pstatlist;
{
    *pstatlist = malloc(sizeof(char) * 256);
    if (!*pstatlist) return 0;
    if (fread(*pstatlist, sizeof(char), 256, infp) != 256) return 0;
    return 1;
}

/*
    NAME
	read_surrfile - read the uncompiled surrogate file

    DESCRIPTION
	Read in the uncompiled surrogate file.
*/
static void read_surrfile()
{
    static const char pn[] = "read_surrfile";
    string *cbuf = 0;
    string *ffield = 0;
    register int i = 0;
    int curresolved = 1;

    FILE *sfp = fopen(mailsurr, "r");

    if (!sfp)
	{
	struct stat statbuf;
	Tout(pn,"cannot open '%s'\n", mailsurr);
	if ((stat(mailsurr, &statbuf) != -1) || (errno == EACCES))
	    lfmt(stderr, MM_ERROR, ":485:Cannot open /etc/mail/mailsurr: check /etc/mail permissions and levels.\n");
	return;
	}

    (void) setsurg_rc((string*)0, DEFAULT, t_transport, (int*)0, (int*)0, (int*)0, (int*)0);
    (void) setsurg_rc((string*)0, DEFAULT, t_translate, (int*)0, (int*)0, (int*)0, (int*)0);

    for (;;)
	{
	t_surrfile nsurr;
	int surr_type;
	int rc;

	nsurr.orig_pattern = s_new();
	cbuf = s_reset(cbuf);

	/* Get the first pattern */
	s_putc(nsurr.orig_pattern, '^');
	if ((rc = getsurr(sfp, nsurr.orig_pattern, TRUE)) == 0)
	    {
	    /* Natural end of file in mailsurr */
	    Tout(pn,"---------- End of '%s' ----------\n", mailsurr);
	    s_free(nsurr.orig_pattern);
	    break;
	    }

	Tout(pn,"---------- Next '%s' entry ----------\n", mailsurr);

	/* Get the second pattern and the command entry */
	nsurr.recip_pattern = s_new();
	s_putc(nsurr.recip_pattern, '^');
	if ((rc < 0) ||
	    (getsurr(sfp, nsurr.recip_pattern, FALSE) < 0) ||
	    (getsurr(sfp, cbuf, FALSE) < 0))
	    {
	    s_terminate(nsurr.recip_pattern);
	    s_terminate(cbuf);
	    Tout(pn, "badly formed mailsurr entry.\n");
	    Tout("", "\toriginator field = '%s'\n", s_to_c(nsurr.orig_pattern));
	    Tout("", "\trecipient field = '%s'\n", s_to_c(nsurr.recip_pattern));
	    Tout("", "\tcommand field = '%s'\n", s_to_c(cbuf));
	    s_free(nsurr.orig_pattern);
	    s_free(nsurr.recip_pattern);
	    break;
	    }

	/* Got one! */
	/* Anchor patterns to the ends of the string */
	s_putc(nsurr.orig_pattern, '$');
	s_terminate(nsurr.orig_pattern);
	s_putc(nsurr.recip_pattern, '$');
	s_terminate(nsurr.recip_pattern);
	Tout(pn, "\toriginator field = '%s'\n", s_to_c(nsurr.orig_pattern));
	Tout("", "\trecipient field = '%s'\n", s_to_c(nsurr.recip_pattern));
	Tout("", "\tcommand field = '%s'\n", s_to_c(cbuf));

	/* Check for appropriate command type */
	s_restart(cbuf);
	switch (surr_type = s_ptr_to_c(cbuf)[0])
	    {
	    /* '< S=;C=;F=;B=; command' */
	    case '<':			/* Delivery */
		Tout(pn, "Found delivery command\n");
		s_skipc(cbuf);

		/* split off any S=C=F=B= */
		s_skipwhite(cbuf);
		switch (s_ptr_to_c(cbuf)[0])
		    {
		    case 'S': case 's':
		    case 'C': case 'c':
		    case 'F': case 'f':
		    case 'B': case 'b':
			if (s_ptr_to_c(cbuf)[1] == '=')
			    {
			    string *statstr = s_tok(cbuf, " \t");
			    nsurr.statlist = setsurg_rc(statstr, REAL, t_transport, &nsurr.batchsize, (int*)0, (int*)0, (int*)0);
			    s_free(statstr);
			    break;
			    }
			/* FALLTHROUGH */

		    default:
			nsurr.statlist = setsurg_rc((string*)0, REAL, t_transport, &nsurr.batchsize, (int*)0, (int*)0, (int*)0);
			break;
		    }

		/* store rest of line */
		nsurr.cmd_left = s_clone(cbuf);
		Tout(pn, "Command = '%s'\n", s_to_c(nsurr.cmd_left));
		nsurr.surr_type = t_transport;
		break;

	    /* '> B=;W=; command' */
	    case '>':			/* Success Postprocessing */
	    /* 'Error B=;W=; command' */
	    case 'E': case 'e':		/* Error Postprocessing */
		Tout(pn, "Found %s postprocessing command\n",
		    (surr_type == '>' ? "Success" : "Error"));

		/* verify spelling of "error" */
		ffield = s_tok(cbuf, " \t");
		if ((surr_type != '>') && casncmp(s_to_c(ffield), "errors", strlen(s_to_c(ffield))))
		    {
		    Tout(pn, "Unknown command field type SKIPPED!\n");
		    s_free(nsurr.orig_pattern);
		    s_free(nsurr.recip_pattern);
		    s_free(ffield);
		    continue;
		    }
		s_free(ffield);

		/* split off any B=W= */
		s_skipwhite(cbuf);
		switch (s_ptr_to_c(cbuf)[0])
		    {
		    case 'B': case 'b':
		    case 'W': case 'w':
			if (s_ptr_to_c(cbuf)[1] == '=')
			    {
			    string *statstr = s_tok(cbuf, " \t");
			    setsurg_bw(statstr, &nsurr.batchsize, &nsurr.nowait4postprocess);
			    s_free(statstr);
			    break;
			    }
			/* FALLTHROUGH */

		    default:
			nsurr.batchsize = BATCH_OFF;
			break;
		    }

		/* store rest of line */
		nsurr.cmd_left = s_clone(cbuf);
		Tout(pn, "Command = '%s'\n", s_to_c(nsurr.cmd_left));
		nsurr.surr_type = (surr_type == '>' ? t_postprocess : t_error);
		nsurr.statlist = 0;
		break;

	    /* 'Translate S=;F=;B=;T=;L=; command' */
	    case 'T': case 't':		/* Translation */
		Tout(pn, "Found translation command\n");
		/* verify spelling of "translate" */
		ffield = s_tok(cbuf, " \t");
		if (casncmp(s_to_c(ffield), "translate", strlen(s_to_c(ffield))))
		    {
		    Tout(pn, "Unknown command field type SKIPPED!\n");
		    s_free(nsurr.orig_pattern);
		    s_free(nsurr.recip_pattern);
		    s_free(ffield);
		    continue;
		    }
		s_free(ffield);

		/* split off any S=F=B=T=L= */
		s_skipwhite(cbuf);
		switch (s_ptr_to_c(cbuf)[0])
		    {
		    case 'S': case 's':
		    case 'F': case 'f':
		    case 'B': case 'b':
		    case 'T': case 't':
		    case 'L': case 'l':
		    case 'E': case 'e':
			if (s_ptr_to_c(cbuf)[1] == '=')
			    {
			    int fullyresolved = 0;
			    int lquit_translate = 0;
			    int lremove_exclams = 0;
			    string *statstr = s_tok(cbuf, " \t");
			    nsurr.statlist = setsurg_rc(statstr, REAL, t_translate, &nsurr.batchsize, &fullyresolved, &lquit_translate, &lremove_exclams);
			    if (fullyresolved)
				nsurr.fullyresolved = curresolved++;
			    else
			        nsurr.fullyresolved = 0;
			    /* overload quit_translate with two boolean values */
			    nsurr.quit_translate = (lquit_translate != 0) | ((lremove_exclams != 0) << 1);
			    s_free(statstr);
			    break;
			    }
			/* FALLTHROUGH */

		    default:
			nsurr.fullyresolved = 0;
			nsurr.quit_translate = 0;
			nsurr.statlist = setsurg_rc((string*)0, REAL, t_translate, &nsurr.batchsize, (int*)0, (int*)0, (int*)0);
			break;
		    }

		/* store rest of line */
		if (casncmp(s_ptr_to_c(cbuf), "r=", 2) == 0)
		    {
		    s_skipc(cbuf);
		    s_skipc(cbuf);
		    s_skipwhite(cbuf);
		    if (s_ptr_to_c(cbuf)[0] == '|')
			{
			s_skipc(cbuf);
			s_skipwhite(cbuf);
			}
		    else
			{
			if (nsurr.batchsize != BATCH_OFF)
			    Tout(pn, "Cannot use batching on replacement translation strings!\n");
			nsurr.batchsize = BATCH_STRING;
			}
		    nsurr.cmd_left = s_clone(cbuf);
		    }

		else
		    {
		    Tout(pn, "Unknown translation type; field SKIPPED!\n");
		    s_free(nsurr.orig_pattern);
		    s_free(nsurr.recip_pattern);
		    continue;
		    }

		Tout(pn, "Translation Command (%s) = '%s'\n",
		    (nsurr.batchsize == BATCH_STRING ? "Replacement" : "Executed"),
		    s_to_c(nsurr.cmd_left));
		nsurr.surr_type = t_translate;
		break;

	    /* 'Accept' */
	    case 'A': case 'a':		/* Accept */
		Tout(pn, "Found Accept command\n");
		/* verify spelling of "accept" */
		if (casncmp(s_to_c(cbuf), "accept", strlen(s_to_c(cbuf))))
		    {
		    Tout(pn, "Unknown command field type SKIPPED!\n");
		    s_free(nsurr.orig_pattern);
		    s_free(nsurr.recip_pattern);
		    continue;
		    }

		nsurr.surr_type = t_accept_name;
		nsurr.batchsize = BATCH_OFF;
		nsurr.cmd_left = 0;
		nsurr.statlist = 0;
		break;

	    /* 'Local' */
	    case 'L': case 'l':		/* Local */
		Tout(pn, "Found Local command\n");
		/* verify spelling of "local" */
		if (casncmp(s_to_c(cbuf), "local", strlen(s_to_c(cbuf))))
		    {
		    Tout(pn, "Unknown command field type SKIPPED!\n");
		    s_free(nsurr.orig_pattern);
		    s_free(nsurr.recip_pattern);
		    continue;
		    }

		nsurr.surr_type = t_quit;
		nsurr.batchsize = BATCH_OFF;
		nsurr.cmd_left = 0;
		nsurr.statlist = 0;
		break;

	    /* 'Deny message' */
	    case 'D': case 'd':		/* Deny */
		Tout(pn, "Found Deny command\n");

		/* verify spelling of deny */
		ffield = s_tok(cbuf, " \t");
		if (casncmp(s_to_c(ffield), "deny", strlen(s_to_c(ffield))))
		    {
		    Tout(pn, "Unknown command field type SKIPPED!\n");
		    s_free(nsurr.orig_pattern);
		    s_free(nsurr.recip_pattern);
		    s_free(ffield);
		    continue;
		    }
		s_free(ffield);

		nsurr.surr_type = t_deny_name;
		nsurr.batchsize = BATCH_OFF;
		nsurr.deny_msg = (s_ptr_to_c(cbuf)[0] != '\0') ? s_clone(cbuf) : 0;
		nsurr.statlist = 0;
		break;

	    default:
		Tout(pn, "Unknown command field type SKIPPED!\n");
		s_free(nsurr.orig_pattern);
		s_free(nsurr.recip_pattern);
		continue;
	    }

	if (nsurr.batchsize >= 0)
	    {
	    nsurr.cmd_right = s_new();
	    if (getsurr(sfp, nsurr.cmd_right, FALSE) < 0)
		{
		Tout(pn, "badly formed mailsurr entry SKIPPED.\n");
		Tout("", "\toriginator field = '%s'\n",
		    s_to_c(nsurr.orig_pattern));
		Tout("", "\trecipient field = '%s'\n",
		    s_to_c(nsurr.recip_pattern));
		Tout("", "\tcommand field = '%s'\n", s_to_c(nsurr.cmd_left));
		Tout("", "\tbatch field = '%s'\n", s_to_c(nsurr.cmd_right));
		s_free(nsurr.orig_pattern);
		s_free(nsurr.recip_pattern);
		s_free(nsurr.cmd_left);
		s_free(nsurr.cmd_right);
		break;
		}

	    else
		Tout("", "Batch field = '%s'\n", s_to_c(nsurr.cmd_right));
	    }

	else
	    nsurr.cmd_right = 0;

	/*
	 * Compile the patterns.
	 */
	nsurr.orig_regex =
	    mailcompile(nsurr.orig_pattern, &nsurr.orig_nbra);
	if (!nsurr.orig_regex)
	    {
	    Tout(pn, "originator pattern compilation failed!\n");
	    s_free(nsurr.orig_pattern);
	    s_free(nsurr.recip_pattern);
	    s_free(nsurr.cmd_left);
	    s_free(nsurr.cmd_right);
	    continue;
	    }
	Dout(pn, 3, "orig_nbra = '%d'\n", nsurr.orig_nbra);

	nsurr.recip_regex =
	    mailcompile(nsurr.recip_pattern, &nsurr.recip_nbra);
	if (!nsurr.recip_regex)
	    {
	    Tout(pn, "recipient pattern compilation failed!\n");
	    s_free(nsurr.orig_pattern);
	    s_free(nsurr.recip_pattern);
	    s_free(nsurr.cmd_left);
	    s_free(nsurr.cmd_right);
	    continue;
	    }
	Dout(pn, 3, "recip_nbra = '%d'\n", nsurr.recip_nbra);

	/* save the info into the surrfile array */
	surrfile[i] = nsurr;

	/* test for a full surrfile array */
	if (++i == surrsize)
	    {
	    surrsize += 10;
	    if (surrfile == _surrfile)
	        {
		surrfile = (t_surrfile*) malloc(surrsize * sizeof(t_surrfile));
		if (surrfile) memcpy(surrfile, _surrfile, sizeof(_surrfile));
		}
	    else
	        surrfile = (t_surrfile*) realloc((char*)surrfile, surrsize * sizeof(t_surrfile));
	    if (!surrfile)
		{
		Tout(pn, "Cannot reallocate space for surrogate file, further entries SKIPPED!\n");
		surrsize = SURRSIZE;
		s_free(nsurr.orig_pattern);
		s_free(nsurr.recip_pattern);
		s_free(nsurr.cmd_left);
		s_free(nsurr.cmd_right);
		i--;
		break;
		}
	    }
	}

    surrfile[i].surr_type = t_eof;
    surr_len = i;
    s_free(cbuf);
}

/*
    NAME
	write_compiled_surrfile - write out the precompiled surrofate file

    SYNOPSIS
	void write_compiled_surrfile()

    DESCRIPTION
	Write the surrogate information to a temporary
	file, then rename it to the real filename.
*/
static void write_compiled_surrfile()
{
    static const char pn[] = "write_compiled_surrfile";
    FILE *outfp;
    int i;

    outfp = fopen(Tmailsurr, "w");
    if (!outfp)
	{
	Dout(pn, 2, "Cannot open temp surrogate file\n");
	lfmt(stderr, MM_ERROR, ":486:Cannot open temp surrogate file: check /etc/mail permissions and levels.\n");
	return;
	}

#ifdef SVR4_1
    {
    /* Try forcing a level onto the new compiled surrogate */
    /* file. Don't worry if lvlfile() fails. */
    struct stat statbuf;
    if (stat("/etc/mail", &statbuf) == 0)
        {
	level_t l = statbuf.st_level;
	(void) lvlfile(Tmailsurr, MAC_SET, &l);
	}
    }
#endif

    if (fwrite(MAGIC, sizeof(char), MAGICLEN, outfp) != MAGICLEN)
	{
    err1:
	(void) fclose(outfp);
    err2:
	Dout(pn, 2, "problem writing temp surrogate file\n");
	lfmt(stderr, MM_ERROR, ":487:Problem writing temp surrogate file: check /etc/mail permissions and levels.\n");
	(void) unlink(Tmailsurr);
	return;
	}

    if (!write_cstr(mailsystem(1), outfp)) goto err1;
    if (!write_cstr(maildomain(), outfp)) goto err1;
    if (!write_int(surr_len, outfp)) goto err1;
    for (i = 0; i < surr_len; i++)
	{
	if (!write_string(surrfile[i].orig_pattern, outfp)) goto err1;
	if (!write_regex(surrfile[i].orig_regex, outfp)) goto err1;
	if (!write_int(surrfile[i].orig_nbra, outfp)) goto err1;

	if (!write_string(surrfile[i].recip_pattern, outfp)) goto err1;
	if (!write_regex(surrfile[i].recip_regex, outfp)) goto err1;
	if (!write_int(surrfile[i].recip_nbra, outfp)) goto err1;

	if (!write_int(surrfile[i].surr_type, outfp)) goto err1;

	if (surrfile[i].statlist)
	    {
	    if (!write_int(1, outfp)) goto err1;
	    if (fwrite(surrfile[i].statlist, sizeof(char), 256, outfp) != 256) goto err1;
	    }
	else
	    if (!write_int(0, outfp)) goto err1;

	if (surrfile[i].cmd_left)
	    {
	    if (!write_int(1, outfp)) goto err1;
	    if (!write_string(surrfile[i].cmd_left, outfp)) goto err1;
	    }
	else
	    if (!write_int(0, outfp)) goto err1;

	if (surrfile[i].cmd_right)
	    {
	    if (!write_int(1, outfp)) goto err1;
	    if (!write_string(surrfile[i].cmd_right, outfp)) goto err1;
	    }
	else
	    if (!write_int(0, outfp)) goto err1;

	if (!write_int(surrfile[i].batchsize, outfp)) goto err1;
	if (!write_int(surrfile[i].nowait4postprocess, outfp)) goto err1;
	if (!write_int(surrfile[i].fullyresolved, outfp)) goto err1;
	}

    if (fclose(outfp) == EOF)
	goto err2;

    if ((chown(Tmailsurr, (uid_t)2, (gid_t)getegid()) == -1) &&
	(posix_chown(Tmailsurr) == -1))
	{
	Dout(pn, 2, "problem with chown()\n");
	lfmt(stderr, MM_ERROR, ":488:Cannot chown temp surrogate file: check /etc/mail permissions and levels.\n");
	return;
	}

    if (rename(Tmailsurr, Cmailsurr) == -1)
	{
	Dout(pn, 2, "problem with rename()\n");
	lfmt(stderr, MM_ERROR, ":489:Cannot replace surrogate file: check /etc/mail permissions and levels.\n");
	return;
	}

    Dout(pn, 2, "finished writing compiled surrogate file\n");
}

/* write out a string */
static int write_string(str, outfp)
string *str;
FILE *outfp;
{
    return write_cstr(s_to_c(str), outfp);
}

/* write out a character string */
static int write_cstr(str, outfp)
char *str;
FILE *outfp;
{
    int len = strlen(str);
    if (!write_int(len, outfp)) return 0;
    if (len > 0 && fwrite(str, sizeof(char), len, outfp) != len) return 0;
    return 1;
}

/* write out a regular expression */
static int write_regex(re, outfp)
re_re *re;
FILE *outfp;
{
    re_refile(re, outfp);
    if (ferror(outfp)) return 0;
    return 1;
}

/* write out an integer */
static int write_int(i, outfp)
int i;
FILE *outfp;
{
    if (fwrite((char*)&i, sizeof(int), 1, outfp) != 1) return 0;
    return 1;
}
