/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)libDtI:execute.h	1.1"
#endif

/*
 *      OPEN LOOK(tm) File Manager
 */

#ifndef _execute_h
#define _execute_h

extern void ExecuteAction();
extern void ExecuteProgram();

extern void Delete();
extern void Copy();
extern void Move();
extern void Link();
extern void CreateDir();
extern void CreateFile();
extern void SelectAll();
extern void BeginInternalProcess();
extern void ExecuteInternalProcess();
extern void Undo();
extern void Kill();

#endif
