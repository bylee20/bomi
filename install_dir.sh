#!/bin/sh

if test ! $# -eq 2 || test ! -d $1 || ! install -d $2
then
    echo "terminate"
    exit 1
fi

orig="$(pwd)"
cd "$1"
abs1="$(pwd)"
cd "$orig" && cd "$2"
abs2="$(pwd)"
cd "$orig"

cd "$1" && find . \( ! -regex '.*/\..*' \) | while read one
do
    src="$abs1/$one"
    dest="$abs2/$one"
    if test -d "$src"; then
        echo "install -d -m 755 '$dest'"
        install -d -m 755 "$dest"
    else
        echo "install -m 644 '$src' '$dest'"
        install -m 644 "$src" "$dest"
    fi
done
