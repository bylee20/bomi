#!/bin/sh

if test ! $# -eq 2 || test ! -d $1 || ! install -d $2
then
	echo "terminate"
	exit 1
fi

#skins=`ls -l | grep ^d | awk '{print $9}'`
all=`cd $1 && find . \( ! -regex '.*/\..*' \)`

for one in $all; do
	src="$1/$one"
	dest="$2/$one"
	echo "install $src to $dest"
	if test -d $src; then
		install -d "$dest"
	else
		install -m 644 "$src" "$dest"
	fi
done
