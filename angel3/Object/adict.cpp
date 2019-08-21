#include <memory.h>
#include "data.h"
#include "execute.h"
#include "lib.h"
#include "hash.h"
#include "amem.h"


object_dict initdictionary(int count)
{
	object_dict res = (object_dict)angel_alloc_block(SETBASESIZE);
	res->type = DICT;
	res->alloc_size = count;
	res->hashtable = (object_entry *)angel_alloc_page((object)res,count * sizeof(object_entry));
	memset(res->hashtable,0,count*sizeof(object));
	res->len = 0;

	return res;
}
void resizedict(object_dict od)
{
	int len = od->alloc_size;
	od->alloc_size *= 2;
	object_entry *newaddr = (object_entry *)angel_alloc_page((object)od,od->alloc_size*sizeof(object));
	object_entry *addr = od->hashtable;
	memset(newaddr,0,od->alloc_size*sizeof(object));
	od->hashtable = newaddr;
	od->len = 0;
	//重分布
	for(int i = 0; i<len; i++)
	{
		object_entry o = addr[i];
		if(o)
		{
			_adddict(od,o);
		}
	}
	angel_free_page(addr);
}




/*
字典的操作
*/
long getdictindex(object_dict dict,object key)
{
	long index,i;
	long hash=globalhash(key);
	char *s = GETSTR(key)->s;
	index=(unsigned long)hash%dict->alloc_size; 
	i=index;
	while(dict->hashtable[i]) 
	{
		object test=dict->hashtable[i]->key;
		if(test->type == key->type)  //类型相同
		{
			if(globalhash(test) == globalhash(key))
				return i;
		}
		i=(i+1)%dict->alloc_size; 
	}
	return i;
}
int _adddict(object_dict dict,object_entry entry)
{
	long index;
	if(!entry)
		return 1;
	if(dict->len > dict->alloc_size*2/3)
		resizedict(dict);
	index=getdictindex(dict,entry->key);
	//然后判断键是否重复,判断方法是：
	dict->hashtable[index] = entry;
	dict->len++;
}
int adddict(object_dict dict,object_entry entry)
{
	long index;
	if(!entry)
		return 1;
	if(dict->len > dict->alloc_size*2/3)
		resizedict(dict);
	index=getdictindex(dict,entry->key);
	//然后判断键是否重复,判断方法是：
	assign_execute((object *)dict->hashtable,index,(object)entry);
	dict->len++;
}
int adddict(object_dict dict,object key,object value)
{
	int index;
	object_entry entry;
	if(dict->len > dict->alloc_size*2/3)
		resizedict(dict);
	index=getdictindex(dict,key);
	entry = dict->hashtable[index];
	if(!entry)
	{
		entry = initentry(key,value);
		dict->len++;
		dict->hashtable[index] = entry;
	}
	else
	{
		_assign_execute_ref(&entry->value,value);
	}
	return 1;
}
int _adddict(object_dict dict,object key,object value)
{
	int res = adddict(dict,key,value);
	DECREF(key);
	DECREF(value);
	return res;
}
object_dict copydict(object_dict d)
{
	long i;
	object_dict res=initdictionary(d->alloc_size);
	for(i=0; i<d->alloc_size; i++)
	{
		object_entry entry=d->hashtable[i];
		if(entry)
		{
			object_entry temp = initentry(entry->key,entry->value);
			res->hashtable[i] = temp;
		}
	}
	res->len = d->len;
	return res;
}


/*
字典库
*/
object syssize_dict(object o)
{
	return (object)initinteger(GETDICT(o)->len);
}
object sysiskey_dict(object key,object o)
{
	int index = getdictindex(GETDICT(o),key);
	return GETDICT(o)->hashtable[index]?GETTRUE : GETFALSE;
}
object syskeys_dict(object o)
{
	object_set keys = initset();
	for(int i = 0; i < GETDICT(o)->alloc_size; i++)
	{
		object_entry entry = GETDICT(o)->hashtable[i];
		if(entry)
		{
			addset(keys,entry->key);
		}
	}
	return (object)keys;
}