#!/bin/sh

LUAVER=`find /usr/lib /usr/lib64\
	-name 'liblua[0-9]\.[0-9]\.a'\
	-or -name 'liblua-[0-9]\.[0-9]\.so'\
        | sort\
        | grep -v 5.4\
        | tail -1\
        | sed 's/\.a$//'\
        | sed 's/\.so$//'\
        | sed 's/.*liblua//'`

if [ -z "$LUAVER" ] ; then
  echo "-lCANNOT-FIND-libluaX.Y.a"
  exit 1
else
  echo "-llua$LUAVER"
  exit 0
fi
