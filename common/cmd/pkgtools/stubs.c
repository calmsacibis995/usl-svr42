/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)pkgtools:stubs.c	1.2"

/* stubs for security system calls which are not available on a
   non SVR4.1ES system
*/

#include "sys/types.h"
#include "priv.h"

int lvldom(const level_t *a, const level_t *b) {
	return(0);
}
int lvlfile(const char *a, int n, level_t *b){
	return(0);
}
int lvlproc(int a,  level_t *b){
	return(0);
}
int lvlin(const char *a, level_t *n){
	return(0);
}
int lvlout(const level_t *a, char *b, int c, int d){
	return(0);
}
int lvlvalid(const level_t *a){
	return(0);
}
int secsys(int a, char *b){
	return(0);
}
int	filepriv(const char *a, int b, priv_t *c, int d){
	return(0);
}
