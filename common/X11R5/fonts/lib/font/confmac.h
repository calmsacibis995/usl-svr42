/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fontlib:font/confmac.h	1.1"
/*copyright     "%c%"*/

#ifndef CONFIGMAC_H

#define CONFIGMAC_H
#define	iseol(c)	((c) == '\n' || (c) == '\r' || (c) == '\f')
#define	skip_specialwhitespace(c)	while(isspace(*(c)) || (*(c) == '.')  || *(c) == ',') (c)++;
#define	skip_whitespace(c)	while(isspace(*(c)) || *(c) == ',') (c)++;
#define	get_conf_val(c)	while(!isspace(*(c)) && *(c) != ',' && *(c) != '\0')\
						(c) ++;
#define	get_conf_val2(c)	while(!iseol(*(c)) && !isspace(*(c)) && *(c) != ',' && *(c) != '.' && *(c) != '\0')\
						(c) ++;
#define	get_conf_num(c)	while(!isdigit(*(c)) && !iseol(*(c))) (c) ++;
#define	skip_list_val(c)	while(!iseol(*(c)) && !isspace(*(c)) && *(c) != '\0')\
						(c) ++;
#define	blank_comment(c)	while (!iseol(*(c)) && *(c) != '\0')	\
						*(c)++= ' ';
#endif /*CONFIGMAC_H*/
