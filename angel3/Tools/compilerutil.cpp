#include <string.h>
#include <stdlib.h>
#include "util.h"
#include "data.h"
#include "shell.h"
#include "compilerutil.h"
#include "runtime.h"
//目前所有的池查询操作已经完全划归到set操作中，剩下的就是常量的存储方式。是malloc还是本系统的

object_set string_pool,num_pool;
object_string test_for_string;
object_int test_for_int;
object_float test_for_float;
object_string getsharedstring(char *s);
object checkelement(object_set s,object o)
{
	int index = getsetindex(s,o);
	object test = s->element[index];
	return test;
}
void set_test_forstr(char *s)
{
	test_for_string->s = s;
	test_for_string->len = strlen(s);
}
void init_test_obj()
{
	//利用test目的是为了兼容set和dict的方法，即用object的形式传入
	test_for_string = getsharedstring("");
	test_for_string->flag = FLAG_HASH_TEST;
	test_for_int = (object_int)malloc(sizeof(object_intnode));
	test_for_int->type = INT;
	test_for_float = (object_float)malloc(sizeof(object_floatnode));
	test_for_float->type = FLOAT;
}
void init_const_pool()
{
	string_pool = init_perpetual_set();
	num_pool = init_perpetual_set();
}
void extent_const_pool()
{
	resizeset(string_pool);  //这个让
}
object_string getsharedstring(char *s)
{
	object_string res = (object_string)calloc(1,sizeof(object_stringnode));
	res->type = STR;
	res->s = s;
	res->len = strlen(s);
	res->refcount = 2;
	res->flag = FLAG_CONST;
	return res;
}
object_string getsharedstring_wide(char *s)  //这个用在需要表示大范围的情况下
{
	object_string ret = getsharedstring(s);
	char *res;
	int len = ret->len;
	if(*s)
		res = towide(s,&len);
	else
		res = (char *)calloc(2,sizeof(char));
	if(len == -1)
	{
		angel_error("编码失败！");
		return NULL;
	}
	ret->flag = FLAG_POOL;
	ret->s = res;
	ret->len = len;
	//free(s);
	return ret;
}
object_int getsharednum(long val)
{
	object_int res = (object_int)calloc(1,sizeof(object_intnode));
	res->type = INT;
	res->val =val;
	return res;
}
object_list init_perpetual_list(int len)
{
	object_list res = initarray(len);
	res->refcount = 2;
	res->flag = FLAG_CONST;
	return res;
}
object_int init_perpetual_number(int64_t val)
{
	object_int res = initinteger(val);
	res->refcount = 2;
	res->flag = FLAG_CONST;
	return res;
}
object_set init_perpetual_set()
{
	object_set res = initset();
	res->refcount = 2;
	res->flag = FLAG_CONST;
	return res;
}
object_dict init_perpetual_dict()
{
	object_dict res = initdictionary();
	res->refcount = 2;
	res->flag = FLAG_CONST;
	return res;
}
object_entry init_perpetual_entry(object key,object value)
{
	object_entry res = initentry(key,value);
	res->refcount = 2;
	res->flag = FLAG_CONST;
	return res;
}
object checkobjbystr(object_set s,char *name)
{
	set_test_forstr(name);
	return checkelement(s,(object)test_for_string);
}
object getnamefrompool(object_set set,char *s)
{
	set_test_forstr(s);
	object o = checkelement(set,(object)test_for_string);
	if(o)
	{
		free(s);
		return o;
	}
	else
	{
		object_string str = getsharedstring(s);
		addset(set,(object)str);
		return (object)str;
	}
}
object getconstbystr(char *s)
{
	object_string str = getsharedstring_wide(s);
	object o = checkelement(string_pool,(object)str);
	if(o)
	{
		free(str->s);
		free(str);
		return o;
	}
	else
	{
		addset(string_pool,(object)str);
		return (object)str;
	}
}
object getconstbyint(int64_t val)
{
	test_for_int->val = val;
	object o = checkelement(num_pool,(object)test_for_int);
	if(o) return o;
	else
	{
		object_int i = (object_int)calloc(1,sizeof(object_intnode)+2);
		i->type = INT;
		i->refcount = 0;
		i->val = val;
		i->osize = 0;
		i->flag = FLAG_POOL;
		addset(num_pool,(object)i);
		return (object)i;
	}
}
object getconstbyfloat(double val)
{
	test_for_float->val = val;
	object o = checkelement(num_pool,(object)test_for_float);
	if(o) return o;
	else
	{
		object_float f = (object_float)calloc(1,sizeof(object_floatnode)+2);
		f->type = FLOAT;
		f->val = val;
		f->osize = 0;
		f->flag = FLAG_POOL;
		f->refcount = 0;
		addset(num_pool,(object)f);
		return (object)f;
	}
}
int getoffset(object_set map,char *name)
{
	if(!map)
		return -1;
	set_test_forstr(name);
	object check = (object)checkelement(map,(object)test_for_string);
	object_string testcheck = GETSTR(check);
	if(check)
	{
		return GETSTR(check)->extra;
	}
	return -1;  //表示没有成功获得变量。
}
int addmap(object_set head,char *name,uint16_t offset)
{
	set_test_forstr(name);
	if(checkelement(head,(object)test_for_string))  //表示此时冲突
		return 0;
	//这里与name_pool相关的东西已经在gettoken中解决了。
	object_string map = getsharedstring(name);
	map->extra= offset;
	addset(head,(object)map);
	return 1;
}