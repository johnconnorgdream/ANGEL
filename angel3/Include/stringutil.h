#ifndef stringutil_def
#define stringutil_def
#ifdef _cplusplus
extern "c"{
#endif



void calcnext(int16_t *pattern,int *next);
object strfind(wchar * s,wchar *pattern,int begin,int end,int patternlen);
object strfindall(wchar * s,wchar *pattern,int begin,int end,int patternlen);



#ifdef _cplusplus
}
#endif
#endif