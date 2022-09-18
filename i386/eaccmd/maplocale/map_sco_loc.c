/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)eac:i386/eaccmd/maplocale/map_sco_loc.c	1.1"
#ident	"$Header: $"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <locale.h>
#include <nl_types.h>
#include <langinfo.h>
#include <ctype.h>
#include <unistd.h>
#include "maplocale.h"

extern int abbrev_name;

extern char locale_name[];

/**
 *  Map the character classification data
 **/
void
sco_map_ctype()
{
	register int i;		/*  Loop variable  */
	int ofd;			/*  Output file descriptor  */

	char tmp;			/*  Holds the magic number  */
	static char ctype[CTYPE_TBL_SIZE];	/*  character type table  */
	static char cuclc[CTYPE_TBL_SIZE];	/*  character conversion table  */
	char of_name[STR_SIZE];		/*  Output file name  */

	sprintf(of_name, "%s/%s", locale_name, CTYPE);

	if ((ofd = open(of_name, O_WRONLY | O_CREAT, MODES)) < 0)
		error("Unable to open ctype file");

	/*  Copy the ctype and character conversion tables straight from
	 *  sysv.  Position 0 is not used in the SCO table
	 */
	for (i = 1; i < CTYPE_TBL_SIZE; i++)
		ctype[i] = (__ctype + 1)[i - 1];

	for (i = 1; i < CTYPE_TBL_SIZE; i++)
		cuclc[i] = (__ctype + SYSV_CU_OFFST)[i - 1];

	/*  Output the magic number and the tables to the file  */
    tmp = MN_CTYPE;
    write(ofd, &tmp, 1);
	write(ofd, ctype, sizeof(ctype));
	write(ofd, cuclc, sizeof(cuclc));

	close(ofd);
}

/**
 *  Map the numeric formating information
 **/
void
sco_map_numeric()
{
	int ofd;

	char tmp;
	char *tptr;		/*  Temporary pointer  */
	char of_name[STR_SIZE];

	static struct _lct_numeric lct_numeric;	/*  Numeric struct (SCO)  */

	sprintf(of_name, "%s/%s", locale_name, NUMERIC);

	if ((ofd = open(of_name, O_WRONLY | O_CREAT, MODES)) < 0)
		error("Unable to open numeric file");

	/*  Extract the decimal point and thousands separator characters  */
	tptr = nl_langinfo(RADIXCHAR);
	lct_numeric.decimal = *tptr;
	tptr = nl_langinfo(THOUSEP);
	lct_numeric.thousands = *tptr;

    tmp = MN_NUMERIC;
    write(ofd, &tmp, 1);
    write(ofd, &lct_numeric, sizeof(lct_numeric));

	close(ofd);
}

/**
 *  Map the date and time formating information
 **/
void
sco_map_date()
{
	register int i;
	int ofd;

	short scount;	/*  Keeps track of index of strings  */

	char of_name[STR_SIZE];
	char tmp;
	char *tptr;		/*  Pointer to data returned by nl_langinfo  */
	char *sptr;		/*  Pointer to the strings section of the SCO struct  */

	static struct _lct_time lct_time;

	/*  Struct to allow us to loop through the information to be extracted
	 *  using nl_langinfo
	 */
	static struct {
		int nl_key;
		short *nl_ptr;
	} dt_tab[N_DT_FIELDS] = {
		D_FMT,		&lct_time.date_fmt,
		T_FMT,		&lct_time.time_fmt,
		AM_STR,		&lct_time.f_noon,
		PM_STR,		&lct_time.a_noon,
		D_T_FMT,	&lct_time.d_t_fmt,
		DAY_1,		&lct_time.day[0],
		DAY_2,		&lct_time.day[1],
		DAY_3,		&lct_time.day[2],
		DAY_4,		&lct_time.day[3],
		DAY_5,		&lct_time.day[4],
		DAY_6,		&lct_time.day[5],
		DAY_7,		&lct_time.day[6],
		ABDAY_1,	&lct_time.abday[0],
		ABDAY_2,	&lct_time.abday[1],
		ABDAY_3,	&lct_time.abday[2],
		ABDAY_4,	&lct_time.abday[3],
		ABDAY_5,	&lct_time.abday[4],
		ABDAY_6,	&lct_time.abday[5],
		ABDAY_7,	&lct_time.abday[6],
		MON_1,		&lct_time.mon[0],
		MON_2,		&lct_time.mon[1],
		MON_3,		&lct_time.mon[2],
		MON_4,		&lct_time.mon[3],
		MON_5,		&lct_time.mon[4],
		MON_6,		&lct_time.mon[5],
		MON_7,		&lct_time.mon[6],
		MON_8,		&lct_time.mon[7],
		MON_9,		&lct_time.mon[8],
		MON_10,		&lct_time.mon[9],
		MON_11,		&lct_time.mon[10],
		MON_12,		&lct_time.mon[11],
		ABMON_1,	&lct_time.abmon[0],
		ABMON_2,	&lct_time.abmon[1],
		ABMON_3,	&lct_time.abmon[2],
		ABMON_4,	&lct_time.abmon[3],
		ABMON_5,	&lct_time.abmon[4],
		ABMON_6,	&lct_time.abmon[5],
		ABMON_7,	&lct_time.abmon[6],
		ABMON_8,	&lct_time.abmon[7],
		ABMON_9,	&lct_time.abmon[8],
		ABMON_10,	&lct_time.abmon[9],
		ABMON_11,	&lct_time.abmon[10],
		ABMON_12,	&lct_time.abmon[11],
	};

	sprintf(of_name, "%s/%s", locale_name, TIME);

	if ((ofd = open(of_name, O_WRONLY | O_CREAT, MODES)) < 0)
		error("Unable to open time file");

	/*  Initialise strings pointer and strings index counter  */
	sptr = lct_time.strings;
	scount = 0;

	/*  Loop through filling in each field in the struct  */
	for (i = 0; i < N_DT_FIELDS; i++)
	{
		*dt_tab[i].nl_ptr = scount;	/*  Record where this string is  */

		/*  Extract the string and copy it to the SCO struct  */
		tptr = nl_langinfo(dt_tab[i].nl_key);
		strcpy(sptr, tptr);

		/*  Bump up the string pointer and index counter  */
		sptr += (strlen(tptr) + 1);
		scount += (strlen(tptr) + 1);
	}

    tmp = MN_TIME;
    write(ofd, &tmp, 1);
    write(ofd, &lct_time, sizeof(lct_time));

	close(ofd);
}

/**
 *  Map the collation information
 **/
void
sco_map_collate(sysv_loc)
char *sysv_loc;
{
	register int i;
	register int d_siz = 0;	/*  Size of double table  */
	int dbl_index;	/*  Index of next free collation struct  */
	int ifd;
	int ofd;

	char lc_coll_file[STR_SIZE];	/*  sysv locale data file  */
	char of_name[STR_SIZE];
	char tmp;
	char *db_ptr;		/*  Ptr to data read from file  */
	char *db_str;		/*  Ptr to substitution string data  */

	struct stat loc_stat;

	static struct _lct_collate lct_collate;

	hd *db_hd;		/*  Header structure for SVR4 collation data  */
	xnd *db_coll;	/*  Collation element structure  */
	xnd	*db_fchar;	/*  Pointer to second char info for 2 char sequences  */
	subtent *db_sub;	/*  Substitution string structure  */

	sprintf(of_name, "%s/%s", locale_name, COLLATE);

	if ((ofd = open(of_name, O_WRONLY | O_CREAT, MODES)) < 0)
		error("Unable to open collate file");

	sprintf(lc_coll_file, "%s/%s", sysv_loc, LC_COLL);

	/*  Stat the LC_COLLATE file, as we need to know how big it is.  */
	if (stat(lc_coll_file, &loc_stat) < 0)
		error("Failed to stat LC_COLLATE");

	/*  Allocate space to read the LC_COLLATE file into - this makes
	 *  processing easier and faster.
	 */
	if ((db_ptr = (char *)malloc((unsigned int)loc_stat.st_size)) == NULL)
		error("Out of memory");

	/*  Extract the data from the file.
	 */
	if ((ifd = open(lc_coll_file, O_RDONLY)) < 0)
		error("Unable to open LC_COLLATE file");

	if (read(ifd, db_ptr, (unsigned int)loc_stat.st_size) < loc_stat.st_size)
		error("Read of LC_COLLATE failed");

	close(ifd);

	/*  Set up the pointers to the structures contained in the file
	 */
	db_hd = (hd *)db_ptr;
	db_coll = (xnd *)(db_ptr + db_hd->coll_offst);
	db_fchar = db_coll + COLL_TBL_SIZE;
	db_sub = (subtent *)(db_ptr + db_hd->sub_offst);
	db_str = db_ptr + db_hd->str_offst;

	/*  Initialise the size of the collation table to 256.  It may be
	 *  larger than this depending on whether there are any 2-to-1
	 *  collation elements
	 */
	lct_collate.size.siz_coll = COLL_TBL_SIZE;
	dbl_index = COLL_TBL_SIZE;		/*  First free collation struct  */

	/*  Convert the substitution string pointers from an offset
	 *  to real pointers.
	 */
	for (i = 0; i < db_hd->sub_cnt; i++)
	{
		(db_sub + i)->exp = db_str + (int)(db_sub + i)->exp;
		(db_sub + i)->repl = db_str + (int)(db_sub + i)->repl;
	}

	/*  Copy the collation table, not all of the SVR4 info is required
	 *  for the SCO table.
	 */
	for (i = 0; i < COLL_TBL_SIZE; i++)
	{
		lct_collate.coll[i].prim = db_coll->pwt;
		lct_collate.coll[i].sec = db_coll->swt;

		/*  The following section of code is #ifdef'd out as there is
		 *  a bug in the SCO implementation of strxfrm.  Although
		 *  strxfrm can correctly use the 2-to-1 collation mapping
		 *  data, if the first character of one of these is the last 
		 *  char passed to strxfrm, the program will hang in an endless
		 *  loop waiting for the next char.  
		 *  I will leave the code here, so that should SCO fix this we
		 *  can implement support for this very quickly.
		 */
#if 0
		/*  We need to check here whether there is a 2-to-1 collating
		 *  sequence that starts with this character.
		 */
		/*  NOTE:  SCO have not implemented this feature in their version
		 *  of coltbl, but the strxfrm function uses the data.  There is
		 *  no guarentee that this is the correct method, but it looks
		 *  right, and it has been tested and works with the SCO strxfrm
		 *  and strcoll functions.
		 */
		if (db_coll->ch == FOLLOW_ON)
		{
			/*  The two chars are put in the dbl.chars element, then we
			 *  set the index to be the next free coll array element
			 *  above 256.  We fill in this coll struct with the primary
			 *  and secondary weights of the two character collation
			 *  sequence.  Simple, huh?
			 */
			lct_collate.dbl[d_siz].chars = ((unsigned char)i) << 8;
			lct_collate.dbl[d_siz].chars |= (db_fchar + db_coll->ns)->ch;
			lct_collate.dbl[d_siz++].index = dbl_index;
			lct_collate.coll[dbl_index].prim = (db_fchar + db_coll->ns)->pwt;
			lct_collate.coll[dbl_index++].sec = (db_fchar + db_coll->ns)->swt;
			++lct_collate.size.siz_coll;
			SET_FIRST((unsigned char)i);	/*  Don't forget the bit map  */

			/*  SCO has a maximum of MAXDBL (64) string substitutions.  We need
			 *  to warn the user if the number in the SVR4 locale exceeds this
			 */
			if (d_siz == MAXDBL)
			{
				printf("Warning: SVR4 locale contains too many replacement strings,\n");
				printf("only %d will be converted in the XENIX locale\n", MAXDBL);
				break;
			}
		}
#endif		/*  End of unimplemented code  */

		++db_coll;
	}

	/*  Set the size of the string replacement table, now that we know
	 *  what it is
	 */
	lct_collate.size.siz_dbl = d_siz;

	/*  Now deal with one-to-two char mappings  */
	for (i = 0; i < db_hd->sub_cnt; i++, db_sub++)
	{
		/*  SCO only allows one-to-two character substitution, whereas SVR4
		 *  allows many-to-many.  We print a warning and ignore mappings
		 *  that are incompatible.
		 */
		if (strlen(db_sub->exp) > 1 || strlen(db_sub->repl) > 2)
		{
			printf("Warning: the SVR4 locale contains a many-to-many collation mapping\n");
			printf("\n    %s -> %s\n\n", db_sub->exp, db_sub->repl);
			printf("This will not be converted for use in the XENIX locale.\n\n");
			continue;
		}

		/*  Record the two characters in the primary and secondary weights
		 *  field of the coll struct
		 */
		lct_collate.coll[(unsigned char)*db_sub->exp].prim = *db_sub->repl;
		lct_collate.coll[(unsigned char)*db_sub->exp].sec = *(db_sub->repl + 1);

		/*  The SCO struct has a bit map to identify one-to-two mappings,
		 *  so we need to set this up
		 */
		SET_DIPH((unsigned char)*db_sub->exp);
	}

    tmp = MN_COLLATE;
    write(ofd, &tmp, 1);
	write(ofd, &lct_collate, sizeof(lct_collate));

	close(ofd);
}

/**
 *  Map the yes/no messages - this a file which does not exist as
 *  a separate entity on SVR4
 **/
void
sco_map_messages()
{
	int ofd;

	char of_name[STR_SIZE];
	char tmp;

	static struct _lct_messages lct_messages;

	/*  Extract the yes/no messages into the SCO struct  */
	strcpy(lct_messages.yesstr, nl_langinfo(YESSTR));
	strcpy(lct_messages.nostr, nl_langinfo(NOSTR));

	sprintf(of_name, "%s/%s", locale_name, MESSAGES);

	if ((ofd = open(of_name, O_WRONLY | O_CREAT, MODES)) < 0)
		error("Unable to open message file");

    tmp = MN_MESSAGES;
    write(ofd, &tmp, 1);
    write(ofd, &lct_messages, sizeof(lct_messages));

	close(ofd);
}

/**
 *  Map the currency formatting information
 **/
void
sco_map_currency()
{
	int ofd;

	char of_name[STR_SIZE];
	char tmp;
	static char lct_currency[CRNCYMAX];

	sprintf(of_name, "%s/%s", locale_name, CURRENCY);

	if ((ofd = open(of_name, O_WRONLY | O_CREAT, MODES)) < 0)
		error("Unable to open currency file");

	strcpy(lct_currency, nl_langinfo(CRNCYSTR));
    tmp = MN_MONETARY;
    write(ofd, &tmp, 1);
    write(ofd, lct_currency, sizeof(lct_currency));

	close(ofd);
}

/**
 *  Convert the language_country.codeset format to the locale directory
 *  for SCO i.e. /usr/lib/lang/language/country/codeset and generate
 *  directories as required.
 **/
void
conv_sco_locale(locale_str, territory, codeset)
char *locale_str;	/*  String for directory name  */
char *territory;
char *codeset;
{
	register int i = 0;

	char lang[SHRT_STR];
	char country[SHRT_STR];
	char encoding[SHRT_STR];

	abbrev_name = 0;
	strcpy(locale_name, SCO_LOC_DIR);
	path_create(locale_name);
	strcat(locale_name, "/");

	/*  Grab the language part  */
	while(*locale_str != NULL && *locale_str != LANG_SEP)
		lang[i++] = *locale_str++;

	lang[i] = NULL;
	strcat(locale_name, lang);
	path_create(locale_name);
	strcat(locale_name, "/");

	/*  Check for EOS  */
	if (*locale_str == NULL)
	{
		/*  Check we have a territory and code set  */
		if (strlen(territory) == 0)
			error("Territory not specified");

		strcat(locale_name, territory);
		abbrev_name = 1;
	}
	else
	{
		if (strlen(territory) != 0)
			printf("Territory not required, -t ignored\n");

		++locale_str;
	}

	i = 0;

	while(*locale_str != NULL && *locale_str != CS_SEP)
		country[i++] = *locale_str++;

	country[i] = NULL;
	strcat(locale_name, country);
	path_create(locale_name);
	strcat(locale_name, "/");

	if (*locale_str == NULL || *(locale_str + 1) == NULL)
	{
		/*  Check we have a code set  */
		if (strlen(codeset) == 0)
			error("Code set not specified");

		strcat(locale_name, codeset);
		abbrev_name = 1;
	}
	else
	{
		if (strlen(codeset) != 0)
			printf("Code set not required, -c ignored\n");

		++locale_str;
	}

	i = 0;

	while(*locale_str != NULL)
		encoding[i++] = *locale_str++;

	encoding[i] = NULL;
	strcat(locale_name, encoding);
	path_create(locale_name);
}

/**
 *  Function to update the /etc/default/lang file, when we convert a 
 *  locale that uses an abbreviated name in SVR4.
 **/
void
sco_update_default()
{
	register int i = 0;

	char inp[STR_SIZE];
	char lang[STR_SIZE];
	char *mode;
	char *ptr;

	FILE *fp;

	ptr = locale_name + strlen(SCO_LOC_DIR) + 1;

	while (*ptr != '/')
		lang[i++] = *ptr++;

	lang[i++] = LANG_SEP;
	++ptr;

	while (*ptr != '/')
		lang[i++] = *ptr++;

	lang[i++] = CS_SEP;
	++ptr;

	while (*ptr != NULL)
		lang[i++] = *ptr++;

	lang[i] = NULL;
	
	if (access(SCO_DEF_FILE, F_OK) != 0)
		mode = "w+";
	else
		mode = "r+";

	if ((fp = fopen(SCO_DEF_FILE, mode)) == NULL)
		error("Could not create default file");

	rewind(fp);

	/*  Check that the locale is not already in the default file  */
	while (fgets(inp, STR_SIZE - 1, fp) != NULL)
	{
		if (inp[0] == NULL || inp[0] == HASH || isspace(inp[0]))
			continue;

		inp[strlen(inp) - 1] = NULL;

		if (strcmp(&inp[SCO_LOC_POS], lang) == 0)
		{
			fclose(fp);
			return;
		}
	}

	/*  Now add the line to the file  */
	fprintf(fp, "LANG=%s\n", lang);
	fclose(fp);
}
