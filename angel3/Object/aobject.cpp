#include "data.h"
#include "runtime.h"
#include "execute.h"
#include "compilerutil.h"
#include "shell.h"
#include "amem.h"

object initobject()
{
	object o = angel_alloc_block(APPLYSIZE(sizeof(objectnode)));
	o->type = OBJECT;
	o->mem_value = initdictionary();
	o->pri_mem_value = initdictionary();//这个当object销毁时再销毁之
	return o;
}


//建立两个变量来判断是否是中间变量
fun getclassfun(pclass c,char *name,uint16_t count)
{
	if(!c)
		return NULL;
	if(!c->static_f)
		return NULL;
	int ret = getfunoffset(c->static_f,c->static_f_map,name,count);
	if(ret == -1)
		return NULL;
	return c->static_f->fun_item[ret];
}
fun getobjmemfun(object o,char *name,int count)
{
	if(!o)
	{
		angel_error("对象未初始化！");
		return NULL;
	}
	else if(o->type==NU)  //表示为空对象
	{
		angel_error("空对象没有成员函数");
		return NULL;
	}
	else
	{
		pclass c = o->c;
		int ret = getfunoffset(c->mem_f,c->mem_f_map,name,count);
		if(ret == -1)
			return NULL;
		return c->mem_f->fun_item[ret];
	}
}
fun getmemfun(object o,char *name,uint16_t count)
{
	fun f;
	f=getobjmemfun(o,name,count);
	if(!f)
	{
		f=getclassfun(o->c,name,count);
		if(!f)
			return NULL;
	}
	return f;
}
object_entry getobjmember(object o,object_string name)
{
	char *p;
	if(!o->mem_value)
		return NULL;
	int index = getdictindex(o->mem_value,(object)name);
	object_entry *addr = o->mem_value->hashtable;
	return addr[index];
}
object_entry getclassmember(pclass c,object_string name)
{
	char *p;
	if(!c)
		return NULL;
	int index = getdictindex(c->static_value,(object)name);
	object_entry *addr = c->static_value->hashtable;
	return addr[index];
}
object_entry getmember(object o,object_string name)
{
	object_entry temp;
	pclass c=o->c;
	if(o->type==NU)
	{
		angel_error("空对象不能操作成员变量！");
		return NULL;
	}

	temp=getobjmember(o,name);
	if(!temp)
	{
		return getclassmember(c,name);
	}
	else
	{
		return temp;
	}
}
