#ifndef shell_def
#define shell_def
#ifdef _cplusplus
extern "c"{
#endif



#include <stdio.h>
#include "data.h"
#include "aexpimp.h"

char *read_line(FILE *f);
FILE *getangelfile();

ANGEL_API void shell(int argc,char **argv);
void print(object o);
void _print(object o);
void angel_error(char *errorinfo);
void angel_out(char *out,FILE *f = stdout);
void angel_outc(char c,FILE *f=NULL);
object_ext getcurrentthread();



#ifdef _cplusplus
}
#endif
#endif