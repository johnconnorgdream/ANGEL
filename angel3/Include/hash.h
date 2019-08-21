#ifndef hash_def
#define hash_def
#ifdef _cplusplus
extern "c"{
#endif



#include "data.h"
#include "runtime.h"
inline long stringhash(char *str,int length)
{
	unsigned int hash = 0;
	int i = 0;
    while (i < length)
    {
        hash = (*str++) + (hash << 6) + (hash << 16) - hash;
		i ++;
    }
 
    return (hash & 0x7FFFFFFF);
}
inline long floathash(double d)
{
	return long(d);
}
inline long globalhash(object o)
{
	char *temp;
	unsigned long res,res1;
	switch(o->type)
	{
	case INT:
		return GETINT(o);
	case FLOAT:
		return floathash(GETFLOAT(o));
	case STR:
		//GETSTR(o)->hash = res;
		res = GETSTR(o)->hash;
		if(ISHASHTEST(o))
		{
			res = stringhash(GETSTR(o)->s,GETSTR(o)->len);
			return res;
		}
		if(!res)
		{
			res = stringhash(GETSTR(o)->s,GETSTR(o)->len);
			GETSTR(o)->hash = res;
		}
		return res;
	default :
		return (int)o;
	}
}



#ifdef _cplusplus
}
#endif
#endif