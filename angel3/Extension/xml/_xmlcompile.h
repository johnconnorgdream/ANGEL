#ifndef xmlcompile_def
#define xmlcompile_def


#ifdef _cplusplus
extern "c"{
#endif

#define maxsize 64
#define xmlsize 10
typedef struct attrnode{
	char *key,*value;
	attrnode * next;
}* attr;     //属性是键值对组成的链表
typedef struct contentnode {
	char *text;  //标签内的文本内容
	attr fistattr;  //元素的属性。
} * content;
typedef struct tagnode {
	char *name;
	content c;
	tagnode *parent,*neighbor,*firstchild;
}* tag;
typedef struct xmlnode{
	tag *xmltag;
	int len;
	int alloc_size;
}* xml;
typedef struct xmlresnode{
	char *type;
	tag res;
}*xmlres; 


typedef struct stacknode
{
	tag data[maxsize];
	int top;
}* stack;

typedef struct stacknode_o
{
	int data[100];
	int top;
}* stack_o;
typedef struct queuenode{
    tag data[maxsize];
	int rear,front;
}* queue;

#ifdef data_def
object sysparse(object,object);
#endif


char *addstr(char *dest,char *src);
char *addchar(char *dest,char ch);

int stackempty(stack_o s);
int pop(stack_o s);
void push(stack_o s,int t);
int gettop(stack_o s);



tag outqueue(queue q);
int queueempty(queue q);
int queuefull(queue q);
void inqueue(queue a,tag t);
//int readline(char* ,FILE *); //读文件的一行
void dealcode(char *);
char *getstrcat(char *des,char *src);


xml  findallbyname_b(tag t,char *name);
xml  findallbyname(tag target,char *name);//这里的tag*是指返回tag类型的数组的首地址
xml  findbyname(tag target,char *name);
xml  findallbyattr(tag target,char *key,char *value);
xml  findbyattr(tag target,char *key,char *value);
int  getattr(tag t,char * key,char **value);
xmlres  xmlanalysis(char *buf,int *error);
xml  getchildren(tag t);

#ifdef _cplusplus
}
#endif
#endif