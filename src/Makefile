OBJ=\
  curl-wrapper.o\
  daemon.o\
  date-utils.o\
  error.o\
  fs-utils.o\
  globs.o\
  http.o\
  json-parser.o\
  lua-wrapper.o\
  mem-utils.o\
  nargv.o\
  net-utils.o\
  proc-utils.o\
  sha1.o\
  sha256.o\
  base64.o\
  sha-wrapper.o\
  str-utils.o\
  tag-value.o\

utils.a: $(OBJ)
	ar rcs utils.a $(OBJ)

LUADIR := $(shell /usr/bin/find /usr/include -type d -name 'lua*' | sort | tail -1)
LUAPARTS := $(subst /, ,$(LUADIR))
LUAVER := $(lastword $(LUAPARTS))
INCLUDES=-I$(LUADIR)

CFLAGS=-Wall -O2 $(INCLUDES)
# CFLAGS=-Wall -fsanitize=address -rdynamic -g $(INCLUDES)
CC=gcc $(CFLAGS)

%.o: %.c $(wildcard *.h)
	$(CC) -c $<

clean: NULL
	rm -f $(OBJ) utils.a

NULL:
