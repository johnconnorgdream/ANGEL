#ifndef util_def
#define util_def
#ifdef _cplusplus
extern "c"{
#endif




#include "data.h"

int encode_format(char *buffer,char **decode_entry);
int64_t digitstimes(int64_t l);
int digits(int64_t l);
int64_t toint(char *a);
double todecimal(char *a);
double tofloat(char *a);
char* tointchar(int64_t a);



char *getstrcat(char *des,char *src);
char *tonative(object_string s);
char *tomult(object_string s,int *size = NULL);
char *towide_n(char *s,int *reslen);
char *towide(char *s,int *reslen);


int UnicodeToUtf8(char* pInput, char *pOutput);
int Utf8ToUnicode(char* pInput, char* pOutput);
int GbkToUnicode(char *gbk_buf, uint16_t *unicode_buf,int);
int UnicodeToGbk(uint16_t *unicode_buf, char *gbk_buf,int);






#ifdef _cplusplus
}
#endif
#endif