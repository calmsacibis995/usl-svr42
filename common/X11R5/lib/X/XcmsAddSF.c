/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XcmsAddSF.c	1.2"

#include "Xlibint.h"
#include "Xcmsint.h"

/*
 *      DEFINES
 */
#define NextUnregDdCsID(lastid) \
	    (XCMS_UNREG_ID(lastid) ? ++lastid : XCMS_FIRST_UNREG_DD_ID)
#define MIN(x,y) ((x) > (y) ? (y) : (x))


/*
 *      EXTERNS
 */
extern XPointer *_XcmsPushPointerArray();
extern XcmsColorFormat _XcmsRegFormatOfPrefix();
extern XcmsFunctionSet **_XcmsSCCFuncSets;
extern XcmsFunctionSet **_XcmsSCCFuncSetsInit;
extern XcmsColorSpace **_XcmsDDColorSpaces;
extern XcmsColorSpace **_XcmsDDColorSpacesInit;



/*
 *	NAME
 *		XcmsAddFunctionSet - Add an Screen Color Characterization
 *					Function Set
 *
 *	SYNOPSIS
 */
Status
XcmsAddFunctionSet(pNewFS)
    XcmsFunctionSet *pNewFS;
/*
 *	DESCRIPTION
 *		Additional Screen Color Characterization Function Sets are
 *		managed on a global basis.  This means that with exception
 *		of the provided DD color spaces:
 *			    RGB and RGBi
 *		DD color spaces may have different XcmsColorFormat IDs between
 *		clients.  So, you must be careful when using XcmsColorFormat
 *		across clients!  Use the routines XcmsFormatOfPrefix()
 *		and XcmsPrefixOfFormat() appropriately.
 *
 *	RETURNS
 *		XcmsSuccess if succeeded, otherwise XcmsFailure
 *
 *	CAVEATS
 *		Additional Screen Color Characterization Function Sets
 *		should be added prior to any use of the routine
 *		XcmsCreateCCC().  If not, XcmsCCC structures created
 *		prior to the call of this routines will not have had
 *		a chance to initialize using the added Screen Color
 *		Characterization Function Set.
 */
{
    XcmsFunctionSet **papSCCFuncSets = _XcmsSCCFuncSets;
    XcmsColorSpace **papNewCSs;
    XcmsColorSpace *pNewCS, **paptmpCS;
    XcmsColorFormat lastID = 0;


    if (papSCCFuncSets != NULL) {
	if ((papNewCSs = pNewFS->DDColorSpaces) == NULL) {
	    /*
	     * Error, new Screen Color Characterization Function Set
	     *	missing color spaces
	     */
	    return(XcmsFailure);
	}
	while ((pNewCS = *papNewCSs++) != NULL) {
	    if ((pNewCS->id = _XcmsRegFormatOfPrefix(pNewCS->prefix)) != 0) {
		if (XCMS_DI_ID(pNewCS->id)) {
		    /* This is a Device-Independent Color Space */
		    return(XcmsFailure);
		}
		/*
		 * REGISTERED DD Color Space
		 *    therefore use the registered ID.
		 */
	    } else {
		/*
		 * UNREGISTERED DD Color Space
		 *    then see if the color space is already in
		 *    _XcmsDDColorSpaces.
		 *	    a. If same prefix, then use the same ID.
		 *	    b. Otherwise, use a new ID.
		 */
		for (paptmpCS = _XcmsDDColorSpaces; *paptmpCS != NULL;
			paptmpCS++){
		    lastID = MIN(lastID, (*paptmpCS)->id);
		    if (strcmp(pNewCS->prefix, (*paptmpCS)->prefix) == 0) {
			pNewCS->id = (*paptmpCS)->id;
			break;
		    }
		}
		if (pNewCS->id == 0) {
		    /* still haven't found one */
		    pNewCS->id = NextUnregDdCsID(lastID);
		    if ((paptmpCS = (XcmsColorSpace **)_XcmsPushPointerArray(
		   	    (XPointer *) _XcmsDDColorSpaces,
			    (XPointer) pNewCS,
			    (XPointer *) _XcmsDDColorSpacesInit)) == NULL) {
			return(XcmsFailure);
		    }
		    _XcmsDDColorSpaces = paptmpCS;
		}
	    }
	}
    }
    if ((papSCCFuncSets = (XcmsFunctionSet **)
	    _XcmsPushPointerArray((XPointer *) _XcmsSCCFuncSets,
	    (XPointer) pNewFS,
	    (XPointer *)_XcmsSCCFuncSetsInit)) == NULL) {
	return(XcmsFailure);
    }
    _XcmsSCCFuncSets = papSCCFuncSets;

    return(XcmsSuccess);
}
