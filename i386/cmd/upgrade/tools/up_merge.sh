#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)upgrade:i386/cmd/upgrade/tools/up_merge.sh	1.14"
#ident	"$Header: $"

## This script merges SVR4.2 files with SVR4 V4 system files that
## are saved during upgrade installation (from V4 to SVR4.2).
## The merging is done using the "patch" command. 
## The patch command, a patch file as its input, when applied on a 
## file will result in a merged file.
##
## The patch files (some times referred to as context diffs) are 
## generated with the "-c" or "-C N" options of "diff" command.
## ("-c" generates three lines of context, "-C N" generates 
## N lines of context)
##
## It is important to note that the files diff'ed are NOT
## the current Version 4 file and the freshly installed SVR4.2 file.
## The files diff'ed are freshly installed Version 4 files and 
## freshly installed SVR4.2 files.
## These context diffs are being generated as part of the development
## of the upgrade feature and are included in the package.
## These diffs are then "applied", via patch command, to the saved 
## Version 4 file.
##
## The diff output files have been edited so that more hunks will match.
##
## patch returns 0 if everything is OK; non-zero, otherwise.
## If patch fails it tells which hunk failed and the failed hunk(s)
## are saved in reject files.
##
## upgrade_merge return codes:
##
##	0	Success
##	2	Failure
##	3	Usage Error
##	
## USAGE=upgrade_merge 	<file>

process_merge() {

	[ "$UPDEBUG" = "YES" ] && set -x

	while read FILENAME
	do
		[ "$UPDEBUG" = "YES" ] && goany </dev/tty

		ORIGFILE=${UPGRADEDIR}/${FILENAME}
		SVR4_2FILE=${SVR4_2DIR}/${FILENAME}
		PATCHFILE=${PATCHDIR}/${FILENAME}
		MERGEDFILE=${MERGEDIR}/${FILENAME}
		REJECTFILE=${REJECTDIR}/${FILENAME}

		rm -f ${MERGEDFILE}* ${REJECTFILE}*

		# Check if the (saved) file exists; Also check if saved 
		# file is not a symbolic link

		[ ! -f ${ORIGFILE} -o -h ${ORIGFILE} ] && {
			continue
		}

		# If there is no patch file, go onto next one
		[ ! -f ${PATCHFILE} ] && continue

		case ${FILENAME} in

		[a-zA-Z]*)

			# Certain files are considered special: these files will not
			# be merged; they will be saved at a standard place.

			egrep "special case" ${PATCHFILE}  && continue

			egrep "No differences" ${PATCHFILE} > /dev/null 2>&1
			if [ ${?} -eq 0 ]
			then
				# no merge is needed; origfile is the mergefile.
				# cp will preserve the links

				cp ${ORIGFILE} ${ROOT}/${FILENAME} 2>> ${UPERR}
			else
				# merge needed.

				temp=`dirname ${SVR4_2FILE}` 
				[ ! -d ${temp} ] && mkdir -p ${temp}

				temp=`dirname ${MERGEDFILE}` 
				[ ! -d ${temp} ] && mkdir -p ${temp}

				temp=`dirname ${REJECTFILE}` 
				[ ! -d ${temp} ] && mkdir -p ${temp}

				[ "$UPDEBUG" = "YES" ] && goany </dev/tty

				${PATCH} ${OPTIONS} ${ORIGFILE} ${PATCHFILE} -o ${MERGEDFILE} -r ${REJECTFILE} 2>> ${UPERR}
				RTNCODE=${?}

				if [ ${RTNCODE} -ne 0 ]
				then
					echo ${FILENAME} >> ${REJECTFILE_LIST}
				else

					# save the SVR4.2 file first anad then 
					# copy merged file to appropriate place
					# cp does preserve modes/owner/group

					cp ${ROOT}/${FILENAME} ${SVR4_2FILE} 2>> ${UPERR}
					cp ${MERGEDFILE} ${ROOT}/${FILENAME} 2>> ${UPERR}
				fi
			fi
			;;

		*) continue ;;
		esac

	done < ${1}

	
}

########################################################
## MAIN starts here

SBINPKGINST=/usr/sbin/pkginst

. $SBINPKGINST/updebug

[ "$UPDEBUG" = "YES" ] && set -x

OPTIONS="-c" 	## options to patch tool
PATCH=/usr/sbin/pkginst/patch	 ## path for PATCH command 
TMP=/tmp
ROOT=/

UPGRADEDIR=${ROOT}/var/sadm/upgrade
SVR4_2DIR=${UPGRADEDIR}/SVR4.2
REJECTDIR=${UPGRADEDIR}/reject
MERGEDIR=${UPGRADEDIR}/merge
PATCHDIR=${ROOT}/etc/inst/up/patch
FILE_LIST=${1}
REJECTFILE_LIST=${1}.rej
rm -f ${REJECTFILE_LIST}

[ "$UPDEBUG" = "YES" ] && goany

for i in ${REJECTDIR} ${MERGEDIR} 
do
	if [ ! -d ${i} ]
	then mkdir -p ${i}
	fi
done

USAGE="\nupgrade_merge usage: \tupgrade_merge <file>"

# Remove comments and blank lines

cat $FILE_LIST |
	grep -v '^[ 	]*#' |
	grep -v '^[ 	]*$' |
	awk '{print $1}' >/tmp/merge.$$

[ "$UPDEBUG" = "YES" ] && goany
 
if [ ${#} -eq 1  ]
then
	process_merge /tmp/merge.$$
else
	echo ${USAGE}
   	exit 3
fi

rm -f /tmp/merge.$$			# clean up /tmp

[ "$UPDEBUG" = "YES" ] && goany

if [ -f ${REJECTFILE_LIST} ]
then
	exit 2		# at least one failure
else
	exit 0		# succcess
fi

