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
  tiny-aes.o\
  aes-wrapper.o\
  base64.o\
  sha-wrapper.o\
  str-utils.o\
  tag-value.o\
  session-cookie.o\
  htpasswd-wrapper.o\

utils.a: $(OBJ)
	ar rcs utils.a $(OBJ)

LUADIR := $(shell ./lua-dir.sh)
INCLUDES=$(LUADIR)

CFLAGS=-Wall -O2 $(INCLUDES)
# CFLAGS=-Wall -fsanitize=address -rdynamic -g $(INCLUDES)
CC=gcc $(CFLAGS)

%.o: %.c $(wildcard *.h)
	$(CC) -c $<

clean: NULL
	rm -f $(OBJ) utils.a

NULL:
