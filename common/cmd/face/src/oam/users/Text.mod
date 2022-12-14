#ident	"@(#)face:src/oam/users/Text.mod	1.3.4.3"
#ident  "$Header: Text.mod 1.4 91/10/15 $"

title=$$uxface:390:"Results of Modifying a FACE Environment for a FACE User"

framemsg=$$uxface:387:"Press CANCEL to return to the previous frame."

lifetime=longterm

help=open TEXT $INTFBASE/Text.itemhelp text:F1

begrow=any
begcol=any

close=`unset -l HMSG -l LL; rm -f /tmp/f.md.${VPID}`close ${ARG5} `getfrm`

`$VMSYS/bin/vmodify "$ARG1" "$ARG2" "$ARG3" "$ARG4" > /tmp/f.md.${VPID};
if [ "${RET}" -ne "0" ];
then
	set -l HMSG="$$uxface:391:\\
ERROR: The user could not be modified.
What follows is the output of the modify.
";
else
	set -l HMSG="$$uxface:392:\\
The FACE user has been successfully modified.
";
fi`

header="${HMSG}"

text="`readfile /tmp/f.md.${VPID}`"
columns=`longline | set -l LL;
if [ "${LL}" -gt "72" ];
then
	echo 72;
elif [ "${LL}" -lt "48" ];
then
	echo 48;
else
	echo ${LL};
fi`
rows=8
