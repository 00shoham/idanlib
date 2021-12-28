OBJ=\
  daemon.o\
  date-utils.o\
  error.o\
  fs-utils.o\
  globs.o\
  http.o\
  json-parser.o\
  mem-utils.o\
  nargv.o\
  net-utils.o\
  proc-utils.o\
  sha256.o\
  sha-wrapper.o\
  str-utils.o\
  tag-value.o\

utils.a: $(OBJ)
	ar rcs utils.a $(OBJ)

CC=gcc
CFLAGS=-Wall -O2 -g

%.o: %.c $(wildcard *.h)
	$(CC) $(CFLAGS) -c $<

clean: NULL
	rm -f $(OBJ) utils.a

NULL:
