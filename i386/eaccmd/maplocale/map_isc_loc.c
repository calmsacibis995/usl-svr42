/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)eac:i386/eaccmd/maplocale/map_isc_loc.c	1.1"
#ident	"$Header: $"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <locale.h>
#include <nl_types.h>
#include <langinfo.h>
#include <ctype.h>
#include <unistd.h>
#include "maplocale.h"

#include <errno.h>
extern int errno;

/*  Static global variables for isc_adjust_rpl  */
static colhdr_t hdr;		/*  Header for ISC file  */
static caddr_t lc_mm_addr;	/*  Pointer for memeory mapped file  */

extern char locale_name[];

void
isc_map_collate(sysv_loc)
char *sysv_loc;
{
	register int i;		/*  Loop variables  */
	register int j;
	int coll_base;		/*  Value at which to start adjusting  */
	int coll_diff;		/*  Adjustment value for collation table  */
	int coll_offset;	/*  Offset for collation value  */
	int coll_max = 0;	/*  Largest collation weight in table  */
	int ifd;			/*  Input file descriptor  */
	int ofd;			/*  Output file descriptor  */
	int table_size;		/*  Size of collation table  */
	int repl_index = 0;	/*  Replacement table index  */
	int str_offset;		/*  Offset for strings table  */
	int write_len;		/*  Length to write to file  */

	short coll_wt[COLL_TBL];	/*  Collation values  */
	short weights[2][COLL_TBL];	/*  Weights tables  */

	char buf[BUF_SIZE];	/*  Buffer for transferring strings table  */
	char lc_coll_file[STR_SIZE];    /*  sysv locale data file  */
	char of_name[STR_SIZE];	/*  Output file name  */
	char mcce[3];		/*  MCCE string for strings table  */
	char *db_ptr;		/*  Ptr to data read from file  */ 
	char *db_str;		/*  Ptr to substitution string data  */
	char *str_ptr;		/*  Pointer for SVR4 replacement table  */

	hd *db_hd;			/*  Header structure for SVR4 collation data  */
	xnd *db_coll;		/*  Collation element structure  */
	xnd *db_fchar;		/*  Pointer to second char info for 2 char sequences  */
	subtent *db_sub;    /*  Substitution string structure  */
	rpl_t rpl;			/*  Replacement table structure  */

	FILE *tmp_fp;		/*  File pointer for strings table temp file  */

	struct stat loc_stat;	/*  For size of file  */

	sprintf(of_name, "%s/LC_COLLATE", locale_name);

	/*  Need to unlink the file in case it's been created before  */
	unlink(of_name);

	if ((ofd = open(of_name, O_RDWR | O_CREAT, MODES)) < 0)
		error("Unable to open LC_COLLATE file");

	/*  Create the full pathname for the SVR4 LC_COLLATE file  */
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

	/*  Fill in the bits of the header structure that we know  */
	hdr.mag0 = ISC_MAGIC_1;
	hdr.mag1 = ISC_MAGIC_2;
	hdr.version = ISC_VERSION;
	hdr.length = sizeof(colhdr_t);
	hdr.len_rpl_tbl = 1;	/*  Must have at least NULL entry  */
	hdr.rpl_tbl = (rpl_t *)hdr.length;
	hdr.nbr_weights = 2;
	hdr.weights[2] = (short *)0;
	hdr.weights[3] = (short *)0;

	/*  We don't actually write out the header yet, so we need to seek to
	 *  the correct place in the file to start writing the replacement table
	 *  entries.
	 */
	if (lseek(ofd, hdr.length, SEEK_SET) < 0)
		error("Bad seek in LC_COLLATE file");

	/*  We use a temporary file for the strings table, as we don't know 
	 *  where it will go in the final output.  This could be done as a
	 *  malloc, but this seems a lot simpler, since we don't know how big
	 *  it'll be either.  Also, I don't think the extra processing
	 *  overhead will degrade performance.
	 */
	tmp_fp = tmpfile();
	str_offset = 1;		/*  Initialise string offset  */
	fputc('\0', tmp_fp);	/*  Put first null in string table file  */

	/*  Using the SVR4 information we fill in the strings table and create
	 *  the necessary entries for the replacement table.
	 *
	 *  Strictly speaking things like the German sharp-s which is treated 
	 *  as ss for collation purposes should go in the second part of this
	 *  table.  However, since SVR4 allows many-to-many substitutions we
	 *  put them in the first half (the effect will be the same anyway).
	 */
	for (i = 0; i < db_hd->sub_cnt; i++)
	{
		/*  First the string to be replaced -
		 *  Record the strings table offset for this string, set up the
		 *  pointer to the string in the SVR4 information, and the length
		 *  of the string.
		 */
		rpl.stroff = (char *)str_offset;
		str_ptr = db_str + (int)db_sub->exp;
		write_len = db_sub->explen + 1;
		str_offset += write_len;	/*  Update the offset value  */

		/*  Write the string to the file  */
		if ((int)fwrite(str_ptr, sizeof(char), write_len, tmp_fp) < write_len)
			error("Write to strings table failed");

		/*  Now the replacement string  */
		rpl.t.repoff = (char *)str_offset;
		str_ptr = db_str + (int)db_sub->repl;
		write_len = strlen(str_ptr) + 1;
		str_offset += write_len;

		if ((int)fwrite(str_ptr, sizeof(char), write_len, tmp_fp) < write_len)
			error("Write to strings table failed");

		/*  Having sorted out the strings table, write the rpl_tbl
		 *  structure to the ISC LC_COLLATE file.
		 */
		if (write(ofd, &rpl, sizeof(rpl_t)) < sizeof(rpl_t))
			error("Write to LC_COLLATE file failed");

		--repl_index;	/*  Decrement replacement table index value  */
		++hdr.len_rpl_tbl;	/*  Bump up number of entries in header struct  */
		++db_sub;	/*  Move db_sub to next structure  */
	}

	/*  The replacement table must be separated from the MCCE part with
	 *  an empty rpl_t structure
	 */
	rpl.stroff = 0;
	rpl.t.repoff = 0;
	--repl_index;

	if (write(ofd, &rpl, sizeof(rpl_t)) < sizeof(rpl_t))
		error("Write to LC_COLLATE file failed");
	
	/*  The structure of the ISC file is rather complicated, so we need to 
	 *  do some fairly involved massaging of the information read from the 
	 *  SVR4 file to produce the ISC format.
	 */
	mcce[2] = NULL;
	table_size = COLL_BASE + 1;

	/*  The 0'th element in the weights arrays are used to indicate the
	 *  direction of the collation.  For SVR4 conversion we set both of
	 *  these to zero for left-to-right.
	 */
	coll_wt[0] = 0;
	weights[0][0] = 0;
	weights[1][0] = 0;

	/*  This loop runs through the SVR4 collation table, attempting to 
	 *  build up as much of the information as possible
	 */
	for (i = 1; i <= COLL_TBL_SIZE; i++)
	{
		/*  If this is the first character of an MCCE we need to add it to
		 *  the strings table, and create a rpl_t structure
		 */
		if (db_coll->ch == FOLLOW_ON)
		{
			/*  Set the coll_wt to the negative index into the repl table  */
			coll_wt[i] = repl_index--;
			weights[0][i] = COLL_BASE + db_coll->pwt;
			weights[1][i] = weights[0][i] + db_coll->swt;

			/*  Add the MCCE string to the strings table  */
			mcce[0] = i - 1;
			mcce[1] = (db_fchar + db_coll->ns)->ch;

			if ((int)fwrite(mcce, sizeof(char), 3, tmp_fp) < 3)
				error("Bad write to LC_COLLATE file");

			/*  Create a rpl structure for this MCCE  */
			rpl.stroff = (char *)str_offset;
			str_offset += 3;
			rpl.t.wts_index = ++table_size;
			weights[0][table_size] = COLL_BASE + db_coll->pwt;
			weights[1][table_size] = weights[0][table_size] + db_coll->swt;
			coll_wt[table_size] = weights[1][table_size];

			/*  Write it to the file  */
			if (write(ofd, &rpl, sizeof(rpl_t)) < 0)
				error("Bad write to LC_COLLATE FILE");

			/*  We also need another entry for the collation value of
			 *  this combination
			 */
			rpl.stroff = 0;
			rpl.t.wts_index = COLL_BASE + db_coll->pwt + db_coll->swt;

			if (write(ofd, &rpl, sizeof(rpl_t)) < 0)
				error("Bad write to LC_COLLATE FILE");

			--repl_index;
			hdr.len_rpl_tbl += 2;	/*  Bump up rpl count in header  */
		}
		else
		{
			/*  Characters that are ignored have weights of zero.
			 *  Although coll_wt still needs to be set we leave it as
			 *  zero for now.
			 */
			if (db_coll->pwt == 0)
			{
				coll_wt[i] = 0;
				weights[0][i] = 0;
				weights[1][i] = 0;
			}
			/*  Normal characters.  We make the coll_wt the same as the
			 *  secondary collation weight, as this seems right.
			 */
			else
			{
				weights[0][i] = COLL_BASE + db_coll->pwt;
				weights[1][i] = weights[0][i] + db_coll->swt;
				coll_wt[i] = weights[1][i];
			}

			/*  We have to remember what the largest value is for the
			 *  next part.
			 */
			if (coll_wt[i] > coll_max)
				coll_max = coll_wt[i];
		}

		++db_coll;	/*  Move db_coll to next element  */
	}

	/*  Having made all the entries in the strings table we can set the
	 *  length in the header structure and the location that it will have
	 *  in the file
	 */
	if ((hdr.strings = (char *)lseek(ofd, 0, SEEK_CUR)) < (char *)0)
		error("Bad lseek in LC_COLLATE");

	hdr.len_strings = ftell(tmp_fp);
	rewind(tmp_fp);

	/*  We now write out the strings file to the LC_COLLATE file, after
	 *  recording the length.
	 */
	while((i = fread(buf, sizeof(char), BUF_SIZE, tmp_fp)) > 0)
		if (write(ofd, buf, i) < i)
			error("Bad write to LC_COLLATE file");

	/*  Now we record the position of the coll_wt table  */
	if ((hdr.coll_wt = (short *)lseek(ofd, 0, SEEK_CUR)) < (short *)0)
		error("Bad lseek in LC_COLLATE");

	/*  Due to the layout of the ISC file we will later need to modify 
	 *  the collation values that have been stored in the rpl_t structures.
	 *  The easiest way to do this is to memeory map the file that has
	 *  been written out so far.  This avoids nasty complications with 
	 *  repositioning file pointers and reading and writing to the file
	 *
	 *  First we need to know how big the file currently is
	 */
	if (fstat(ofd, &loc_stat) < 0)
		error("Bad stat of LC_COLLATE file");

	if ((lc_mm_addr = mmap((caddr_t)0, loc_stat.st_size,
		PROT_WRITE, MAP_SHARED, ofd, 0)) < (caddr_t)0)
		error("Unable to memeory map LC_COLLATE file");

	coll_base = COLL_BASE;
	coll_diff = 2;

	/*  We now have to loop though the tables to adjust the collation
	 *  values so that none of them clash.
	 *
	 *  This is a two phase process due the sheer complexity and differences
	 *  of the two files.  The first phase goes through and changes all the
	 *  zero value coll_wt entries to valid values, and shuffles all the 
	 *  existing coll_wt and weights values up to make room for this value.
	 */
	for (i = 1; i <= table_size; i++)
	{
		if (coll_wt[i] == 0)
		{
			coll_wt[i] = COLL_BASE + coll_diff;

			/*  Now we have to shuffle everythng along.  This involves 
			 *  making the assumption that SVR4 will never have a character
			 *  with a primary collation value of 2 or less.  Given the
			 *  format of the collation information I think this is fairly
			 *  safe.
			 */
			for (j = 1; j <= table_size; j++)
			{	
				if (weights[0][j] > coll_wt[i])
				{
					if (coll_wt[j] > 0)
						++coll_wt[j];
					++weights[0][j];
					++weights[1][j];
				}

				/*  Because ISC store a collation value in the rpl_t struct
				 *  for MCCE's we need to make sure that these get updated
				 *  as well.  
				 */
				if (coll_wt[j] < 0)
					isc_adjust_rpl(coll_wt[j], coll_wt[i], 1);
			}

			++coll_diff;
		}
	}

	/*  Need to change coll_max to be the correct value  */
	coll_max += coll_diff;

	/*  This while statement will look a little odd when you go through
	 *  the code, as we're going to change the value of both coll_base
	 *  and coll_max.  This seems to be about the most efficient way of
	 *  doing it though.
	 */
	while (++coll_base < coll_max)
	{
		coll_diff = 0;

		/*  Scan through the table and work out which is the largest 
		 *  difference between the primary and secondary weights for
		 *  all entries with a primary weight of coll_base
		 */
		for (i = 1; i <= table_size; i++)
			if (weights[0][i] == coll_base)
				if ((weights[1][i] - weights[0][i]) > coll_diff)
					coll_diff = weights[1][i] - weights[0][i];

		if (coll_diff == 0)
			continue;

		/*  We then have to run though the table again to add this 
		 *  number to all entries which have a primary weight greater
		 *  than coll_base.
		 */
		for (i = 1; i <= table_size; i++)
		{
			if (weights[0][i] > coll_base)
			{
				if (coll_wt[i] > 0)
					coll_wt[i] += coll_diff;

				weights[0][i] += coll_diff;
				weights[1][i] += coll_diff;
			}

			/*  Again we have to do the same thing with the rpl_t stuff  */
			if (coll_wt[i] < 0)
				isc_adjust_rpl(coll_wt[i], coll_base, coll_diff);
		}

		coll_base += coll_diff;
		coll_max += coll_diff;
	}

	/*  unmap the file to avoid any possible complications when we arite out
	 *  the reast of the data.
	 */
	munmap(lc_mm_addr, loc_stat.st_size);

	/*  Having re-arranged all the data, we can now write the tables to the
	 *  file
	 */

	/*  First the coll_wt's  */
	write_len = table_size * sizeof(short);
	hdr.len_wts = table_size;

	if (write(ofd, coll_wt, write_len) < write_len)
		error("Bad write to LC_COLLATE file");

	hdr.weights[0] = (short *)((int)hdr.coll_wt + write_len);
	hdr.weights[1] = (short *)((int)hdr.weights[0] + write_len);

	/*  Now the weights tables  */
	for (i = 0; i < 2; i++)
		if (write(ofd, weights[i], write_len) < write_len)
			error("Bad write to LC_COLLATE file");

	/*  Seek back to the start of the file to write out the header  */
	if (lseek(ofd, 0, SEEK_SET) < 0)
		error("Bad lseek in LC_COLLATE file");

	if (write(ofd, &hdr, sizeof(colhdr_t)) < sizeof(colhdr_t))
		error("Unable to write header to LC_COLLATE file");

	close(ofd);
}

/**
 *  Map the yes/no messages - this a file which does not exist as
 *  a separate entity on SVR4
 **/
void
isc_map_messages()
{
	char of_name[STR_SIZE];	/*  Output file name  */
	char *nl_ptr;	/*  Pointer to string returned from nl_langinfo  */

	FILE *ofp;	/*  Output file pointer  */

	sprintf(of_name, "%s/LC_MESSAGES", locale_name);

	if ((ofp = fopen(of_name, "w+")) == NULL)
		error("Unable to open LC_MESSAGES file");

	fprintf(ofp, "LC_MESSAGES\n#\n");

	nl_ptr = nl_langinfo(YESSTR);

	/*  ISC use a regular expression for there response strings.  We can 
	 *  make this up quite easily using toupper and tolower.
	 */
	fprintf(ofp, "yesexpr \"[%c%c][[:alpha:]]*\"\n",
		tolower(*nl_ptr), toupper(*nl_ptr));
	nl_ptr = nl_langinfo(NOSTR);
	fprintf(ofp, "noexpr  \"[%c%c][[:alpha:]]*\"\n",
		tolower(*nl_ptr), toupper(*nl_ptr));
	fprintf(ofp, "#\nEND LC_MESSAGES\n");
	fclose(ofp);
}

/**
 *  Map the currency formatting information
 **/
void
isc_map_currency()
{
	char of_name[STR_SIZE];	/*  Output file name  */

	struct lconv *lc_ptr;	/*  Monetary formatting information structure  */

	FILE *ofp;	/*  Output file pointer  */

	sprintf(of_name, "%s/LC_MONETARY", locale_name);

	if ((ofp = fopen(of_name, "w+")) == NULL)
		error("Unable to open LC_MONETARY file");

	/*  Pretty simple really - extract the SVR4 information using localeconv
	 *  and then output the information with the appropriate ISC keywords
	 *  around it.
	 */
	lc_ptr = localeconv();
	fprintf(ofp, "LC_MONETARY\n#\n");
	fprintf(ofp, "int_curr_symbol         \"%s\"\n", lc_ptr->int_curr_symbol);
	fprintf(ofp, "currency_symbol         \"%s\"\n", lc_ptr->currency_symbol);
	fprintf(ofp, "mon_decimal_point       \"%s\"\n", lc_ptr->mon_decimal_point);
	fprintf(ofp, "mon_thousands_sep       \"%s\"\n", lc_ptr->mon_thousands_sep);

	/*  Need to be a little careful with this, as SVR4 doesn't always have
	 *  a second value for the grouping.  In this case we substitute 0.
	 */
	fprintf(ofp, "mon_grouping            %c;%c\n",
		*lc_ptr->mon_grouping == NULL ? '0' : *lc_ptr->mon_grouping, 
		*(lc_ptr->mon_grouping+1) == NULL ? '0' : *(lc_ptr->mon_grouping+1));

	fprintf(ofp, "positive_sign           \"%s\"\n", lc_ptr->positive_sign);
	fprintf(ofp, "negative_sign           \"%s\"\n", lc_ptr->negative_sign);
	fprintf(ofp, "int_frac_digits         %d\n", lc_ptr->int_frac_digits);
	fprintf(ofp, "frac_digits             %d\n", lc_ptr->frac_digits);
	fprintf(ofp, "p_cs_precedes           %d\n", lc_ptr->p_cs_precedes);
	fprintf(ofp, "p_sep_by_space          %d\n", lc_ptr->p_sep_by_space);
	fprintf(ofp, "n_cs_precedes           %d\n", lc_ptr->n_cs_precedes);
	fprintf(ofp, "n_sep_by_space          %d\n", lc_ptr->n_sep_by_space);
	fprintf(ofp, "p_sign_posn             %d\n", lc_ptr->p_sign_posn);
	fprintf(ofp, "n_sign_posn             %d\n", lc_ptr->n_sign_posn);
	fprintf(ofp, "#\nEND LC_MONETARY\n");
}

/**
 *  Map the character classification data
 **/
void
isc_map_ctype(sysv_name)
char *sysv_name;	/*  SVR4 locale directory name  */
{
	int ifd;	/*  Input file descriptor  */
	int ofd;	/*  Output file descriptor  */
	int nbr;	/*  Number of bytes read from file  */
	int nbw;	/*  Number of bytes written to file  */

	char of_name[STR_SIZE];	/*  Output file pathname  */
	char if_name[STR_SIZE];	/*  Input file pathname  */
	char buf[1024];	/*  Buffer read from file  */

	/*  Create full pathnames for input and output files  */
	sprintf(of_name, "%s/LC_CTYPE", locale_name);
	sprintf(if_name, "%s/LC_CTYPE", sysv_name);

	/*  The ISC binary format of the LC_CTYPE file is the same as used for
	 *  SVR4, except that it is a little shorter (they don't use six bytes at 
	 *  the end of the SVR4 file).  Consequently, all we need to do is copy
	 *  the file from SVR4 to the ISC directory.
	 */
	if ((ifd = open(if_name, O_RDONLY)) < 0)
		error("Unable to open SVR4 LC_CTYPE file for reading");

	if ((ofd = open(of_name, O_WRONLY | O_CREAT, MODES)) < 0)
		error("Unable to create LC_CTYPE file");

	/*  Read in file in 1k chunks - currently the file is smaller than this
	 *  so we only need one read.
	 */
	while ((nbr = read(ifd, buf, 1024)) > 0)
	{
		/*  Check that the number of bytes written is the same as the
		 *  number read.
		 */
		if ((nbw = write(ofd, buf, nbr)) < nbr)
			error("Bad write to LC_CTYPE file (%d != %d)\n", nbr, nbw);
	}

	close(ifd);		/*  Tidy up  */
	close(ofd);
}

/**
 *  Map the numeric formating information
 **/
void
isc_map_numeric()
{
	char of_name[STR_SIZE];	/*  Output file name  */

	struct lconv *lc_ptr;	/*  Struct pointer returned by localeconv  */

	FILE *ofp;	/*  Output file pointer  */

	sprintf(of_name, "%s/LC_NUMERIC", locale_name);

	if ((ofp = fopen(of_name, "w+")) == NULL)
		error("Unable to open LC_NUMERIC file");

	/*  Again a simple case of extracting the SVR4 data and then outputting
	 *  it with the appropriate ISC keywords around it
	 */
	fprintf(ofp, "LC_NUMERIC\n#\n");
	lc_ptr = localeconv();
	fprintf(ofp, "decimal_point             \"%s\"\n", lc_ptr->decimal_point);
	fprintf(ofp, "thousands_sep             \"%s\"\n", lc_ptr->thousands_sep);
	fprintf(ofp, "grouping                  %s\n", lc_ptr->grouping);
	fprintf(ofp, "#\nEND LC_NUMERIC\n");
	fclose(ofp);
}

/**
 *  Map the date and time formating information
 **/
void
isc_map_date()
{
	register char *p;	/*  Working pointer  */
	char of_name[STR_SIZE];		/*  Output file name  */
	char t_fmt[STR_SIZE];	/*  Odd ISC date/time/am/pm format string  */

	FILE *ofp;	/*  Output file pointer  */

	sprintf(of_name, "%s/LC_TIME", locale_name);

	if ((ofp = fopen(of_name, "w+")) == NULL)
		error("Unable to open LC_TIME file");

	/*  isc_date_loop extract the date info and outputs it correctly
	 *  for ISC.
	 */
	fprintf(ofp, "LC_TIME\n#\n");
	fprintf(ofp, "abday	");
	isc_date_loop(ofp, ABDAY_1, 7);
	fprintf(ofp, "day	");
	isc_date_loop(ofp, DAY_1, 7);
	fprintf(ofp, "abmon	");
	isc_date_loop(ofp, ABMON_1, 12);
	fprintf(ofp, "mon	");
	isc_date_loop(ofp, MON_1, 12);
	fprintf(ofp, "d_t_fmt	\"%s\"\n", nl_langinfo(D_T_FMT));
	fprintf(ofp, "d_fmt	\"%s\"\n", nl_langinfo(D_FMT));
	fprintf(ofp, "t_fmt	\"%s\"\n", nl_langinfo(T_FMT));
	fprintf(ofp, "am_pm	\"%s\";\"%s\"\n",
		nl_langinfo(AM_STR), nl_langinfo(PM_STR));

	/*  This bit is a littly yucky.  ISC have a part of the date/time file
	 *  which is the time format with the AM/PM string attached.  This 
	 *  does not exist in the SVR4 locale, so we have to create it.  This 
	 *  is fairly simple, as we add a %p to the T_FMT entry.  However, 
	 *  most T_FMT entries use %H (hours 0-23), which doesn't make sense
	 *  when using AM/PM.  To get round this we scan down the T_FMT string
	 *  and change a %H to %I (hours 1-12).
	 */
	strcpy(t_fmt, nl_langinfo(T_FMT));
	p = t_fmt;

	while (*p != NULL)
	{
		if (*p == 'H' && p != t_fmt && *(p-1) == '%')
			*p = 'I';
		++p;
	}

	fprintf(ofp, "t_fmt_ampm	\"%s %%p\"\n", t_fmt);
	fprintf(ofp, "#\nEND LC_TIME\n");
	fclose(ofp);
}

/**
 *  Convert the language_country.codeset format to the locale directory
 *  for ISC i.e. /lib/locale/ISC/language[[_country[.code_set]] and generate
 *  directories as required.
 **/
void
conv_isc_locale(locale_str)
char *locale_str;
{
	strcpy(locale_name, ISC_LOC_DIR);
	path_create(locale_name);
	strcat(locale_name, "/");
	strcat(locale_name, locale_str);
	path_create(locale_name);
}

/**
 *  Function to output the necessary date information to the output file.
 *  This has been made a separate function to avoid duplicating a tedious
 *  loop.
 **/
void
isc_date_loop(ofp, base, loop)
FILE *ofp;	/*  Output file pointer  */
int base;	/*  Base value for nl_langinfo parameter  */
int loop;	/*  Number of values to output  */
{
	register int i;

	for (i = 0; i < loop; i++)
	{
		fprintf(ofp, "\"%s\"", nl_langinfo(base + i));

		/*  Entries need to be separated with a ; and we need a newline
		 *  at the end
		 */
		if (i != (loop - 1))
			fprintf(ofp, ";");
		else
			fprintf(ofp, "\n");

		/*  To avoid untidy lines we put a newline after every forth 
		 *  entry
		 */
		if (i != 0 && i != (loop - 1) && (i % 3) == 0)
			fprintf(ofp, " \\\n	");
	}
}

/**
 *  Function to adjust the collation values stored in the rpl_t structures
 **/
void
isc_adjust_rpl(tbl_off, comp_val, adj)
short tbl_off;	/*  rpl_t table offset  */
int comp_val;	/*  Value to test against  */
int adj;		/*  Adjustment value  */
{
	caddr_t addr;
	rpl_t *rpl;	/*  Pointer to the structure to check/change  */

	/*  The table offset is a negative value, so we need to change it 
	 *  back to a positive value
	 */
	tbl_off *= -1;

	/*  Calculate where the rpl structure is  */
	addr = lc_mm_addr + (int)hdr.rpl_tbl + ((tbl_off + 1) * sizeof(rpl_t));

	rpl = (rpl_t *)addr;

	/*  Check whether this value needs changing, and if so change it  */
	if (rpl->t.wts_index > comp_val)
		rpl->t.wts_index += adj;
}
