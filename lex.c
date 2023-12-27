#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <ctype.h>
#include <assert.h>

#define BUFSZ 512

int hold = 0;
int holdc = 0;
int fd = 0;

typedef enum {
	UNDEFINED = 0,

	EOI, // End-Of-Input
	IMMEDIATE,
	IDENTIFIER,
	NEWLINE,
	SEMICOLON,
	SPC,
	COLON,
	COMMA,
	DOT,
	LPAREN,
	RPAREN,
	LBRACKETS,
	RBRACKETS,
	LBRACES,
	RBRACES,

// OPERATORS
	MUL,
	DIV,
	MOD,
	ADD,
	SUB,
	LSHIFT,
	RSHIFT,
	LT,
	GT,
	LE,
	GE,
	EQUAL,
	NEQUAL,
	BAND,
	BXOR,
	BOR,
	LAND,
	LOR,
	BNOT,
	LNOT,
	ASSIGN,
	BAND_ASSIGN,
	BOR_ASSIGN,
	BXOR_ASSIGN,
	BNOT_ASSIGN,
	LAND_ASSIGN,
	LOR_ASSIGN,
	LSHIFT_ASSIGN,
	RSHIFT_ASSIGN,
	ADD_ASSIGN,
	SUB_ASSIGN,
	MUL_ASSIGN,
	DIV_ASSIGN,
	MOD_ASSIGN,
	AT,

	NTOK, // must be the last of "regular tokens"

	FOR,
	IF,
	STRUCT,
	SIZEOF,
	RETURN,
	NKEYWORDS, // must be the last "keyword"
} ETok;

typedef struct {
	ETok type;
} Tok;

char *keywords[] = {
	[FOR]    = "for",
	[IF]     = "if",
	[STRUCT] = "struct",
	[SIZEOF] = "sizeof",
	[RETURN] = "return",
};

char *tokenstrs[] = {
	[EOI]        = "EOI",
	[FOR]        = "FOR",
	[IF]         = "IF",
	[STRUCT]     = "STRUCT",
	[SIZEOF]     = "SIZEOF",
	[RETURN]     = "RETURN",

	[IMMEDIATE]	 = "IMMEDIATE",
	[IDENTIFIER] = "IDENTIFIER",
	[NEWLINE]    = "NEWLINE",
	[SEMICOLON]  = "SEMICOLON",
	[SPC]        = "SPC",
	[COLON]      = "COLON",
	[COMMA]      = "COMMA",
	[DOT]		 = "DOT",
	[LPAREN]     = "LPAREN",
	[RPAREN]     = "RPAREN",
	[LBRACKETS]  = "LBRACKETS",
	[RBRACKETS]  = "RBRACKETS",
	[LBRACES]    = "LBRACES",
	[RBRACES]    = "RBRACES",

	[MUL] = "MUL",
	[DIV] = "DIV",
	[MOD] = "MOD",
	[ADD] = "ADD",
	[SUB] = "SUB",
	[LSHIFT] = "LSHIFT",
	[RSHIFT] = "RSHIFT",
	[LT] = "LT",
	[GT] = "GT",
	[LE] = "LE",
	[GE] = "GE",
	[EQUAL] = "EQUAL",
	[NEQUAL] = "NEQUAL",
	[BAND] = "BAND",
	[BXOR] = "BXOR",
	[BOR] = "BOR",
	[LAND] = "LAND",
	[LOR] = "LOR",
	[BNOT] = "BNOT",
	[LNOT] = "LNOT",
	[ASSIGN] = "ASSIGN",
	[BAND_ASSIGN] = "BAND_ASSIGN",
	[BOR_ASSIGN] = "BOR_ASSIGN",
	[BXOR_ASSIGN] = "BXOR_ASSIGN",
	[BNOT_ASSIGN] = "BNOT_ASSIGN",
	[LAND_ASSIGN] = "LAND_ASSIGN",
	[LOR_ASSIGN] = "LOR_ASSIGN",
	[LSHIFT_ASSIGN] = "LSHIFT_ASSIGN",
	[RSHIFT_ASSIGN] = "RSHIFT_ASSIGN",
	[ADD_ASSIGN] = "ADD_ASSIGN",
	[SUB_ASSIGN] = "SUB_ASSIGN",
	[MUL_ASSIGN] = "MUL_ASSIGN",
	[DIV_ASSIGN] = "DIV_ASSIGN",
	[MOD_ASSIGN] = "MOD_ASSIGN",
	[AT] = "AT",

	[UNDEFINED]  = "UNDEFINED",
};

#ifdef DEBUG
int identsz = 0;
char* IDENTIFIER_STACK[1024] = {0};
int immedsz = 0;
int64_t IMMEDIATE_STACK[1024] = {0};
#endif // DEBUG

char*
strdup(const char *s)
{
	int len = strlen(s);
	char *d = calloc(len, sizeof(char));
	strcpy(d, s);
	return d;
}

void
forward(void)
{
	holdc = 0;
}

char
peekc(void)
{
	static char buf[BUFSZ];
	static char c;
	static int hd = -1;
	ssize_t sz;

	/* return the same char until forward() is not called */
	if (holdc) {
		if (c == 0) {
			/* fprintf(stderr, "peekc(): returning EOF\n"); */
			return EOF;
		}
		/* fprintf(stderr, "peekc(): hold: returning '%c'\n", c); */
		return c;
	}
	holdc = 1;

	if (hd < 0 || hd == BUFSZ) {
		hd = 0;
		sz = read(fd, buf, sizeof(buf));
		if (sz < 0) {
			perror("read");
			exit(1);
		}
		if (sz == 0) {
			/* fprintf(stderr, "peekc(): returning EOF\n"); */
			return EOF;
		}
	}
	c = buf[hd];

	if (c == 0) {
		/* fprintf(stderr, "peekc(): returning EOF\n"); */
		return EOF;
	}

	hd++;
	/* fprintf(stderr, "peekc(): returning '%c'\n", c); */
	return c;
}

void
printtok(FILE *o, Tok t)
{
	fprintf(o, "%s(%d)\n", tokenstrs[t.type], t.type);
}

#ifdef DEBUG
void
printident(FILE *o)
{
	for (int i = 0; i < identsz; i++) {
		fprintf(o, "%s\n", IDENTIFIER_STACK[i]);
	}
}

void
printimmed(FILE *o)
{
	for (int i = 0; i < immedsz; i++) {
		fprintf(o, "%ld\n", IMMEDIATE_STACK[i]);
	}
}
#endif // DEBUG

// TODO(all): find a way to save the IMMEDIATE.
void
peek_float(int64_t n)
{
	(void) n;
	int64_t val = 0;
	char c = peekc();
	while (c >= '0' && c <= '9') {
		forward();
		int ic = c - '0';
		val *= 10;
		val += ic;
		c = peekc();
	}
#ifdef DEBUG
	IMMEDIATE_STACK[immedsz++] = n;
#endif
}

void
peek_dec(int sign, char c)
{
	int64_t val = c - '0';
	c = peekc();
	while (c >= '0' && c <= '9') {
		forward();
		int ic = c - '0';
		val *= 10;
		val += ic;
		c = peekc();
	}
	val *= sign;

	if (c == '.') {
		forward();
		peek_float(val);
		return;
	}

#ifdef DEBUG
	IMMEDIATE_STACK[immedsz++] = val;
#endif // DEBUG
}

void
peek_bin()
{
	int64_t val = 0;
	char c = peekc();

	while (c == '0' || c == '1') {
		forward();
		int ic = c - '0';
		val <<= 1;
		val += ic;
		c = peekc();
	}

#ifdef DEBUG
	IMMEDIATE_STACK[immedsz++] = val;
#endif // DEBUG
}

void
peek_hex()
{
	int64_t val = 0;
	char c = peekc();

	while (1) {
		int ic;
		if ( c >= 'A' && c <= 'F') {
			ic = 10 + c - 'A';
		} else if (c >= 'a' && c <= 'f') {
			ic = 10 + c - 'a';
		} else if (c >= '0' && c <= '9') {
			ic = c - '0';
		} else {
			break;
		}
		forward();
		c = peekc();

		val <<= 4;
		val += ic;
	}

#ifdef DEBUG
	IMMEDIATE_STACK[immedsz++] = val;
#endif // DEBUG
}

void
peek_immediate(char c)
// => called only if peek() is in [0..9]
{
	// Can be hex bin or dec
	if (c == '0') {
		char c = peekc();

		if (c == 'X' || c == 'x') {
			forward();
			peek_hex();
			return;
		}

		if (c == 'B' || c == 'b') {
			forward();
			peek_bin();
			return;
		}
	}

	peek_dec(1, c);
}

void
peek_identifier(char *buf, char c)
{
	int len = 0;
	while (9) {
		buf[len++] = c;
		c = peekc();

		if (!(isalnum(c) || c == 'c')) {
			break;
		}
		forward();
	}
	buf[len++] = 0;
}

Tok
peek()
{
	static Tok tok;
	char buf[64];
	char c;

	if (hold) {
		return tok;
	}
	hold = 1;

	memset(&tok, 0, sizeof(tok));

	c = peekc();
	forward();

	switch (c) {
	case EOF:
		tok.type = EOI;
		break;
	case ' ': // fallthrough
	case '\t':
		tok.type = SPC;
		do {
			forward();
			c = peekc();
		} while (c == ' ' || c == '\t');
		break;
	case ';':
		tok.type = SEMICOLON;
		break;
	case ':':
		tok.type = COLON;
		break;
	case ',':
		tok.type = COMMA;
		break;
	case '(':
		tok.type = LPAREN;
		break;
	case ')':
		tok.type = RPAREN;
		break;
	case '[':
		tok.type = LBRACKETS;
		break;
	case ']':
		tok.type = RBRACKETS;
		break;
	case '{':
		tok.type = LBRACES;
		break;
	case '}':
		tok.type = RBRACES;
		break;
	case '.':
		c = peekc();
		if (c >= '0' && c <= '9') {
			peek_float(0);
			tok.type = IMMEDIATE;
		} else {
			tok.type = DOT;
		}
		break;
	case '<':
		c = peekc();
		if (c == '=') {
			forward();
			tok.type = LE;
		} else if (c == '<') {
			forward();
			c = peekc();
			if (c == '=') {
				forward();
				tok.type = LSHIFT_ASSIGN;
			} else {
				tok.type = LSHIFT;
			}
		} else {
			tok.type = LT;
		}
		break;
	case '>':
		c = peekc();
		if (c == '=') {
			forward();
			tok.type = GE;
		} else if (c == '>') {
			forward();
			c = peekc();
			if (c == '=') {
				forward();
				tok.type = RSHIFT_ASSIGN;
			} else {
				tok.type = RSHIFT;
			}
		} else {
			tok.type = GT;
		}
		break;
#define CASE2(_tok1, _tok2, _val1, _val2) \
	case _tok1:							\
		c = peekc();					\
		if (c == _tok2) {				\
			forward();					\
			tok.type = _val2;			\
		}								\
		else tok.type = _val1;			\
		break;

		CASE2('=', '=', ASSIGN, EQUAL)
		CASE2('!', '=', LNOT, NEQUAL)
		CASE2('+', '=', ADD, ADD_ASSIGN)
		CASE2('*', '=', MUL, MUL_ASSIGN)
		CASE2('/', '=', DIV, DIV_ASSIGN)
		CASE2('%', '=', MOD, MOD_ASSIGN)
		CASE2('^', '=', BXOR, BXOR_ASSIGN)
		CASE2('~', '=', BNOT, BNOT_ASSIGN)
#undef CASE2
	case '-':
		c = peekc();
		if (c == '=') {
			forward();
			tok.type = SUB_ASSIGN;
		} else if (c >= '0' && c <= '9') {
			forward();
			peek_dec(-1, c);
			tok.type = IMMEDIATE;
		} else {
			tok.type = SUB;
		}
		break;
	case '&':
		c = peekc();
		if (c == '=') {
			forward();
			tok.type = BAND_ASSIGN;
		} else if (c == '&') {
			forward();
			c = peekc();
			if (c == '=') {
				forward();
				tok.type = LAND_ASSIGN;
			} else {
				tok.type = LAND;
			}
		} else {
			tok.type = BAND;
		}
		break;
	case '|':
		c = peekc();
		if (c == '=') {
			forward();
			tok.type = BOR_ASSIGN;
		} else if (c == '|') {
			forward();
			c = peekc();
			if (c == '=') {
				forward();
				tok.type = LOR_ASSIGN;
			} else {
				tok.type = LOR;
			}
		} else {
			tok.type = BOR;
		}
		break;
	case '@':
		tok.type = AT;
		break;
	case '\n':
		tok.type = NEWLINE;
		break;
	default:
		// IMMEDIATE
		if (c >= '0' && c <= '9') {
			peek_immediate(c);
			tok.type = IMMEDIATE;
		} else {
			peek_identifier(buf, c);
			int i;
			for (i = NTOK + 1; i < NKEYWORDS; i++) {
				/* fprintf(stderr, "(i == %d) comparing '%s' to '%s'..\n", i, buf, keywords[i]); */
				if (strcmp(buf, keywords[i]) == 0) {
					tok.type = i;
					break;
				}
			}
			if (i == NKEYWORDS) {
				tok.type = IDENTIFIER;
#ifdef DEBUG
				IDENTIFIER_STACK[identsz++] = strdup(buf);
#endif // DEBUG
				// TODO(ghuter): save the buffer somewhere, link it to the token
			}
		}
		break;
	}
	return tok;
}

void
tok_forward(void)
{
	hold = 0;
}

Tok
getnext(void)
{
	/* fprintf(stderr, "gettin next token..\n"); */
	Tok t = peek();
	tok_forward();
	return t;
}

int
main(int argc, char *argv[])
{
	Tok t;
	do {
		t = getnext();
		printtok(stdout, t);
	} while (t.type != EOI);
#ifdef DEBUG
	printident(stdout);
	printimmed(stdout);
#endif // DEBUG
}

