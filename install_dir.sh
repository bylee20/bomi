#!/bin/sh

if test ! $# -eq 2 || test ! -d $1 || ! install -d $2
then
    echo "terminate"
    exit 1
fi

cd "$1" && find . \( ! -regex '.*/\..*' \) | while read one
do
    src="$one"
    dest="$2/$one"
    echo "install "$src" to $dest"
    if test -d "$src"; then
        install -d -m 755 "$dest"
    else
        install -m 644 "$src" "$dest"
    fi
done
