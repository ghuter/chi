.POSIX:
.SUFFIXES: .o .c

CFLAGS = -g3 -Og -Wall -Wextra -pedantic -I./lib

BIN = lexer parser analyzer mapper marshal

all: $(BIN)

MAIN_MARSHAL_OBJ = fatarena.o token.o map.o lex.o parser.o analyzer.o marshal.o
marshal: $(MAIN_MARSHAL_OBJ) main-marshal.c
	$(CC) $(CFLAGS) $(LDFLAGS) main-marshal.c -o marshal $(MAIN_MARSHAL_OBJ)

MAIN_ANALYZER_OBJ = fatarena.o token.o map.o lex.o parser.o analyzer.o
analyzer: $(MAIN_ANALYZER_OBJ) main-analyzer.c
	$(CC) $(CFLAGS) $(LDFLAGS) main-analyzer.c -o analyzer $(MAIN_ANALYZER_OBJ)

analyzer.o: analyzer.c
	$(CC) $(CFLAGS) -c analyzer.c -o analyzer.o

MAIN_PARSER_OBJ = fatarena.o token.o map.o lex.o parser.o
parser: $(MAIN_PARSER_OBJ) main-parser.c
	$(CC) $(CFLAGS) $(LDFLAGS) main-parser.c -o parser $(MAIN_PARSER_OBJ)

parser.o: parser.c
	$(CC) $(CFLAGS) -c parser.c -o parser.o

LEXER_OBJS = map.o lexer.o lex.o fatarena.o token.o
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
	sh tests/test.sh ./lexer tests/tests-lexer
	sh tests/test.sh ./parser tests/tests-parser

rebuild-test:
	sh tests/build-test.sh ./lexer tests/tests-lexer
	sh tests/build-test.sh ./parser tests/tests-parser

.PHONY: all clean format rebuild-test

