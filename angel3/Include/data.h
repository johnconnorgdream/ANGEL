#ifndef data_def
#define data_def
#ifdef _cplusplus
extern "c"{
#endif



#include "parse.h"
#include "typeconfig.h"


#define avl_bit_count  8
#define runtime_max_size 65536  //所有的变量数目不能超过runtime_max_size*2,其中一部分要用于全局变量的定义和使用
#define list_base_size  8   //以乘2的速度增加
#define string_base_size 32
//主要是为了拦截由于angel.c包含的头文件引发的冲突
//进一步优化对象的内存结构，将对象中的成分全变为对象，这样有利于内存管理
#ifndef base_def
#define base_def
//这里flag的高四位表示对象的额外类型属性。
//低四位表示的是对象基本属性（比如对象生成方式heap还是xxx）
#define _BASE unsigned char type,extra_flag,osize,flag;int refcount;
#define BASEDEF _BASE
#endif






typedef struct collectionnode{
	void **element;
	int size,alloc;
}*collection;
/*
各种数据类型的头文件，object也包括在内

*/
#include "aobject.h"
#include "a_re.h"
#include "astring.h"
#include "alist.h"
#include "aset.h"
#include "anumber.h"
#include "abytes.h"
#include "adict.h"
#include "arange.h"
#include "afunction.h"
/*

heap-stack
*/
typedef struct object_iteratornode{
	BASEDEF;
	object base;
	int pointer;
}*object_iterator;
//本质上就是等差数列
typedef struct ext_typenode{
	char *type_name;
	void (* dealloc)(object o);
	void (* deep_flag)(object o);
	void (* deep_recovery)(object o);
}*ext_type;
#define EXT_HEAD  \
	BASEDEF; \
	ext_type _type;
typedef struct object_extnode{
	EXT_HEAD
}*object_ext;

object_iterator inititerator(object base);
object_range initrange(long b,long e,long step = 1);


#define GETTRUE angel_true
#define GETFALSE angel_false
#define GETNULL angel_null
#define ISTRUE(o) (o == angel_true)
#define ISFALSE(o) (o == angel_false)


#define GETINT(o) ((object_int)o)->val
#define GETFLOAT(o) ((object_float)o)->val
#define TRANSFLOAT(o) ((double)GETINT(o))
#define GETSTR(o) ((object_string)o)
#define GETLIST(o) ((object_list)o)
#define GETDICT(o) ((object_dict)o)
#define GETSET(o) ((object_set)o)
#define GETENTRY(o) ((object_entry)o)
#define GETFUN(o) ((object_fun)o)
#define GETBYTES(o) ((object_bytes)o)
#define GETRANGE(o) ((object_range)o)
#define GETSLICE(o) ((object_slice)o)
#define GETITER(o)  ((object_iterator)o)
#define GETREGULAR(o) ((object_regular)o)
#define GETEXT(o) ((object_ext)o)

#define ISINT(o) (o->type==INT)
#define ISFLOAT(o) (o->type==FLOAT)
#define ISNUM(o) (ISINT(o) || ISFLOAT(o))
#define ISSTR(o) (o->type==STR)
#define ISBYTES(o) (o->type==BYTES)
#define ISLIST(o) (o->type==LIST)
#define ISDICT(o) (o->type==DICT)
#define ISSET(o) (o->type==SET)
#define ISNU(o) (o->type==NU)
#define ISBOOLEAN(o) (o->type == BOOLEAN)
#define ISRANGE(o) (o->type == RANGE)
#define ISREGULAR(o) (o->type == REGULAR)
#define ISNOTYPE(o) (o->type == NOTYPE)
#define ISOBJECT(o) (o->type == OBJECT)
#define ISFUNP(o) (o->type == FUNP)

#define NUMNAME "number"
#define INTNAME "integer"
#define FLOATNAME "float"
#define BYTESNAME "bytes"
#define STRNAME "string"
#define LISTNAME "array"
#define DICTNAME "dictionary"
#define SETNAME "set"
#define NUNAME "null"
#define RANGENAME "range"
#define REGULARNAME "regular"
#define BOOLEANNAME "boolean"
#define OBJECTNAME "object"
#define FUNPNAME "function"

inline char * gettypedesc(object o)
{
	switch(o->type)
	{
	case INT:
		return "integer";
	case FLOAT:
		return "float";
	case BYTES:
		return "bytes";
	case STR:
		return "string";
	case ENTRY:
		return "entry";
	case BOOLEAN:
		return "boolean";
	case LIST:
		return "array";
	case SET:
		return "set";
	case DICT:
		return "dictionary";
	case OBJECT:
		return "object";
	case EXT_TYPE:
		return "extense_type";
	case FUNP:
		return "function";
	default:
		return "unknow";
	}
}

#ifdef _cplusplus
}
#endif
#endif

