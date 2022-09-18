#ident	"@(#)libfolio1.2:folio/f3font.h	1.2"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



/*  Copyright (c) 1985 by Sun Microsystems, Inc.  */
/*
 * Definitions for psfont, fontfamily and related structures, 

 * and the font caching mechanism.
 */

#ifndef _cs_psfont_h
#define _cs_psfont_h
#include "f3config.h"
#include "Xproto.h"
#include "dixfont.h"
#include "fontstruct.h"
#include "fontxlfd.h"
/*-
	Font related declarations

*/

/*-	The following diagram explains some of the important vectors in
	the definition of a glyph
   (0,0)
   *-----------------+
   |                 |
   |                 |
   |                 |
   |                 |
   |                 |
   |                 |
   |                 |
   |  origin         |
   |  *--------------+--->*width[relative to the origin]
   |                 |
   |                 |
   |                 |
   +-----------------*size


	A font is written to a file as a font struct followed by all the
	glyphs.  All the pointers are relative to the beginning of the
	font.  When the font is read in or written out the pointers are
	converted from and to this offset form.

	The wglyph structure is separate from the glyph structure because
	we need to be able to build remapped versions of a font that differ
	only in widths.
*/


#ifndef _SH_CONFIG
#include <f3config.h>
#endif

#ifndef _SH_FRACT
#include <frmath.h>
/*#include <sh_fract.h>*/
#endif


struct fpoint {
	fract x, y;
				/* A point in subpixel coordinates */
};

struct spoint	{	/* A point rounded to pixel coordinates */
	short x, y;
} ;



#ifndef _SH_GLYPH

/*
 * GLYPH internal data representation
 */
typedef struct sh_glyph
  {
   Unsgn8	gl_TYPE;	/* glyph depth and alignment */
   Unsgn8	gl_empty;	/* empty for now */
   Unsgn16	gl_WIDTH;	/* glyph width */
   Unsgn16	gl_HEIGHT;	/* glyph height */
   Unsgn16	gl_LINEBYTES;	/* struct size must be multiple of 4 bytes */
  } *GLYPH;

#define	_SH_GLYPH
#endif	/* _SH_GLYPH */



struct wglyph {			/* A glyph with its associated width
				 * and origin information */
    GLYPH glyph;		/* The Shapes glyph. */
    struct spoint origin;	/* The origin position vector */
    struct fpoint width;	/* The origin-to-origin width.  Note that this
				 * is in subpixel coordinates. */
};

typedef struct { unsigned int t; int   x, y; } POINT_B2D;

#define DrawsLeftToRight	0	/* Values for drawDirection */
#define DrawsRightToLeft 	1

typedef struct _ExtentInfo TEXTEXTENT_INFO;
typedef struct {
    Unsgn32 name;
    Unsgn32 value;
} XFONT_PROP;

typedef struct {

    xCharInfo maxBounds;
    xCharInfo minBounds;
    Unsgn16 minCharOrByte2, 
	    maxCharOrByte2;
    Unsgn8  minByte1, 
	    maxByte1;
    Unsgn16 defaultChar;
    Unsgn16 nFontProps;
    Unsgn8  drawDirection;
    Unsgn8  allCharsExist;
    Sgn16   fontAscent, 
	    fontDescent;
    Unsgn32 nCharsInfo;
} XFONT_INFO;

typedef struct _scaleInfo {
        CharInfoRec  charMetrics;     /* character info  metrics and glyph pointer */
	unsigned short	exists:1;
	unsigned short  scaled:1;
	unsigned short  allocated:1;
	unsigned short  cached:1;
	short glyphsize;

} f3ScaleInfo, *f3ScaleInfoPtr;



typedef struct _f3font {
	Sgn32         magic;	/* Magic number that identifies this font
				 * format */
    unsigned int ftype:8;
    unsigned int left_to_right:1; /* true iff for all glyphs, wy==0 && wx>=0 */
    unsigned int narrow:1;	/* true iff for all glyphs, sx<=16 */
    unsigned int fixed_width:1;	/* true iff all glyphs have the same width */
    unsigned int printermatched:1; /* true iff the widths in this font
				    * have been matched to the widths on
				    * a printer */
    unsigned int monotonic:1;	/* true iff the signs of the x widths match
				 * for all glyphs and the signs of the y
				 * widths match for all glyphs */
    unsigned int sharedglyphs:1; /* true iff glyphs are shared w/ another font*/
    unsigned int cached:1;	/* true iff font is in the unused cache. */
    unsigned int pad1:9;	/* Pad bitfields to a full word. */
    struct spoint origin,
                  size;		/* The bounding box for all glyphs in the
				 * font.  Think of superimposing all the
				 * glyphs so that thei coincide. */
    struct fpoint ascent;	/* Logical font ascent (baseline up) */
    struct fpoint descent;	/* Logical font descent (baseline down) */
    XFONT_PROP   *properties;	/* Special properties associated with this font */    
        Sgn8   *folioname;		/* The name of this font's family */
	Sgn8	*familyname;	/* the family this font belongs to */

    Unsgn8 min1, max1, min2, max2;
    Unsgn16       matrix[3][2];	/* A 3x2 matrix describing this font's 
				 * scaling and rotation */
    Unsgn32 	fsize;		/* The number of glyphs in this font */
    Unsgn16 	nglyphs;	/* The number of glyphs in this font */
    Unsgn16 	minglyph;	/* the lowest numbered valid glyph.
				 * The glyphs array starts at this
				 * index */
    Unsgn16 	defaultchar;	/* The "default" character -- replaces
				 * all missing characters */
    Unsgn16     transX;
    Unsgn16     transY;
    Unsgn16     ptsize;		/* Pad out for compatibility wit*/
    Unsgn16     pixelsize;		/* Pad out for compatibility wit*/
				 /* machines that do long word int alignment */
    Unsgn32	gsize;		/* The size of the glyphs in bytes */
    struct wglyph glyphs[1];	/* The array of wglyphs that make up this font */

    unsigned int allexist:1;
    Unsgn16 	refcount;	/* The "refcount is the number of instances of
				this font */

    FontScalableRec vals;
 
    f3ScaleInfoPtr  scaled;
    CharInfoPtr pDefault;
    FontPtr fontPtr;
 
} f3Font, *f3FontPtr;

#define NullF3Font ((f3FontPtr)0)

/*
 * font magic number 
 */
#define FONT_MAGIC 	0x137A2B45


/*
 * A psfont - used as a handle to cscript font information.
 * The 'translation' table for a remapped font is a 256-
 * element array of shorts.  The array is allocated in
 * encodefont() in nucleus and is shared by multiple fonts.
 */
enum ff_buildchar_op {
    BC_WIDTH_ONLY, BC_WIDTH_AND_SIZE_ONLY, BC_PAINT
};

struct fontfamily_opsvector {
    void (*initialize_font)();
    void (*buildchar)();
    void (*charpath)();
    void (*bbox)();
};

#define folio_defaultChar	0x20

#define font_get_type(xxx) ((xxx)->type)
#define font_set_type(xxx,yyy) ((xxx)->type = (yyy))

#define font_get_magic(xxx) ((xxx)->u.magic)
#define font_set_magic(xxx,yyy) ((xxx)->u.magic = (yyy))

#define font_get_encoding(xxx) ((xxx)->encoding)
#define font_set_encoding(xxx,yyy) ((xxx)->encoding = (yyy))

#define font_get_left_to_right(xxx) ((xxx)->left_to_right)
#define font_set_left_to_right(xxx,yyy) ((xxx)->left_to_right = (yyy))

#define font_get_narrow(xxx) ((xxx)->narrow)
#define font_set_narrow(xxx,yyy) ((xxx)->narrow = (yyy))

#define font_get_fixed_width(xxx) ((xxx)->fixed_width)
#define font_set_fixed_width(xxx,yyy) ((xxx)->fixed_width = (yyy))

#define font_get_printermatched(xxx) ((xxx)->printermatched)
#define font_set_printermatched(xxx,yyy) ((xxx)->printermatched = (yyy))

#define font_get_monotonic(xxx) ((xxx)->monotonic)
#define font_set_monotonic(xxx,yyy) ((xxx)->monotonic = (yyy))

#define font_get_origin(xxx) ((xxx)->origin)
#define font_set_origin(xxx,yyy) ((xxx)->origin = (yyy))

#define font_get_size(xxx) ((xxx)->size)
#define font_set_size(xxx,yyy) ((xxx)->size = (yyy))

#define font_get_name(xxx) ((xxx)->name)
#define font_set_name(xxx,yyy) ((xxx)->name = (yyy))

#define font_get_comment(xxx) ((xxx)->comment)
#define font_set_comment(xxx,yyy) ((xxx)->comment = (yyy))

#define font_get_nglyphs(xxx) ((xxx)->nglyphs)
#define font_set_nglyphs(xxx,yyy) ((xxx)->nglyphs = (yyy))

#define font_get_pad(xxx) ((xxx)->pad)
#define font_set_pad(xxx,yyy) ((xxx)->pad = (yyy))

#define font_get_ascent(xxx) ((xxx)->ascent)
#define font_set_ascent(xxx,yyy) ((xxx)->ascent = (yyy))

#define font_get_descent(xxx) ((xxx)->descent)
#define font_set_descent(xxx,yyy) ((xxx)->descent = (yyy))

#define font_get_defaultchar(xxx) ((xxx)->defaultchar)
#define font_set_defaultchar(xxx,yyy) ((xxx)->defaultchar = (yyy))


#define font_get_wglyphs(f) ((f)->glyphs)
#define font_get_the_wglyph(f,i) (&((f)->glyphs[i]))
#define font_get_Xascent(f) (cfloorfr((f)->ascent.y))
#define font_get_Xdescent(f) (-cfloorfr((f)->descent.y))

/*
 * font magic number 
 */
#define FONT_MAGIC 	0x137A2B45

#endif /* _cs_psfont_h */
