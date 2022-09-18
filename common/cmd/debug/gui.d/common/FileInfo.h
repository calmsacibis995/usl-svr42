/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef FILEINFO_H
#define FILEINFO_H
#ident	"@(#)debugger:gui.d/common/FileInfo.h	1.1"

enum FileType
{
	FT_UNKNOWN,
	FT_EXEC,
	FT_CORE,
	FT_TEXT
};

class FileInfo 
{
	FileType 	ftype;
	int		fd;
	char 		*fname;
	char 		*note_data;
	int		note_sz;
	char 		*obj_name;
public:
	FileInfo(char *name);
	~FileInfo();

	FileType type() { return ftype; }
	char	*get_obj_name();
};

#endif
