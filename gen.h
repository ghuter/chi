#ifndef GEN_H
#define GEN_H

/*
  generates C0 code into `code`
  returns the number of bytes written
*/
size_t gen(char *code, Symbols typesym, Symbols identsym, Symbols signatures, Symbols funsym);

#endif