.POSIX:
.SUFFIXES: .o .c

CFLAGS = -g3 -Og -Wall -Wextra -pedantic
OFLAGS =

all: lex parser

lex: lex.o
parser: parser.o

.o:
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

.c.o:
	$(CC) $(CFLAGS) $(OFLAGS) -c $@ $<

format:
	astyle -q -Z -n -A3 -t8 -p -xg -H -j -xB *.[ch]

clean:
	rm -f lex lex.o
	rm -f tests/*.err
	rm -f tests/*.res

tests: lex
	sh tests/test-lex.sh

debug: OFLAGS = -DDEBUG
debug: all

rebuild-test:
	sh tests/build-test-lex.sh

.PHONY: all clean format rebuild-test

