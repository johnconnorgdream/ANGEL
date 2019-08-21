#ifndef compilerutil_def
#define compilerutil_def
#ifdef _cplusplus
extern "c"{
#endif



#include "data.h"
#include "runtime.h"
#include "aexpimp.h"

int getoffset(object_set map,char *name);
int addmap(object_set head,char *name,uint16_t offset);

object getnamefrompool(object_set set,char *s);
void init_const_pool();
void extent_const_pool();
void init_test_obj();
object checkobjbystr(object_set s,char *name);
object getconstbystr(char *s);
object getconstbyint(int64_t val);
object getconstbyfloat(double val);



int findpivot(_switch *a,int low,int high);
int quicksort(_switch *a,int low,int high);

int findpivot(_switch *a,int low,int high);
int quicksort(_switch *a,int low,int high);



object_list init_perpetual_list(int len=list_base_size);
object_set init_perpetual_set();
object_dict init_perpetual_dict();
object_entry init_perpetual_entry(object key,object value);
object_int init_perpetual_number(int64_t val = 0);


void free_perpetual(object o);

#ifdef _cplusplus
}
#endif
#endif