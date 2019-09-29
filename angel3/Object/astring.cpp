#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include "data.h"
#include "lib.h"
#include "execute.h"
#include "stringutil.h"
#include "shell.h"
#include "amem.h"
#include "util.h"
/*

字符串的相关操作

*/
#define STRCPY(s1,s2,len) memcpy(s1,s2,len); (s1)[len] = 0; (s1)[len+1] = 0;


object_string initchar(uint16_t c)
{
	//object_string _char= (object_string )angel_alloc_page(cellhead,SETBASESIZE);
	//_char-
	return NULL;
}
object_string initstring(int len)
{
	//这是一个底层的内核函数，len代表的是字节数而非字符串的字符数
	object_string res;
	char *str;
	if(len < PAGEALLOCMIN)
	{
		res = (object_string)angel_alloc_block(STRBASESIZE+len+2);
		str = (char *)res+STRBASESIZE;
	}
	else
	{
		res = (object_string)angel_alloc_block(STRBASESIZE);
		str = (char *)angel_alloc_page((object)res,len+2);
	}
	res->s = str;
	res->type = STR;
	res->len  = len;
	return res;
}
object_string initstring_n(char *c)   //对本地字符串的一种包装
{
	int i,len=strlen(c);
	object_string s;
	char *res;
	if(*c)
		res = towide_n(c,&len);
	else
		res = (char *)calloc(2,sizeof(char));
	if(len == -1)
	{
		angel_error("编码失败！");
		return NULL;
	}
	s=initstring(len);
	memcpy(s->s,res,len);
	*(int16_t *)(s->s+len) = 0;
	free(res);
	//free(c);
	return s;
}
object_string initstring(char *c)
{
	int i,len=strlen(c);
	object_string s;
	char *res;
	if(*c)
		res = towide(c,&len);
	else
		res = (char *)calloc(2,sizeof(char));
	if(len == -1)
	{
		angel_error("编码失败！");
		return NULL;
	}
	s=initstring(len);
	memcpy(s->s,res,len);
	*(int16_t *)(s->s+len) = 0;

	free(res);
	//free(c);
	return s;
}





int comparestring(object_string s1,object_string s2)
{
	int cplen = s1->len<s2->len?s1->len:s2->len,flag=0;
	int longcplen = cplen/sizeof(long);
	long *s1_long = (long *)s1->s;
	long *s2_long = (long *)s2->s;
	for(int i=0; i<longcplen; i++)
	{
		if(s1_long[i] == s2_long[i])
			continue ;
		else if(s1_long[i] > s2_long[i])
		{
			flag = 1;
			break ;
		}
		else
		{
			flag = -1;
			break ;
		}
	}
	switch(flag)
	{
	case 1:
		return 1;
	case -1:
		return -1;
	case 0:
		for(int i=longcplen*sizeof(long); i<cplen; i++)
		{
			if(s1->s[i] == s2->s[i])
				continue ;
			else if(s1->s[i] > s2->s[i])
				return 1;
			else
				return -1;

		}
		if(s1->len == s2->len)
			return 0;
		else if(s1->len>s2->len)
			return 1;
		else
			return -1;
	}
}
object_string concatstr(object_string str,object_string s1)
{
	object_string res = initstring(GETSTR(str)->len+GETSTR(s1)->len);
	STRCPY(res->s,str->s,str->len);
	STRCPY(res->s+str->len,s1->s,s1->len);
	return res;
}
object_string insertstr(object_string str,object_string s1,int pos)
{
	char *s=s1->s;
	int i,j,lentemp=s1->len;
	int copynum;
	object_string res = initstring(GETSTR(str)->len+GETSTR(s1)->len);
	STRCPY(res->s,str->s,pos+1);
	STRCPY(res->s+pos+1,s1->s,s1->len);
	STRCPY(res->s+pos+1+s1->len,str->s+pos+1,str->len-pos-1);
	return res;
}
object_string strrepeat(object_string s,int count)
{
	int i,copynum;
	int eachlen = s->len;
	object_string res = initstring(s->len*count);
	for(i=0; i<count; i++)
	{
		memcpy(res->s+eachlen*i,s->s,eachlen);
	}
	*(uint16_t *)&res->s[s->len*count] = 0;
	return res;
}
object_string copystring_str(char *s,int len)
{
	object_string res = initstring(len);
	STRCPY(res->s,s,len);
	return res;
}
object_string copystring(object_string s)
{
	object_string res = initstring(s->len);
	STRCPY(res->s,s->s,s->len);
	return res;
}
object_string slicestring(object_string s,object_range range)
{
	int e,b,n,step;
	RANGETOSLICE(range,s->len/2-1,step,b,e,n);
	int byteslen = range->end - range->begin + 1;
	object_string res = initstring(n*2);
	wchar *in = (wchar *)res->s;
	wchar *out = (wchar *)s->s;
	for(int i = 0; i < n; i++)
	{
		int index = b + i*step;
		*in++ = out[index];
	}
	*in++ = 0;
	return res;
}
void joinstring(object_string ret,object join)  //这里要保证空间足够
{
	char *fill_ptr = ret->s+ret->len;
	for(int i = 0; i < GETLIST(join)->len; i++)
	{
		object item = GETLIST(join)->item[i];
		int eachlen = 0;
		if(ISSTR(item))
		{
			memcpy(fill_ptr,GETSTR(item)->s,GETSTR(item)->len);
			eachlen = GETSTR(item)->len;
			fill_ptr += GETSTR(item)->len;
		}
		else
		{
			int num ;
			char *wide = towide(tointchar(GETINT(item)),&num);
			memcpy(fill_ptr,wide,num);
			eachlen = num;
			fill_ptr += num;
		}
		ret->len += eachlen;
	}
	*fill_ptr++ = 0;
	*fill_ptr = 0;
}

/*
字符串库
*/
object syssize_string(object o)
{
	return (object)initinteger(GETSTR(o)->len/2);
}
object sysjoin_string(object join,object o)
{
	MEM_ARG_CHECK(o,join,LIST,"join",1);
	int allocsize = GETSTR(o)->len;
	for(int i = 0; i<GETLIST(join)->len; i++)
	{
		object item = GETLIST(join)->item[i];
		if(ISSTR(item))
		{
			allocsize += GETSTR(item)->len;
		}
		else if(ISINT(item))
		{
			allocsize += digits(GETINT(item))*2;
		}
		else
		{
			angel_error("字符串join方法的列表元素必须是整数或者字符串！");
			return GETNULL;
		}
	}
	object_string ret = initstring(allocsize);
	char *fill_ptr = ret->s;
	memcpy(fill_ptr,GETSTR(o)->s,GETSTR(o)->len);
	ret->len = GETSTR(o)->len;
	joinstring(ret,join);
	return (object)ret;
}
object sysupper_string(object o)
{
	int diff = 'A'-'a';
	int16_t *s = (int16_t *)GETSTR(o)->s;
	object_string res = initstring(GETSTR(o)->len);
	int16_t *p = (int16_t *)res->s;
	while(*s)
	{
		if(*s <= 'z' && *s >= 'a')
			*p++ = *s++ + diff;
		else
			*p++ = *s++;
	}
	*p = 0;
	return (object)res;
}
object syslower_string(object o)
{
	int diff = 'a'-'A';
	int16_t *s = (int16_t *)GETSTR(o)->s;
	object_string res = initstring(GETSTR(o)->len);
	int16_t *p = (int16_t *)res->s;
	while(*s)
	{
		if(*s <= 'Z' && *s >= 'A')
			*p++ = *s++ + diff;
		else
			*p++ = *s++;
	}
	*p = 0;
	return (object)res;
}
object sysfind_string(object pattern,object range,object o)
{
	object regular = checkpatternparam(pattern);
	if(!regular)
	{
		angel_error("字符串find方法第一个参数必须是字符串或者正则类型！");
		return GETNULL;
	}
	int res[2];
	if(!checkrangeparam(range,GETSTR(o)->len/2-1,res))
	{
		angel_error("字符串find方法第二个参数必须是range类型！");
		return GETNULL;
	}
	object ret;
	if(ISREGULAR(regular))  //表示此时是正则表达式
	{
		ret = reg_find(GETREGULAR(regular),(wchar *)GETSTR(o)->s,res[0],res[1]);
		if(!ISREGULAR(pattern)) {
			DECREF(regular);
		}
	}
	else
	{
		ret = strfind((wchar *)GETSTR(o)->s,(wchar *)GETSTR(pattern)->s,
			res[0],res[1],GETSTR(pattern)->len/2);
	}
	return ret;
}
object sysfindall_string(object pattern,object range,object o)
{
	object regular = checkpatternparam(pattern);
	if(!regular)
	{
		angel_error("字符串findall方法第一个参数必须是字符串或者正则类型！");
		return GETNULL;
	}
	int res[2];
	if(!checkrangeparam(range,GETSTR(o)->len/2-1,res))
	{
		angel_error("字符串findall方法第二个参数必须是range类型！");
		return GETNULL;
	}
	object ret;
	if(ISREGULAR(regular))  //表示此时是正则表达式
	{
		ret = reg_findall(GETREGULAR(regular),(wchar *)GETSTR(o)->s,res[0],res[1]);
		if(!ISREGULAR(pattern)) {
			DECREF(regular);
		}
	}
	else
	{
		ret = strfindall((wchar *)GETSTR(o)->s,(wchar *)GETSTR(pattern)->s,
			res[0],res[1],GETSTR(pattern)->len/2);
	}
	return ret;
}
object sysmatch_string(object pattern,object range,object o)
{
	object regular = checkpatternparam(pattern);
	if(!regular || ISSTR(regular))
	{
		angel_error("字符串match方法第一个参数必须是正则类型！");
		return GETNULL;
	}
	int res[2];
	if(!checkrangeparam(range,GETSTR(o)->len/2-1,res))
	{
		angel_error("字符串match方法第二个参数必须是range类型！");
		return GETNULL;
	}
	object ret = (object)initinteger(reg_match(GETREGULAR(regular),(wchar *)GETSTR(o)->s,
		res[0],res[1]));
	if(!ISREGULAR(pattern)) {
		DECREF(regular);
	}
	return ret;
}
