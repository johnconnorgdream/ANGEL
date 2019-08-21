#include "data.h"
#include "amem.h"
#include "runtime.h"
#include "execute.h"


object_int initinteger(int64_t val)
{
	object_int  i = (object_int)angel_alloc_block(NUMSIZE);
	i->val = val;
	i->type = INT;
	return i;
}
object _stacktoheap(object i)
{
	object res;  //Ŀǰʱ�����ģ���Ϊ���ǵ�δ����
	if(!ISHEAPINSTACK(i)) {
		return i;
	}
	if(i->type == STR)  //��ʾ��ʱ�ǳ���256���ַ�
	{
		res = (object)initstring(2);
		*((int16_t *)GETSTR(res)->s) = *((int16_t *)GETSTR(i)->s);
	}
	else
	{
		res = angel_alloc_block(NUMSIZE);
		GETINT(res) = GETINT(i);
	}
	res->type = i->type;
	res->flag = FLAG_HEAPED;
	return res;
}
object stacktoheap(object i)  //������ֵ��1
{
	if(!i) return NULL;
	if(!ISHEAPINSTACK(i)) {
		ASCREF(i);
		return i;
	}
	return _stacktoheap(i);
}
object_float initfloat(float64_t d)
{
	object_float  f = (object_float)angel_alloc_block(FLOATSIZE);
	f->val = d;
	f->type = FLOAT;
	return f;
}