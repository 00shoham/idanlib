#!/bin/sh

LUADIR=`find /usr/include -type d -name '*lua*' | grep -v 5.4 | sort -rn | head -1`
if [ -z "$LUADIR" ] ; then
  if [ -f "/usr/include/lua.h" ]; then
    echo "No special include directive required" > /dev/null
    exit 0
  else
    echo "-ICannot-locate-lua.h"
    exit 1
  fi
fi

if [ -f "$LUADIR/lua.h" ] ; then
  echo "-I$LUADIR"
  exit 0
else
  echo "-Ino-lua.h-in-$LUADIR"
  exit 1
fi
