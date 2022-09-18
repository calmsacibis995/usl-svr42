/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _CRYPT_H
#define _CRYPT_H

#ident	"@(#)sgs-head:crypt.h	1.4"

/* Password and file encryption functions */

#if defined(__STDC__)

extern char *crypt(const char *, const char *);
extern int crypt_close(int *);
extern char *des_crypt(const char *, const char *);
extern void des_encrypt(char *, int);
extern void des_setkey(const char *);
extern void encrypt(char *, int);
extern int run_crypt(long, char *, unsigned, int *);
extern int run_setkey(int *, const char *);
extern void setkey(const char *);

extern void enigma_setkey(const char *);
extern void enigma_encrypt(char *, int);

extern void cryptbuf(char *, unsigned int, char *, char *, int);

#else

extern char *crypt();
extern int crypt_close();
extern char *des_crypt();
extern void des_encrypt();
extern void des_setkey();
extern void encrypt();
extern int run_crypt();
extern int run_setkey();
extern void setkey();

extern void enigma_setkey();
extern void enigma_encrypt();

extern void cryptbuf();

#endif 

#define X_ENCRYPT	   0
#define X_DECRYPT	  01

#define X_ECB		   0
#define	X_CBC		 010
#define X_OFM		 020
#define	X_CFB		 040
#define X_MODES		 070

#define X_DES		0000
#define X_ENIGMA	0100
#define X_ALGORITHM	0700

#endif	/* _CRYPT_H */

