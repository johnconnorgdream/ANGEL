#ifndef string_def
#define string_def
#include "arange.h"
#include "aexpimp.h"
#include "typeconfig.h"
#ifdef _cplusplus
extern "c"{
#endif

typedef struct object_stringnode{
	BASEDEF
	char *s;
	unsigned long len,hash,extra;  //这个参数在表示字符串数据时为字符串的长度，在用作映射表时作为偏移量
}*object_string;

object_string initstring(int len);
object_string initstring(char *c);
object_string initstring_n(char *c);
object_string concatstr(object_string str,object_string s1);
object_string insertstr(object_string str,object_string s1,int pos);
object_string copystring(object_string s);
object_string slicestring(object_string s,object_range range);
void joinstring(object_string ret,object join);
object_string copystring_str(char *s,int len);
object_string strrepeat(object_string s,int count);
int comparestring(object_string s1,object_string s2);


/*
库函数定义
*/
object syssize_string(object o);
object sysjoin_string(object join,object o);
object sysupper_string(object o);
object syslower_string(object o);
object sysfind_string(object pattern,object range,object o);
object sysfindall_string(object pattern,object range,object o);
object sysmatch_string(object pattern,object range,object o);


#ifdef _cplusplus
}
#endif
#endif