/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/v256rop.c	1.1"

/*
 *	Copyright (c) 1991 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 */

/*
 * THIS SECTION OF THE CODE IS VALID ONLY FOR the Intel architecture
 */
/*  Rasterop for the 386 for Z format pixmaps.
 *
 *  A Reiser-style implementation of rasterop tailored for the
 *  386 processor with an VGA display controller.  All 16 logical
 *  functions are supported.
 *
 *  Combines the blue src plane with the blue dest plane, the green
 *  src plane with the green dest plane, and so on, taking into account
 *  the effects of the plane mask.
 *
 *  *** WARNING ***
 *  Reiser-style rasterops generate code to perform the operation
 *  at the time of execution.  Not for the weak-hearted.
 *
 *  Assumptions:
 *
 *  1)  This function assumes that clipping has already be done.
 *
 *  2)  If the source or destination drawables are windows, then the points
 *      have already been translated to their absolute screen coordinates.
 *
 *  3)  The depths of the pixmaps must be a power of 2.
 */

/*
#define	SI_ROP_DEBUG 1
*/
#include "v256rop.h"
#include <sys/types.h>
#include <sys/kd.h>
#include "vtio.h"
#include "v256.h"

#ifdef	SI_ROP_DEBUG
#include <stdio.h>
#endif	/* SI_ROP_DEBUG */

#define UNROLL_LIMIT	2

/* Macros to write phrases onto the stack. */

#define add_phrase(phrase, p)	memcpy ((p), (phrase)+1, *(phrase)),	\
	(p) += *(phrase)

#define add_constant(constant, p)	*((int *) (p)) = (int) (constant), \
	(p) += sizeof (int)

/* The code phrases from which rasterops are built.  The first byte
 * of each phrase is the length of the phrase.  The stack frame
 * looks something like this:
 *
 *	top of stack
 *	dest Plane Base	-32(%ebp)	(=0xe0)
 *	src Plane Base	-28(%ebp)	(=0xe4)
 *	inner loop cnt	-24(%ebp)	(=0xe8)
 *	dest word	-20(%ebp)	(=0xec)
 *	plane #		-16(%ebp)	(=0xf0)		(Obsolete)
 *	planeMask	-15(%ebp)	(=0xf1)		(Obsolete)
 *	write Mask	-14(%ebp)	(=0xf2)		(Obsolete)
 *	pdestBase	-12(%ebp)	(=0xf4)
 *	psrcBase	-8(%ebp)	(=0xf8)
 *	height		-4(%ebp)	(=0xfc)
 *	prev frame ptr	(%ebp)
 *	return address
 */

/* Destination Grabbers:  read a "word" from the destination pixmap
 * and store in register edx.  edi points to the destination word.
 */

static unsigned char getMemdest [] = {
    2,
    0x8b, 0x17,			/*	movl	(%edi),%edx	*/
};

/* Destination Writers:  write the "word" in register eax into the
 * destination pixmap.  edi points to the destination word.  Advance
 * the pointer to the next word (the immediate value to add must be
 * fixed up when the direction is known).  Also provide a phrase to
 * save the dest word in %edx in a temporary.
 */

static unsigned char moveMemdest [] = {
    5,
    0x89, 0x07,			/*	movl	%eax,(%edi)	*/
    0x83, 0xc7, 0x04,		/*	addl	$0x4,%edi	*/
};

static unsigned char saveDest [] = {
    3,
    0x89, 0x55, 0xec,		/*	movl	%edx,0xec(%ebp)	*/
};

/* Source Grabbers:  get a "word" from the source pixmap and store in
 * the indicated register (either eax or ebx).  These also get the
 * "special sources", when the function is CLEAR, SET, NOOP, or REVERSE.
 * esi points to the destination word, if it is needed; also advance it
 * to point to the next word (immediate value must be fixed up when
 * direction is known).
 */

static unsigned char getMemsrcEAX [] = {
    5,
    0x8b, 0x06,			/*	movl	(%esi),%eax	*/
    0x83, 0xc6, 0x04,		/*	addl	$0x4,%esi	*/
};

static unsigned char getMemsrcEBX [] = {
    5,
    0x8b, 0x1e,			/*	movl	(%esi),%ebx	*/
    0x83, 0xc6, 0x04,		/*	addl	$0x4,%esi	*/
};

static unsigned char clearEAX [] = {
    2,
    0x33, 0xc0,			/*	xorl	%eax,%eax	*/
};

static unsigned char setEAX [] = {
    5,
    0xb8, 0xff, 0xff, 0xff, 0xff,	/*	movl	$-1,%eax	*/
};

static unsigned char getDestEAX [] = {	/* assume that the dest word is in edx */
    2,
    0x8b, 0xc2,			/*	movl	%edx,%eax	*/
};

/* Next Line Repositioners:  Calculate the address of the next line.
 * Leave the result in memory.
 */

static unsigned char nextSrcLine [] = {
    5,
    0x8b, 0x75, 0xf8,		/*	movl	0xf8(%ebp),%esi	*/
    0x81, 0xc6,			/*	addl	$(4byte),%esi	*/
};

static unsigned char nextDestLine [] = {
    5,
    0x8b, 0x7d, 0xf4,		/*	movl	0xf4(%ebp),%edi	*/
    0x81, 0xc7,			/*	addl	$(4byte),%edi	*/
};

/* Shifters:  take the doubleword in registers eax and ebx and shift them
 * left or right by some number of bits and leave the result in eax.  The
 * number of bits to shift must be fixed up when known.  Note that for LSB,
 * you actually shift right to move the pixels left on the screen.
 */

static unsigned char leftShift [] = {
    4,
    0x0f, 0xac, 0xd8, 0x12,	/*	shrdl	$0x12,%ebx,%eax	*/
};

static unsigned char rightShift [] = {
    4,
    0x0f, 0xa4, 0xd8, 0x12,	/*	shldl	$0x12,%ebx,%eax	*/
};

/* Logical Operators:  perform a boolean function on two arguments, one
 * in eax and the other in edx.  Leave the result in eax.
 */

static unsigned char not [] = {
    2,
    0xf7, 0xd0,			/*	notl	%eax		*/
};

static unsigned char and [] = {
    2,
    0x23, 0xc2,			/*	andl	%edx,%eax	*/
};

static unsigned char or [] = {
    2,
    0x0b, 0xc2,			/*	orl	%edx,%eax	*/
};

static unsigned char xor [] = {
    2,
    0x33, 0xc2,			/*	xorl	%edx,%eax	*/
};

static unsigned char nor [] = {
    4,
    0x0b, 0xc2,			/*	orl	%edx,%eax	*/
    0xf7, 0xd0,			/*	notl	%eax		*/
};

static unsigned char nand [] = {
    4,
    0x23, 0xc2,			/*	andl	%edx,%eax	*/
    0xf7, 0xd0,			/*	notl	%eax		*/
};

static unsigned char equiv [] = {
    4,
    0xf7, 0xd0,			/*	notl	%eax		*/
    0x33, 0xc2,			/*	xorl	%edx,%eax	*/
};

static unsigned char orReverse [] = {
    6,
    0xf7, 0xd0,			/*	notl	%eax		*/
    0x23, 0xc2,			/*	andl	%edx,%eax	*/
    0xf7, 0xd0,			/*	notl	%eax		*/
};

static unsigned char andReverse [] = {
    6,
    0xf7, 0xd0,			/*	notl	%eax		*/
    0x0b, 0xc2,			/*	orl	%edx,%eax	*/
    0xf7, 0xd0,			/*	notl	%eax		*/
};

static unsigned char andInverse [] = {
    4,
    0xf7, 0xd0,			/*	notl	%eax		*/
    0x23, 0xc2,			/*	andl	%edx,%eax	*/
};

static unsigned char orInverse [] = {
    4,
    0xf7, 0xd0,			/*	notl	%eax		*/
    0x0b, 0xc2,			/*	orl	%edx,%eax	*/
};

/* Since there are not enough registers to hold all the values needed,
 * we have to define proper subroutine linkage with a few automatic
 * variables.
 */

static unsigned char prologue [] = {
    10,
    0x55,			/*	pushl	%ebp		*/
    0x8b, 0xec,			/*	movl	%esp,%ebp	*/
    0x83, 0xec, 0x20,		/*	subl	$0x20,%esp	*/
    0x57,			/*	pushl	%edi		*/
    0x56,			/*	pushl	%esi		*/
    0x53,			/*	pushl	%ebx		*/
    0x9c,			/*	pushf			*/
};

static unsigned char epilogue [] = {
    6,
    0x9d,			/*	popf			*/
    0x5b,			/*	popl	%ebx		*/
    0x5e,			/*	popl	%esi		*/
    0x5f,			/*	popl	%edi		*/
    0xc9,			/*	leave			*/
    0xc3,			/*	ret			*/
};

/* Phrases for moving the bit pointers to their proper places.  The
 * immediate values is not coded here, but rather, added when known.
 */

static unsigned char getSrcPtr [] = {
    1,
    0xbe,			/*	movl	$(4byte),%esi	*/
};

static unsigned char getDestPtr [] = {
    1,
    0xbf,			/*	movl	$(4byte),%edi	*/
};

static unsigned char savepLine [] = {
    6,
    0x89, 0x75, 0xf8,		/*	movl	%esi,0xf8(%ebp)	*/
    0x89, 0x7d, 0xf4,		/*	movl	%edi,0xf4(%ebp)	*/
};

/* Phrases for setting the loop count and looping */

static unsigned char getHeight [] = {
    3,
    0xc7, 0x45, 0xfc,		/*	movl	$(4byte),0xfc(%ebp)	*/
};

static unsigned char movEbxImmed [] = {
    1,
    0xbb,			/*	movl	$(4byte),%ebx	*/
};

static unsigned char movEcxImmed [] = {
    1,
    0xb9,			/*	movl	$(4byte),%ecx	*/
};

static unsigned char movCntImmed [] = {
    3,
    0xc7, 0x45, 0xe8,		/*	movl	$(4byte),0xe8(%ebp)	*/
};

static unsigned char memCntLoop [] = {
    5,
    0xff, 0x4d, 0xe8,		/*	decl	0xe8(%ebp)	*/
    0x0f, 0x85,			/*	jnz	(4 byte disp)	*/
};

static unsigned char EcxCntLoop [] = {
    3,
    0x49,			/*	decl	%ecx		*/
    0x0f, 0x85,			/*	jnz	(4 byte disp)	*/
};

static unsigned char lineLp [] = {
    5,
    0xff, 0x4d, 0xfc,		/*	decl	0xfc(%ebp)	*/
    0x0f, 0x85,			/*	jnz	(4 byte disp)	*/
};

static unsigned char loopEbx [] = {
    3,
    0x4b,			/*	decl	%ebx		*/
    0x0f, 0x85,			/*	jnz	(4 byte disp)	*/
};

/* Miscellaneous phrases for doing useful stuff */

static unsigned char andEdxImmed [] = {
    2,
    0x81, 0xe2,			/*	andl	$(4byte),%edx	*/
};

static unsigned char andEaxImmed [] = {
    1,
    0x25,			/*	andl	$(4byte),%eax	*/
};

static unsigned char movAlImmed [] = {
    1,
    0xb0,			/*	movb	$(1byte),%al	*/
};

static unsigned char movEbxEax [] = {
    2,
    0x8b, 0xc3,			/*	movl	%ebx,%eax	*/
};

static unsigned char movEcxEdx [] = {
    2,
    0x8b, 0xd1,			/*	movl	%ecx,%edx	*/
};

static unsigned char advanceSrc [] = {
    3,
    0x83, 0xc6, 0x04,		/*	addl	$0x4,%esi	*/
};

static unsigned char advanceDest [] = {
    3,
    0x83, 0xc7, 0x04,		/*	addl	$0x4,%edi	*/
};

static unsigned char notEDX [] = {
    2,
    0xf7, 0xd2,			/*	notl	%edx		*/
};

static unsigned char noOp [] = {
    0,
};

/* NOTE:
 * the first element in starttab could be 0xffffffff.  making it 0
 * lets us deal with a full first word in the middle loop, rather
 * than having to do the multiple reads and masks that we'd
 * have to do if we thought it was partial.
 *
 * These tables are correct for LSB/LSB pixmaps.
 */

static unsigned int Starttab[32] =
{
	0x00000000,
	0xFFFFFFFE,
	0xFFFFFFFC,
	0xFFFFFFF8,
	0xFFFFFFF0,
	0xFFFFFFE0,
	0xFFFFFFC0,
	0xFFFFFF80,
	0xFFFFFF00,
	0xFFFFFE00,
	0xFFFFFC00,
	0xFFFFF800,
	0xFFFFF000,
	0xFFFFE000,
	0xFFFFC000,
	0xFFFF8000,
	0xFFFF0000,
	0xFFFE0000,
	0xFFFC0000,
	0xFFF80000,
	0xFFF00000,
	0xFFE00000,
	0xFFC00000,
	0xFF800000,
	0xFF000000,
	0xFE000000,
	0xFC000000,
	0xF8000000,
	0xF0000000,
	0xE0000000,
	0xC0000000,
	0x80000000
};

static unsigned int Endtab[32] =
{
	0x00000001,
	0x00000003,
	0x00000007,
	0x0000000F,
	0x0000001F,
	0x0000003F,
	0x0000007F,
	0x000000FF,
	0x000001FF,
	0x000003FF,
	0x000007FF,
	0x00000FFF,
	0x00001FFF,
	0x00003FFF,
	0x00007FFF,
	0x0000FFFF,
	0x0001FFFF,
	0x0003FFFF,
	0x0007FFFF,
	0x000FFFFF,
	0x001FFFFF,
	0x003FFFFF,
	0x007FFFFF,
	0x00FFFFFF,
	0x01FFFFFF,
	0x03FFFFFF,
	0x07FFFFFF,
	0x0FFFFFFF,
	0x1FFFFFFF,
	0x3FFFFFFF,
	0x7FFFFFFF,
	0x00000000,
};




void
v256_rop (pSrc, pDest, srcx, srcy, destx, desty, width, height, alu, planeMask)
    register	RASTER	pSrc, pDest;
    int		srcx, srcy, destx, desty;
    int		width, height;
    int		alu;
    unsigned	planeMask;
{
    unsigned char	mem [700];
    register unsigned char	*cp = mem;
    unsigned int	startMask, endMask;
    int			startIndex, endIndex, xIncrement;
    int			srcDiff, middleCnt, loopCnt, loopSize;
    int			widthSrc, widthDest;
    int			widthBits;
    unsigned		addrMask;

    unsigned char	narrow, specialNarrowCase;
    unsigned char	left, srcUsed, dstUsed;
    unsigned char	extraLeft, extraRight, extraStart, extraEnd;

    unsigned int	*psrcBase, *pdestBase;

    unsigned char	*getDest, *getSrcEAX, *getSrcEBX;
    unsigned char	*shift, *shiftEDX, *logicOp;
    unsigned char	*putDest, *getCnt, *innerLoop;

    unsigned char	*topLineLoop, *xLoopTop;

    unsigned int	i;	/* loop cntr (ANSI) */
    int			pixelsPerWord;

#ifdef	SI_ROP_DEBUG
	fprintf(stderr, "\
v256_rop :\n\
	psrc->Bwidth= %d, pdest->width= %d, pdest->height= %d\n\
	raster_bits = %d\n\
	srcx= %d, srcy= %d, destx= %d, desty= %d\n\
	width= %d, height= %d\n\
	alu= %d\n\
	planeMask= %d\n",
	pSrc->Bwidth, pDest->Bwidth, pDest->Bheight,
	(char *)pDest->Bptr - (char *)v256_fb,
    srcx, srcy, destx, desty,
    width, height,
    alu,
    planeMask
	);
#endif	/* SI_ROP_DEBUG */

    /* Make sure the plane mask is sensible for this depth */

    planeMask &= (1 << RASTER_DEPTH(pSrc)) - 1;
    if (RASTER_DEPTH(pDest) != RASTER_DEPTH(pSrc))
    {
	ERROR_MSG("v256_rop: src, dest depths don't match\n");
	return;
    }

    if (planeMask == 0)
	return;		/* nothing to do */

    /* If the height or are weird, reject it. */

    if (height < 0 || width < 0)
    {
	ERROR_MSG("v256_rop:  width and/or height less than zero\n");
	return;
    }

    if (height == 0 || width == 0)
	return;

    /* Simple vertical scrolling should be special cased.  It is assumed
     * that this is done elsewhere.
     */

    /* Determine the amount to shift and if an additional word
     * should be read at either end of the scan line.  Also determine
     * the number of whole words.
     */

    pixelsPerWord = 32 / RASTER_DEPTH (pSrc);
    addrMask = pixelsPerWord - 1;

    startIndex = (destx & addrMask) * RASTER_DEPTH (pSrc);
    endIndex = (((destx + width - 1) & addrMask) + 1) * RASTER_DEPTH(pSrc) - 1;
    srcDiff = (srcx & addrMask) * RASTER_DEPTH (pSrc) - startIndex;
    if (srcDiff < 0)
    {
	extraLeft = FALSE;
	srcDiff += 32;
    }
    else
	extraLeft = TRUE;

    if (((((srcx + width - 1) & addrMask) + 1) * RASTER_DEPTH (pSrc) - 1) <
	endIndex)
	extraRight = TRUE;
    else
	extraRight = FALSE;

    widthBits = width * RASTER_DEPTH (pSrc);
    middleCnt = Starttab [startIndex] ?
	((widthBits - (32 - startIndex)) >> 5) :
	    widthBits >> 5;

    /* If this is a narrow case (does not span a word boundry), then
     * correct the number of words to read.  The case of 32 bits,
     * word aligned, is not a narrow case.  Force narrow cases to
     * be processed left to right.
     */

    if (startIndex + widthBits <= 32 && startIndex != 0)
    {
	narrow = left = TRUE;
	middleCnt = 0;
	extraLeft = ((srcx & addrMask) + width) > pixelsPerWord;

	/* specialNarrowCase can be interpreted as "both
	 * the source and the dest bits fit in one word,
	 * and the source must be shifted logically to
	 * the left (ie, srcx & addrMask >= destx & addrMask)."
	 */

	specialNarrowCase = !(extraLeft || extraRight);
    }
    else
    {
	narrow = specialNarrowCase = FALSE;
	left = (srcx >= destx);
    }

    /* Determine which x direction to process pixmaps and select
     * the proper phrases to shift bits for that direction.  Also
     * get the masks used to mask off the ends of the line.
     */

    if (left)	/* process left to right */
    {
	xIncrement = sizeof (int);
	leftShift [sizeof (leftShift) - 1] = srcDiff;
	shift = leftShift;
	extraStart = extraLeft;
	extraEnd = extraRight;
	startMask = Starttab [startIndex];
	endMask = Endtab [endIndex];
	if (narrow)
	{
	    startMask &= endMask ? endMask : ~0;
	    endMask = 0;
	}
    }
    else			/* process right to left */
    {
	xIncrement = -(sizeof (int));
	rightShift [sizeof (rightShift) - 1] = 32 - srcDiff;
	shift = rightShift;
	extraStart = extraRight;
	extraEnd = extraLeft;
	startMask = Endtab [endIndex];
	endMask = Starttab [startIndex];
    }

    /* Get the pointers to the pixmaps, and select phrases to read
     * and write words in them.
     */

    srcUsed = TRUE;
    widthSrc = RASTER_BYTES_PER_LINE(pSrc) >> 2;

    psrcBase = (unsigned int *) (RASTER_BITS(pSrc));

    getMemsrcEAX [sizeof (getMemsrcEAX) - 1] = xIncrement;
    getSrcEAX = getMemsrcEAX;
    getMemsrcEBX [sizeof (getMemsrcEBX) - 1] = xIncrement;
    getSrcEBX = getMemsrcEBX;

    widthDest = RASTER_BYTES_PER_LINE(pDest) >> 2;

    pdestBase = (unsigned int *) (RASTER_BITS(pDest));

    getDest = getMemdest;
    moveMemdest [sizeof (moveMemdest) - 1] = xIncrement;
    putDest = moveMemdest;

    /* Calculate the y direction to go and get pointers to the first
     * word in the line.
     */

    if (srcy < desty)	/* process bottom to top */
    {
	psrcBase += (srcy + height - 1) * widthSrc;
	pdestBase += (desty + height - 1) * widthDest;
	widthSrc = -widthSrc;
	widthDest = -widthDest;
    }
    else			/* process top to bottom */
    {
	psrcBase += srcy * widthSrc;
	pdestBase += desty * widthDest;
    }

    if (left)	/* process left to right */
    {
	psrcBase += srcx / pixelsPerWord;
	pdestBase += destx / pixelsPerWord;;
    }
    else			/* process right to left */
    {
	psrcBase += (srcx + width - 1) / pixelsPerWord;
	pdestBase += (destx + width - 1) / pixelsPerWord;
    }

    getCnt = movEcxImmed;
    innerLoop = EcxCntLoop;

    /* Select which phrases to use on the basis of the rasterop */

    dstUsed = TRUE;
    switch (alu)
    {
    case ROP_CLEAR:
	getSrcEAX = clearEAX;
	logicOp = noOp;
	srcUsed = dstUsed = FALSE;
	break;

    case ROP_AND:
	logicOp = and;
	break;

    case ROP_AND_REVERSE:
	logicOp = andReverse;
	break;

    case ROP_COPY:
	logicOp = noOp;
	dstUsed = FALSE;
	break;

    case ROP_AND_INVERSE:
	logicOp = andInverse;
	break;

    case ROP_NO_OP: /* we should be nice and just return, but
		     * in some of the stranger functions, this is
		     * not really a no op. */
	getSrcEAX = getDestEAX;
	logicOp = noOp;
	srcUsed = FALSE;
	break;

    case ROP_XOR:
	logicOp = xor;
	break;

    case ROP_OR:
	logicOp = or;
	break;

    case ROP_NOR:
	logicOp = nor;
	break;

    case ROP_EQUIV:
	logicOp = equiv;
	break;

    case ROP_INVERT:
	getSrcEAX = getDestEAX;
	logicOp = not;
	srcUsed = FALSE;
	break;

    case ROP_OR_REVERSE:
	logicOp = orReverse;
	break;

    case ROP_COPY_INVERT:
	logicOp = not;
	dstUsed = FALSE;
	break;

    case ROP_OR_INVERSE:
	logicOp = orInverse;
	break;

    case ROP_NAND:
	logicOp = nand;
	break;

    case ROP_SET:
	getSrcEAX = setEAX;
	logicOp = noOp;
	srcUsed = dstUsed = FALSE;
	break;

    default:
	ERROR_MSG("v256_rop:  bad alu\n");
	return;
    }

    if (srcUsed == FALSE)
	srcDiff = 0;

    /* If the plane mask is not all ones, then we have to get the destination
     * word and do masking similar to that done at the edges of the pixmap.
     * Replicate the plane mask for each pixel in a word.
     */
    if (planeMask != (1 << RASTER_DEPTH (pSrc)) - 1)
    {
	dstUsed = TRUE;
	for (i=RASTER_DEPTH (pSrc); i<32; i <<= 1)
	    planeMask |= planeMask << i;
	startMask &= planeMask;
	endMask &= planeMask;
    }
    else
	planeMask = 0;		/* All planes on--no need to mask */


    /* Enough playing; let's generate some code! */

    add_phrase (prologue, cp);
    add_phrase (getSrcPtr, cp);
    add_constant (psrcBase, cp);
    add_phrase (getDestPtr, cp);
    add_constant (pdestBase, cp);

    add_phrase (getHeight, cp);
    add_constant (height, cp);

    /* Top of "for each line" loop; save ptrs to current line */

    topLineLoop = cp;
    add_phrase (savepLine, cp);

    /* Deal with the partial word at the beginning of the
     * line, if there is one.
     */

    if (startMask)
    {
	add_phrase (getDest, cp);

	if (srcDiff != 0)
	{
	    if (extraStart)
		add_phrase (getSrcEAX, cp);

	    if (specialNarrowCase)
		add_phrase (getSrcEAX, cp);
	    else
		add_phrase (getSrcEBX, cp);
	    add_phrase (shift, cp);
	}
	else
	    add_phrase (getSrcEAX, cp);

	add_phrase (logicOp, cp);

	add_phrase (andEaxImmed, cp);
	add_constant (startMask, cp);
	add_phrase (andEdxImmed, cp);
	add_constant (~startMask, cp);
	add_phrase (or, cp);

	add_phrase (putDest, cp);

    }  /* end if startMask */
    else
    {
	if (srcDiff != 0)
	    add_phrase (getSrcEBX, cp);
    }

    /* generate code for whole words in middle of pixmap, if any */

    if (middleCnt > 0)
    {
	/* unroll the loop, if sensible, removing the loop overhead
	 * if it is not needed.
	 */

	loopCnt = (middleCnt >= UNROLL_LIMIT) ? middleCnt >> 1 :
	    middleCnt;
	if (loopCnt > 1)
	{
	    add_phrase (getCnt, cp);
	    add_constant (loopCnt, cp);
	}

	/* The real work:  get the dest and source, shifted if
	 * necessary, do the logical operation, and write the
	 * result to the destination.
	 */

	xLoopTop = cp;

	if (dstUsed)
	    add_phrase (getDest, cp);

	if (srcDiff != 0)
	{
	    add_phrase (movEbxEax, cp);
	    add_phrase (getSrcEBX, cp);
	    add_phrase (shift, cp);
	}
	else
	    add_phrase (getSrcEAX, cp);

	add_phrase (logicOp, cp);

	if (planeMask)
	{
	    add_phrase (andEaxImmed, cp);
	    add_constant (planeMask, cp);
	    add_phrase (andEdxImmed, cp);
	    add_constant (~planeMask, cp);
	    add_phrase (or, cp);
	}

	add_phrase (putDest, cp);

	/* To unroll the loop, just make a copy of what we have
	 * just generated.  If the loop count is odd, make 2 copies,
	 * and put only the copies in the loop (not the original).
	 */

	if (middleCnt >= UNROLL_LIMIT)
	{
	    loopSize = cp - xLoopTop;
	    if (middleCnt & 0x1)
	    {
		memcpy (cp, xLoopTop, loopSize);
		cp += loopSize;
		xLoopTop += loopSize;
	    }
	    memcpy (cp, xLoopTop, loopSize);
	    cp += loopSize;
	}

	if (loopCnt > 1)
	{
	    add_phrase (innerLoop, cp);
	    add_constant (xLoopTop - (cp + sizeof (int)), cp);
	}
    }

    /* Deal with the partial word at the end of the
     * line, if there is one.
     */

    if (endMask)
    {
	add_phrase (getDest, cp);

	if (srcDiff != 0)
	{
	    add_phrase (movEbxEax, cp);
	    if (extraEnd)
		add_phrase (getSrcEBX, cp);
	    add_phrase (shift, cp);
	}
	else
	    add_phrase (getSrcEAX, cp);

	add_phrase (logicOp, cp);

	add_phrase (andEaxImmed, cp);
	add_constant (endMask, cp);
	add_phrase (andEdxImmed, cp);
	add_constant (~endMask, cp);
	add_phrase (or, cp);

	add_phrase (putDest, cp);

    }  /* end if endMask */

    /* Point to the next line and loop until we have done all lines */

    if (srcUsed)
    {
	add_phrase (nextSrcLine, cp);
	add_constant (widthSrc<<2, cp);
    }
    add_phrase (nextDestLine, cp);
    add_constant (widthDest<<2, cp);

    add_phrase (lineLp, cp);
    add_constant (topLineLoop - (cp + sizeof (int)), cp);

    /* We're done!  Generate the code to return and MAKE IT GO! */

    add_phrase (epilogue, cp);

    ((void (*)()) mem) ();

    return;
}
