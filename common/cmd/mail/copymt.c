/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/copymt.c	1.12.2.2"
#ident "@(#)copymt.c	2.17 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	copymt - copy mail (f1) to temp (f2)

    SYNOPSIS
	void copymt(FILE *f1, FILE *f2)

    DESCRIPTION
	The mail messages in /var/mail are copied into
	the temp file. The file pointers f1 and f2 point
	to the files, respectively.
*/

static const char MAmwrerr[] = ":357:Write error in copymt(): %s\n";
static char Unknown[] = "Unknown";

static char *getencdlocale ARGS((const char *line));

void copymt(pletinfo, f1, f2)
Letinfo *pletinfo;
register FILE *f1, *f2;
{
	static const char pn[] = "copymt";
	char	line[LSIZE];	/* holds a line of a letter */
	long nextadr;
	int n, newline = 1;
	int StartNewMsg = TRUE;
	int ToldUser = FALSE;
	int mesg = 0;
	int ctf = FALSE; 		/* header continuation flag */
	long clen = (long)0;
	int hdr = 0;
	int cflg = 0;			/* found Content-length in header */
	int fnuhdrtype = 0;		/* set when non-UNIX From headers are found */

	Dout(pn, 0,"entered\n");
	if (!pletinfo->let[1].adr) {
		pletinfo->nlet = nextadr = 0;
		pletinfo->let[0].adr = 0;
		pletinfo->let[0].binflag = C_Text;	/* until proven otherwise.... */
		pletinfo->let[0].change = ' ';
		pletinfo->let[0].encoding_type = Unknown;
	} else {
		nextadr = pletinfo->let[pletinfo->nlet].adr;
	}

	while ((n = getline(line, sizeof line, f1)) > 0) {
		if (!newline) {
			goto putout;
		} else if ((hdr = isheader (line, &ctf, 0, fnuhdrtype)) == FALSE) {
			ctf = FALSE;	/* next line can't be cont. */
		}
		if (!hdr && cflg) {	/* nonheader, Content-length seen */
			if (clen < n) {	/* read too much */
				/* NB: this only can happen if the content-length
				 * says a smaller number than what's seen on the
				 * first non-header line.
				 */
				if (pletinfo->let[pletinfo->nlet-1].binflag != C_Binary) {
					pletinfo->let[pletinfo->nlet-1].binflag = istext((unsigned char*)line,(int)clen, pletinfo->let[pletinfo->nlet-1].binflag);
					Dout(pn, 0, "1, let[%d].binflag = %s\n",
						pletinfo->nlet-1,
						(pletinfo->let[pletinfo->nlet-1].binflag == C_Text) ? Text:
						(pletinfo->let[pletinfo->nlet-1].binflag == C_GText) ? GenericText :
												      Binary);
				}
				if (fwrite(line,1,(int)clen,f2) != clen) {
					fclose(f1); fclose(f2);
					errmsg(E_FILE, MAmwrerr, strerror(errno));
					done(0);
				}
				nextadr += clen;
				n -= clen;
				strmove (line, line+clen);
				cflg = 0;
				ctf = FALSE;
				fnuhdrtype = 0;
				hdr = isheader(line, &ctf, 0, fnuhdrtype);
				goto dohdr;
			}
			/* here, clen >= n */
			if (n == 1 && line[0] == '\n'){	/* leading empty line */
				clen++;		/* cheat */
			}
			nextadr += clen;
			for (;;) {
				if (pletinfo->let[pletinfo->nlet-1].binflag != C_Binary) {
					pletinfo->let[pletinfo->nlet-1].binflag =
						istext((unsigned char*)line, n,
						       pletinfo->let[pletinfo->nlet-1].binflag);
					Dout(pn, 0, "2, let[%d].binflag = %s\n",
						pletinfo->nlet-1,
						(pletinfo->let[pletinfo->nlet-1].binflag == C_Text) ? Text:
						(pletinfo->let[pletinfo->nlet-1].binflag == C_GText) ? GenericText :
												      Binary);
				}
				if (fwrite(line,1,n,f2) != n) {
					fclose(f1); fclose(f2);
					errmsg(E_FILE, MAmwrerr, strerror(errno));
					done(0);
				}
				clen -= n;
				if (clen <= 0) {
					break;
				}
				n = clen < sizeof line ? clen : sizeof line;
				if ((n = fread(line, 1, n, f1)) <= 0) {
				    pfmt(stderr, MM_ERROR,
					":32:%c\tYour mailfile was found to be corrupted.\n",
					BELL);
				    pfmt(stderr, MM_NOSTD,
					":33:\t(Unexpected end-of-file).\n");
				    pfmt(stderr, MM_NOSTD,
					":34:\tMessage #%d may be truncated.%c\n\n",
					pletinfo->nlet, BELL);
					nextadr -= clen;
					clen = 0; /* stop the loop */
				}
			}
			/* All done, go to top for next message */
			cflg = 0;
			StartNewMsg = TRUE;
			fnuhdrtype = 0;
			continue;
		}

dohdr:
		switch (hdr) {
		case H_FROM:
			if (pletinfo->nlet >= (MAXLET-2)) {
				if (!mesg) {
					pfmt(stderr, MM_ERROR,
						":35:Too many letters, overflowing letters concatenated\n\n");
					mesg++;
				}
			} else {
				pletinfo->let[pletinfo->nlet++].adr = nextadr;
				pletinfo->let[pletinfo->nlet].binflag = C_Text;
				pletinfo->let[pletinfo->nlet].change = ' ';
				pletinfo->let[pletinfo->nlet].encoding_type = Unknown;
			}
			Dout(pn, 5, "setting StartNewMsg to FALSE\n");
			StartNewMsg = FALSE;
			ToldUser = FALSE;
			break;
		case H_FROM1:
			break;
		case H_CLEN:
			if (cflg) {
				break;
			}
			cflg = TRUE;	/* mark for clen processing */
			clen = atol(strpbrk (line, ":")+1);
			fnuhdrtype = hdr;
			break;
		case H_ENCDTYPE:
			if (pletinfo->let[pletinfo->nlet-1].encoding_type == Unknown)
				pletinfo->let[pletinfo->nlet-1].encoding_type =
					getencdlocale(strpbrk(line, ":")+1);
			fnuhdrtype = hdr;
			break;
		default:
			fnuhdrtype = hdr;
			break;
		}

putout:
		if (pletinfo->nlet == 0) {
			fclose(f1);
			fclose(f2);
			errmsg(E_FILE,":358:mailfile does not begin with a 'From' line\n");
			done(0);
		}
		nextadr += n;
		if (pletinfo->let[pletinfo->nlet-1].binflag != C_Binary) {
			pletinfo->let[pletinfo->nlet-1].binflag =
				istext((unsigned char*)line, n,
				       pletinfo->let[pletinfo->nlet-1].binflag);
				Dout(pn, 0, "3, let[%d].binflag = %s\n",
					pletinfo->nlet-1,
					(pletinfo->let[pletinfo->nlet-1].binflag == C_Text) ? Text:
					(pletinfo->let[pletinfo->nlet-1].binflag == C_GText) ? GenericText :
											      Binary);
		}
		if (fwrite(line,1,n,f2) != n) {
			fclose(f1);
			fclose(f2);
			errmsg(E_FILE, MAmwrerr, strerror(errno));
			done(0);
		}
		if (line[n-1] == '\n') {
			newline = 1;
			if (n == 1) { /* Blank line. Skip StartNewMsg */
				      /* check below                  */
				continue;
			}
		} else {
			newline = 0;
		}
		if (StartNewMsg == TRUE && ToldUser == FALSE) {
			pfmt(stderr, MM_ERROR,
				":37:%c\tYour mailfile was found to be corrupted\n",
				BELL);
			pfmt(stderr, MM_NOSTD,
				":38:\t(Content-length mismatch).\n");
			pfmt(stderr, MM_NOSTD,
				":39:\tMessage #%d may be truncated,\n", pletinfo->nlet);
			pfmt(stderr, MM_NOSTD,
				":40:\twith another message concatenated to it.%c\n\n",
				BELL);
			ToldUser = TRUE;
		}
	}

	/*
		last plus 1
	*/
	pletinfo->let[pletinfo->nlet].adr = nextadr;
	pletinfo->let[pletinfo->nlet].change = ' ';
	pletinfo->let[pletinfo->nlet].binflag = C_Text;
	pletinfo->let[pletinfo->nlet].encoding_type = Unknown;
}

/*
    NAME
	getencdlocale - get the locale information from Encoding-Type: header

    SYNOPSIS
	char *getencdlocale(const char *line)

    DESCRIPTION
	Getencdlocale looks through the header for the locale information,
	such as "/locale=french". Just the "french" part is returned. Other
	/xyz fields may also be present, all prefaced with a "/".
*/

static char *getencdlocale(line)
const char *line;
{
    /* look for strings starting with "/" */
    for (line = skipspace(line); line && *line; line = strchr(line, '/'))
	/* Did we find "/locale="? */
	if (casncmp(line, "/locale=", 8) == 0)
	    {
	    /* find the end of the locale name and copy it */
	    int localelen = strcspn(line+8, " \t\n,/");
	    char *duparea = malloc(localelen + 1);
	    if (!duparea)
		return Unknown;
	    strncpy(duparea, line+8, localelen);
	    duparea[localelen] = '\0';
	    return duparea;
	    }

    return Unknown;
}
