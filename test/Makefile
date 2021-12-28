BIN=\
  async-command-io-test\
  hextime\
  http-get-test\
  json-parse-test\
  print-env-from-cgi\
  regexp-test\
  sha-test\
  tag-value-json-test\
  tag-value-test\
  time-parse-test\

all: $(BIN)

UTILDIR=../src
UTILSLIB=$(UTILDIR)/utils.a

CC=gcc
CFLAGS=-Wall -O2 -g -I$(UTILDIR)
# CFLAGS=-Wall -g -rdynamic -fsanitize=address
LIBS=-lm -lpthread -luuid

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
