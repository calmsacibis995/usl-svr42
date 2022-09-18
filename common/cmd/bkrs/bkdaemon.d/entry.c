/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/bkdaemon.d/entry.c	1.5.7.2"
#ident  "$Header: entry.c 1.3 91/06/21 $"

#include	<sys/types.h>
#include	<backup.h>
#include	<bkmsgs.h>
#include	<bkdaemon.h>
#include	<bkreg.h>
#include	<table.h>

extern void free();
extern char *strdup();
extern int atoi();
extern void bkr_init();
extern unsigned char *p_weekday1();

/* Free all memory associated with an entry structure */
void
en_free( entry )
brentry_t	*entry;
{
	if( entry->tag ) {
		free( entry->tag );
		entry->tag = NULL;
	}

	if( entry->oname ) {
		free( entry->oname );
		entry->oname = NULL;
	}

	if( entry->odevice ) {
		free( entry->odevice );
		entry->odevice = NULL;
	}

	if( entry->olabel ) {
		free( entry->olabel );
		entry->olabel = NULL;
	}

	if( entry->options ) {
		free( entry->options );
		entry->options = NULL;
	}

	if( entry->dgroup ) {
		free( entry->dgroup );
		entry->dgroup = NULL;
	}

	if( entry->ddevice ) {
		free( entry->ddevice );
		entry->ddevice = NULL;
	}

	if( entry->dependencies ) {
		free( entry->dependencies );
		entry->dependencies = NULL;
	}

	if( entry->dchar ) {
		free( entry->dchar );
		entry->dchar = NULL;
	}
}

static
unsigned char *
en_getfield( tid, eptr, fieldname )
int tid;
ENTRY eptr;
unsigned char *fieldname;
{
	register unsigned char *field;

	field = TLgetfield( tid, eptr, fieldname );
	if( !field || !*field ) 
		return( (unsigned char *)strdup((char *)""));

	return( (unsigned char *)strdup( (char *)field ) );
}


/* return values
	 0 for sucess
	-1 for no match in criteria
	-2 required field is null or malloc space for field failed
*/

int
en_parse( tid, eptr, entry )
int tid;
ENTRY	eptr;
brentry_t	*entry;
{
	unsigned char *priority, *week, *day;

#ifdef TRACE
	brlog( "en_parse(): tid %d eptr 0x%x entry 0x%x", tid, eptr, entry );
#endif

	week = (unsigned char *)TLgetfield( tid, eptr, R_WEEK );
	day = (unsigned char *)TLgetfield( tid, eptr, R_DAY );

	/* Entries in the backup register table must contain a
	 * valid week and day field. If an invalid week/day field
	 * is found the entry is skipped.
	 */


	if (!week && !day )
		return(-1);

	bkr_init( entry->date );

	if( !p_weekday1( week, day, entry->date ) )
		return (-1);

	if ((priority = en_getfield( tid, eptr, R_PRIORITY)) == NULL ||
		*priority == NULL)

		entry->priority = DEFAULT_PRIORITY;
	else
		if (!(entry->priority = atoi( (char *)priority)))
			entry->priority = DEFAULT_PRIORITY;

	if ((entry->tag = en_getfield( tid, eptr, R_TAG)) == NULL ||
		*entry->tag == NULL)
	{
		brlog("en_parse(): tag field malloc failed or null");
		return (-2);
	}

	if ((entry->oname = en_getfield( tid, eptr, R_ONAME)) == NULL ||
		*entry->oname == NULL)
	{
		brlog("en_parse(): tag '%s' - malloc failed or null 'oname' field",
			entry->tag);
		return (-2);
	}
		
	if ((entry->odevice = en_getfield( tid, eptr, R_ODEVICE )) == NULL ||
		*entry->odevice == NULL)
	{

		brlog("en_parse(): tag '%s'- malloc failed or null 'odevice' field",
			 entry->tag);
		return (-2);
	}

	if ((entry->olabel = en_getfield( tid, eptr, R_OLABEL )) == NULL)
	{
		brlog("en_parse(): tag '%s' - malloc failed for 'olabel' field",
			entry->tag);
		return (-2);
	}

	if ((entry->options = en_getfield( tid, eptr, R_OPTIONS )) == NULL)
	{
		brlog("en_parse(): tag '%s' - malloc failed for 'options' field",
			entry->tag);
		return (-2);
	}

	if ((entry->method = en_getfield( tid, eptr, R_METHOD )) == NULL ||
		*entry->method == NULL)
	{
		brlog("en_parse(): tag '%s' - malloc failed or null 'method' field",
			entry->tag);
		return (-2);
	}

	if ((entry->dgroup = en_getfield( tid, eptr, R_DGROUP )) == NULL)
	{
		brlog("en_parse(): tag '%s' - malloc failed for 'dgroup' field",
			entry->tag);
		return (-2);
	}

 	if ((entry->ddevice = en_getfield( tid, eptr, R_DDEVICE )) == NULL)
	{
		brlog("en_parse(): tag '%s' - malloc failed for 'ddevice' field",
			entry->tag);
		return (-2);
	}

	if ((entry->dchar = en_getfield( tid, eptr, R_DCHAR )) == NULL)
	{
		brlog("en_parse(): tag '%s' - malloc failed for 'dchar' field",
			entry->tag);
		return (-2);
	}

	if ((entry->dlabel = en_getfield( tid, eptr, R_DMNAME )) == NULL)
	{
		brlog("en_parse(): tag '%s' - malloc failed for 'dmname' field",
			entry->tag);
		return (-2);
	}

	if ((entry->dependencies = en_getfield( tid, eptr, R_DEPEND )) == NULL)
	{
		brlog("en_parse(): tag '%s' - malloc failed for 'depend' field",
			entry->tag);
		return (-2);
	}
#ifdef TRACE
	brlog("tag '%s', oname '%s', odevice '%s', olabel '%s', method '%s',\
		dgroup '%s', ddevice '%s', dchar '%s', dlabel '%s', dpend '%s'",
		entry->tag, entry->oname, entry->odevice, entry->olabel,
		entry->method, entry->dgroup, entry->ddevice, entry->dchar,
		entry->dlabel, entry->dependencies);
#endif
	return (0);
}
