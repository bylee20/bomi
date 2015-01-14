#!/bin/sh

# from firefox running script

# found=0
# progname="$0"
# curdir=`dirname "$progname"`
# progbase=`basename "$progname"`
# run_bomi="$curdir/bomi-bin"
# if test -x "$run_bomi"; then
# 	dist_bin="$curdir"
# 	found=1
# else
# 	here=`/bin/pwd`
# 	while [ -h "$progname" ]; do
# 		bn=`basename "$progname"`
# 		cd `dirname "$progname"`
# 		progname=`/bin/ls -l "$bn" | sed -e 's/^.* -> //' `
# 		progbase=`basename "$progname"`
# 		if [ ! -x "$progname" ]; then
# 			break
# 		fi
# 		curdir=`dirname "$progname"`
# 		run_bomi="$curdir/bomi-bin"
# 		if [ -x "$run_bomi" ]; then
# 			cd "$curdir"
# 			dist_bin=`pwd`
# 			run_bomi="$dist_bin/bomi-bin"
# 			found=1
# 			break
# 		fi
# 	done
# 	cd "$here"
# fi
# 
# if [ $found = 0 ]; then
# 	echo "Cannot find bomi runtime directory. Exiting."
# 	exit 1
# fi
# 
# LD_LIBRARY_PATH=${dist_bin}/lib:${LD_LIBRARY_PATH} \
# CMPLAYER_TRANSLATION_PATH=${dist_bin}/translations \
# CMPLAYER_PLUGIN_PATH=${dist_bin}/plugins ${run_bomi} "$@"

bomi "$@"

exit $exitcode
