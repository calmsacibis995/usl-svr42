/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)r4xtetris:score.c	1.1"
#endif

#include "defs.h"

update_highscore_table()
{
    /* This version only allows 1 entry in the HIGH SCORE TABLE per user */
        int     i, j;
        long    when;
        extern char *ctime();
        extern long time();
        char    hostname[BUFSIZ];
        char    buf[BUFSIZ];

	if (!resources.usescorefile) return;
	
        /* re-read high-score table in case someone else on the network is
         * playing at the same time */
        read_high_scores();

        /* Check for previous best score */
        for (i = 0; (i < HIGH_TABLE_SIZE) && (strcmp(name, high_scores[i].name) != 0); i++);
        if (i < HIGH_TABLE_SIZE) {
                if (high_scores[i].score >= score)
                        return;         /* Same/worse score - no update */
                for (j = i; j > 0; j--) /* Remove previous best */
                        high_scores[j] = high_scores[j - 1];
        }
        /* Next line finds score greater than current one */
        for (i = 0; ((i < HIGH_TABLE_SIZE) && (score >= high_scores[i].score)); i++);
        i--;
        if (i >= 0) {
                for (j = 0; j < i; j++)
                        high_scores[j] = high_scores[j + 1];
                strcpy(high_scores[i].name, name);
                high_scores[i].score = score;
                high_scores[i].rows = rows;
                high_scores[i].level = rows / 10;
                if (_XGetHostname(hostname, BUFSIZ) == -1)
                        strcpy(high_scores[i].hostname, "unknown-host");
                else
                        strcpy(high_scores[i].hostname, hostname);
                time(&when);
                strcpy(buf, ctime(&when));      /* ctime() adds a newline
                                                 * char */
                strip_eoln(buf);                /* so remove it */
                strcpy(high_scores[i].date, buf);
                write_high_scores();
        }
}


read_high_scores()
{
        FILE   *fp;
        int     i;
        char   buf[BUFSIZ];

	if (!resources.usescorefile) return;
	
        for (i = 0; i < HIGH_TABLE_SIZE; i++) {
                strcpy(high_scores[i].name, " ");
                high_scores[i].score = 0;
                high_scores[i].rows = 0;
                high_scores[i].level = 0;
                strcpy(high_scores[i].hostname, " ");
                strcpy(high_scores[i].date, " ");
        }
        if ((fp = fopen(resources.scorefile, "r")) == NULL) {
	  write_high_scores();
          if ((fp = fopen(resources.scorefile, "r")) == NULL) {
	    resources.usescorefile = False;
	    fprintf(stderr, "tetris: No High score file.  Run with '-noscore' to avoid this message.\n");
                return;
	    }
        }
        for (i = 0; i < HIGH_TABLE_SIZE; i++) {
                fgets(buf, BUFSIZ, fp);
                strip_eoln(buf);
                strcpy(high_scores[i].name, buf);
                fgets(buf, BUFSIZ, fp);
                strip_eoln(buf);
                high_scores[i].score = atoi(buf);
                fgets(buf, BUFSIZ, fp);
                strip_eoln(buf);
                high_scores[i].rows = atoi(buf);
                fgets(buf, BUFSIZ, fp);
                strip_eoln(buf);
                high_scores[i].level = atoi(buf);
                fgets(buf, BUFSIZ, fp);
                strip_eoln(buf);
                strcpy(high_scores[i].hostname, buf);
                fgets(buf, BUFSIZ, fp);
                strip_eoln(buf);
                strcpy(high_scores[i].date, buf);
        }
        fclose(fp);
}

strip_eoln(s)
        char   *s;
{
        char   *s1;

        while (*s != '\0') {
                if (*s == '\n') {       /* End of line char */
                        s1 = s;
                        do {
                                *s1 = *(s1 + 1);        /* Copy rest of string */
                                s1++;
                        } while (*s1 != '\0');
                } else
                        s++;
        }
}

write_high_scores()
{
        FILE   *fp;
        int     i;

        if ((fp = fopen(resources.scorefile, "w")) == NULL) {
                fprintf(stderr, "tetris: Couldn't open high score file %s\n", resources.scorefile);
                return;
        }
        for (i = 0; i < HIGH_TABLE_SIZE; i++)
                fprintf(fp, "%s\n%d\n%d\n%d\n%s\n%s\n",
                        high_scores[i].name,
                        high_scores[i].score,
                        high_scores[i].rows,
                        high_scores[i].level,
                        high_scores[i].hostname,
                        high_scores[i].date);
        fclose(fp);
}

void
print_high_scores()
{
        int     i,j;
        char    buf[BUFSIZ];
	Arg args[20];

	if (!resources.usescorefile) return;

        /* re-read high-score table in case someone else on the network is
         * playing at the same time */
        read_high_scores();

        for (i = HIGH_TABLE_SIZE - 1; i >= 0; i--) {
                sprintf(buf, "%3d) %-15s %6d %5d %3d  %-10s  %s \n",
                        HIGH_TABLE_SIZE - i,
                        high_scores[i].name,
                        high_scores[i].score,
                        high_scores[i].rows,
                        high_scores[i].level,
                        high_scores[i].hostname,
                        high_scores[i].date);
                j=0;
                XtSetArg(args[j], XtNlabel, buf); j++;
                XtSetValues(high_score_item[HIGH_TABLE_SIZE - i],args,j);
        }
	XtPopup(score_frame, XtGrabExclusive);
}

