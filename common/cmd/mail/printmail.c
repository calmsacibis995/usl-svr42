/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/printmail.c	1.15.2.3"
#ident "@(#)printmail.c	2.34 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	printmail - High level mail printing routine

    SYNOPSIS
	void printmail()

    DESCRIPTION
	This is the main interactive mail reading loop.
*/

static	char	MAminvalidcmd[] = ":137:Invalid command\n";
static int readlock ARGS((void));
static void readunlock ARGS((void));

void printmail()
{
	static const char pn[] = "printmail";
	Letinfo	letinfo;
	int	flg, curlet, showlet, k, print, aret, stret, rc;
	int	pflg;		/* Typed 'p' command' */
	int	Pflg;		/* Typed 'P' command' */
	int	nsmbox = 0;	/* 1 ==> mailbox is in non-standard place */
	int	sav_showlet = -1;
	char	*p;
	struct	stat stbuf;
	CopyLetFlags ttyf = isatty(1) ? TTY : ORDINARY;
	char	readbuf[LSIZE];	/* holds user's response in interactive mode */
	char	tmpbuf[1024];
	char	*resp;
	FILE	*malf;
	long	file_size = 0;
	struct utimbuf utimeb;
	struct stat statbuf;
	int	readonly;

	init_Letinfo(&letinfo);

	/* Secure the mailfile to guarantee integrity */
	(void) lock(my_name, 0);

	/* Check for old-style forwarding */
	oldforwarding(my_name);

	/* See if mail is to be forwarded to another system */
	if (areforwarding(my_name, tmpbuf, sizeof(tmpbuf)))
		pfmt(stdout, MM_INFO, ":404:Your mail is being forwarded to %s\n", tmpbuf);

	unlock();

	/* create working directory mbox name, either $HOME/mbox or $HOME/mbox/mbox */
	if ((hmbox = malloc((unsigned)(strlen(home) + 2 * strlen(mbox) + 2))) == NULL) {
		errmsg(E_MBOX,"");
		fini_Letinfo(&letinfo);
		return;
	}
	cat(hmbox, home, mbox);
	if ((stat(hmbox, &statbuf) == 0) && ((statbuf.st_mode & S_IFMT) == S_IFDIR))
		strcat(hmbox, mbox);

	nsmbox = setmailfile();

	/*
		Get ACCESS and MODIFICATION times of mailfile BEFORE we
		use it. This allows us to put them back when we are
		done. If we didn't, the shell would think NEW mail had
		arrived since the file times would have changed.
	*/
	stret = CERROR;
	if (access(mailfile, F_OK) == CSUCCESS) {
		if ((stret = stat(mailfile, &stbuf)) == CERROR) {
			errmsg(E_FILE,":401:Cannot access mailfile\n");
			fini_Letinfo(&letinfo);
			return;
		}
		mf_gid = stbuf.st_gid;
		mf_uid = stbuf.st_uid;
		utimeb.actime = stbuf.st_atime;
		utimeb.modtime = stbuf.st_mtime;
		file_size = stbuf.st_size;
	} else {
		utimeb.modtime = utimeb.actime = -1;
	}

	/* Check accessibility of mail file, and open it */
	if ((aret=access(mailfile, R_OK)) == CSUCCESS) malf = fopen(mailfile, "r");
	/* stat succeeded, but we cannot access the mailfile */
	if (stret == CSUCCESS && aret == CERROR) {
		errmsg(E_PERM, ":402:Invalid permissions on %s\n", mailfile);
		fini_Letinfo(&letinfo);
		return;
	} else if (!nsmbox && flgf && (aret == CERROR || (malf == NULL))) {
		/* using an alternate mailfile, but we failed on access */
		errmsg(E_FILE, ":403:Cannot open mailfile: %s\n", strerror(errno));
		fini_Letinfo(&letinfo);
		return;
	}
	/*
		we failed to access OR the file is empty
	*/
	else if (aret == CERROR || (malf == NULL) || (stbuf.st_size == 0)) {
		pfmt(stdout, MM_INFO, ":141:No mail.\n");
		error = E_FLGE;
		Dout(pn, 0, "error set to %d\n", error);
		fini_Letinfo(&letinfo);
		return;
	}

	/* copy mail to temp file and mark each letter in the let array */
	dlock(mailfile, 0);
	mktmp(&letinfo.tmpfile);
	copymt(&letinfo, malf, letinfo.tmpfile.tmpf);
	letinfo.onlet = letinfo.nlet;
	fclose(malf);
	fclose(letinfo.tmpfile.tmpf);
	unlock();	/* All done, OK to unlock now */
	letinfo.tmpfile.tmpf = doopen(letinfo.tmpfile.lettmp,"r+",E_TMP);
	print = 1;

	readonly = readlock();

	for (curlet = 0; curlet < letinfo.nlet; ) {
		/* reverse order? */
		showlet = flgr ? curlet : letinfo.nlet - curlet - 1;

		if (setjmp(sjbuf) == 0 && print != 0) {
				/* -h says to print the headers first */
				if (flgh) {
					gethead(&letinfo, showlet,0);
					flgh = 0;	/* Only once */
					/* set letter # to invalid # */
					curlet--;
					showlet = flgr ? curlet : letinfo.nlet - curlet - 1;
				} else {
					if (showlet != sav_showlet) {
						/* Looking at new message. */
						/* Reset flag to override */
						/* non-display of binary */
						/* contents */
						sav_showlet = showlet;
						pflg = 0;
						Pflg = flgP;
					}
					copylet(&letinfo, showlet, stdout, ttyf, pflg, Pflg);
				}
		}

		/* print only */
		if (flgp) {
			curlet++;
			continue;
		}

		/* Interactive */
		setjmp(sjbuf);
		stat(mailfile, &stbuf);
		if (stbuf.st_size != file_size) {
			/* New mail has arrived, load it */
			int countnew;
			k = letinfo.nlet;
			dlock(mailfile, 0);
			malf = doopen(mailfile,"r",E_FILE);
			fclose(letinfo.tmpfile.tmpf);
			letinfo.tmpfile.tmpf = doopen(letinfo.tmpfile.lettmp, "a", E_TMP);
			fseek(malf, letinfo.let[letinfo.nlet].adr, 0);
			copymt(&letinfo, malf, letinfo.tmpfile.tmpf);
			file_size = stbuf.st_size;
			fclose(malf);
			fclose(letinfo.tmpfile.tmpf);
			unlock();
			letinfo.tmpfile.tmpf = doopen(letinfo.tmpfile.lettmp,"r+",E_TMP);
			countnew = letinfo.nlet - k;
			if (countnew > 1)
				pfmt(stdout, MM_INFO,
					":143:New mail loaded into letters %d - %d\n",
					++k, letinfo.nlet);
			else
				pfmt(stdout, MM_INFO,
					":144:New mail loaded into letter %d\n",
					letinfo.nlet);
			if (!flgr) {
				curlet += countnew;
				showlet = letinfo.nlet - curlet - 1;
			}
		}

		/* read the command */
		if (debug)
			printf("c=%d,s=%d ? ", curlet, showlet+1);
		else
			printf("? ");
		fflush(stdout);
		fflush(stderr);
		if (fgets(readbuf, sizeof(readbuf), stdin) == NULL)
			break;
		resp = (char*)skipspace(readbuf);
		Dout(pn, 0, "resp = '%s'\n", resp);
		print = 1;
		if ((rc = atoi(resp)) != 0) {
			if (!validmsg(&letinfo, rc)) print = 0;
			else curlet = flgr ? rc - 1 : letinfo.nlet - rc;
		} else switch (resp[0]) {
			default:
				pfmt(stdout, MM_ERROR, ":120:Incorrect usage\n");
				pfmt(stdout, MM_ACTION, ":145:Usage:\n");
				/* FALLTHROUGH */
			/* help */
			case '?':
				print = 0;
				for (rc = 0; help[rc]; rc++)
					pfmt(stdout, MM_NOSTD, help[rc]);
				break;

			/* print message number of current message */
			case '#':
				print = 0;
				if ((showlet==letinfo.nlet) || (showlet<0)) {
					pfmt(stdout, MM_INFO,
						":146:No message selected yet.\n");
				} else {
					pfmt(stdout, MM_INFO,
						":147:Current message number is %d\n",
						showlet+1);
				}
				break;

			/* headers */
			case 'h':
				print = 0;
				if ( resp[2] != 'd' && resp[2] != 'a' &&
				    (rc = getnumbr(&letinfo, resp+1)) > 0) {
					showlet = rc - 1;
					curlet = flgr ? rc - 1 : letinfo.nlet - rc- 1;
				}
				if (rc == -1 && resp[2] != 'a' && resp[2] != 'd') break;
				if (resp[2] == 'a') rc = 1;
				else if (resp[2] == 'd') rc = 2;
					else rc = 0;

				gethead(&letinfo, showlet,rc);
				break;

			/* skip entry */
			case '+':
			case 'n':
			case '\0':
				curlet++;
				break;

			/* print ALL headers */
			case 'P':
				Pflg++;
				break;

			/* print even if binary */
			case 'p':
				pflg++;
				break;

			/* quit, but don't update */
			case 'x':
				letinfo.changed = 0;
				/* FALLTHROUGH */

			/* quit */
			case 'q':
				goto donep;

			/* Previous entry */
			case '^':
			case '-':
				if (--curlet < 0) curlet = 0;
				break;

			/* Save in file without header */
			case 'y':
			case 'w':
			/* Save mail with header */
			case 's':
				print = 0;
				trimnl(resp);
				if (!validmsg(&letinfo, curlet)) break;
				if (resp[1] == '\0') {
					strcpy(resp + 1, hmbox);
				} else if (resp[1] != ' ') {
					pfmt(stdout, MM_ERROR, MAminvalidcmd);
					break;
				}
				(void) umask(umsave);
				flg = 0;
				if (getarg(tmpbuf, resp + 1) == NULL) {
					strcpy(resp + 1, hmbox);
				}
				malf = (FILE *)NULL;
				for (p = resp + 1;
				    (p = getarg(tmpbuf, p)) != NULL; ) {
					if (flg) {
					    pfmt(stderr, MM_WARNING,
						":148:File '%s' skipped\n", tmpbuf);
					    continue;
					}
					malf = NULL;
					if ((aret = legal(tmpbuf)) != 0) {
						malf = fopen(tmpbuf, "a");
					}
					if ((malf == NULL) || (aret == 0)) {
					    pfmt(stderr, MM_WARNING,
						":149:Cannot append to %s\n", tmpbuf);
					    flg++;
					} else if (aret == 2) {
						if (chown(tmpbuf,my_euid,my_gid) == -1)
						    (void) posix_chown(tmpbuf);
					}
					if (!flg &&
					    copylet(&letinfo, showlet,malf,resp[0] ==
					      's'? ORDINARY : ZAP, pflg, Pflg) == FALSE) {
						pfmt(stderr, MM_WARNING,
						    ":405:Cannot save mail to '%s': %s\n",
						    tmpbuf, strerror(errno));
						flg++;
					} else
						Dout(pn, 0, "!saved\n");
					if (malf != (FILE *)NULL) {
						fclose(malf);
					}
				}
				(void) umask(MAILMASK);
				if (!flg) {
					setletr(&letinfo, showlet, resp[0]);
					print = 1;
					curlet++;
				}
				break;

			/* Reply to a letter */
			case 'r':
			/* Reply to a letter, including copy of current letter */
			case 'R':
				print = 0;
				if (!validmsg(&letinfo, curlet)) break;
				trimnl(resp);
				if (goback(&letinfo, showlet, (char*)skipspace(resp+1), 1, 1, (resp[0] == 'R')))
				    setletr(&letinfo, showlet, 'r');
				break;

			/* Mail letter to someone else */
			case 'm':
			/* Mail letter to someone else with comments first */
			case 'M':
				print = 0;
				if (!validmsg(&letinfo, curlet)) break;
				trimnl(resp);
				if (goback(&letinfo, showlet, (char*)skipspace(resp+1), 0, (resp[0] == 'M'), 1))
				    setletr(&letinfo, showlet, resp[0]);
				break;

			/* Undelete */
			case 'u':
				print = 0;
				if ((k = getnumbr(&letinfo, resp+1)) <= 0) k=showlet;
				else k--;
				if (!validmsg(&letinfo, k)) break;
				setletr(&letinfo, k, ' ');
				break;

			/* Read new letters */
			case 'a':
				if (letinfo.onlet == letinfo.nlet) {
					pfmt(stdout, MM_INFO,
						":153:No new mail\n");
					print = 0;
					break;
				}
				curlet = 0;
				print = 1;
				break;

			/* Escape to shell */
			case '!':
				resp = (char*)skipspace(resp + 1);
				trimnl(resp);
				printf("!%s\n", resp);
				systm(resp);
				printf("!\n");
				print = 0;
				break;

			/* Delete an entry */
			case 'd':
				print = 0;
				k = 0;
				if (strncmp("dq",resp,2) != SAME &&
					strncmp("dp",resp,2) != SAME)
					if ((k = getnumbr(&letinfo, resp+1)) == -1) break;
				if (k == 0) {
					k = showlet;
					if (!validmsg(&letinfo, curlet)) break;
					print = 1;
					curlet++;
				}
				else k--;

				setletr(&letinfo, k, 'd');
				if (resp[1] == 'p') print = 1;
				else if (resp[1] == 'q') goto donep;
				break;
		}
	}

   donep:
	readunlock();
	/* Copy updated mailfile back */
	if (letinfo.changed) {
		if (readonly) {
			pfmt(stderr, MM_INFO, ":467:Reminder, this instance of mail is read only.\n");
		} else {
			copyback(&letinfo);
			stamp(&utimeb);
		}
	}
	fini_Letinfo(&letinfo);
}

static int rdlock = 0;

static int readlock()
{
    string *maillock;
    int reason = mailrdlock(my_name);

    switch (reason)
	{
	case L_SUCCESS:
	    rdlock = 1;
	    return 0;

	case L_MAXTRYS:
	    pfmt(stderr, MM_WARNING, ":271:You are already reading mail.\n");
	    pfmt(stderr, MM_NOSTD, ":466:\tThis instance of mail is read only.\n");
	    rdlock = 0;
	    return 1;

	default:
	    maillock = s_xappend((string*)0, RDLKDIR, my_name, ".lock", (char*)0);
	    pfmt(stderr, MM_WARNING, ":465:Cannot create read lock '%s': %s\n", s_to_c(maillock), strerror(errno));
	    s_free(maillock);
	    rdlock = 1;
	    return 0;
	}
}

static void readunlock()
{
    if (rdlock)
	{
	mailurdlock();
	rdlock = 0;
	}
}
