/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:include/f3io.h	1.1"
/*
 * @(#)f3io.h 1.2 89/03/10
 *
 */
/*
**    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
**   
**                   PROPRIETARY NOTICE (Combined) 
**   
**            This source code is unpublished proprietary 
**            information constituting, or derived under 
**            license from AT&T's UNIX(r) System V. 
**   
**                       Copyright Notice 
**   
**            Notice of copyright on this source code 
**            product does not indicate publication. 
**   
**    Copyright (C) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
**    Copyright (C) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T
**   
**                      All rights reserved. 
**   
**    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
**   
**                    RESTRICTED RIGHTS LEGEND: 
**    Use, duplication, or disclosure by the Government is subject 
**    to restrictions as set forth in subparagraph (c)(1)(ii) of 
**    the Rights in Technical Data and Computer Software clause at 
**    DFARS 52.227-7013 and in similar clauses in the FAR and NASA 
**    FAR Supplement. 
**   
**    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
*/


extern	long	f3_EncryptState;
extern	void	f3_SetFontEncryption( /* bool */ );
extern	void	f3_Seed( /* int32 */ );

extern	int32	f3_FontReadBINTNE( /* FILE * */ );
extern	int32 	f3_FontReadWINTNE( /* FILE * */ );
extern	int32 	f3_FontReadLINTNE( /* FILE * */ );
extern	fract 	f3_FontReadSREALNE( /* FILE * */ );
extern	fract 	f3_FontReadFREALNE( /* FILE * */ );

extern	void 	f3_FontWriteBINTNE( /* FILE *, int32 */ );
extern	void 	f3_FontWriteWINTNE( /* FILE *, int32 */ );
extern	void 	f3_FontWriteLINTNE( /* FILE *, int32 */ );
extern	void 	f3_FontWriteSREALNE( /* FILE *, fract */ );
extern	void 	f3_FontWriteFREALNE( /* FILE *, fract */ );

extern	int32	f3_FontReadBINTE( /* FILE * */ );
extern	int32 	f3_FontReadWINTE( /* FILE * */ );
extern	int32 	f3_FontReadLINTE( /* FILE * */ );
extern	fract 	f3_FontReadSREALE( /* FILE * */ );
extern	fract 	f3_FontReadFREALE( /* FILE * */ );

extern	void 	f3_FontWriteBINTE( /* FILE *, int32 */ );
extern	void 	f3_FontWriteWINTE( /* FILE *, int32 */ );
extern	void 	f3_FontWriteLINTE( /* FILE *, int32 */ );
extern	void 	f3_FontWriteSREALE( /* FILE *, fract */ );
extern	void 	f3_FontWriteFREALE( /* FILE *, fract */ );

extern	int32	(*f3_FontReadBINTXX)( /* FILE * */ );
extern	int32 	(*f3_FontReadWINTXX)( /* FILE * */ );
extern	int32 	(*f3_FontReadLINTXX)( /* FILE * */ );
extern	fract 	(*f3_FontReadSREALXX)( /* FILE * */ );
extern	fract 	(*f3_FontReadFREALXX)( /* FILE * */ );

extern	void 	(*f3_FontWriteBINTXX)( /* FILE *, int32 */ );
extern	void 	(*f3_FontWriteWINTXX)( /* FILE *, int32 */ );
extern	void 	(*f3_FontWriteLINTXX)( /* FILE *, int32 */ );
extern	void 	(*f3_FontWriteSREALXX)( /* FILE *, fract */ );
extern	void 	(*f3_FontWriteFREALXX)( /* FILE *, fract */ );

#define	f3_FontReadBINT(fp)		(*f3_FontReadBINTXX)(fp)
#define	f3_FontReadWINT(fp) 		(*f3_FontReadWINTXX)(fp)
#define	f3_FontReadLINT(fp) 		(*f3_FontReadLINTXX)(fp)
#define	f3_FontReadSREAL(fp) 		(*f3_FontReadSREALXX)(fp)
#define	f3_FontReadFREAL(fp) 		(*f3_FontReadFREALXX)(fp)

#define	f3_FontWriteBINT(fp,v) 		(*f3_FontWriteBINTXX)(fp,v)
#define	f3_FontWriteWINT(fp,v)		(*f3_FontWriteWINTXX)(fp,v)
#define	f3_FontWriteLINT(fp,v)		(*f3_FontWriteLINTXX)(fp,v)
#define	f3_FontWriteSREAL(fp,v)		(*f3_FontWriteSREALXX)(fp,v)
#define	f3_FontWriteFREAL(fp,v)		(*f3_FontWriteFREALXX)(fp,v)

#define f3_MAGICNUMBER     0x137A2950
#define f3_EMAGICNUMBER    0x137A2951   /* magic number for encrypted file */




