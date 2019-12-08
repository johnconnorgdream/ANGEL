#define AMEM_MOUDLE

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include "amem.h"
#include "execute.h"
#include "runtime.h"
#include "compilerutil.h"
#include "shell.h"

/*


���һ���ڲ���������������Ҫ�������ͺ꣬ʵ��������������ӵ�dec_element�У���deep_flag��deep_recovery_angel��ʵ��������
angel����Ĭ������utf-8����ʾ
towide��ת��Ϊͬ��Ŀ��ַ�unicode��tomult��ת��Ϊͳһ��utf-8��Ȼ���������ʱ��ת��Ϊ���ر��롣
�̶��ڴ棨���󣩺Ϳɱ��ڴ棨�ַ����ͼ������������ݶ����Ǿ�����Ҫ�����ͷŵģ��Ĺ���
Ϊ��Ч�ʡ�ȫ�ֱ����;ֲ�������ֱ�����б�object_list���Ͷ����ӳ���(map���ͣ���ֻ�Ǽ򵥵�����ӳ��)��
һ����
�ڴ����
������ڴ������ÿ�������ķ�ʽ���״���Ӧ�㷨������Ϊ����Ĵ�С��Ϊ�̶��Ҵ�С������100B�����ÿ����б��Ϊ���㣨blockģʽ����
�ַ����ͼ��ϵ��ڴ����������ڴ治ͬ�������ǿ������б�����ģ�����ֱ���ƶ�ָ�룬����ʱֱ�Ӻϲ�
������Կ��ǽ������ڴ�ĳ�������һ���Ժ�Ͳ���ɨ���ˣ�pageģʽ����
����ʱpage����block�ڴ涼������field�ṹ���С�

����GC:
GC���������ü����Ϳɴ��Է�����ϵķ��������ڶ���ͼ������͵ĸ�ֵ�������ü����ķ��������ǵ��ɴ��Է���ɨ�����̫��
�ֲ�������ȫ�ֱ���ʹ�ÿɴ��Է���������
���ж���GC��gc_block��Ϊ�˼������Ƿ����ѭ�����ã������ڶ���ͼ������ͣ�����ϵͳ����һ�ֿɴ��Է�����Ķ����Ϊ
�����ö����������������������Ϊѭ�����ö�����Ҫ���գ�����ʱ������������angel_alloc_block����Ķ���û���ü���
�����������á�
�������ж�ѭ�����ò��ܼ򵥵����ɴ��Է�����������������ȱ����ҵ�����ѭ�����ã���detect_loop_reference������LOOP_CHECK_FLAG��IS_LOOPED��ǣ�
����callable�������Ը��ݶ����Ƿ���ΪIS_LOOPED�ж϶����Ƿ���Ի��ա�
detect_loop_reference����֮��������ж϶�����IS_LOOPED���������������г��˱��ɴ��Է�����ǵĶ�����IS_LOOPED
������жϵĶ���û�б��ΪIS_LOOPED��Ҫ���м����ʱ���LOOP_CHECK_FLAG��Ϊ����״̬���ο�recovery_check_flag_to_normal��

GC���ԣ�
blockģʽ�µ�gc��pageģʽ�µ�gc�����в�ͬ��
block_gc��������Χgc����ͨgc��ֻ��������field����gc������gc��Ŀǰ��ÿ5������field(����feild�Ĵ���������gc_block����)��ÿ�δ�field_head��ʼɨ�裩
ȫ��gc��ȫ��gc��ͬ������gc�����Ƕ����еĶ������ɴ��Է��������ѭ�����õļ�⣩��gc�������ÿ�������
page_gc��������Χ��gc����ͨgc��ֻ��������field����gc��������gc��Ŀǰ��ÿ4������fieldһ�Σ�����ͬ�ϣ�
gc���������ڴ��ƶ��������Ƕ�field�ڲ����ڴ��ƶ���field֮��û���ڴ��ƶ�������ÿ������gc�Ժ����һ���ڴ���ƶ���global_merge��
����ڴ���ƶ�������Ҫ�������field
δ���Ķ����Ա�����������ֵ�洢���������set���ʹ�ã�ͬʱ���ֽ����д洢�������棬��ֹÿ�ζ�Ҫ��set����������


cycle GC���ڵĺ�����
cycle���ܻ�����ϵͳ���ٶ�

*/

static angel_memry angel_object_heap,angel_data_heap;
static collection angel_traversal_stack;
static block free_block;
object_string charset[cellnum];  //������Ӣ�ĵ��ַ��غ��ֽڳ�
object_bytes byteset[cellnum];
object angel_true,angel_false,angel_null,angel_uninitial;
extern linkcollection angel_stack_list;
extern object_list global_value_list,obj_list;
extern classlist clist;


#define MEM_LOCK fast_lock();
#define MEM_UNLOCK fast_unlock();

field init_block_field();
void block_gc();
void page_gc();
void page_extend_gc();


angel_memry get_object_heap()
{
	return angel_object_heap;
}
angel_memry get_data_heap()
{
	return angel_data_heap;
}
void *angel_sys_malloc(int size)
{
	void *ret = malloc(size);
	if(ret == NULL){
		angel_error("�ڴ������");
		exit(0);
	}
	return ret;
}
void *angel_sys_memset(void* addr,int val,int size)
{
	void *ret = memset(addr,val,size);
	if(ret == NULL){
		angel_error("�ڴ������");
		exit(0);
	}
	return ret;
}
void *angel_sys_memcpy(void *target,void *source,int size)
{
	return memcpy(target,source,size);
}
void *angel_sys_calloc(int count,int size)
{
	return calloc(count,size);
}


#define ADDFREELIST_O(_free) _free->next = free_block->next; \
							free_block->next = _free; \
							angel_object_heap->free_size += _free->block_size;
#define TEST_O(o) ((object)o)


void addextendfield(angel_memry am,field f,int allocsize)
{
	f->next = am->extend_head->next;
	am->extend_head->next = f;
	am->extend_size += allocsize;
	am->total_size += allocsize;
}
void addfield(angel_memry am,field f)
{
	am->field_current->next = f;
	am->field_current = f;
	am->field_count++;
	am->total_size += max_block_size;
}
/*
��ʼ�����ڴ�
*/
void lock_const_sector()
{
	;
}
void add_free_block(field f)
{
	block init = (block)f->memry;
	angel_sys_memset(init,0,max_block_size);
	init->block_size = max_block_size;
	init->next = free_block->next;
	free_block->next = init; //�⽫ǰ��Ļ����յĽ������̭������ĩβ��
}
field init_block_field()
{
	field f = (field)angel_sys_calloc(1,sizeof(fieldnode));
	char *addr = (char *)angel_sys_malloc(max_block_size);
	f->memry = addr;
	add_free_block(f);
	return f;
}
void init_page_field_info(field f,int allocsize=max_page_size)
{
	void *addr = f->memry;
	angel_sys_memset(addr,0,allocsize);
	f->free_size = allocsize;
	f->alloc_ptr = 0;
	page p = (page)addr;
	p->page_size = allocsize;
	p->ref = NULL;
}
field init_page_field(int allocsize = max_page_size)
{
	field f = (field)angel_sys_calloc(1,sizeof(fieldnode));
	char *addr = (char *)angel_sys_malloc(allocsize);
	f->memry = addr;
	init_page_field_info(f,allocsize);
	return f;
}
angel_memry init_block_heap()
{
	angel_memry am = (angel_memry)angel_sys_calloc(1,sizeof(angel_memrynode));
	am->total_size += max_block_size;
	am->field_head = am->field_current = init_block_field();
	am->field_count = 1;
	return am;
}
angel_memry init_page_heap()
{
	angel_memry am = (angel_memry)angel_sys_calloc(1,sizeof(angel_memrynode));
	am->total_size += max_page_size;
	am->field_head = am->field_current = init_page_field();
	am->extend_head = (field)angel_sys_calloc(1,sizeof(fieldnode));
	am->field_count = 1;
	return am;
}
object getpuppet()
{
	object res = (object)angel_sys_malloc(sizeof(int)*2);
	res->type = INT;
	res->refcount = 2;
	return res;
}
void init_block_const()
{
	angel_null = (object)init_perpetual_number();
	angel_null->type = NU;
	angel_true = (object)init_perpetual_number();
	angel_true->type = BOOLEAN;
	angel_false = (object)init_perpetual_number();
	angel_false->type = BOOLEAN;
	angel_uninitial = (object)init_perpetual_number();
	angel_uninitial->type = NOTYPE;
}
void init_page_const()
{
	object cellhead = getpuppet();
	object_stringnode *head = (object_stringnode *)angel_alloc_page(cellhead,(STRBASESIZE+4)*cellnum);
	for(int i = 0; i < cellnum; i++)
	{
		object_string item = &head[i];
		item->type = STR;
		item->refcount = 2;
		item->flag = FLAG_POOL;
		int16_t *content = (int16_t *)((char *)item+STRBASESIZE);
		content[0] = i;
		content[1] = 0;
		item->s = (char *)content;
		item->len = 2;
		charset[i] = item;
	}
	object_bytesnode *headb = (object_bytesnode *)angel_alloc_page(cellhead,(STRBASESIZE+4)*cellnum);
	for(int i = 0; i < cellnum; i++)
	{
		object_bytes item = &headb[i];
		item->type = BYTES;
		item->refcount = 2;
		item->flag = FLAG_POOL;
		char *content = ((char *)item+STRBASESIZE);
		content[0] = i;
		item->bytes = (char *)content;
		item->len = 1;
		byteset[i] = item;
	}
}
void init_heap()
{
	free_block = (block)angel_sys_calloc(1,sizeof(blocknode));
	angel_object_heap = init_block_heap();
	angel_data_heap = init_page_heap();
	angel_traversal_stack = initcollection();
}
void free_field(field f)
{
	free(f->memry);
	free(f);
}
void reset_object_heap()
{
	free_block->next = NULL;
	field left = angel_object_heap->field_head;
	field p = left->next;
	left->next = NULL;
	while(p)
	{
		field temp = p;
		p = p->next;
		free_field(temp);
	}
	add_free_block(left);

	angel_object_heap->free_size = 0;
	angel_object_heap->total_size = max_block_size;
	
	angel_object_heap->field_current = angel_object_heap->field_head;
	angel_object_heap->field_count = 1;

	init_block_const();
}
void reset_data_heap()
{
	field left = angel_data_heap->field_head,p;
	field base = angel_data_heap->extend_head;
	p = base->next;
	while(p)
	{
		field temp = p;
		p = p->next;
		base->next = temp->next;
		free_field(temp);
	}
	angel_data_heap->extend_head->next = NULL;

	p = left->next;
	left->next = NULL;
	while(p)
	{
		field temp = p;
		p = p->next;
		free_field(temp);
	}
	init_page_field_info(left);
	
	angel_data_heap->free_size = 0;
	angel_data_heap->total_size = max_page_size;

	angel_data_heap->field_current = angel_data_heap->field_head;
	angel_data_heap->field_count = 1;


	init_page_const();
}
void reset_heap()
{
	reset_object_heap();
	reset_data_heap();
}

void angel_init(object o)
{
	angel_sys_memset(o+1,0,sizeof(object)-4);
}



/*
�ڴ���������ͷ�
*/
 
object angel_alloc_block(int tsize)  //��ȡ���еĶ����С����16�ֽ����ڣ���Ȼ�ַ������б����
{
	object o;
	block p,pre;
	MEM_LOCK
alloc:
	for(pre=free_block,p=pre->next; p; pre=p,p=p->next)
	{
		//���ڴ�����һ�ο��ܺ�ʱ�ϳ�
		int left = p->block_size-tsize;
		block old;
		if(left >= 0)  
		{
			o = (object)((char *)p + left);
			if(left < MINSIZE)
			{
				pre->next = p->next;
				*(char *)p = left; //�ܴ󵨵ĳ��ԣ���Ҫ��һ��Ҫ���ֽڱ������Χ��
			}
			else
				p->block_size = left;
			*(int *)o = 0;
			o->type = DRIFT_SIZE;
			o->refcount = 1;  //ע�����￪ʼ��ʼ��Ϊ1
			o->osize = tsize;

			MEM_UNLOCK
			return o;
		}
	}
	//������ʱû�з���Ҫ���
	block_gc();
	goto alloc;
}
void* angel_alloc_page(object head,int len)								
{
#define HUGE_SIZE 5
	page res;
	int allocsize = len + data_head;
	MEM_LOCK
	if(allocsize > max_page_size/2)  //�޴���ڴ�����
	{
		if(angel_data_heap->extend_size > max_page_size*5)
			page_extend_gc(); 
		field f = init_page_field(allocsize);
		res = (page)f->memry;
		addextendfield(angel_data_heap,f,allocsize);
		res->ref = head;
		
		MEM_UNLOCK
		return (void *)(res+1);
	}
alloc:
	for(field f = angel_data_heap->field_head; f; f = f->next)
	{
		if(f->flag == 1) continue ;
		if(f->free_size > allocsize) //��һ�ι���
		{
			//����������Ǵ�ǰ���ĵ�
			res = (page)((char *)f->memry+f->alloc_ptr);
			res->page_size = allocsize;
			res->ref = head;
			f->alloc_ptr += allocsize;
			f->free_size -= allocsize;
			
			MEM_UNLOCK
			return (void *)(res+1);
		}
	}
	//��ʼ��������
	page_gc();
	goto alloc;
}




void sys_realloc_list(object_list l,int resize)
{
	void *addr = l->item;
	angel_free_page(addr);
	l->item = (object *)angel_sys_malloc(sizeof(object)*resize);
}




/*
��������
*/

__forceinline void dec_element(object p,int mode = 0)
{
	//�κ�һ������������gc�����Ƕ�Ӧ�õ���dec_element
	//ע�Ȿ�ڴ�ϵͳ�����ü����Ӽ�����շ�������ͬ�׶���
#define DEEP_DECREF(o) do{ \
			if(mode != 1){ \
				if(0 == --(o)->refcount) dec_element((object)o);  \
			}\
		}while(0);
#define DECFIX(addr) ((page)(addr)-1)->ref = NULL;
	if(!p)return ;
	char *test = (char *)p;
	fun fp;
	object_ext ext;
	if(!ISDECED(p))
		p->extra_flag = IS_DECED;
	else
		return ;
	char *ss;
	switch(TEST_O(p)->type)
	{
	case NOTYPE:
	case INT:
	case FLOAT:
	case NU:
	case TRUE:
	case FALSE:
	case BOOLEAN:
		return ;
	case FUNP:
		if(ISUSERFUN(GETFUN(p)))
		{
			fp = GETFUN(p)->funinfo.user;
			if(fp->name) //������˵���ǹ�������
				return ;
			clearlink(fp->current_scope);
			deletetoken(fp->grammar);
			clearlink(fp->local_scope);
			free(fp);
		}
		return ;
	case REGULAR:
		free(GETREGULAR(p)->repeat_predict_set->element);
		free(GETREGULAR(p)->repeat_predict_set);
		freebytecode(GETREGULAR(p)->code);
		free(GETREGULAR(p)->group);
		free(GETREGULAR(p)->or_jump_set);
		free(GETREGULAR(p)->match_record);
		free(GETREGULAR(p)->repeat_for_duplicate_record);
		clearcollection(GETREGULAR(p)->match_state);
		return ;
	case ITERATOR:
		DEEP_DECREF(GETITER(p)->base);
		return ;
	case SLICE:
		DEEP_DECREF(GETSLICE(p)->base);
		DEEP_DECREF(GETSLICE(p)->range);
		return ;
	case ENTRY:
		DEEP_DECREF(GETENTRY(p)->key);
		DEEP_DECREF(GETENTRY(p)->value);
		return ;
	case STR:
		ss = GETSTR(p)->s;
		if(GETSTR(p)->len >= PAGEALLOCMIN)
		{
			DECFIX(GETSTR(p)->s);
		}
		return ;
	case LIST:
		if(mode != 1)
		{
			for(int i = 0; i<GETLIST(p)->len; i++)
			{
				object item = GETLIST(p)->item[i];
				if(item)
				{
					DEEP_DECREF(item);
				}
			}
		}
		DECFIX(GETLIST(p)->item);
		return ;
	case SET:
		if(mode != 1)
		{
			for(int i = 0; i<GETSET(p)->alloc_size; i++)
			{
				object item = GETSET(p)->element[i];
				if(item)
				{
					DEEP_DECREF(item);
				}
			}
		}
		DECFIX(GETSET(p)->element);
		return ;
	case DICT:
		if(mode != 1)
		{
			for(int i = 0; i<GETDICT(p)->alloc_size; i++)
			{
				object_entry entry = GETDICT(p)->hashtable[i];
				if(entry)
				{
					DEEP_DECREF(entry);
				}
			}
		}
		DECFIX(GETDICT(p)->hashtable);
		return ;
	case OBJECT:
		DEEP_DECREF((object)p->mem_value);
		DEEP_DECREF((object)p->pri_mem_value);
		return ;
	case EXT_TYPE:
		ext = (object_ext)p;
		if(ext->_type->dealloc)
			ext->_type->dealloc(p);
		return ;
	default:
		//���������չ�����������������
		return ;
	}
}
void traversal_root(void (*deal)(object o,int mode),int mode = 0)
{
	int i;
	for(i = global_value_list->alloc_size; i<global_value_list->len; i++){
		deal(global_value_list->item[i],mode);
	}
	for(linkcollection p = angel_stack_list->next; p != angel_stack_list; p = p->next)
	{
		runtime_stack stack = ((runable)p->data)->_stack;
		if(!stack) continue;
		for(int j = 0; j<stack->push_pos+1; j++)
			deal(stack->data[j],mode);
	}
}
void deal_deep(object o,int (*deal)(object o))  //deal�����Ƿ���Եݹ�
{
	//ע�Ȿ�ڴ�ϵͳ�����ü����Ӽ�����շ�������ͬ�׶���
#define PUSHIT(o) do{ if(o)addcollection(angel_traversal_stack,o); }while(0);
#define POPIT do{p = (object)popcollection(angel_traversal_stack); if(!p) return ;}while(0);

	object p;
	PUSHIT(o)
	while(1)
	{
		POPIT
		
		if(!deal(p))
			continue ;

		switch(p->type)
		{
		case NOTYPE:
		case FUNP:
		case INT:
		case FLOAT:
		case NU:
		case TRUE:
		case FALSE:
		case BOOLEAN:
		case STR:
			break ;
		case ITERATOR:
			PUSHIT(GETITER(p)->base);
			break ;
		case SLICE:
			PUSHIT(GETSLICE(p)->base);
			PUSHIT(GETSLICE(p)->range);
			break ;
		case ENTRY:
			PUSHIT(GETENTRY(p)->key);
			PUSHIT(GETENTRY(p)->value);
			break ;
		case LIST:
			for(int i = 0; i<GETLIST(p)->len; i++)
			{
				object item = GETLIST(p)->item[i];
				if(item)
				{
					PUSHIT(item);
				}
			}
			break ;
		case SET:
			for(int i = 0; i<GETSET(p)->alloc_size; i++)
			{
				object item = GETSET(p)->element[i];
				if(item)
				{
					PUSHIT(item);
				}
			}
			break ;
		case DICT:
			for(int i = 0; i<GETDICT(p)->alloc_size; i++)
			{
				object_entry entry = GETDICT(p)->hashtable[i];
				if(entry)
				{
					PUSHIT((object)entry);
				}
			}
			break ;
		case OBJECT:
			PUSHIT(p->mem_value);
			PUSHIT(p->pri_mem_value);
			break ;
		default:
			//���������չ�����������������
			break ;
		}
	}
}
#define GETPAGE(base,i) (page)((char *)base+i)
#define ISDRIFT(b) (TEST_O(b)->type < DRIFT_SIZE)
inline void _flag(object o)
{
	ASCREF(o);
}
int deep_flag(object p)
{
	//ע�Ȿ�ڴ�ϵͳ�����ü����Ӽ�����շ�������ͬ�׶���
	if(ISFLAGED(p))
		return 0;
	p->extra_flag = IS_FLAGED;
	return 1;
}
void flag(object o,int mode)
{
	if(!o)
		return ;
	if(mode == 0)
		_flag(o);
	else
		deal_deep(o,deep_flag);
}
void angel_flag(int mode)  //���ж��Ƿ��ǵ�ǰ��
{
	//Ӧ���ֶ�һЩϵͳ���б���б��
	traversal_root(flag,mode);
}
inline void _recovery_angel(object o)
{
	DECREF(o);
}
int deep_recovery_angel(object o)
{
	if(!ISFLAGED(o))
		return 0;
	o->extra_flag = 0;
	return 1;
}
void recovery_angel(object o,int mode)
{
	if(!o)
		return ;
	if(mode == 0)
		_recovery_angel(o);
	else
		deal_deep(o,deep_recovery_angel);
}
void recovery_angel_flag(int mode)  //���ж��Ƿ��ǵ�ǰ��
{
	traversal_root(recovery_angel,mode);
}
int detect_loop_reference(object o)
{
	//ע�⣬û�б�flag�Ķ��������࣬
	//һ����������м����û���ü��������ã�����һ�־���ѭ������û���ü��ͷ�
	if(ISFLAGED(o))  //��Ҫ������������ˣ���ⲻ��ѭ�����õ�
		return 0;
	if(ISDECED(o))  //����Ѿ����ͷ�������Ҫ��������
		return 0;
	o->extra_flag <<= 4; //��ԭ����flag���浽����λ
	if(ISLOOPED(o) || ISCHECKING(o))  //��ʾ�Ѿ���ǹ��ˣ����γ��˻�
	{
		o->extra_flag |= IS_LOOPED;
		return 0;
	}
	else
	{
		o->extra_flag |= LOOP_CHECK_FLAG;
	}
	//���û�в���ѭ�����õĶ�������ȥ����λָ�

	return 1;
}
int recovery_check_flag_to_normal(object o)
{
	//���checkreferenceloop�Ķ����˳�������CHECKING�����ֱ�ӽ���ְλ��ͨ���
	if(ISCHECKING(o)){
		o->extra_flag >>= 4;
		return 1;
	}
	return 0;
}


int block_gc_count = 0;
block test_block;
inline int callable(object o,int mode)
{
	if(mode == 1)
	{
		//ע���Ƿ��Ž�һֱ��������Ϊ�����Ѿ��������ˣ�
		//���⻹����Ϊ����ѭ�����ö���Ļ����ṩ����
		if(ISFLAGED(o))
			return 0;  //һ����������
		//ʣ�µľ���û��ǵģ�������Ҫ����Ƿ����ѭ������
		
		if(ISLOOPED(o))  //�������
			return 1;
		
		//����ѭ�����õļ��

		deal_deep(o,detect_loop_reference);

		if(!ISLOOPED(o))  //�������
		{
			//�ָ�û���γɻ��Ĳ���
			deal_deep(o,recovery_check_flag_to_normal);
			return 0;
		}
		else
		{
			return 1;
		}
		//���ʱ��ʱѭ������
	}
	return o->refcount == 0;
}
void select_free_block(field scan,int mode)
{
#define PRE_DEC_ELEMENT(o) dec_element(o,mode);
	free_block->next = NULL;
	page prepage;
	for(field f = scan; f; f=f->next)
	{
		angel_object_heap->field_current = f;
		char *base = (char *)f->memry,*p;
		p = base;
		while(p-base < max_block_size)  //ֱ�ӱ���
		{
			int driftsize = -1;
			block _free;
			object test = (object)p;
			if(ISDRIFT(p))
			{
				driftsize = *p;
				p += driftsize;
				_free = (block)p;
				goto recycle;
			}
			if(callable(TEST_O(p),mode))  //δ������
			{
				_free = (block)(p);
				int freesize=0;
				int flag = 0;
				object test = TEST_O(p);
				p += TEST_O(p)->osize;
				PRE_DEC_ELEMENT(test);
recycle:
				while(p-base < max_block_size)
				{
					test = TEST_O(p);
					if(ISDRIFT(p))
					{
						p += *p;
						continue ;
					}
					if(!callable(TEST_O(p),mode))  //��ʾ��ʱҪ������,
						break ;
					PRE_DEC_ELEMENT(TEST_O(p));
					p += TEST_O(p)->osize;
				}

				freesize = p-(char *)_free;
				if(freesize >= MINSIZE)
				{
					if(freesize == 32)
					{
						test_block = _free;
					}
					_free->block_size = freesize;
					ADDFREELIST_O(_free);
				}
				continue ;
			}
			p += TEST_O(p)->osize;
		}
		angel_object_heap->field_current = f;
	}
}
void gc_with_mode(field scan_field,int mode)
{
	stopworld();

	//����Ҫ��flag��recover�Ŀռ���һ����
	angel_flag(mode);
	
	//merge_page(FLAG_FLAGCLEAN);
	select_free_block(scan_field,mode);  //ѡ���Դ���scan_field�Ǵ�������,���������Ҳ����dec_element
	
	recovery_angel_flag(mode);

	goahead();
}

#define BLOCK_GC_CYCLE_STEP 5
inline int is_block_cycle_gc_step()
{
	return angel_object_heap->field_count % BLOCK_GC_CYCLE_STEP == 0;

}
inline int64_t get_heap_size()
{
	//������Ҫ���ǵ��߳�ջ�Ĵ�С
	return angel_object_heap->total_size + angel_data_heap->total_size;
}
inline int is_global_gc_step()
{
	int64_t used_size = get_heap_size();
	int size = angel_object_heap->field_count;
	if(used_size > 100*1024*1024)  //100M
		return size % (BLOCK_GC_CYCLE_STEP*BLOCK_GC_CYCLE_STEP) == 0;
	return ((size - 1) & size) == 0 && size > 4;
}
void block_gc()
{
#define STEP_UNIT 1/5
#define STEP1_CYCLE 1/2
#define STEP2_CYCLE 3/5

	angel_object_heap->free_size = 0;
	field scan_field;
	scan_field = angel_object_heap->field_current;
	int flag_mode = 0;
	field oldcur = angel_object_heap->field_current;

	if(angel_object_heap->flag == GLOBAL_GC_FLAG)//��Ҫ����ȫ��gc
	{
		flag_mode = 1;
	}

	gc_with_mode(scan_field,flag_mode);

	int freesize = angel_object_heap->free_size,totalsize;

	if(oldcur == angel_object_heap->field_head)
	{
		totalsize = angel_object_heap->total_size;
	}
	else
		totalsize = max_block_size;
	//�����ڴ���Զ������ϲ���ֻ���������б�ʵ�ڲ��о������µĿռ䣬�����µĿռ䲻���ٿ��ǷǶ����ڴ棨���ԷǶ����ڴ���л��գ�����ٲ��о�GG��
	
	
	if(angel_object_heap->flag == CYCLE_GC_FLAG || angel_object_heap->flag == GLOBAL_GC_FLAG)
	{
		if(freesize < totalsize * STEP1_CYCLE)  //һ������gc֮���Ǻܶ�����
		{
			angel_object_heap->flag = 0;
			goto addfield;
		}
		else if(freesize < totalsize * STEP2_CYCLE)
		{
			//��һ�β��������
			angel_object_heap->flag = 0;
		}
		else
		{
			//һ��CYCLE_GC���յ������ܶ࣬��һ�ο�������һ��CYCLE_GC_FLAG
			goto cycle_gc;
		}
	}
	if(freesize < totalsize * STEP_UNIT)  //��ʾ��ʱ����
	{
addfield:
		//��ʼ�����µ��ڴ�ռ䲢��ԭ������̭��
		field f = init_block_field();
		addfield(angel_object_heap,f);
		if(is_global_gc_step())//��Ҫ����ȫ��gc,ȫ��gc������������һ�㣬���Է���ǰ�棬������Ҫȫ��gcʱһ�����ܵ���
		{
			angel_object_heap->field_current = angel_object_heap->field_head; //ȫ��GC
			angel_object_heap->flag = GLOBAL_GC_FLAG;
		}
		else if(is_block_cycle_gc_step())
		{
cycle_gc:
			angel_object_heap->flag = CYCLE_GC_FLAG;
			angel_object_heap->field_current = angel_object_heap->field_head;
		}
	}
}



void flag_page(object o,int mode)  //�䱾���Ͼ���ѡ���ԵĶԶ�����
{
	if(!o) return ;
	if(IS_GC_REFCOUNT_WAY(o))
	{
		ASCREF(o);
	}
}
void angel_flag_page()
{
	//Ӧ���ֶ�һЩϵͳ���б���б��
	traversal_root(flag_page);
}
void recovery_page(object o,int mode)
{
	if(!o) return ;
	if(IS_GC_REFCOUNT_WAY(o))
	{
		DECREF(o);
	}
}
void recovery_page_flag()  //���ж��Ƿ��ǵ�ǰ��
{
	traversal_root(recovery_page);
}
#define CALLABLE(o) (o->refcount == 0)
inline int _merge(page p,page q)
{
	int offset = q->page_size;
	object test = q->ref;
	memmove(p,q,offset);   //��q������ֽ�copy��_freeΪ�׵��ڴ���
	switch(test->type)
	{
	case STR:
		GETSTR(test)->s = (char *)(p+1);
		break ;
	case LIST:
		GETLIST(test)->item = (object *)(p+1);
		break ;
	case SET:
		GETSET(test)->element = (object *)(p+1);
		break ;
	case DICT:
		GETDICT(test)->hashtable = (object_entry *)(p+1);
		break ;
	}
	return offset;
}
void merge_page(field scan_field)
{
#define MERGE(_free,q) i += _merge(_free,q); merge += _free->page_size;
	int i=0,merge;
	//free_block->next = NULL;
	angel_data_heap->free_size = 0;
	for(field f = scan_field; f; f=f->next)
	{
		i = 0;
		page base = (page)f->memry,p;
		int end = f->alloc_ptr;
		
		merge = i;
		while(i < end)  //ֱ�ӱ���
		{
			p = GETPAGE(base,i);
			page _free = GETPAGE(base,merge);
			object test = p->ref;
			if(!test) //��ô����Ŀ϶���û��ʹ������ֱ�ӷ��ؼ���
				goto _free_;
			if(CALLABLE(test))  //δ������
			{
				dec_element((object)test);	
_free_:
				page q = p;
				int freesize=0;
				
				while(i < end)
				{
					q = GETPAGE(base,i);  //��̽�Ŀ�
					object tmp = q->ref;
					if(tmp)
					{
						if(!CALLABLE(tmp))  //��ʾ��ʱҪ������,
						{
							MERGE(_free,q);
							break ;
						}
						//�������ڴ�ϲ���ʱ����Ѿ�����Ԥ������
						dec_element((object)tmp);
					}
					i += q->page_size;
				}
			}
			else 
			{
				MERGE(_free,p);
			}
		}
		//angel_sys_memset((char *)f->memry+merge,0,`-merge);
		f->alloc_ptr = merge;
		f->free_size = max_page_size - merge - 1;
		angel_data_heap->free_size += f->free_size;
		angel_data_heap->field_current = f;
	}
}
void merge_page()
{
	merge_page(angel_data_heap->field_current);
}
int g_test[max_block_size];
#define SWAP(i,j) {field temp; temp = base[i]; base[i] = base[j]; base[j] = temp;}
void ajust(field *base,int index,int size)
{
	int i = index;
	while(i*2 <= size)
	{
		int pi = i;
		i *= 2;
		field test = base[i];  //��ʾ��������С��һ��
		if(i+1 <= size)
		{
			if(base[i]->alloc_ptr < base[i+1]->alloc_ptr)
			{
				test = base[i];
			}
			else
			{
				test = base[i+1];
				i++;
			}
		}
		if(base[pi]->alloc_ptr > test->alloc_ptr)
		{
			SWAP(pi,i);
		}
	}
}
field *createheap()
{
	int heap_size = angel_data_heap->field_count,i = 1;
	field *heap = (field *)angel_sys_calloc(heap_size+1,sizeof(field));
	field p = angel_data_heap->field_head;
	while(p)
	{
		heap[i++] = p;
		p = p->next;
	}
	for(int i = heap_size/2; i > 0; i--)
	{
		ajust(heap,i,heap_size);
	}
	return heap;
}
void _global_merge(field base,field merge)
{
	int i = 0;
	while(i < merge->alloc_ptr)
	{
		page p = GETPAGE(merge->memry,i);
		int freesize = p->page_size;
		_merge(GETPAGE(base->memry,base->alloc_ptr),p);
		base->alloc_ptr += freesize;
		base->free_size -= freesize;
		i += freesize;
	}
	merge->alloc_ptr = 0;
	merge->free_size = max_page_size;
}
int global_merge()
{
	field *base = createheap();
	int end = angel_data_heap->field_count;
	int totalsize = 0,select_count = 0;
	for(int i = 1; i <= angel_data_heap->field_count; i++)
	{
		if(base[i]->alloc_ptr > max_page_size/4)  //��֤ÿ���ƶ���Ԫ�ز�Ҫ̫��
		{
			totalsize += base[i]->alloc_ptr;
			if(totalsize <= max_page_size)  //����ȫ�ֺϲ�һ��page
			{
				SWAP(i,end);
				end--;
				select_count++;
				ajust(base,1,end);
			}
			else
			{
				field empty_field = base[end+1];
				if(select_count == 1)
					goto exit;  //����϶�����ʵ�ֺϲ�
				for(int i = 1; i < select_count; i++)
				{
					int index = end+1+i;
					_global_merge(empty_field,base[index]); //����ȫ�ֺϲ�
				}
			}
		}
		else
		{
exit:
			free(base);
			break ;
		}
	}
	return select_count;
}
#define PAGE_GC_CYCLE_STEP 4
inline int is_page_cycle_gc_step()
{
	return angel_data_heap->field_count % BLOCK_GC_CYCLE_STEP == 0;

}
void page_gc()
{
#define UNIT_STEP 1/4
#define CYCLE_STEP 1/2
#define MERGE_STEP 1/3

	object test;

	field oldcur = angel_data_heap->field_current;


	stopworld();
	angel_flag_page();
	merge_page(angel_data_heap->field_current);
	recovery_page_flag();
	goahead();


	int freesize = angel_data_heap->free_size,totalsize;

	if(oldcur == angel_data_heap->field_head)
	{
		totalsize = angel_data_heap->total_size;
	}
	else
		totalsize = max_page_size;

	if(angel_data_heap->flag == GLOBAL_GC_FLAG)  //
	{
		totalsize = angel_data_heap->total_size;
		int merge_count = global_merge();
		if(merge_count < angel_data_heap->field_count*MERGE_STEP)
			goto addfield;
		if(freesize < totalsize * CYCLE_STEP)
		{
			goto addfield; //���ģ���˷�
		}
		else//��ͷ��ʼ��
		{
			//Ҫô�����࣬Ҫô��8�ı���ǿ�ƻ���
			angel_data_heap->field_current = angel_data_heap->field_head;  //ά��
		}
		return ;
	}
	if(freesize < totalsize * UNIT_STEP)  //��ʾ��ʱ����
	{
addfield:
		//��ʼ�����µ��ڴ�ռ䲢��ԭ������̭��
		angel_data_heap->flag = 0;
		field f = init_page_field();
		addfield(angel_data_heap,f);
		if(is_page_cycle_gc_step())
		{
			angel_data_heap->field_current = angel_data_heap->field_head;  //��ʼcurrentָ��
			angel_data_heap->flag = GLOBAL_GC_FLAG;
		}
	}
}
void free_extend_page()
{
	field pre = angel_data_heap->extend_head,p = pre->next;
	while(p)
	{
		object test = ((page)p->memry)->ref;
		if(test)  //�������ڴ漫��ʹ�ã�����һ�����տ��������ͷ�
		{
			if(CALLABLE(test))
			{
				dec_element(test);
				goto _free_big;
			}
		}
		else
		{
_free_big:
			field temp = p;
			angel_data_heap->total_size -= temp->free_size;
			angel_data_heap->extend_size -= temp->free_size;
			p = temp->next;
			pre->next = p;
			free(temp->memry);
			free(temp);
			continue ;
		}
		pre = p;
		p = pre->next;
	}
}
void page_extend_gc()
{
	object test;
	angel_flag_page();
	free_extend_page();
	recovery_page_flag();
}




/*
��������Ĵ���
*/
object_entry initentry(object key,object value)
{
	//�������Ѿ�ʵ����ref��1
	STACKTOHEAP(key);
	STACKTOHEAP(value);


	object_entry res = (object_entry)angel_alloc_block(APPLYSIZE(sizeof(object_entrynode)));

	res->key = key;
	res->value = value;
	res->type = ENTRY;
	return res;
}
int getnumber_binary(int val)
{
	int count = 0;
	while(val)
	{
		val >>= 1;
		count++;
	}
	return count;
}
void free_perpetual(object o)
{
	o->refcount = 0;
	//angel_free_page(NULL,0);
}
object_slice initslice(object base,object_range range)
{
	STACKTOHEAP(base);
	ASCREF(range);
	object_slice res = (object_slice)angel_alloc_block(sizeof(object_slicenode));
	res->type = SLICE;
	res->base = base;
	res->range = range;
	return res;
}
/*
��չ���͵�����
*/
object initext(int size)
{
	object ext = angel_alloc_block(size);
	ext->type = EXT_TYPE;
	return ext;
}