#!/bin/sh

LUAVER=`find /usr/lib -name 'liblua[0-9]\.[0-9]\.a'\
        | sort\
        | tail -1\
        | sed 's/\.a$//'\
        | sed 's/.*liblua//'`

if [ -z "$LUAVER" ] ; then
  echo "-lCANNOT-FIND-libluaX.Y.a"
  exit 1
else
  echo "-llua$LUAVER"
  exit 0
fi
