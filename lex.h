#ifndef LEX_H
#define LEX_H

#define POK(_v, _msg) if (!(_v)) {perror(_msg); exit(1);}

extern FatArena ftident;
extern FatArena ftimmed;
extern FatArena ftlit;
extern FatArena fttmp;

Tok	getnext(void);
void printtok(FILE *o, Tok t);
int stri_insert(Str key);

#endif /* LEX_H */
