#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include "lib/fatarena.h"
#include "lib/map.h"
#include "lib/token.h"

#include "lex.h"
#include "parser.h"
#include "analyzer.h"
#include "marshal.h"


void
marshal(int fd, Symbols modsym[NMODSYM])
{
	(void) fd;
	(void) modsym;
}

void
unmarshal(int fd, Symbols modsym[NMODSYM])
{
	(void) fd;
	(void) modsym;
}
