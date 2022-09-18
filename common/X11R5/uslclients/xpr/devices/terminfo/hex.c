#ident	"@(#)xpr:devices/terminfo/hex.c	1.2"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "xpr.h"

int			convert_to_hex;

char			**hex_table;

/**
 ** unites_to_hex()
 **/

char			*units_to_hex (bytes, bytes_per_unit, units)
	unsigned char		*bytes;
	int			bytes_per_unit,
				units;
{
	register char		*ph;

	register unsigned char	*pb		= bytes;

	register int		_units		= units;

	static int		last_units	= -1;

	static char		*hex_bytes	= 0;


	if (units != last_units) {
		if (hex_bytes)
			free (hex_bytes);
		hex_bytes = Malloc(2 * units * bytes_per_unit);
		last_units = units;
	}

	ph = hex_bytes;
	while (_units--) {
		register int		_bytes_per_unit	= bytes_per_unit;


		while (_bytes_per_unit--) {
			register char		*hex = hex_table[*pb++];


			*ph++ = *hex++;
			*ph++ = *hex;
		}
	}

	return (hex_bytes);
}

char			*hex_table_MSNFirst[256] = {
	"00","01","02","03","04","05","06","07",
	"08","09","0A","0B","0C","0D","0E","0F",
	"10","11","12","13","14","15","16","17",
	"18","19","1A","1B","1C","1D","1E","1F",
	"20","21","22","23","24","25","26","27",
	"28","29","2A","2B","2C","2D","2E","2F",
	"30","31","32","33","34","35","36","37",
	"38","39","3A","3B","3C","3D","3E","3F",
	"40","41","42","43","44","45","46","47",
	"48","49","4A","4B","4C","4D","4E","4F",
	"50","51","52","53","54","55","56","57",
	"58","59","5A","5B","5C","5D","5E","5F",
	"60","61","62","63","64","65","66","67",
	"68","69","6A","6B","6C","6D","6E","6F",
	"70","71","72","73","74","75","76","77",
	"78","79","7A","7B","7C","7D","7E","7F",
	"80","81","82","83","84","85","86","87",
	"88","89","8A","8B","8C","8D","8E","8F",
	"90","91","92","93","94","95","96","97",
	"98","99","9A","9B","9C","9D","9E","9F",
	"A0","A1","A2","A3","A4","A5","A6","A7",
	"A8","A9","AA","AB","AC","AD","AE","AF",
	"B0","B1","B2","B3","B4","B5","B6","B7",
	"B8","B9","BA","BB","BC","BD","BE","BF",
	"C0","C1","C2","C3","C4","C5","C6","C7",
	"C8","C9","CA","CB","CC","CD","CE","CF",
	"D0","D1","D2","D3","D4","D5","D6","D7",
	"D8","D9","DA","DB","DC","DD","DE","DF",
	"E0","E1","E2","E3","E4","E5","E6","E7",
	"E8","E9","EA","EB","EC","ED","EE","EF",
	"F0","F1","F2","F3","F4","F5","F6","F7",
	"F8","F9","FA","FB","FC","FD","FE","FF",
};

char			*hex_table_LSNFirst[256] = {
	"00","10","20","30","40","50","60","70",
	"80","90","A0","B0","C0","D0","E0","F0",
	"01","11","21","31","41","51","61","71",
	"81","91","A1","B1","C1","D1","E1","F1",
	"02","12","22","32","42","52","62","72",
	"82","92","A2","B2","C2","D2","E2","F2",
	"03","13","23","33","43","53","63","73",
	"83","93","A3","B3","C3","D3","E3","F3",
	"04","14","24","34","44","54","64","74",
	"84","94","A4","B4","C4","D4","E4","F4",
	"05","15","25","35","45","55","65","75",
	"85","95","A5","B5","C5","D5","E5","F5",
	"06","16","26","36","46","56","66","76",
	"86","96","A6","B6","C6","D6","E6","F6",
	"07","17","27","37","47","57","67","77",
	"87","97","A7","B7","C7","D7","E7","F7",
	"08","18","28","38","48","58","68","78",
	"88","98","A8","B8","C8","D8","E8","F8",
	"09","19","29","39","49","59","69","79",
	"89","99","A9","B9","C9","D9","E9","F9",
	"0A","1A","2A","3A","4A","5A","6A","7A",
	"8A","9A","AA","BA","CA","DA","EA","FA",
	"0B","1B","2B","3B","4B","5B","6B","7B",
	"8B","9B","AB","BB","CB","DB","EB","FB",
	"0C","1C","2C","3C","4C","5C","6C","7C",
	"8C","9C","AC","BC","CC","DC","EC","FC",
	"0D","1D","2D","3D","4D","5D","6D","7D",
	"8D","9D","AD","BD","CD","DD","ED","FD",
	"0E","1E","2E","3E","4E","5E","6E","7E",
	"8E","9E","AE","BE","CE","DE","EE","FE",
	"0F","1F","2F","3F","4F","5F","6F","7F",
	"8F","9F","AF","BF","CF","DF","EF","FF",
};
