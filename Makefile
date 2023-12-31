.POSIX:
.SUFFIXES: .o .c

CFLAGS = -g3 -Og -Wall -Wextra -pedantic -I./lib
OFLAGS =

all: lex

lex: lex.o fatarena.o

fatarena.o: lib/fatarena.c
	$(CC) $(CFLAGS) $(OFLAGS) -c lib/fatarena.c -o fatarena.o

.o: 
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< fatarena.o

.c.o:
	$(CC) $(CFLAGS) $(OFLAGS) -c -o $@ $<


format:
	astyle -q -Z -n -A3 -t8 -p -xg -H -j -xB *.[ch]
	astyle -q -Z -n -A3 -t8 -p -xg -H -j -xB lib/*.[ch]

clean:
	rm -f *.o
	rm -f tests/*.err
	rm -f tests/*.res

tests: lex
	sh tests/test-lex.sh

debug: OFLAGS = -DDEBUG
debug: all

rebuild-test:
	sh tests/build-test-lex.sh

.PHONY: all clean format rebuild-test

