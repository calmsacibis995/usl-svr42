/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:include/typesclr.h	1.1"
/*
 * @(#)typesclr.h 1.4 89/05/26
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


/*
---------------------------------------------------------------------
    	T Y P E   S C A L E R    
---------------------------------------------------------------------
*/
#ifdef COMMENT

    INITIALIZATION:
    	    <type_BootInit>
    Use this funtion to initialize the typescaler at boot time.
    
    There are two distinct stages for using the typescaler, these
    are namely the character DEFINITION and character ACCESS stages.

    CHARACTER DEFINITION:
    	    <type_SetTrans>
    	    <type_SetPixelRatio>
    	    <type_SetFont>
    	    <type_SetSize>
    	    <type_SetCode>
    	The above functions create the current character. The current
    	character may be modified by issuing ANY of the above functions
    	and using the values set previously by the other functions.
    	<type_SetTrans> is used to set the current transformation, the
    	default is the identity transformation. The current pixel ratio 
    	(real pixel size over ideal pixel size for the device) can be set
    	using <type_SetPixelRatio>. The default pixel ratio is 1.0.

    CHARACTER ACCESS:
    	    <type_InitAccess>
    	    <type_GetBBox>
            <type_GetFontBBox>
    	    <type_GetDisplacement>
    	    <type_GetAdvance>
    	    <type_MakeBitmap>
    	The above functions are used to access the character under the
    	current transformation. 
    	<type_InitAccess> must be called before calling any of the other
    	character access functions and only if the current character
    	and/or the current transformation has been modified.

    ADDITIONAL FUNCTIONS:
    	<type_GetSymbolCodes> - to get all symbol codes for current font file

    Please see below for more detailed explanations for typescaler functions.
#endif /*COMMENT*/


/*
    The following definitions are used to configure work areas
    for the type scaler.  The sizes of these areas will generally
    affect the performance and limits of the type scaler.
*/
#define type_WORKAREASIZE   	((int32)((int32)142 * 1024))	/* size of work area */
#define type_FONTNAMESTORAGESIZE ((int32)0)	/* storage for unique font names */
#define type_EXECSTACKSIZE  	 ((int32)2000)	/* execution stack size in bytes */
#define type_OPSTACKSIZE    	 ((int32)1000)	/* no. of entries in operand stack */
#define type_LOCALSTACKSIZE 	 ((int32)5500)	/* no. of entries for local storage */
#define type_INDEXCACHESIZE 	   ((int32)10)	/* no. of diff. font headers cached */


/* Definition for the memory to be filled by type scaler 
    <origin> is lower left hand corner of bitmap in GRID dimensions
    <width> is in pixels *** MUST BE A MULTIPLE OF 32 ***
    <height> is the number of rows in the bitmap
    <bitmap> is a pointer to the actual bitmap in bit/pixels
*/
typedef struct _type_memory{
    pair_iXY	origin;	    
    int32   	width,height;
    uint32   	*bitmap; 
} type_memory;


/* 
---------------------------------------------------------------------
    	G E T     S Y M B O L S
--------------------------------------------------------------------- 
*/
/* 
    Get all symbol codes for the current font. A maximum of 
    <maxcount> symbol codes are placed in the given <codes> array.
    The return value contains the actual number of symbols in the
    file or <maxcount> whichever is less.
*/
extern int32	type_GetSymbolCodes(/*codes,maxcount*/);
/*    int32   *codes;	    */
/*    int32   maxcount;	    */

/* 
---------------------------------------------------------------------
    	S E T      F O N T
--------------------------------------------------------------------- 
*/
/* 
    Set the current Font file name 
*/
extern bool	type_SetFont(/*name*/);
/*    char    *name;	*/


/* 
---------------------------------------------------------------------
    	S E T      S I Z E
--------------------------------------------------------------------- 
*/
/* 
    Set the current character size in units of 'em' 
*/
extern void	type_SetSize(/*size*/);
/*    double  size; */

/* 
---------------------------------------------------------------------
    	S E T     C O D E
--------------------------------------------------------------------- 
*/
/*  
    Set the current character code 
*/
extern void	type_SetCode(/*code*/);
/*    int32   code; */


/* 
---------------------------------------------------------------------
    	S E T   T R A N S F O R M A T I O N
--------------------------------------------------------------------- 
*/
/*
    Set the current transformation to the given <*trans>
*/
extern void	type_SetTrans(/*trans*/);
/*  trans_dTrans	*trans;	*/

/* 
---------------------------------------------------------------------
    	S E T   P I X E L R A T I O
--------------------------------------------------------------------- 
*/
/*
    Set the current pixel ratio <ratio> for the device
   pixelratio: (real pixel size over ideal pixel size)
*/
extern void	type_SetPixelRatio(/*ratio*/);
/*    float   ratio;	*/
/* 
---------------------------------------------------------------------
    	I N I T   A C C E S S
--------------------------------------------------------------------- 
*/
/*
    Initialize the current character under the current transformation.
    Must be called AFTER modifying the current character definition  and/or the
    current transformation AND  BEFORE accessing functions that operate 
    on the current character under the current transformation. 
    Returns FALSE if the current character definition is not valid.
*/
extern	bool	type_InitAccess();


/* 
---------------------------------------------------------------------
            G E T   F O N T  B B O X
--------------------------------------------------------------------- 
*/
/*
    Computes the bounding box <*bbox> for the font.
*/
extern	bool    type_GetFontBBox(/*trans,bbox*/);
/*trans_dTrans    *trans; */
/*bbox_dBBox              *bbox; */


/* 
---------------------------------------------------------------------
    	    G E T   B B O X
--------------------------------------------------------------------- 
*/
/*
    Computes the Grid fitted bounding box <*bbox> for the current 
    character transformed by the current transformation. This information 
    may be used to set the dimension of the bitmap to be filled by
    <type_MakeBitmap>
*/
extern  void	type_GetBBox(/*bbox*/);
/*    bbox_iBBox	*bbox;	*/


/* 
---------------------------------------------------------------------
    	    G E T   D I S P L A C E M E N T
--------------------------------------------------------------------- 
*/
/*
    Returns the displacement <*disp> for the current character transformed 
    by the current transformation.
*/
extern void	type_GetDisplacement(/*disp*/);
/*    pair_dXY	*disp;	*/

/* 
---------------------------------------------------------------------
    	    G E T   A D V A N C E
--------------------------------------------------------------------- 
*/
/*
    Get the advance width of the current character under the current 
    transformation, remember to add this to the current position for 
    the next character.

*/
extern void	type_GetAdvance(/*advance*/);
/*    pair_dXY	*advance;   */

/* 
---------------------------------------------------------------------
    	   T Y P E   S C A L E R
--------------------------------------------------------------------- 
*/
/* 
    Transform the current character using the current transformation and
    generate a bitmap defined by <mem>. If only an outline of the
    character is required set <outline> to TRUE otherwise set it to FALSE.
*/
extern void	type_MakeBitmap(/*mem,outline*/);
/*    type_memory   *mem;   	*/
/*    bool    	    outline;	*/



/* 
---------------------------------------------------------------------
    	   B O O T  I N I T
--------------------------------------------------------------------- 
*/
/* 
    Boot time initialization for typescaler
    Returns FALSE if typescaler can not initialize
    under current configuration;
*/
extern bool	type_BootInit();
