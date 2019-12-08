#include <stdlib.h>
#include "data.h"
#include "execute.h"
#include "lib.h"
#include "amem.h"
#include "shell.h"
#include "aenv.h"




object_list initarray(int count)
{
	object_list res = (object_list)angel_alloc_block(SETBASESIZE);
	res->type = LIST;
	res->alloc_size = count;
	res->item = (object *)angel_alloc_page((object)res,count * sizeof(object));
	angel_sys_memset(res->item,0,res->alloc_size*sizeof(object));
	res->len = 0;
	return res;
}

void expendlistsize(object_list ol,int resizelen)
{
	int len = ol->alloc_size;
	object *newaddr = (object *)angel_alloc_page((object)ol,resizelen*sizeof(object)); //ע��������ܻ���gc֮��ı�ԭ��addr��ָ��
	object *addr = ol->item;
	angel_sys_memset(newaddr+len,0,(resizelen-len)*sizeof(object));
	ol->alloc_size = resizelen;
	angel_sys_memcpy(newaddr,addr,len*sizeof(object));
	angel_free_page(addr);
	ol->item = newaddr;
}



/*
�б�Ļ�������
*/
int _addlist(object_list l,object item)  //�������item�Ѿ���ȷ��alloc�õ��ģ�Ϊ�˱�֤�̰߳�ȫ��ֱ�Ӹ�ֵ����Ҫref++
{
	if(l->len>=l->alloc_size)
		expendlistsize(l,2*l->alloc_size);
	l->item[l->len++] = item;
	return 1;
}
int addlist(object_list l,object item)  //��ʱitem����alloc�õ��ģ�����Ҫ����stackheap�ڴ�
{
	object *old = l->item;
	if(l->len>=l->alloc_size)
		expendlistsize(l,2*l->alloc_size);
	assign_execute(l->item,l->len++,item);
	//l->item[l->len++] = item;
	return 1;
}
void clearlist(object_list l)  //��ʱitem����alloc�õ��ģ�����Ҫ����stackheap�ڴ�
{
	while(l->len != 0)
	{
		object o = l->item[--l->len];
		DECREF(o);
	}
}
int appendlist(object_list l,object_list m)
{
	int llen = l->len,mlen = m->len;
	if(l->alloc_size < llen + mlen)
		expendlistsize(l,(llen>mlen?llen:mlen)*2);
	for(int i = llen; i<mlen+llen; i++)
		assign_execute(l->item,i,m->item[i-llen]);
	l->len += m->len;
	return 1;
}
object_list concatlist(object_list l,object_list m)
{
	object_list res = initarray(l->len+m->len);
	appendlist(res,l);
	appendlist(res,m);
	return res;
}
int insertlist(object_list l,object_list m,int pos)
{
	int llen = l->len,mlen = m->len;
	if(l->alloc_size < llen + mlen)
		expendlistsize(l,(llen>mlen?llen:mlen)*2);
	angel_sys_memcpy(l->item+pos+mlen+1,l->item+pos,mlen*sizeof(object));
	for(int i = pos; i<mlen+pos; i++)
		assign_execute(l->item,i,m->item[i-pos]);
	l->len += mlen;
	return 1;
}
object_list listrepeat(object_list l,int count)
{
	int eachlen=l->len;
	object_list res = initarray(l->len*count);
	for(int i=0; i<count; i++)
		appendlist(res,l);
	return res;
}
object_list copylist(object_list l)
{
	object_list res=initarray(l->alloc_size);
	appendlist(res,l);
	return  res;
}
object_list slicelist(object_list l,object_range range)
{
	object *out = l->item;
	int e,b,n,step;
	RANGETOSLICE(range,l->len-1,step,b,e,n);
	object_list res = initarray(n);
	for(int i = 0; i < n; i++)
	{
		int index = b + i*step;
		addlist(res,l->item[index]);
	}
	return res;
}
void storeslicelist(object_list l,object_range range,object_list m)
{
	int e,b,n,step;
	RANGETOSLICE(range,l->len-1,step,b,e,n);
	for(int i = 0; i < n; i++)
	{
		int index = b + i * step;
		object temp = m->item[i];
		assign_execute(l->item,index,temp);
	}
}
int storeslicelist_asslice(object_list l,object_range targetrange,object_slice slice)
{
	object_range range = slice->range;
	object base = slice->base;
	if(!ISLIST(base))
	{
		angel_error("��Ƭ��ֵ��������ͬ���ͣ�");
		return 0;
	}
	
	int e1,e2,b1,b2,n,n1,n2,step1,step2;
	object_list m = GETLIST(base);
	RANGETOSLICE(targetrange,l->len-1,step1,b1,e1,n1);
	RANGETOSLICE(range,m->len-1,step2,b2,e2,n2);


	n = getmin(n1, n2);
	
	int scan;
	if(m != l)
	{
		for(int i = 0; i < n; i++)
		{
			int in = b1 + i * step1;
			int out = b2 + i * step2;
			object temp = m->item[out];
			assign_execute(l->item, in, temp);
		}
	}
	else
	{
		if(step1 * step2 > 0 && step1 == step2)  //ͬ�����Ҳ������
		{
			e1 = b1 + n * step1;
			e2 = b2 + n * step2;
			int delta = b2 - b1;
			if(b1 > b2)  //��Ҫ�������Ҹ���
			{
				int scan = b1;
				if(b1 > e1) //��ʱ��b��e��
				{
					scan = e1;
					step1 = -step1;
				}
				for(int i = n-1; i >= 0; i--)
				{
					int in = scan + i * step1;
					int out = in + delta;
					object temp = m->item[out];
					assign_execute(l->item,in,temp);
				}
			}
			else
			{
				int scan = e1;
				if(e1 < b1) //��ʱ��b��e��
				{
					scan = b1;
					step1 = -step1;
				}
				for(int i = 0; i < n; i++)
				{
					int in = b1 + i * step1;
					int out = in + delta;
					object temp = m->item[out];
					assign_execute(l->item, in, temp);
				}
			}
		}
		else
		{
			//��Ҫ���û�����
			object *cache = (object *)angel_sys_calloc(sizeof(object),n);
			for (int i = 0; i < n; i++) {
				int out = b2 + i * step2;
				cache[i] = GETLIST(base)->item[out];		
			}
			for (int j = 0; j < n; j++) {
				int in = b1 + j * step1;
				object temp = cache[j];
				assign_execute(l->item, in, temp);
			}
			free(cache);
		}
	}
	return 1;
}

/*
�����
*/
object syssize_list(object o)
{
	return (object)initinteger(GETLIST(o)->len);
}
object sysadd_list(object item,object o)
{
	//��������Ҫ����stacktoheap���
	object_list otemp = GETLIST(item);
	STACKTOHEAP(item);
	//�������������
	_addlist(GETLIST(o),item);
	return GETTRUE;
}
object sysextend_list(object extend,object o)
{
	if(!ISLIST(extend))
	{
		angel_error("list��Աextend�����Ĳ�������Ϊ�б����ͣ�");
		return GETFALSE;
	}
	appendlist(GETLIST(o),GETLIST(extend));
	return GETTRUE;
}
object syspop_list(object o)
{
	object item = (object)GETLIST(o)->item[--GETLIST(o)->len];
	DECREF(item);
	return item;
}