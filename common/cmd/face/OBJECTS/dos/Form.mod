#ident	"@(#)face:OBJECTS/dos/Form.mod	1.3.4.2"
#ident  "$Header: Form.mod 1.10 92/01/17 $"

form=$$uxface:119:"Modify MS-DOS Programs"

`set -l YES="$$uxface:61:yes" -l NO="$$uxface:56:no";
set -l I18N_TT="$$uxface:97:Terminal Type:";
set -l I18N_PMN="$$uxface:102:Program Menu Name:";
set -l I18N_DN="$$uxface:107:Drive Number and Full Pathname of Command:";
set -l I18N_WD="$$uxface:112:Working Directory:";
set -l I18N_PFA="$$uxface:116:Prompt for Arguments:";
fmlmax -c 2 "$I18N_TT" "$I18N_PMN" "$I18N_WD" "$I18N_PFA" | set -l FCOL;
fmlmax -l $YES $NO | set -l YESCOL`

close=`unset -l I18N_TT -l I18N_PMN -l I18N_DN -l I18N_WD -l I18N_PFA -l FCOL`
done=`indicator -w;message "";
$VMSYS/bin/delserve "${ARG1}" "${ARG3}";
$VMSYS/bin/creaserve "$F1" "$F2" "$F3" "$F4" "$F5" "${ARG3}" "dos"`close OBJECTS/dos/Form.mod OBJECTS/dos/Menu.list

help=OPEN TEXT OBJECTS/Text.h "$$uxface:78:'HELP on' $TITLE" dos/T.hmod"$ITEM"
autolayout=true

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
lininfo=`set -l TITLE="$$uxface:98:Terminal Type" -l ITEM=1;message -f $$uxface:120:"Enter the correct Terminal type needed for the command invoked."`
value=const "${OF1}"
valid=`echo ${F1} | fmlcut -c1 | set -l TDIR;
if [ -z "${F1}" ];
then
	set -l IMSG=$$uxface:100:"You must enter a value for this field.";
	echo false;
elif [ -f "${TERML}/${TDIR}/${F1}" -a -s "${TERML}/${TDIR}/${F1}" ];
then
	echo true;
else
	set -l IMSG=$$uxface:101:"${F1} is not a valid terminal on your system.";
	echo false;
fi`
invalidmsg="${IMSG}"

name=$I18N_PMN
nrow=2
ncol=1
rows=1
columns=45
# frow=2
# fcol=24
lininfo=`set -l TITLE="$$uxface:103:Program Menu Name" -l ITEM=2;message -f $$uxface:104:"Enter a name, then press SAVE when you complete the form."`
value=const "${OF2}"
valid=`indicator -w;
if [ -z "${F2}" ];
then
	set -l IMSG=$$uxface:100:"You must enter a value for this field.";
	echo false;
elif [ "${F2}" = "${OF2}" ];
then
	echo true;
elif echo "${F2}"|fmlgrep '^.*;.*$' > /dev/null;
then
	set -l IMSG=$$uxface:105:"Semi-colons are not allowed in this field.";
	echo false;
elif fmlgrep "name=\"${F2}\"" $HOME/pref/services > /dev/null 2> /dev/null;
then
	set -l IMSG=$$uxface:106:"${F2} already exists.";
	echo false;
elif fmlgrep "name=\"${F2}\"" $VMSYS/lib/services > /dev/null 2> /dev/null;
then
	set -l IMSG=$$uxface:106:"${F2} already exists.";
	echo false;
else
	echo true;
fi`
invalidmsg="${IMSG}"
scroll=true


name=$I18N_DN
nrow=3
ncol=1
rows=1
columns=45
# too long for autolayout
frow=4
fcol=24
lininfo=`set -l TITLE="$$uxface:121:Name of Command" -l ITEM=3;message -f $$uxface:109:"Enter a command name, then press SAVE when you complete the form."`
value=const "${OF3}"
valid=`indicator -w;
echo "${F3}"|fmlcut -f1 -d" "|set -l NF3;
if [ -z "${F3}" ];
then
	set -l IVAL=false -l IMSG=$$uxface:110:"A value must be entered for this field.";
elif regex -v "${NF3}" '^[a-zA-Z]:/.*$' > /dev/null;
then
	set -l IVAL=true;
else
	set -l IVAL=false -l IMSG=$$uxface:122:"${NF3} contains an illegal character.";
fi`${IVAL}
invalidmsg=${IMSG}
scroll=true

name=$I18N_WD
nrow=5
ncol=1
rows=1
columns=45
# frow=5
# fcol=24
lininfo=`set -l TITLE="$$uxface:113:Working Directory" -l ITEM=4;message -f $$uxface:114:"Enter a directory name, then press SAVE when you complete the form."`
value=const "${OF4}"
valid=`test -d $F4`
invalidmsg=const $$uxface:123:"The Path entered must be a valid directory"
wrap=true
scroll=true

name=$I18N_PFA
nrow=6
ncol=1
rows=1
columns=$YESCOL
# frow=6
# fcol=24
lininfo=`set -l TITLE="$$uxface:117:Prompt for Arguments" -l ITEM=5;message -f $$uxface:79:"Press CHOICES to select, then press SAVE when you complete the form."`
value=const "${OF5}"
rmenu={ $YES $NO }
menuonly=true
invalidmsg=$$uxface:118:"The only valid responses are yes and no."

name=$$uxface:93:"RESET"
button=8
action=reset
