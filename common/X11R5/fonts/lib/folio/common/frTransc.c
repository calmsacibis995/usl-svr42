/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:common/frTransc.c	1.1"
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
 *	Copyright 1988 Sun Microsystems, Inc
 */

#ifndef lint 
static char sccsid[] = "@(#)frTransc.c	1.7 9/28/89 Copyright 1988 Sun Microsystems";
#endif

/*
 *
 *	@@@@@  @@@@   @@@@@  @@@@    @@@   @   @   @@@    @@@
 *	@      @   @    @    @   @  @   @  @@  @  @      @
 *	@@@    @@@@     @    @@@@   @@@@@  @ @ @   @@@   @
 *	@      @  @     @    @  @   @   @  @  @@      @  @
 *	@      @   @    @    @   @  @   @  @   @   @@@    @@@
 *
 *	fract transcendental routines
 *
 */
/*#include "FBconfig.h"*/
#include <math.h>
#include "sh_fract.h"

/* Table of sines as fracts from 0 to 90 degrees, every half degree */
static fract sinetable[] =
{
	0, 572, 1144, 1716, 2287, 2859, 3430, 4001, 4572, 5142, 5712, 6281,
	6850, 7419, 7987, 8554, 9121, 9687, 10252, 10817, 11380, 11943,
	12505, 13066, 13626, 14185, 14742, 15299, 15855, 16409, 16962,
	17514, 18064, 18613, 19161, 19707, 20252, 20795, 21336, 21876,
	22415, 22951, 23486, 24019, 24550, 25080, 25607, 26132, 26656,
	27177, 27697, 28214, 28729, 29242, 29753, 30261, 30767, 31271,
	31772, 32271, 32768, 33262, 33754, 34242, 34729, 35212, 35693,
	36172, 36647, 37120, 37590, 38057, 38521, 38982, 39441, 39896,
	40348, 40797, 41243, 41686, 42126, 42562, 42995, 43425, 43852,
	44275, 44695, 45112, 45525, 45935, 46341, 46744, 47143, 47538,
	47930, 48318, 48703, 49084, 49461, 49834, 50203, 50569, 50931,
	51289, 51643, 51993, 52339, 52682, 53020, 53354, 53684, 54010,
	54332, 54650, 54963, 55273, 55578, 55879, 56175, 56468, 56756,
	57040, 57319, 57594, 57865, 58131, 58393, 58650, 58903, 59152,
	59396, 59635, 59870, 60100, 60326, 60547, 60764, 60976, 61183,
	61386, 61584, 61777, 61966, 62149, 62328, 62503, 62672, 62837,
	62997, 63152, 63303, 63449, 63589, 63725, 63856, 63983, 64104,
	64220, 64332, 64439, 64540, 64637, 64729, 64816, 64898, 64975,
	65048, 65115, 65177, 65234, 65287, 65334, 65376, 65414, 65446,
	65474, 65496, 65514, 65526, 65534, 65536, 65534, 
};

#ifdef undef
/* The following table is no longer used, because linear interpolation
 * provides too little accuracy. (eg arccos(cos(11)) = 12.246).
 * I'm comenting the table out rather than deleting it on the long-shot
 * chance we might want it again someday.
 */
/* Table of arccosines as fracts from 0 to 1 by .01 */
static fract arccostable[] =
{ /* ix  cos	   acos		fract	     ix  cos	   acos         fract */
 /*   0, 0.000000, 90.000002 */ 5898240, /*   1, 0.007813, 89.552374 */ 5868904,
 /*   2, 0.015625, 89.104719 */ 5839566, /*   3, 0.023438, 88.657009 */ 5810225,
 /*   4, 0.031250, 88.209217 */ 5780879, /*   5, 0.039063, 87.761316 */ 5751525,
 /*   6, 0.046875, 87.313277 */ 5722162, /*   7, 0.054688, 86.865075 */ 5692789,
 /*   8, 0.062500, 86.416680 */ 5663403, /*   9, 0.070313, 85.968065 */ 5634003,
 /*  10, 0.078125, 85.519203 */ 5604586, /*  11, 0.085938, 85.070065 */ 5575151,
 /*  12, 0.093750, 84.620622 */ 5545697, /*  13, 0.101563, 84.170848 */ 5516220,
 /*  14, 0.109375, 83.720713 */ 5486720, /*  15, 0.117188, 83.270189 */ 5457195,
 /*  16, 0.125000, 82.819246 */ 5427642, /*  17, 0.132813, 82.367855 */ 5398059,
 /*  18, 0.140625, 81.915987 */ 5368446, /*  19, 0.148438, 81.463613 */ 5338799,
 /*  20, 0.156250, 81.010702 */ 5309117, /*  21, 0.164063, 80.557224 */ 5279398,
 /*  22, 0.171875, 80.103148 */ 5249639, /*  23, 0.179688, 79.648443 */ 5219840,
 /*  24, 0.187500, 79.193078 */ 5189997, /*  25, 0.195313, 78.737022 */ 5160109,
 /*  26, 0.203125, 78.280241 */ 5130173, /*  27, 0.210938, 77.822704 */ 5100188,
 /*  28, 0.218750, 77.364376 */ 5070151, /*  29, 0.226563, 76.905226 */ 5040060,
 /*  30, 0.234375, 76.445217 */ 5009913, /*  31, 0.242188, 75.984317 */ 4979708,
 /*  32, 0.250000, 75.522489 */ 4949441, /*  33, 0.257813, 75.059698 */ 4919112,
 /*  34, 0.265625, 74.595908 */ 4888717, /*  35, 0.273438, 74.131080 */ 4858254,
 /*  36, 0.281250, 73.665178 */ 4827721, /*  37, 0.289063, 73.198164 */ 4797114,
 /*  38, 0.296875, 72.729997 */ 4766433, /*  39, 0.304688, 72.260637 */ 4735673,
 /*  40, 0.312500, 71.790044 */ 4704832, /*  41, 0.320313, 71.318177 */ 4673908,
 /*  42, 0.328125, 70.844991 */ 4642897, /*  43, 0.335938, 70.370444 */ 4611797,
 /*  44, 0.343750, 69.894491 */ 4580605, /*  45, 0.351563, 69.417087 */ 4549318,
 /*  46, 0.359375, 68.938184 */ 4517932, /*  47, 0.367188, 68.457734 */ 4486446,
 /*  48, 0.375000, 67.975688 */ 4454854, /*  49, 0.382813, 67.491997 */ 4423155,
 /*  50, 0.390625, 67.006607 */ 4391344, /*  51, 0.398438, 66.519466 */ 4359419,
 /*  52, 0.406250, 66.030519 */ 4327376, /*  53, 0.414063, 65.539710 */ 4295210,
 /*  54, 0.421875, 65.046980 */ 4262918, /*  55, 0.429688, 64.552271 */ 4230497,
 /*  56, 0.437500, 64.055521 */ 4197942, /*  57, 0.445313, 63.556667 */ 4165249,
 /*  58, 0.453125, 63.055643 */ 4132414, /*  59, 0.460938, 62.552382 */ 4099432,
 /*  60, 0.468750, 62.046814 */ 4066300, /*  61, 0.476563, 61.538868 */ 4033011,
 /*  62, 0.484375, 61.028469 */ 3999561, /*  63, 0.492188, 60.515540 */ 3965946,
 /*  64, 0.500000, 60.000001 */ 3932160, /*  65, 0.507813, 59.481770 */ 3898197,
 /*  66, 0.515625, 58.960761 */ 3864052, /*  67, 0.523438, 58.436885 */ 3829719,
 /*  68, 0.531250, 57.910050 */ 3795193, /*  69, 0.539063, 57.380159 */ 3760466,
 /*  70, 0.546875, 56.847113 */ 3725532, /*  71, 0.554688, 56.310808 */ 3690385,
 /*  72, 0.562500, 55.771135 */ 3655017, /*  73, 0.570313, 55.227981 */ 3619420,
 /*  74, 0.578125, 54.681228 */ 3583588, /*  75, 0.585938, 54.130753 */ 3547513,
 /*  76, 0.593750, 53.576427 */ 3511184, /*  77, 0.601563, 53.018115 */ 3474595,
 /*  78, 0.609375, 52.455676 */ 3437735, /*  79, 0.617188, 51.888960 */ 3400594,
 /*  80, 0.625000, 51.317813 */ 3363164, /*  81, 0.632813, 50.742071 */ 3325432,
 /*  82, 0.640625, 50.161561 */ 3287388, /*  83, 0.648438, 49.576101 */ 3249019,
 /*  84, 0.656250, 48.985501 */ 3210313, /*  85, 0.664063, 48.389558 */ 3171258,
 /*  86, 0.671875, 47.788057 */ 3131838, /*  87, 0.679688, 47.180773 */ 3092039,
 /*  88, 0.687500, 46.567464 */ 3051845, /*  89, 0.695313, 45.947876 */ 3011240,
 /*  90, 0.703125, 45.321737 */ 2970205, /*  91, 0.710938, 44.688757 */ 2928722,
 /*  92, 0.718750, 44.048626 */ 2886770, /*  93, 0.726563, 43.401016 */ 2844328,
 /*  94, 0.734375, 42.745570 */ 2801373, /*  95, 0.742188, 42.081909 */ 2757879,
 /*  96, 0.750000, 41.409623 */ 2713821, /*  97, 0.757813, 40.728270 */ 2669167,
 /*  98, 0.765625, 40.037374 */ 2623889, /*  99, 0.773438, 39.336416 */ 2577951,
 /* 100, 0.781250, 38.624834 */ 2531317, /* 101, 0.789063, 37.902014 */ 2483946,
 /* 102, 0.796875, 37.167286 */ 2435795, /* 103, 0.804688, 36.419914 */ 2386815,
 /* 104, 0.812500, 35.659088 */ 2336954, /* 105, 0.820313, 34.883912 */ 2286152,
 /* 106, 0.828125, 34.093391 */ 2234344, /* 107, 0.835938, 33.286417 */ 2181458,
 /* 108, 0.843750, 32.461746 */ 2127412, /* 109, 0.851563, 31.617976 */ 2072115,
 /* 110, 0.859375, 30.753520 */ 2015462, /* 111, 0.867188, 29.866565 */ 1957335,
 /* 112, 0.875000, 28.955025 */ 1897596, /* 113, 0.882813, 28.016483 */ 1836088,
 /* 114, 0.890625, 27.048111 */ 1772625, /* 115, 0.898438, 26.046563 */ 1706987,
 /* 116, 0.906250, 25.007834 */ 1638913, /* 117, 0.914063, 23.927062 */ 1568083,
 /* 118, 0.921875, 22.798248 */ 1494106, /* 119, 0.929688, 21.613846 */ 1416485,
 /* 120, 0.937500, 20.364135 */ 1334583, /* 121, 0.945313, 19.036234 */ 1247558,
 /* 122, 0.953125, 17.612439 */ 1154248, /* 123, 0.960938, 16.067252 */ 1052983,
 /* 124, 0.968750, 14.361512 */  941196, /* 125, 0.976563, 12.429257 */  814563,
 /* 126, 0.984375, 10.141794 */  664652, /* 127, 0.992188,  7.166644 */  469673,
 /* 128, 1.000000,  0.000000 */       0,
};
#endif
/* return the sin and cosine of an angle given in degrees */
frsincosd(ang, sinp, cosp)
register fract ang;
fract      *sinp,
           *cosp;
{
    register    quad;
    register fract *tp;
    register    forward,
                reverse;

    while (ang >= fracti(360))	/* Reduce the range */
	ang -= fracti(360);
    while (ang < 0)
	ang += fracti(360);
    if (ang >= fracti(180)) {	/* Find the correct quadrant */
	ang -= fracti(180);
	quad = 2;
    } else
	quad = 0;
    if (ang >= fracti(90)) {
	ang -= fracti(90);
	quad++;
    }
    tp = sinetable + (ang >> 15);	/* Look up the angle */
    /*
     * The gibberish at the end of the next equation does a linear
     * interpolation between two consecutive table entries.  The casts on
     * the operands of the multiply are there because it is guaranteed
     * that they are unsigned shorts and because the compiler can use that
     * fact to generate better code. 
     */
    forward = tp[0]
	+ ((unsigned short)(((unsigned short) (tp[1] - tp[0])) * (unsigned short) (ang & (((unsigned short)1 << 15) - 1))
	    + ((unsigned short)1 << 14)) >> 15);
    ang = fracti(90) - ang;	/* Do the same thing for the complement */
    tp = sinetable + (ang >> 15);
    reverse = tp[0]
	+ ((unsigned short)(((unsigned short) (tp[1] - tp[0])) * (unsigned short) (ang & (((unsigned short)1 << 15) - 1))
	    + ((unsigned short)1 << 14)) >> 15);
    switch (quad) {
	case 0:
	    *sinp = forward;
	    *cosp = reverse;
	    break;
	case 1:
	    *sinp = reverse;
	    *cosp = -forward;
	    break;
	case 2:
	    *sinp = -forward;
	    *cosp = -reverse;
	    break;
	case 3:
	    *sinp = -reverse;
	    *cosp = forward;
	    break;
    }
}

fract
frsind(theta)
{
    fract       sin,
                cos;
    frsincosd(theta, &sin, &cos);
    return sin;
}

fract
frcosd(theta)
{
    fract       sin,
                cos;
    frsincosd(theta, &sin, &cos);
    return cos;
}


/* This atantbl differs from atantable below. The other one is
 * for lookups in computing arctans. This table contains a list
 * of angles whose tangents are negative powers of two, for use
 * in the CORDIC rotation algorithm. Most of what you see is
 * explaination. Only the right-hand column is data.
 */
static fract
atantbl[] = {
/* ix	tan		atan		    fract */
/* 0	1	 	45	 	 */ 2949120,
/* 1	.5	 	26.5651	 	 */ 1740967,
/* 2	.25	 	14.0362	 	 */ 919879,
/* 3	.125	 	7.12502	 	 */ 466945,
/* 4	0.0625	 	3.57633	 	 */ 234378,
/* 5	0.03125	 	1.78991	 	 */ 117303,
/* 6	0.015625	.895174	 	 */ 58666,
/* 7	0.0078125	.447614	 	 */ 29334,
/* 8	0.00390625	.223811	 	 */ 14667,
/* 9	0.00195312	.111906	 	 */ 7333,
/* 10	0.000976562	0.0559529	 */ 3666,
/* 11	0.000488281	0.0279765	 */ 1833,
/* 12	0.000244141	0.0139882	 */ 916,
/* 13	0.00012207	0.00699411 	 */ 458,
/* 14	6.10352e-05	0.00349706  	 */ 229,
/* 15	3.05176e-05	0.00174853  	 */ 114,
};

/* 
 * Computes the arc cosine.  Assumes that the cos arg is in the range -1 to 1.
 * The quad arg specifies the quad that the result should lie in.
 * Method: The CORDIC rotation algorithm. Stripped of parts we don't use.
 * Reference: Ken Turkowski, TOG July 82. It's too devious to explain here.
 *          : Tom Duff, "Numerical Methods", Siggraph 84 Course 15 notes.
 */
fract
frarccosd(cos, quad)
fract cos;
int quad;
{   
    register fract x, y;
    register fract xtemp, theta;
    register fract *patantbl;
    register int ix;

    if (cos < 0) cos = -cos;
    if (cos == fracti(1))
	theta = 0;
    else if ( cos == 0)
	    theta = fracti(90);
    else {
	/* Gotta calculate it. */
        x = cos;
        y = frsqrt(fracti(1) - (frmul(cos, cos)));
        theta = 0;

	patantbl = atantbl;
        for(ix=0; ix<16; ix++) {
	    if (y > 0) {	/* negative pseudorotation */
	        xtemp = x + (y>>ix);
	        y = y - (x>>ix);
	        x = xtemp;
	        theta += *patantbl;
	    }
	    else {		/* positive pseudorotation */
	        xtemp = x - (y>>ix);
	        y = y + (x>>ix);
	        x = xtemp;
	        theta -= *patantbl;
	    }
	patantbl++;
        }
    }
    
    switch (quad) {
      case 2:
	theta = fracti(180) - theta;
	break;
      case 3:
	theta += fracti(180);
	break;
      case 4:
	theta = fracti(360) - theta;
	break;
    }
    return theta;
}

fract
fratan2(num, denom)
    register fract num, denom;
{
    register fract x, y;
    register fract xtemp, theta;
    register fract *patantbl;
    register int ix;
	     int flag;

    if ( denom == 0 ) {
	if (num > 0)
	    return fracti(90);
	else if (num < 0)
	    return fracti(270);
	else
	    return (0);              /* we can't divide by zero */
    }

    if ( num == 0) {
	if ( denom > 0 )
	    return 0;
	else
	    return fracti(180);
    }

    else {
	/* Gotta calculate it. */
	if (denom < 0)
	    x = -denom;
	else
	    x = denom;
	y = num;
        theta = 0;

	patantbl = atantbl;
        for(ix=0; ix<16; ix++) {
	    if (y > 0) {	/* negative pseudorotation */
	        xtemp = x + (y>>ix);
	        y = y - (x>>ix);
	        x = xtemp;
	        theta += *patantbl;
	    }
	    else {		/* positive pseudorotation */
	        xtemp = x - (y>>ix);
	        y = y + (x>>ix);
	        x = xtemp;
	        theta -= *patantbl;
	    }
	patantbl++;
        }
    }
    if (denom < 0) 
	return (fracti(180) - theta);
    else
	return theta;
}

#ifdef undef
static fract atantable[257] = {
    0, 29335, 58666, 87990, 117304, 146603, 175884, 205144, 234379,
    263585, 292760, 321899, 350999, 380058, 409070, 438034, 466945,
    495801, 524598, 553333, 582003, 610605, 639135, 667591, 695970,
    724268, 752484, 780613, 808654, 836604, 864460, 892219, 919879,
    947438, 974893, 1002241, 1029481, 1056611, 1083627, 1110529,
    1137313, 1163979, 1190524, 1216947, 1243245, 1269417, 1295461,
    1321376, 1347161, 1372813, 1398332, 1423717, 1448965, 1474076,
    1499049, 1523882, 1548575, 1573127, 1597536, 1621803, 1645926,
    1669904, 1693738, 1717426, 1740967, 1764362, 1787610, 1810710,
    1833663, 1856467, 1879123, 1901631, 1923990, 1946200, 1968261,
    1990173, 2011937, 2033552, 2055018, 2076336, 2097505, 2118526,
    2139399, 2160125, 2180703, 2201134, 2221419, 2241558, 2261551,
    2281398, 2301101, 2320659, 2340074, 2359345, 2378474, 2397460,
    2416306, 2435010, 2453574, 2471999, 2490285, 2508433, 2526443,
    2544317, 2562055, 2579658, 2597126, 2614461, 2631664, 2648734,
    2665673, 2682482, 2699161, 2715711, 2732134, 2748430, 2764600,
    2780644, 2796564, 2812361, 2828035, 2843587, 2859019, 2874330,
    2889523, 2904597, 2919554, 2934395, 2949120,
};

fract fratan2(num, denom)
    register fract num, denom;
{
    register    flags = 0,
                quo;

    if ( denom == 0 ) {
	if (num > 0)
	    return fracti(90);
	else if (num < 0)
	    return fracti(270);
	else
	    return (0);              /* we can't divide by zero */
    }
    if (num < 0) {
	num = -num;
	flags |= 1;
    }
    if (denom < 0) {
	denom = -denom;
	flags |= 2;
    }
    if (num > denom) {
	quo = num;
	num = denom;
	denom = quo;
	flags |= 4;
    }
    /* num >= 0; denom >= 0; num<=denom  => 0<=num/denom<=1 */
    quo = frdiv(num, denom) << 7;
    num = floorfr(quo);
    num = atantable[num] +
	((unsigned) ((unsigned short) (atantable[num + 1]
				       - atantable[num])
		     * (unsigned short) quo) >> 16);
    if (flags & 4)
	num = fracti(90) - num;
    if (flags & 2)
	num = fracti(180) - num;
    if (flags & 1)
	num = fracti(360) - num;
    return num;
}
#endif
/*
 * frhypotenuse(p,q) computes sqrt(p*p + q*q) using an algorithm described in
 * "Replacing Square Roots by Pythagorean Sums", IBM Journal of Research and
 * Development 27, 6: November 1983.  Also in CACM "Programming Pearls",
 * December 1986, V29N12.
 * 
 * With the two iterations done here, the algorithm is guaranteed
 * accurate to 6.5 decimal digits.  It's also guaranteed to never overflow.
 */
fract
frhypotenuse(p, q)
    register fract p,
                q;
{
    register fract r;
    if (p < 0)
	p = -p;
    if (q < 0)
	q = -q;
    if (p < q) {
	r = p;
	p = q;
	q = r;
    }
    if (p == 0)
	return 0;

    /* q <= p && p > 0 */

    r = frdiv(q, p);		/* 0 <= r <= 1 */
    r = frmul(r, r);
    r = frdiv(r, r + fracti(4));/* 0 <= r <= 1/4 */
    p = p + 2 * frmul(p, r);	/* p *= 1.5 (at most) */
    q = frmul(q, r);

    r = frdiv(q, p);
    r = frmul(r, r);
    r = frdiv(r, r + fracti(4));
    p = p + 2 * frmul(p, r);
    /*- q = frmul(q, r); */

    return p;
}
