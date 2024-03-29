#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <ctype.h>
#include <assert.h>
#include <inttypes.h>
#include <stddef.h>

#include "fatarena.h"
#include "token.h"
#include "lib/map.h"
#include "lex.h"

#define BUFSZ 512
#define OK(_v) do {assert(_v);} while(0)
#define PGW(_gw) printf("{ addr: %p, remain: %zu, size: %zu}\n", _gw.addr, _gw.remain, _gw.size)


static int hold = 0;
static int holdc = 0;
static int fd = 0;

static Strimap *mpident = {0};

int
stri_insert(Str key)
{
	Strimap **m = &mpident;

	for (uint64_t h = strhash(key); *m; h <<= 2) {
		if (streq(key, (*m)->key)) {
			return ftidx(&ftident, (*m)->key.data);
		}
		m = &(*m)->child[h >> 62];
	}
	*m = (Strimap*)ftptr(&fttmp, ftalloc(&fttmp, sizeof(Strimap)));

	// Alloc a new string
	int addr = ftalloc(&ftident, key.len + 1);
	uint8_t* str = ftptr(&ftident, addr);

	memcpy(str, key.data, key.len);
	str[key.len] = '\0';
	key.data = str;

	(*m)->key = key;
	return addr;
}

static int
intsave(int64_t v)
{
	int64_t* d;
	int idx = ftalloc(&ftimmed, sizeof(int64_t));
	OK(idx >= 0 && "fail to allocate int64_t");
	OK(((d = (int64_t*) ftptr(&ftimmed, idx)) != NULL));
	*d = v;
	return idx;
}

static int
floatsave(double v)
{
	double* d;
	int idx = ftalloc(&ftimmed, sizeof(double));
	OK(idx >= 0 && "fail to allocate double");
	OK(((d = (double*) ftptr(&ftimmed, idx)) != NULL));
	*d = v;

	return idx;
}

static void
forward(void)
{
	holdc = 0;
}

static char
peekc(void)
{
	static char buf[BUFSZ];
	static char c;
	static int hd = -1;
	static ssize_t sz = BUFSZ;

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

	if (hd < 0 || hd == sz) {
		hd = 0;
		sz = read(fd, buf, sizeof(buf));
		if (sz < 0) {
			perror("read");
			exit(1);
		}
		if (sz == 0) {
			c = EOF;
			return c;
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
	static int prv = -1;
	if (t.type <= NKEYWORDS) {
		fprintf(o, "%s(%d)\n", tokenstrs[t.type], t.type);
	} else {
		switch (prv) {
		case IDENTIFIER:
			fprintf(o, "<%s>\n", (char*) ftptr(&ftident, t.type));
			break;
		case LITERAL:
			fprintf(o, "\"%s\"\n", (char*) ftptr(&ftlit, t.type));
			break;
		case INT:
			fprintf(o, "<%" PRId64 ">\n", (int64_t) * ((int64_t*) ftptr(&ftimmed, t.type)));
			break;
		case FLOAT:
			fprintf(o, "<%f>\n", (double ) * ((double*) ftptr(&ftimmed, t.type)));
			break;
		default:
			assert(0 && "Unreachable, wrong type.");
		}
	}
	prv = t.type;
}

static ETok
peek_float(int *d, int sign, int64_t n)
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

	double dval = (double) val;
	while (dval > 1.0) {
		dval *= 0.1;
	}

	dval += (double) n;
	dval *= sign;

	OK((*d = floatsave(dval)) >= 0);
	return FLOAT;
}

static ETok
peek_dec(int *d, int sign, char c)
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

	if (c == '.') {
		forward();
		return peek_float(d, sign, val);
	}

	val *= sign;
	OK((*d = intsave(val)) >= 0);
	return INT;
}

static ETok
peek_bin(int *d)
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

	OK((*d = intsave(val)) >= 0);
	return INT;
}

static ETok
peek_hex(int *d)
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

	OK((*d = intsave(val)) >= 0);
	return INT;
}

static ETok
peek_immediate(int *d, char c)
{
	// Can be hex bin or dec
	if (c == '0') {
		char c = peekc();

		if (c == 'X' || c == 'x') {
			forward();
			return peek_hex(d);
		}

		if (c == 'B' || c == 'b') {
			forward();
			return peek_bin(d);
		}
	}

	return peek_dec(d, 1, c);
}

static ETok
peek_literal(int *d)
{
	int start = ftalloc(&ftlit, sizeof(char));
	*d = start;
	uint8_t *addr = ftptr(&ftlit, start);

	char c = peekc();
	forward();
	while (c != '"') {
		OK(ftalloc(&ftlit, sizeof(char)) > 0);
		*addr = c;
		addr++;
		c = peekc();
		forward();
	}

	*addr = '\0';
	return LITERAL;
}

static int
peek_identifier(char *buf, char c)
{
	int len = 0;
	while (9) {
		buf[len++] = c;
		c = peekc();

		if (!(isalnum(c) || c == '_')) {
			break;
		}
		forward();
	}
	buf[len++] = 0;
	return len;
}

static Tok
peek(void)
{
	static Tok tok;
	static int idx = 0;
	// in case of INT, FLOAT, IDENTIFIER:
	if (idx != 0) {
		Tok t = {idx};
		idx = 0;
		return t;
	}

	char buf[64];
	char c;

	if (hold) {
		return tok;
	}
	hold = 1;

	memset(&tok, 0, sizeof(tok));

	c = peekc();
	forward();

	if (c == '/') {
		if (peekc() == '/') {
			do {
				forward();
				c = peekc();
			}  while (c != '\n' && c != EOF);
			forward();
		}
	}

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
	case '"':
		tok.type = peek_literal(&idx);
		break;
	case '.':
		c = peekc();
		if (c >= '0' && c <= '9') {
			tok.type = peek_float(&idx, 1, 0);
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
			tok.type = peek_dec(&idx, -1, c);
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
			tok.type = peek_immediate(&idx, c);
		} else {
			Str str = {0};
			str.len = peek_identifier(buf, c) - 1;
			str.data = (uint8_t*) buf;
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
				idx = stri_insert(str);
			}
		}
		break;
	}
	return tok;
}

static void
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

