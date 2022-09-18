#ident	"@(#)oampkg:common/cmd/oampkg/pkgscripts/r.awk	1.7.3.1"
#ident  "$Header: r.awk 1.2 91/06/27 $"

error=no
while read dest
do
	if [ -d $dest ]
	then
		echo "$dest"
		rmdir $dest || error=yes
	elif [ -f $dest ]
	then
		echo "Modifying $dest"
		savepath=$PKGSAV/awk${dest}
		/usr/sadm/install/scripts/cmdexec /usr/bin/awk remove $savepath $dest ||
			error=yes
	else
		[ -r $dest ] && echo "$dest"
		rm -f $dest || error=yes
	fi
done
[ "$error" = yes ] &&
	exit 2
exit 0
