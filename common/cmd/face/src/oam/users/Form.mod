#ident	"@(#)face:src/oam/users/Form.mod	1.4.4.4"
#ident  "$Header: Form.mod 1.9 92/01/17 $"

form=$$uxface:308:"Modify FACE Environment for a FACE User"

`set -l YES="$$uxface:61:yes" -l NO="$$uxface:56:no";
set -l I18N_UL="$$uxface:294:User's Login ID:";
set -l I18N_IFAL="$$uxface:301:Invoke FACE at Login:";
set -l I18N_PUSA="$$uxface:304:Provide UNIX System Access:";
set -l I18N_SS="$$uxface:306:Show System Administration in FACE menu:";
fmlmax "$I18N_UL" "$I18N_IFAL" "$I18N_PUSA" "$I18N_SS" | set -l FCOL;
fmlmax -l "$YES" "$NO" | set -l YESCOL`

close=`unset -l UID -l IMSG;
unset -l I18N_UL -l I18N_IFAL -l I18_PUSA -l I18N_SS -l FCOL`


done=`unset -l I18N_UL -l I18N_IFAL -l I18_PUSA -l I18N_SS -l FCOL`open TEXT $OBJ_DIR/Text.mod "$F1" "$F4" "$F2" "$F3" `getfrm`

init=`if $VMSYS/bin/chkperm -l > /dev/null;
then
	echo true;
else
	message $$uxface:309:"There are no FACE users defined on this system to modify.";
	echo false;
fi`

help=open TEXT $INTFBASE/Text.itemhelp $LININFO
autolayout=true

`fmlgrep '^vmsys:' /etc/passwd | fmlcut -f6 -d: | set -e VMSYS`

name=$I18N_UL
nrow=1
ncol=1
rows=1
columns=8
fcol=$FCOL
# frow=1
# fcol=42
lininfo=`message -f $$uxface:310:"Enter the login ID of the user you wish to modify."`form:F1
rmenu=const { `indicator -w; $VMSYS/bin/chkperm -l | sort` }
value=""
valid=`indicator -w;
fmlgrep "^${F1}:" /etc/passwd | fmlcut -f3 -d":" | set -l UID;
if [ -z "${F1}" ];
then
	set -l IMSG=$$uxface:100:"You must enter a value for this field.";
	echo false;
elif [ -z "${UID}" ];
then
	set -l IMSG=$$uxface:298:"The login specified must exist in the /etc/passwd file first.";
	echo false;
elif $VMSYS/bin/chkperm -v -u "${F1}";
then
	if [ "${UID}" -lt "100" ];
	then
		set -l IMSG=$$uxface:575:"A Login ID with uid less than 100, can not be  modified.";
	echo false;
	else
	echo true;
	fi;
else
	set -l IMSG=$$uxface:311:"${F1} is not a FACE user, use add instead.";
	echo false;
fi`
invalidmsg="${IMSG}"
scroll=true

name=$I18N_IFAL
nrow=2
ncol=1
rows=1
columns=$YESCOL
# frow=2
# fcol=42
value=`test -n "${F1}" && $VMSYS/bin/chkperm -v -u "${F1}" && $VMSYS/bin/chkperm -e invoke -u "${F1}"`
lininfo=`message -f $$uxface:302:"Should FACE be invoked automatically when this user logs onto this system?"`form:F2
rmenu=const { $YES $NO }
menuonly=true
invalidmsg=$$uxface:303:"Only yes or no are valid answers."

name=$I18N_PUSA
nrow=3
ncol=1
rows=1
columns=$YESCOL
# frow=3
# fcol=42
value=`test -n "${F1}" && $VMSYS/bin/chkperm -v -u "${F1}" && $VMSYS/bin/chkperm -e unix -u "${F1}"`
lininfo=`message -f $$uxface:305:"Should this user have access to the UNIX System shell?"`form:F3
rmenu=const { $YES $NO }
menuonly=true
invalidmsg=$$uxface:303:"Only yes or no are valid answers."

name=$I18N_SS
nrow=4
ncol=1
rows=1
columns=$YESCOL
# frow=4
# fcol=42
value=`test -n "${F1}" && $VMSYS/bin/chkperm -v -u "${F1}" && $VMSYS/bin/chkperm -e admin -u "${F1}"`
lininfo=`message -f $$uxface:307:"Should this user have a System Administration entry in the top FACE menu?"`form:F4
rmenu=const { $YES $NO }
menuonly=true
invalidmsg=$$uxface:303:"Only yes or no are valid answers."
