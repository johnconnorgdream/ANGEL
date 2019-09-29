#ifndef amem_def
#define amem_def

#ifdef _cplusplus
extern "c"{
#endif

#include "data.h"
#include "aexpimp.h"
/*
δ������Ҫʵ���ַ��������Ǵ洢�ַ����ݵ��������������Ի������е��ַ�����������Ϊ4���ַ������Ա�����ÿ�ε��õ�ʱ����
Ȼ���������ַ�������Ļ�����ʵ���ֽ�������������ֽ����Ǿ������εĴ��档
*/
//����ڴ��Ϊ512kB����������1kB�ռ�Ҳ����128����λ
#define FLAG_FLAGCLEAN -1
#define max_block_size 256*1024  //��λΪB
#define max_page_size 2*1024*1024 //��λΪB
#define max_apply_size 512 //����Ĵ�С����������ֵ���Ը�ֵ����
#define SETBASESIZE APPLYSIZE(sizeof(object_setnode))
#define STRBASESIZE sizeof(object_stringnode)-4
#define PAGEALLOCMIN 8
#define NUMSIZE APPLYSIZE(sizeof(object_intnode))
#define FLOATSIZE APPLYSIZE(sizeof(object_floatnode))
#define MINSIZE NUMSIZE
#define APPLYSIZE(bytesize) bytesize
#define data_head 8
#define cellnum 256
#define DRIFT_SIZE NOTYPE

#define CYCLE_GC_FLAG 1
#define GLOBAL_GC_FLAG 2


#define ISBUILDINTYPE(o) (((object)o)->type <= REGULAR && ((object)o)->type >= STR)
#define ISCOLLECTION(o) (((object)o)->type <= SET && ((object)o)->type >= STR)


#define IS_GC_REFCOUNT_WAY(o) (ISCOLLECTION(o) || ISOBJECT(o))
//fraction_max_size�ļ�����block_bucket_size/2
typedef struct blocknode{
	int block_size;
	blocknode *next;
}*block;
typedef struct pagenode{
	int page_size;
	object ref;
}*page;
typedef struct fieldnode{
	char *memry;  //��8���ֽ�Ϊ��λ���ļ��㣬�����Ƿ�ʽ�����ڲ�ͬ���ڴ��ʽ�ò�ͬ��������ǿ��ת��
	int free_size,alloc_ptr,flag; //����ֶζ����ڴ���ò���,�����봮ʽ���ݵ�ʱ���ܹ��õ�,flag��ʾ��ʱ��field�ǲ��Ǳ�̬��
	fieldnode *next;
}*field;
typedef struct angel_memrynode{
	char flag ;//�Ƿ���й�gc
	int64_t free_size,total_size,extend_size;
	field field_head,field_current;
	field extend_head;  //�������page�г����
	int field_count;//field����Ŀ
}*angel_memry;



object angel_alloc_block(int tsize);
void* angel_alloc_page(object head,int len);
object initext(int size);


//���ڿɱ���ڴ���������ַ������б�Ŀռ����룬��������ƽ��������ķ�ʽ
angel_memry get_object_heap();
angel_memry get_data_heap();
void gc_object();
void init_heap();
void sys_realloc_list(object_list l,int resize);
void lock_const_sector();
void merge_page();
void reset_heap();
__forceinline void angel_free_page(void *addr)
{
	page test = (page)addr-1;
	test->ref = NULL;
}


#ifdef _cplusplus
}
#endif
#endif