.POSIX:
.SUFFIXES: .o .c

CFLAGS = -g3 -Og -Wall -Wextra -pedantic

all: lex

lex: lex.o

.o:
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

.c.o:
	$(CC) $(CFLAGS) -c $@ $<

format:
	astyle -q -Z -n -A3 -t8 -p -xg -H -j -xB *.[ch]

clean:
	rm -f lex lex.o

.PHONY: all clean format

