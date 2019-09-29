
#include <stdlib.h>
#include <memory.h>
#include "parse.h"
#include "data.h"
#include "runtime.h"
#include "compilerutil.h"
#include "lib.h"
#include "execute.h"
#include "shell.h"
#include "amem.h"

//����ȫ�ֵı����ͺ���ջ
extern funlist global_function;
extern object_list global_value_list,obj_list;
extern object_set global_function_map,class_map;
extern temp_alloc_info angel_temp;
extern classlist clist;
extern funlist global_function;














void *popcollection(collection c)
{
	if(c->size == 0)
		return 0;
	return c->element[--c->size];
}
collection initcollection(int count)
{
	collection c=(collection)malloc(sizeof(collectionnode));
	c->element = (void **)calloc(count,sizeof(void *));
	c->size = 0;
	c->alloc = count;
	return c;
}
void addcollection(collection base,void *el)
{
	if(base->size >= base->alloc)
	{
		base->alloc *= 2;
		base->element=(void **)realloc(base->element,
			base->alloc*sizeof(void *));
		memset(base->element+base->size,0,(base->alloc-base->size)*sizeof(void *));
	}
	base->element[base->size++]=el;
}
void clearcollection(collection c)
{
	for(int i = 0; i < c->alloc; i++)
	{
		free(c->element[i]);
	}
	free(c->element);
	free(c);
}

linkcollection initlink()
{
	linkcollection ret = (linkcollection)malloc(sizeof(linkcollectionnode));
	ret->data = NULL;
	ret->next = ret->pre = ret;
	return ret;
}
void addlink(linkcollection head,linkcollection res)
{
	linkcollection temp = head->next;
	res->next = temp;
	temp->pre = res;
	head->next = res;
	res->pre = head;
}
void deletelink(linkcollection node)
{
	linkcollection pre = node->pre;
	if(pre == node)
		return ;
	pre->next = node->next;
	node->next->pre = pre;
	free(node);
}
void clearlink(linkcollection head)
{
	linkcollection p = head;
	while(p)
	{
		linkcollection temp = p;
		p = p->next;
		free(temp);
	}
}





indexlist initindexlist()
{
	indexlist il=(indexlist)malloc(sizeof(indexlistnode));
	il->item=(uint16_t *)calloc(5,sizeof(uint16_t));
	il->alloc=5;
	il->len=0;
	return il;
}
void addindexlist(indexlist base,uint16_t s)
{
	if(base->len>=base->alloc)
	{
		base->alloc+=5;
		base->item=(uint16_t *)realloc(base->item,base->alloc*sizeof(uint16_t));
	}
	base->item[base->len++]=s;
}

funlist initfunlist()
{
	funlist fl=(funlist)malloc(sizeof(funlistnode));
	fl->alloc_size=10;  //��ʼ��С
	fl->len=0;
	fl->fun_item=(fun *)calloc(10,sizeof(fun));
	return fl;
}
void _addfun(funlist fl,fun f)
{
	if(fl->len>=fl->alloc_size)
	{
		fl->alloc_size*=2;
		//���ﻹ��Ҫ�ж��ڴ���������
		fl->fun_item=(fun *)realloc(fl->fun_item,fl->alloc_size*sizeof(fun));
	}
	fl->fun_item[fl->len++]=f;
}
int addfun(funlist fl,object_set fmap,fun f)
{
	if(fl==global_function) //ֻ���ڶ���ȫ�ֺ�����ʱ����ж���������Ƿ���ϵͳ����
		if(issysfun(f))
		{
			char errorinfo[errorsize];
			sprintf(errorinfo,"����%s��ϵͳ���������������壡",f->name);
			angel_error(errorinfo);
			return 0;
		}
	fun p;
	//�ж��Ƿ��ظ�����
	int funheadindex = getoffset(fmap,f->name);
	if(funheadindex != -1)
	{
		fun overhead = fl->fun_item[funheadindex];
		for(fun p = overhead; p; p=p->overload)
		{
			//�����ֽ���
			int smallest1 = p->paracount - p->default_paracount,smallest2 = f->paracount - f->default_paracount,
				largeest1 = p->paracount,largeest2 = f->paracount;
			if((smallest1 - largeest2)*(largeest1 - smallest2) <= 0)  //ʵ�ֺ�������
			{
				char error[errorsize];
				sprintf(error,"����%s�����ظ����壡\n",f->name);
				angel_error(error);
				return 0;
			}
		}
		//�����ʾ�����غ��������
		f->overload = overhead->overload;
		overhead->overload = f;
		return 1;
	}
	//����û�����ص����
	addmap(fmap,f->name,fl->len);
	_addfun(fl,f);
	return 1;
}
int getfunoffset(funlist head,object_set fmap,char *funname,int count)
{
	fun p;
	int funheadindex = getoffset(fmap,funname);
	if(funheadindex == -1)
		return -1;
	if(count == -1)
	{
		return head->fun_item[funheadindex]->index;
	}
	for(fun p=head->fun_item[funheadindex]; p; p=p->overload)
	{
		if(isparamvalid(p->paracount,p->default_paracount,count))
			return p->index;
	}
	return -1;
}
fun getfun(funlist head,object_set fmap,char *funname,int count)
{
	int ret=getfunoffset(head,fmap,funname,count);
	if(ret == -1)
	{
		char errorinfo[errorsize];
		sprintf(errorinfo,"����%sû�����û��ƥ��Ĳ�������",funname);
		angel_error(errorinfo);
		return NULL;
	}
	return head->fun_item[ret];
}
fun getglobalfun(char *funname,int count)
{
	return getfun(global_function,global_function_map,funname,count);
}

int getclassoffset(char *classname)
{
	return getoffset(class_map,classname);
}


env initenv()
{
	env el=(env)malloc(sizeof(envnode));
	env_elenode *emem = (env_elenode *)malloc(sizeof(env_elenode)*runtime_max_size);
	for(int i=0; i<runtime_max_size; i++)
	{
		el->env_item[i]=&emem[i];
	}
	el->len=0;  
	return el;
}
void freeenv(env el)
{
	free(el->env_item[0]);
	free(el);
}
/*

����ʱջ
*/
int counttempvar_infun(funlist fl)
{
	int total = 0;
	for(int i = 0; i<fl->len; i++)
	{
		fun temp = fl->fun_item[i];
		total += temp->temp_var_num;
	}
	return total;
}
int counttempvar()
{
	int total = 0;
	int i;
	total += angel_temp->inuse+counttempvar_infun(global_function)+1;
	for(i =0 ; i<clist->len; i++)
	{
		total += counttempvar_infun(clist->c[i]->mem_f);
		total += counttempvar_infun(clist->c[i]->static_f);
	}
	return total;
}

runtime_stack initruntime(int stacksize)
{
	unsigned long i;
	runtime_stack rs=(runtime_stack)malloc(sizeof(runtime_stacknode));
	rs->stack_size = stacksize;

	int total_alloc = rs->stack_size*sizeof(object)+rs->stack_size*stack_heap_size;
	rs->data = (object *)malloc(total_alloc);

	//��ջ�����ݳ�ʼ��Ϊangel_uninitial
	memset(rs->data,0,total_alloc);
	for(i = 0; i < stacksize; i++)
		rs->data[i] = angel_uninitial;

	void *base = rs->data+rs->stack_size;
	rs->stack_heap_base = base;
	rs->push_pos = angel_temp->inuse;
	rs->top = rs->push_pos;
	//stack�ײ��Ĵ洢�����Ļ���
	for(i = 0; i<rs->stack_size; i++)
	{
		object_int temp = GETSTACKHEAPASINT(base,i);
		temp->flag = FLAG_STACKHEAP;
		temp->osize = stack_heap_size;
	}
	return rs;
}



/*
�б���Ķ�̬���ӣ�
*/

pclass initclass(char *name)
{
	pclass c=(pclass)malloc(sizeof(classnode));
	memset(c,0,sizeof(classnode));
	c->name=name;
	c->mem_f=initfunlist();
	c->static_f=initfunlist();
	c->static_value = init_perpetual_dict();
	c->mem_f_map = init_perpetual_set();
	c->static_f_map = init_perpetual_set();
	c->entry_token = maketoken(0,NULL);
	return c;
}
void delete_class()
{
	uint16_t i;
	for(i=0; i<clist->len; i++)
	{
		pclass p=clist->c[i];
		free_perpetual((object)p->mem_f_map);
		free_perpetual((object)p->static_f_map);
		free_perpetual((object)p->static_value);
		free(p);
	}
}



/*
��̬�ֽ���洢�Ľṹ
*/
bytecode initbytearray()
{
	bytecode head=(bytecode)malloc(sizeof(bytecodenode));
	memset(head,0,sizeof(bytecodenode));
	head->alloc_size=bytecode_base_size;
	head->code=(char *)calloc(head->alloc_size,sizeof(char));  //��ʼ����
	return head;
}
bytecode resize(bytecode bc)
{
	char *oldvalue=bc->code;
	bc->alloc_size*=2;   //����ռ��С���������̡�
	bc->code=(char *)calloc(bc->alloc_size,sizeof(char));
	memcpy(bc->code,oldvalue,bc->len*sizeof(char));
	free(oldvalue);
	oldvalue=NULL;
	return bc;
}
int oldcode;
void addbyte(bytecode bc,uchar c)
{
	if(bc->len>=bc->alloc_size)   //��������
	{
		//���·���ռ�
		if(bc->len<runtime_max_size*1024)
			bc=resize(bc);
		else
			angel_error("ָ����Ŀ���");
	}
	bc->code[bc->len++]=c;
	oldcode = bc->len;
}
void insertbyte(bytecode bc,uchar c,int offset)
{
	bc->code[bc->len-offset-1]=c;
}
void copybyte(bytecode bc1,bytecode bc2)
{
	unsigned long i;
	for(i=0; i<bc2->len; i++)
		bc1->code[bc1->len++]=bc2->code[i];
}
void addtwobyte(bytecode bc,uint16_t c)
{
	char *p=(char *)&c;
	addbyte(bc,p[0]);  //���λ
	addbyte(bc,p[1]);  //���λ
}
void addfourbyte(bytecode bc,unsigned long c)
{
	uint16_t *p=(uint16_t *)&c;
	
	addtwobyte(bc,p[0]);
	addtwobyte(bc,p[1]);
}
void freebytecode(bytecode bc)
{
	free(bc->code);
	free(bc);
}
