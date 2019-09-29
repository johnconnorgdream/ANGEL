#ifndef amem_def
#define amem_def

#ifdef _cplusplus
extern "c"{
#endif

#include "data.h"
#include "aexpimp.h"
/*
未来我们要实现字符链，就是存储字符数据的链表，这个链表可以回收所有的字符，即物理长度为4的字符串，以避免在每次调用的时候都有
然后我们在字符串对象的基础上实现字节数组对象，其中字节我们就用整形的代替。
*/
//最大内存块为512kB，最大可申请1kB空间也就是128个单位
#define FLAG_FLAGCLEAN -1
#define max_block_size 256*1024  //单位为B
#define max_page_size 2*1024*1024 //单位为B
#define max_apply_size 512 //申请的大小如若超过该值则以该值则用
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
//fraction_max_size的极限是block_bucket_size/2
typedef struct blocknode{
	int block_size;
	blocknode *next;
}*block;
typedef struct pagenode{
	int page_size;
	object ref;
}*page;
typedef struct fieldnode{
	char *memry;  //以8个字节为单位尽心计算，这里是范式，对于不同的内存格式用不同的类型来强制转换
	int free_size,alloc_ptr,flag; //这个字段对象内存会用不到,在申请串式数据的时候能够用到,flag表示此时的field是不是变态大
	fieldnode *next;
}*field;
typedef struct angel_memrynode{
	char flag ;//是否进行过gc
	int64_t free_size,total_size,extend_size;
	field field_head,field_current;
	field extend_head;  //这是针对page中超大块
	int field_count;//field的数目
}*angel_memry;



object angel_alloc_block(int tsize);
void* angel_alloc_page(object head,int len);
object initext(int size);


//对于可变的内存管理，比如字符串和列表的空间申请，我们利用平衡二叉树的方式
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