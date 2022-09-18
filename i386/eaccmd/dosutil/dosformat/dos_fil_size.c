/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)eac:i386/eaccmd/dosutil/dosformat/dos_fil_size.c	1.1.1.2"
#ident  "$Header: dos_fil_size.c 1.1 91/07/03 $"

/* #define		DEBUG		1	/* */

/*
		dos_fil_size(displacement)

	Returns a long indicating the filesize of the file
	in the directory entry at the passed displacement in
	sector_buffer.
*/

#include	<stdio.h>

#include	"MS-DOS.h"

long
dos_fil_size(i)
int	i;	/* displacement */
{
	unsigned	int	byte1;
	unsigned	int	byte2;

	byte1 = ((unsigned char) sector_buffer[FILE_SIZE + i]) + (256 * ((unsigned char) sector_buffer[FILE_SIZE + i + 1]));
	byte2 = ((unsigned char) sector_buffer[FILE_SIZE + i + 2]) + (256 * ((unsigned char) sector_buffer[FILE_SIZE + i + 3]));

#ifdef DEBUG
	(void) fprintf(stderr, "dos_file_size(): DEBUG - Byte1 = %u Byte2 = %u\n", byte1, byte2);
#endif

	return(byte1 + (65536 * byte2));
}
