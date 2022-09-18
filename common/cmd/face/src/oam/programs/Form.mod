#ident	"@(#)face:src/oam/programs/Form.mod	1.8.5.5"
#ident  "$Header: Form.mod 1.11 92/01/17 $"

form=$$uxface:287:"Modify Global Programs"

`set -l YES="$$uxface:61:yes" -l NO="$$uxface:56:no";
set -l SPL_CHKR="$$uxface:237:Spell Checker";
set -l MEL_SRVR="$$uxface:369:Mail Services";
set -l I18N_TT="$$uxface:97:Terminal Type:";
set -l I18N_PME="$$uxface:281:Programs Menu Entry:";
set -l I18N_POC="$$uxface:288:Pathname of Command:"
set -l I18N_WD="$$uxface:112:Working Directory:";
set -l I18N_PFA="$$uxface:116:Prompt for Arguments:";
fmlmax -c 2 "$I18N_TT" "$I18N_PME" "$I18N_POC" "$I18N_WD" "I18N_PFA" | set -l FCOL;
fmlmax -l "$YES" "$NO" | set -l YESCOL`

done=`indicator -w;message "";
$VMSYS/bin/delserve "${ARG1}" "VMSYS";
$VMSYS/bin/creaserve "$F1" "$F2" "$F3" "$F4" "$F5" "VMSYS"`close ${ARG4} `getfrm`

close=`unset -l TITLE -l ITEM -l TERML -l OF1 -l OF2 -l OF3 -l OF4 -l OF5 -l IMSG -l TDIR -l IVAL -l NF3 -l I18N_TT -l I18N_PME -l I18N_POC -l I18N_WD -l I18N_PFA -l FCOL -l SPL_CHKR -l MEL_SRVR`

help=open TEXT $INTFBASE/Text.itemhelp $LININFO
autolayout=true

`set -l TERML="${TERMINFO:-/usr/lib/terminfo}";
fmlgrep TERM= ${ARG2} | fmlcut -d= -f2 | fmlcut -d";" -f1 | set -l OF1;
set -l OF2="${ARG1}";
if fmlgrep '^eval' ${ARG2} > /dev/null;
then
	fmlgrep '^eval' ${ARG2} | fmlcut -d" " -f2- | set -l OF3;
else
	tail -1 ${ARG2} | set -l OF3;
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
scroll=true

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
        elif fmlgrep "name=\"\$\$uxface:369:${F2}\"" $VMSYS/lib/services >
                /dev/null 2> /dev/null;
        then
                if [ "${F2}" = "${MEL_SRVR}" ];
                then
                        set -l IMSG=$$uxface:106:"${F2} already exists.";
                        echo false;
                fi;
        elif fmlgrep "name=\"\$\$uxface:237:${F2}\"" $VMSYS/lib/services >
                /dev/null 2> /dev/null;
        then
                if [ "${F2}" = "${SPL_CHKR}" ];
                then
                        set -l IMSG=$$uxface:106:"${F2} already exists.";
                        echo false;
                fi;
	else
		echo true;
	fi;
else
	echo true;
fi`
invalidmsg="${IMSG}"
scroll=true


name=$I18N_POC
nrow=2
ncol=1
rows=1
columns=45
# frow=2
# fcol=24
lininfo=`message -f "$$uxface:289:Enter the name of the command to execute."`form:F3
value=const "$!{OF3}"
valid=`indicator -w;
echo "${F3}"|fmlcut -f1 -d" "|set -l NF3;
if [ -z "${F3}" ];
then
	set -l IVAL=false -l IMSG="$$uxface:110:A value must be entered for this field.";
elif regex -v "${NF3}" '^/[\.a-zA-Z_/0-9]+$' > /dev/null;
then
	set -l IVAL=true;
elif regex -v "${NF3}" '^/[\.a-zA-Z_0-9]+$' > /dev/null;
then
	set -l IVAL=false -l IMSG="$$uxface:290:${NF3} is not an absolute path.";
else
	set -l IVAL=false -l IMSG="$$uxface:122:${NF3} contains an illegal character.";
fi;
if [ "${IVAL}" = "true" ];
then
	if [ -d "${NF3}" ];
	then
		set -l IVAL=false -l IMSG="$$uxface:235:${NF3} is not a valid command.";
	fi;
	$VMSYS/bin/pathfind "${NF3}";
	if test "$RET" != 0;
	then
		set -l IVAL=false -l IMSG="$$uxface:235:${NF3} is not a valid command.";
	fi;
fi`${IVAL}
invalidmsg=${IMSG}
scroll=true

name=$I18N_WD
nrow=3
ncol=1
rows=1
columns=45
frow=3
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
nrow=4
ncol=1
rows=1
columns=$YESCOL
# frow=4
# fcol=24
lininfo=`message -f "$$uxface:285:Should the user be prompted for arguments when the command is invoked?"`form:F5
value=const "${OF5}"
rmenu={ $YES $NO }
menuonly=true
invalidmsg="$$uxface:118:The only valid responses are yes and no."
