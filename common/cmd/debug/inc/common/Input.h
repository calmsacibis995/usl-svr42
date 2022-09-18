/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	Input_h
#define	Input_h
#ident	"@(#)debugger:inc/common/Input.h	1.2"

// debugger command line input definitions/declarations

#include <stdio.h>

enum	prompt_type {	// types of prompt
	PRI_PROMPT,		// primary (normal) prompt
	MORE_PROMPT,		// secondary (additional input) prompt
};
extern	prompt_type InputPrompt;	// which prompt string to use
extern	const	char	*Pprompt;	// primary prompt string
extern	const	char	*Sprompt;	// secondary prompt string

overload InputFile;	// returns 0 if succesful, -1 otherwise
// open file and push on input
extern	int	InputFile(const char*, int nopop=0, int echo=1);  
// push open file onto input
extern	int	InputFile(int fd, int nopop= 0, int echo=1);   
extern	int	InputTerm();		// TRUE if input is the terminal
extern	void	CloseInput();		// close out latest input file
extern	int	InputEcho();		// true if input is file and commands
					// need to be echoed before executed

extern	int	PromptLen();
extern	const	char *GetLine();

extern 	void	prompt();

#endif	/* Input_h */
