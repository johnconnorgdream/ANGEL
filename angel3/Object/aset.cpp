#include "angel.h"
#include "execute.h"
#include "lib.h"
#include "hash.h"


object_set initset(int count)
{
	object_set res = (object_set)angel_alloc_block(SETBASESIZE);
	res->type = SET;
	res->alloc_size = count;
	res->element = (object *)angel_alloc_page((object)res,count*sizeof(object));
	angel_sys_memset(res->element,0,count*sizeof(object));
	res->len = 0;
	return res;
}
void resizeset(object_set os)
{
	int len = os->alloc_size;
	os->alloc_size *= 2;
	object *newaddr = (object *)angel_alloc_page((object)os,os->alloc_size*sizeof(object));
	object *addr = os->element;
	angel_sys_memset(newaddr,0,os->alloc_size*sizeof(object));
	os->element = newaddr;
	os->len = 0;
	//重分布
	for(int i = 0; i<len; i++)
	{
		object o = addr[i];
		if(o)
		{
			_addset(os,o);
		}
	}
	angel_free_page(addr);
}




long getsetindex(object_set s,object o)
{
	long index,i,hash;
	hash=globalhash(o);
	char *str = GETSTR(o)->s;
	index=hash%s->alloc_size; 
	i=index;
	while(s->element[i]) 
	{
		object test=s->element[i];
		if(test->type == o->type)  //类型相同
		{
			if(globalhash(test) == globalhash(o))
				return i;
		}
		i=(i+1)%s->alloc_size; 
	}
	return i;
}
void _addset(object_set s,object o)
{
	long hash;
	if(s->len >= s->alloc_size*2/3)
		resizeset(s);
	long index=getsetindex(s,o);
	//然后判断键是否重复,判断方法是：
	s->element[index] = o;
	s->len++;
}
void addset(object_set s,object o)
{
	long hash;
	if(s->len >= s->alloc_size*2/3)
		resizeset(s);
	long index=getsetindex(s,o);
	//然后判断键是否重复,判断方法是：
	if(!s->element[index])
		s->len++;
	assign_execute(s->element,index,o);
}
int unionset(object_set s,object_set s1)
{
	for(int i=0; i<s1->alloc_size; i++)
	{
		object test = s1->element[i];
		if(test)
			addset(s,test);
	}
	return 1;
}
object_set concatset(object_set s,object_set s1)
{
	object_set res = copyset(s);
	unionset(res,s1);
	return res;
}
object_set copyset(object_set os)
{
	object_set res=initset(os->alloc_size);
	unionset(res,os);
	return  res;
}
void translisttoset(object_set set,object_list l)
{
	for(int i = 0; i < l->len; i++)
	{
		object item = l->item[i];
		addset(set,item);
	}
}
void transrangetoset(object_set set,object_range r)
{
	for(int i = 0; i < r->n; i++)
	{
		int in = r->begin + i * r->step;
		addset(set, (object)initinteger(in));
	}
}

/*
集合库
*/
object syssize_set(object o)
{
	return (object)initinteger(GETSET(o)->len);
}
object sysadd_set(object item,object o)
{
	addset(GETSET(o),item);
	return GETTRUE;
}
object sysisexist_set(object item,object o)
{
	int index = getsetindex(GETSET(o),item);
	if(GETSET(o)->element[index])
		return GETTRUE;
	else
		return GETFALSE;
}
object sysremove_set(object item,object o)
{
	int index = getsetindex(GETSET(o),item);
	if(GETSET(o)->element[index])
	{
		GETSET(o)->element[index] = NULL;
		GETSET(o)->len--;
		return GETTRUE;
	}
	else
	{
		return GETFALSE;
	}
}
