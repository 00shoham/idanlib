BIN=\
  async-command-io-test\
  hextime\
  http-get-test\
  json-parse-test\
  json-parse-test-2\
  json-parse-test-3\
  lua-test\
  print-env-from-cgi\
  regexp-test\
  sha-test\
  tag-value-json-test\
  tag-value-test\
  time-parse-test\

all: $(BIN)

LUADIR=/usr/include/lua5.3
UTILDIR=../src
UTILSLIB=$(UTILDIR)/utils.a
INCLUDES=-I$(LUADIR) -I$(UTILDIR)

CC=gcc
CFLAGS=-Wall -O2 -g $(INCLUDES)
# CFLAGS=-Wall -g -rdynamic -fsanitize=address
LIBS=-lm -lpthread -luuid -llua5.3 -lcurl

ALLOBJ=\
  $(UTILSLIB)

%.o: %.c $(wildcard *.h)
	$(CC) $(CFLAGS) -c $<

$(BIN): % : %.o $(ALLOBJ)
	$(CC) $(CFLAGS) -o $@ $< $(ALLOBJ) $(LIBS)

clean: NULL
	rm -f $(SHAREDOBJ) $(BIN)
	rm -f *.o

NULL:
