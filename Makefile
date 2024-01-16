.POSIX:
.SUFFIXES: .o .c

CFLAGS = -g3 -Og -Wall -Wextra -pedantic -I./lib

BIN = lexer mapper

all: $(BIN)

LEXER_OBJS = lexer.o lex.o fatarena.o token.o
lexer: $(LEXER_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(LEXER_OBJS)

MAPPER_OBJS = mapper.o fatarena.o map.o
mapper: $(MAPPER_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(MAPPER_OBJS)

token.o: lib/token.c
	$(CC) $(CFLAGS) -c lib/token.c -o $@

fatarena.o: lib/fatarena.c
	$(CC) $(CFLAGS) -c lib/fatarena.c -o $@

map.o: lib/map.c
	$(CC) $(CFLAGS) -c lib/map.c -o $@

.c.o:
	$(CC) $(CFLAGS) -c $<


format:
	astyle -q -Z -n -A3 -t8 -p -xg -H -j -xB *.[ch]
	astyle -q -Z -n -A3 -t8 -p -xg -H -j -xB lib/*.[ch]

clean:
	rm -f $(BIN)
	rm -f *.o
	rm -f tests/*.err
	rm -f tests/*.res

tests: $(BIN)
	sh tests/test-lex.sh

debug: OFLAGS = -DDEBUG
debug: all

rebuild-test:
	sh tests/build-test-lex.sh

.PHONY: all clean format rebuild-test

