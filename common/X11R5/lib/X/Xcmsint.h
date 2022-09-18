/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Xcmsint.h	1.3"

#ifndef _XCMSINT_H_
#define _XCMSINT_H_

#include <X11/Xcms.h>

/*
 *	DEFINES
 */

	/*
	 * Private Status Value
	 */
#define	_XCMS_NEWNAME	-1

	/*
	 * Color Space ID's are of XcmsColorFormat type, which is an
	 *	unsigned short (16 bits).  
	 *
	 *	bit 15 (most significant bit):
	 *	    0 == Device-Independent
	 *	    1 == Device-Dependent
	 *
	 *	bit 14:
         *          0 == Registered with X Consortium
         *          1 == Unregistered
         */
#define       XCMS_DD_ID(id)          ((id) & (XcmsColorFormat)0x80000000)
#define       XCMS_DI_ID(id)          (!((id) & (XcmsColorFormat)0x80000000))
#define       XCMS_UNREG_ID(id)       ((id) & (XcmsColorFormat)0x40000000)
#define       XCMS_REG_ID(id)         (!((id) & (XcmsColorFormat)0x40000000))
#define       XCMS_FIRST_REG_DI_ID    (XcmsColorFormat)0x00000001
#define       XCMS_FIRST_UNREG_DI_ID  (XcmsColorFormat)0x40000000
#define       XCMS_FIRST_REG_DD_ID    (XcmsColorFormat)0x80000000
#define       XCMS_FIRST_UNREG_DD_ID  (XcmsColorFormat)0xc0000000

/*
 *	TYPEDEFS
 */

    /*
     * Structure for caching Colormap info.
     *    This is provided for the Xlib modifications to:
     *		XAllocNamedColor()
     *		XLookupColor()
     *		XParseColor()
     *		XStoreNamedColor()
     */
typedef struct _XcmsCmapRec {
    Colormap cmapID;
    Display *dpy;
    Window windowID;
    Visual *visual;
    struct _XcmsCCC *ccc;
    struct _XcmsCmapRec *pNext;
} XcmsCmapRec;

    /*
     * Intensity Record (i.e., value / intensity tuple)
     */
typedef struct _IntensityRec {
    unsigned short value;
    XcmsFloat intensity;
} IntensityRec;

    /*
     * Intensity Table
     */
typedef struct _IntensityTbl {
    IntensityRec *pBase;
    unsigned int nEntries;
} IntensityTbl;

    /*
     * Structure for storing per-Visual Intensity Tables (aka gamma maps).
     */
typedef struct _XcmsIntensityMap {
    VisualID visualID;
    XPointer	screenData;	/* pointer to corresponding Screen Color*/
				/*	Characterization Data		*/
    void (*pFreeScreenData)();	/* Function that frees a Screen		*/
				/*   structure.				*/
    struct _XcmsIntensityMap *pNext;
} XcmsIntensityMap;


    /*
     * Structure for storing "registered" color space prefix/ID
     */
typedef struct _XcmsRegColorSpaceEntry {
    char *prefix;	/* Color Space prefix (e.g., "CIEXYZ:") */
    XcmsColorFormat id;	/* Color Space ID (e.g., XcmsCIEXYZFormat) */
} XcmsRegColorSpaceEntry;


    /*
     * Xcms Per Display (i.e. connection) related data
     */
typedef struct _XcmsPerDpyInfo {

    XcmsCCC paDefaultCCC; /* based on default visual of screen */
	    /*
	     * Pointer to an array of XcmsCCC structures, one for
	     * each screen.
	     */
    XcmsCmapRec *pClientCmaps;	/* Pointer to linked list of XcmsCmapRec's */

} XcmsPerDpyInfo, *XcmsPerDpyInfoPtr;

/*
 *	DEFINES
 */

#define XDCCC_NUMBER	0x8000000L	/* 2**27 per XDCCC */

#ifdef GRAY
#define XDCCC_SCREENWHITEPT_ATOM_NAME	"XDCCC_GRAY_SCREENWHITEPOINT"
#define XDCCC_GRAY_CORRECT_ATOM_NAME	"XDCCC_GRAY_CORRECTION"
#endif /* GRAY */

#ifndef _ConversionValues
typedef struct _ConversionValues {
    IntensityTbl IntensityTbl;
} ConversionValues;
#endif

#ifdef GRAY
typedef struct {
    IntensityTbl *IntensityTbl;
} GRAY_SCCData;
#endif /* GRAY */

/*
 *	DEFINES
 */

#define XDCCC_MATRIX_ATOM_NAME	"XDCCC_LINEAR_RGB_MATRICES"
#define XDCCC_CORRECT_ATOM_NAME "XDCCC_LINEAR_RGB_CORRECTION"

typedef struct {
    XcmsFloat XYZtoRGBmatrix[3][3];
    XcmsFloat RGBtoXYZmatrix[3][3];
    IntensityTbl *pRedTbl;
    IntensityTbl *pGreenTbl;
    IntensityTbl *pBlueTbl;
} LINEAR_RGB_SCCData;

/*
 *	DESCRIPTION
 *		Include file for defining the math macros used in the
 *		XCMS source.  Instead of using math library routines
 *		directly, XCMS uses macros so that based on the
 *		definitions here, vendors and sites can specify exactly
 *		what routine will be called (those from libm.a or their
 *		custom routines).  If not defined to math library routines
 *		(e.g., sqrt in libm.a), then the client is not forced to
 *		be linked with -lm.
 */

#define XCMS_ATAN(x)		_XcmsArcTangent(x)
#define XCMS_COS(x)		_XcmsCosine(x)
#define XCMS_CUBEROOT(x)	_XcmsCubeRoot(x)
#define XCMS_FABS(x)		((x) < 0.0 ? -(x) : (x))
#define XCMS_SIN(x)		_XcmsSine(x)
#define XCMS_SQRT(x)		_XcmsSquareRoot(x)
#define XCMS_TAN(x)		(XCMS_SIN(x) / XCMS_COS(x))

#if defined(__STDC__)
double _XcmsArcTangent(double a);
double _XcmsCosine(double a);
double _XcmsCubeRoot(double a);
double _XcmsSine(double a);
double _XcmsSquareRoot(double a);
#else
double _XcmsArcTangent();
double _XcmsCosine();
double _XcmsCubeRoot();
double _XcmsSine();
double _XcmsSquareRoot();
#endif

/*
 *  DEFINES FOR GAMUT COMPRESSION AND QUERY ROUTINES
 */
#ifndef PI
#  ifdef M_PI
#    define PI M_PI
#  else
#    define PI 3.14159265358979323846264338327950
#  endif /* M_PI */
#endif /* PI */
#ifndef degrees
#  define degrees(r) ((XcmsFloat)(r) * 180.0 / PI)
#endif /* degrees */
#ifndef radians
#  define radians(d) ((XcmsFloat)(d) * PI / 180.0)
#endif /* radians */

#define XCMS_CIEUSTAROFHUE(h,c)	\
((XCMS_COS((h)) == 0.0) ? (XcmsFloat)0.0 : (XcmsFloat) \
((XcmsFloat)(c) / (XcmsFloat)XCMS_SQRT((XCMS_TAN(h) * XCMS_TAN(h)) + \
(XcmsFloat)1.0)))
#define XCMS_CIEVSTAROFHUE(h,c)	\
((XCMS_COS((h)) == 0.0) ? (XcmsFloat)0.0 : (XcmsFloat) \
((XcmsFloat)(c) / (XcmsFloat)XCMS_SQRT(((XcmsFloat)1.0 / \
(XcmsFloat)(XCMS_TAN(h) * XCMS_TAN(h))) + (XcmsFloat)1.0)))
/* this hue is returned in radians */
#define XCMS_CIELUV_PMETRIC_HUE(u,v)	\
(((u) != 0.0) ? XCMS_ATAN( (v) / (u)) : ((v >= 0.0) ? PI / 2 : -(PI / 2)))
#define XCMS_CIELUV_PMETRIC_CHROMA(u,v)	XCMS_SQRT(((u)*(u)) + ((v)*(v)))

#define XCMS_CIEASTAROFHUE(h,c)		XCMS_CIEUSTAROFHUE((h), (c))
#define XCMS_CIEBSTAROFHUE(h,c)		XCMS_CIEVSTAROFHUE((h), (c))
#define XCMS_CIELAB_PMETRIC_HUE(a,b)	XCMS_CIELUV_PMETRIC_HUE((a), (b))
#define XCMS_CIELAB_PMETRIC_CHROMA(a,b)	XCMS_CIELUV_PMETRIC_CHROMA((a), (b))

#endif /* _XCMSINT_H_ */
