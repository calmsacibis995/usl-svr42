#!/bin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)oampkg:common/cmd/oampkg/pkgaudit/pkgaudit.sh	1.1"
#ident	"$Header: $"

#
# Audit the /var/sadm/install/contents file and packaging information
# contained under /var/sadm/pkg.  Look for patched files that have since
# been overwritten, and remove their patch reference from the contents
# file.  If all patched files have been overwritten, remove the packaging
# information for the patch from /var/sadm/pkg.
#

#
# Perform pkginfo(1) of all fully installed packages (use long listing).
#

pkginfo -li >/var/tmp/pkginf_$$
if [ $? -ne 0 ]
then
	echo "ERROR: pkginfo() exited with unexpected error." >&2
	rm -f /var/tmp/pkginf_$$
	exit 2
fi

#
# Use awk to extract important information from pkginfo listing.  Save the
# pkginst, category, & the instdate.  Calculate a numeral representing the
# instdate in approx. minutes since Jan 1, 1985.  This instdate number will
# be used later to determine if a patch is out dated.
#
awk '
	BEGIN			{
		month["Jan"] = 0;   month["Feb"] = 31;  month["Mar"] = 60
		month["Apr"] = 91;  month["May"] = 121; month["Jun"] = 152
		month["Jul"] = 182; month["Aug"] = 213; month["Sep"] = 244
		month["Oct"] = 274; month["Nov"] = 305; month["Dec"] = 335
		}
	$1 == "PKGINST:"	{
		if (pkg != "") {
			printf("%-20s %-20s %s\n", pkg, category, instdate)
			pkg = category = ""; instdate = "0"
			}
		pkg = $2
		}
	$1 == "CATEGORY:"	{
		if ($2 ~ /patch/)
			category = "patch"
		else if (l = index($2, ",")) 
			category = substr($2,1,l-1)
		else 
			category = $2
		}
	$1 == "INSTDATE:"	{
		instdate = $4 - 1985
		instdate = instdate * 356 + month[$2] + $3 + 0
		instdate = instdate * 24 + substr($5,1,2)
		instdate = instdate * 60 + substr($5,4,2)
		}
	END			{
		if (pkg != "") {
			printf("%-20s %-20s %s\n", pkg, category, instdate)
			}
		printf("END_OF_PKGINFO\n")
		}
' /var/tmp/pkginf_$$ >/var/tmp/pkgadt_$$
if [ $? -ne 0 ]
then
	echo "ERROR: awk() exited with unexpected error." >&2
	rm -f /var/tmp/pkginf_$$ /var/tmp/pkgadt_$$
	exit 2
fi

#
# Search through the contents file.  Select patch references that need to
# be removed (using date information obtained from pkginfo).
#

awk '
	BEGIN		{
		pkgindex["f"] = pkgindex["e"] = pkgindex["v"] = 10
		pkgindex["d"] = pkgindex["x"] = pkgindex["p"] = 7
		pkgindex["b"] = pkgindex["c"] = 9
		pkgindex["l"] = pkgindex["s"] = 4
		while (getline > 0) {
			if ($0 == "END_OF_PKGINFO")
				break
			if ($2 == "patch")
				patch[$1] = "true"
			date[$1] = $3
			}
		}
	$2 ~ /^[fevdxpbcls]$/ && NF > pkgindex[$2]	{
		if ($pkgindex[$2] !~ /^[0-9]*$/) 
			offset = 0;
		else  {
			offset = 3;
			if (NF == pkgindex[$2] + offset)
				continue;
			}
		recent = 0
		for (i=pkgindex[$2] + offset; i <= NF; i++) {
			if (l = index($i, ":"))
				package = substr($i,1,l-1)
			else
				package = $i
			if (date[package] > recent)
				recent = date[package]
			}
		for (i=pkgindex[$2] + offset; i <= NF; i++) {
			if (l = index($i, ":"))
				package = substr($i,1,l-1)
			else
				package = $i
			if (patch[package] == "true") {
				if (date[package] < recent) {
					if (l = index($1, "="))
						path=substr($1,1,l-1)
					else
						path=$1
					printf("removef %s %s\n",package,path) 
					}
				}
			}
		}
' /var/tmp/pkgadt_$$ /var/sadm/install/contents >/var/tmp/content_$$
if [ $? -ne 0 ]
then
	echo "ERROR: awk() exited with unexpected error." >&2
	rm -f /var/tmp/pkginf_$$ /var/tmp/pkgadt_$$ /var/tmp/content_$$
	exit 2
fi

#
# Perform removef from previous step (if any).  Otherwise, we're done.
#

if [ -s /var/tmp/content_$$ ]
then
	sort /var/tmp/content_$$ >/var/tmp/pkgrm_$$
	if [ $? -ne 0 ]
	then
		echo "ERROR: sort() exited with unexpected error." >&2
		rm -f /var/tmp/pkginf_$$ /var/tmp/pkgadt_$$
		rm -f /var/tmp/content_$$ /var/tmp/pkgrm_$$
		exit 2
	fi
	awk '
		{ print }
		$2 != pkginst {
			if (pkginst != "")
				printf("removef -f %s\n", pkginst)
			pkginst = $2
			}
		END	{
			if (pkginst != "")
				printf("removef -f %s\n", pkginst)
			}
	' /var/tmp/pkgrm_$$ >/var/tmp/content_$$
	if [ $? -ne 0 ]
	then
		echo "ERROR: awk() exited with unexpected error." >&2
		rm -f /var/tmp/pkginf_$$ /var/tmp/pkgadt_$$
		rm -f /var/tmp/content_$$ /var/tmp/pkgrm_$$
		exit 2
	fi
	sh -x /var/tmp/content_$$
else
	rm -f /var/tmp/pkginf_$$ /var/tmp/pkgadt_$$
	rm -f /var/tmp/content_$$
	exit 0
fi

#
# Search the new contents file for patch packages that are no longer
# referenced in the contents file.
#

awk '
	BEGIN		{
		pkgindex["f"] = pkgindex["e"] = pkgindex["v"] = 10
		pkgindex["d"] = pkgindex["x"] = pkgindex["p"] = 7
		pkgindex["b"] = pkgindex["c"] = 9
		pkgindex["l"] = pkgindex["s"] = 4
		while (getline > 0) {
			if ($0 == "END_OF_PKGINFO")
				break
			if ($2 == "patch")
				patch[$1] = "true"
			}
		}
	$2 ~ /^[fevdxpbcls]$/ && NF >= pkgindex[$2]	{
		for (i = pkgindex[$2]; i <= NF; i++) {
			if (l = index($i, ":"))
				package=substr($i,1,l-1)
			else
				package=$i
			patch[package] = "false"
			}
		}
	END		{
		for (i in patch) {
			if (patch[i] == "true")
				print i
			}
		}
' /var/tmp/pkgadt_$$ /var/sadm/install/contents >/var/tmp/pkgrm_$$
if [ $? -ne 0 ]
then
	echo "ERROR: awk() exited with unexpected error." >&2
	rm -f /var/tmp/pkginf_$$ /var/tmp/pkgadt_$$
	rm -f /var/tmp/content_$$ /var/tmp/pkgrm_$$
	exit 2
fi

#
# Remove pkginfo files for invalidated patches
#

if [ -s /var/tmp/pkgrm_$$ ]
then
	while read PKG
	do
		echo "Removing '$PKG' from /var/sadm/pkg"
		rm -rf /var/sadm/pkg/$PKG
	done </var/tmp/pkgrm_$$
	if [ $? -ne 0 ]
	then
		echo "ERROR: read() exited with unexpected error." >&2
		rm -f /var/tmp/pkginf_$$ /var/tmp/pkgadt_$$
		rm -f /var/tmp/content_$$ /var/tmp/pkgrm_$$
		exit 2
	fi
fi

rm -f /var/tmp/pkginf_$$ /var/tmp/pkgadt_$$
rm -f /var/tmp/content_$$ /var/tmp/pkgrm_$$

exit 0
