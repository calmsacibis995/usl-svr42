#ident	"@(#)oampkg:common/cmd/oampkg/pkgscripts/r.sed	1.6.3.1"
#ident  "$Header: r.sed 1.2 91/06/27 $"

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
		savepath=$PKGSAV/sed${dest}
		/usr/sadm/install/scripts/cmdexec /bin/sed remove $savepath $dest ||
			error=yes
	else
		[ -r $dest ] && echo "$dest"
		rm -f $dest || error=yes
	fi
done
[ "$error" = yes ] &&
	exit 2
exit 0
