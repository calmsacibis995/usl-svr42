/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)textedit:EucUtil.c	1.2"
#endif

/*
 *	EucUtil.c: miscellaneous utilities for EUC data (unused by libXol)
 *
 */

/*
 *
 * _CodesetOfChar: 
 *
 * Returns the codeset (0-3) of the wide character \fIc\fR.  The
 * algorithm used depends on the actual size of the wide character.
 * (2 or 4 bytes). If the \fIunmasked\fR argument is non-NULL, the 
 * process mask is masked off of c and the resulting value is stored
 * in the address given by \fIunmasked\fR.
 *
 */
#ifdef I18N
static int
_CodesetOfChar OLARGLIST((c, unmasked))
OLARG(int,   c)
OLGRA(int *, unmasked)
{
int codeset = 0;


if (iscodeset0(c))
   codeset = 0;

else if (iscodeset1(c))
   codeset = 1;

else if (iscodeset2(c))
   codeset = 2;

else if (iscodeset3(c))
   codeset = 3;

#if defined(SVR4_0) || defined(SVR4)
#define	MASK_FOR_2_BYTES	H_EUCMASK
#define	MASK_FOR_4_BYTES	EUCMASK
#else  /* SVR3.2.2 */
#define	MASK_FOR_2_BYTES	EUCMASK
#define	MASK_FOR_4_BYTES	F_EUCMASK
#endif /* SVR4 */

if (unmasked)
   if (sizeof(wchar_t) == 2)
      *unmasked = c & ~MASK_FOR_2_BYTES;
   else
      *unmasked = c & ~MASK_FOR_4_BYTES;

return (codeset);
} /* end of _CodesetOfChar */
#endif


/*
 *  _mbCharCount:
 *
 *  Return the number of multibyte characters between address 
 *  \fIstart\fR and address \fIend\fR. The string is assumed
 *  to consist of multibyte characters, and a multibyte character 
 *  boundary is assumed to lie at address \fIend\fR.  
 *  The character that begins at \fIend\fR is not included in the 
 *  count. If \fIstart\fR is greater than \fIend\fR or either 
 *  address is invalid, the function returns -1.
 *
 */
#ifdef I18N
#if OlNeedFunctionPrototypes
extern int 
_mbCharCount(
	char *	start,
	char *	end,
)
#else
extern int
_mbCharCount(start, end)
char *start;
char *end;
#endif
{
	char *current 	= start;
	int count	= 0;
	int len;

	if (start > end || !start || !end)
		return (-1);

	while (current < end){
		len = mblen(current,sizeof(wchar_t));
		count += len;
		current = &current[len];	
	}
	
	return (count);
}	/* end of _mbCharCount */
#endif /* I18N */

/*
 *  _mbStringIndex:
 *
 *  Return a pointer to the Nth character in a string.
 *  The string is assumed to consist of multibyte characters. 
 *  The range of N is between 0 and the length of the string.
 *  The string is assumed to have at least N + 1 characters.
 *
 */
#ifdef I18N
#if OlNeedFunctionPrototypes
extern char *
_mbStringIndex(
	char *	string,
	int 	index
)
#else
extern char *
_mbStringIndex(string, index)
char *string;
int index;
#endif
{
	int i 		= index;
	char *ptr 	= string;

	while (index--)
		ptr = &ptr[mblen(ptr,sizeof(wchar_t))];	

	return(ptr);
}	/* end of _mbStringIndex */
#endif /* I18N */
