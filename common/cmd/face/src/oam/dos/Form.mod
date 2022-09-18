#ident	"@(#)face:src/oam/dos/Form.mod	1.2.4.3"
#ident  "$Header: Form.mod 1.8 92/01/17 $"

form=$$uxface:286:"Modify Global MS-DOS Programs"

`set -l I18N_TT="$$uxface:97:Terminal Type:";
set -l I18N_PME="$$uxface:281:Programs Menu Entry:";
set -l I18N_DN="$$uxface:107:Drive Number and Full Pathname of Command:";
set -l I18N_WD="$$uxface:112:Working Directory:";
set -l I18N_PFA="$$uxface:116:Prompt for Arguments:";
fmlmax -c 2 "$I18N_TT" "$I18N_PME" "$I18N_WD" "$I18N_PFA" | set -l FCOL;
fmlmax -l "$YES" "$NO" | set -l YESCOL`

done=`indicator -w;message "";
$VMSYS/bin/delserve "${ARG1}" "VMSYS";
$VMSYS/bin/creaserve "$F1" "$F2" "$F3" "$F4" "$F5" "VMSYS" "dos"`close ${ARG4} `getfrm`

close=`unset -l TITLE -l ITEM -l TERML -l OF1 -l OF2 -l OF3 -l OF4 -l OF5 -l IMSG -l TDIR -l IVAL -l NF3 -l I18N_TT -l I18N_PME -l I18N_DN -l I18N_WD -l PFA -l FCOL`

help=open TEXT $INTFBASE/Text.itemhelp $LININFO
autolayout=true
`set -l YES="$$uxface:61:yes" -l NO="$$uxface:56:no"`

`set -l TERML="${TERMINFO:-/usr/lib/terminfo}";
fmlgrep TERM= ${ARG2} | fmlcut -d= -f2 | fmlcut -d";" -f1 | set -l OF1;
set -l OF2="${ARG1}";
if fmlgrep '^eval' ${ARG2} > /dev/null;
then
	fmlgrep '^eval' ${ARG2} | fmlcut -d" " -f4- | set -l OF3;
else
	tail -1 ${ARG2} | fmlcut -d" " -f3- | set -l OF3;
fi;
fmlgrep '^cd' ${ARG2} | fmlcut -d" " -f2 | set -l OF4;
if fmlgrep '^echo' ${ARG2} > /dev/null;
then
	set -l OF5=$YES;
else
	set -l OF5=$NO;
fi`

name=$I18N_TT
show=false
nrow=1
ncol=1
rows=1
columns=14
fcol=$FCOL
# frow=1
# fcol=24
lininfo=`message -f "$$uxface:120:Enter the correct Terminal type needed for the command invoked."`form:F1
value=const "${OF1}"
valid=`echo ${F1} | fmlcut -c1 | set -l TDIR;
if [ -z "${F1}" ];
then
	set -l IMSG="$$uxface:100:You must enter a value for this field.";
	echo false;
elif [ -f "${TERML}/${TDIR}/${F1}" -a -s "${TERML}/${TDIR}/${F1}" ];
then
	echo true;
else
	set -l IMSG="$$uxface:101:${F1} is not a valid terminal on your system.";
	echo false;
fi`
invalidmsg="${IMSG}"

name=$I18N_PME
nrow=1
ncol=1
rows=1
columns=45
# frow=1
# fcol=24
lininfo=`message -f "$$uxface:282:Enter the name that should appear in the Programs Menu."`form:F2
value=const "${OF2}"
valid=`indicator -w;
if [ -z "${F2}" ];
then
	set -l IMSG="$$uxface:100:You must enter a value for this field.";
	echo false;
elif [ "${F2}" = "${OF2}" ];
then
	echo true;
elif echo "${F2}"|fmlgrep '^.*;.*$' > /dev/null;
then
	set -l IMSG="$$uxface:105:Semi-colons are not allowed in this field.";
	echo false;
elif [ -f $VMSYS/lib/services ];
then
	if fmlgrep "name=\"${F2}\"" $VMSYS/lib/services > /dev/null 2> /dev/null;
	then
		set -l IMSG="$$uxface:106:${F2} already exists.";
		echo false;
	else
		echo true;
	fi;
else
	echo true;
fi`
invalidmsg="${IMSG}"
scroll=true


name=$I18N_DN
nrow=2
ncol=1
rows=1
columns=45
# frow=3
# fcol=24
lininfo=`message -f "$$uxface:283:Enter the drive number and full pathname of the command to execute."`form:F3
value=const "$!{OF3}"
valid=`indicator -w;
echo "${F3}"|fmlcut -f1 -d" "|set -l NF3;
if [ -z "${F3}" ];
then
	set -l IVAL=false -l IMSG="$$uxface:110:A value must be entered for this field.";
elif regex -v "${NF3}" '^[a-zA-Z]:/.*$' > /dev/null;
then
	set -l IVAL=true;
else
	set -l IVAL=false -l IMSG="$$uxface:111:${NF3} contains an illegal character or is not complete.";
fi`${IVAL}
invalidmsg=${IMSG}
scroll=true

name=$I18N_WD
nrow=4
ncol=1
rows=1
columns=45
# frow=4
# fcol=24
lininfo=`message -f "$$uxface:284:Enter the directory you want to change to when the command is invoked."`form:F4
value=const "${OF4}"
valid=`if [ "$F4" = '$HOME' ];
then
	echo true;
elif [ -d $F4 ];
then
	echo true;
else
	echo false;
fi`
invalidmsg=const "$$uxface:123:The Path entered must be a valid directory"
wrap=true
scroll=true

name=$I18N_PFA
nrow=5
ncol=1
rows=1
columns=$YESCOL
# frow=5
# fcol=24
lininfo=`message -f "$$uxface:285:Should the user be prompted for arguments when the command is invoked?"`form:F5
value=const "${OF5}"
rmenu={ $YES $NO }
menuonly=true
invalidmsg="$$uxface:118:The only valid responses are yes and no."
