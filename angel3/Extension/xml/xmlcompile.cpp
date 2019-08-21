#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "_xmlcompile.h"
int type=0; //为1是xml文件，为2是HTML文件
char *buffer;
tag activetag;  //当前查询语句的活动节点，只能有一个
tag roottag;  //文档树的根节点

tag gettop(stack s)
{
	return s->data[s->top];
}
int stackempty(stack s)
{
	if(s->top==-1)
		return 1;
	else
		return 0;
}
void push(stack s,tag t)
{
	if(s->top==maxsize)
		return;
	s->data[++s->top]=t;
}
tag pop(stack s)
{
	if(stackempty(s))
		return NULL;
	else
	{
		tag temp=s->data[s->top];
		s->top--;
		return temp;
	}
}

int queueempty(queue q)
{
	if(q->front==q->rear)
		return 1;
	else 
		return 0;
}
int queuefull(queue q)
{
	if((q->rear+1)%maxsize==q->front)
		return 1;
	else
		return 0;
}
void inqueue(queue q,tag t)
{
	if(!queuefull(q))
	{
		q->data[q->rear]=t;
		q->rear=(q->rear+1)%maxsize;
	}
	else
		printf("队列满了！");
}
tag outqueue(queue q)
{
	if(!queueempty(q))
	{
		tag ret=q->data[q->front];
		q->front=(q->front+1)%maxsize;
		return ret;
	}
	else 
	{
		printf("队列空了！");
		return NULL;
	}
}
 char *addstr(char *dest,char *src)
 {
	 char *p=dest;
	 while(*p) p++;
	 while(*src) *p++=*src++;
	 *p=0;
	 return dest;
 }
 char *addchar(char *dest,char ch)
 {
	 char *p=dest;
	 while(*p) p++;
	 *p++=ch;
	 *p=0;
	 return dest;
 }

int getfilelength(FILE *f)
{
	int result,cur;
	cur=ftell(f);
	fseek(f,0,SEEK_END);
	result=ftell(f);   //读取
	fseek(f,0,cur);  //还原
	return result;
}

void dealcode(char *filename)   //规范代码的工具
{
	FILE *f,*s;
	f=fopen(filename,"r");
	s=fopen(filename,"r+");
	fseek(f,0,SEEK_SET);
	char ch;
	ch=fgetc(f);
	while(ch!=EOF)
	{
		fputc(ch,s);
		if(ch=='{' ||  ch==';')
			fputc('\n',s);
		if(ch=='}' && fgetc(f)!=';')
		{
			fputc('\n',s);
			fseek(f,-1,SEEK_CUR);
		}
		//以上均是处理结束界符：块，语句，函数体
		ch=fgetc(f);
	}
	fclose(f);
	fclose(s);
}

int resize(char **end,char **begin,int len)
{
	int use=*end-*begin+1;
	len+=maxsize;
	*begin=(char *)realloc(*begin,len);
	*end=*begin+use-1;
	return len;
}
#define ISDEIVE(s) ((s)==' ' || (s) == '\t' || (s) == '\n' || (s) == '\r')
char* filterspace_xml(char *s)  //被过滤的字串的第一个字符必须是空格。即碰到空格需要时才用该过滤。
{
	while(ISDEIVE(*s))
		s++;
	return s;
}
#define CHECKMEM(end,begin) use=end-begin+1; \
							if(use>=len) \
								len=resize(&end,&begin,len);
char * getword(char **s,int *flag)  
{
	char *temp=(char *)malloc(maxsize);//设置一个缓冲区,这里面设置的缓冲区的大小太大了，应该动态进行
	char *p=temp;
	int use;
	*s=filterspace_xml(*s);
	//将应该出现的情况列出来，然后其他的情况全部视为不合法。
	int len = maxsize; //初始分配的空间
	switch(*flag)
	{
	case 0:
		while((!ISDEIVE(**s)) && **s!='>' && **s!='/')
		{
			if(**s=='\0')
			{
				*flag=1;  //表示异常退出
				break ;
			}
			else if(**s=='=' || **s=='<')
			{
				*flag=2; //元素名出错。
				break ;
			}
			else
				*p++=*(*s)++;
			CHECKMEM(p,temp);
		}
		break ;
	case 3:  //表示此时是
		while((!ISDEIVE(**s)) && **s!='=')
		{
			if(**s=='\0')
			{
				*flag=1;
				break ;
			}
			else if(**s=='>' || **s=='<' || **s=='/')
			{
				*flag=4; //元素属性名出错
				break ;
			}
			else
				*p++=*(*s)++;
			CHECKMEM(p,temp);
		}
		break ;
	case 5:
		while(**s!='<')
		{
			if(**s=='\0')
			{
				*flag=1;
				break ;
			}
			else
				*p++=*(*s)++;
			CHECKMEM(p,temp);
		}
		break ;
	case 6:
		if(**s=='\0')
		{
			*flag=1;
			break ;
		}
		else   //属性的书写现在宽松一下。
		{
			if(**s=='"')//注意这里的表达式写法（过滤掉第一个引号）
			{
				(*s)++;
				while(**s!='"')
				{
					if(!**s)
						s=s;
					*p++=*(*s)++;
					CHECKMEM(p,temp);
				}
				(*s)++;             //过滤掉第二个引号
			}
			else if(**s=='\'')
			{
				(*s)++;
				while( **s!='\'')
				{
					*p++=*(*s)++;
					CHECKMEM(p,temp);
				}
				(*s)++;
			}
			else
			{
				(*s)++;
				while((!ISDEIVE(**s)) && **s!='>')
				{
					*p++=*(*s)++;
					CHECKMEM(p,temp);
				}
				if(**s!='>')
					(*s)++;
			}
		}
	}
	//while(**s!=' ' && **s!='>' && **s!='\0' && **s!='='&& **s!='<' && **s!='/')
		//*p++=*(*s)++;
	*p=0;
	*s=filterspace_xml(*s);
	return temp;
}
char * checkword(char *s,int *flag)
{
	return getword(&s,flag);
}
void addattr(tag t, attr a)
{
	a->next=t->c->fistattr;
	t->c->fistattr=a;
}
void addtagchildren(tag parent,tag child)
{
	//这里用尾插法插入较好，能够保证元素的先后位置。
	tag p;
	if(!parent->firstchild)  //防止parent->firstchild为空导致for循环中的p->neighbor访问不到
	{
		child->neighbor=NULL;
		parent->firstchild=child;
	}
	else
	{
		for(p=parent->firstchild;p->neighbor; p=p->neighbor); //获得父元素最后一个子节点，性能问题
		child->neighbor=NULL;
		p->neighbor=child;
	}
	//注意这里里面的neighbor相当于next，只有父元素的第一个子元素才有。
	child->parent=parent;
}

char *getnchar(char **s,int count)
{
	char *ret=(char *)malloc(count+1),*p;
	p=ret;
	while(count>0)
	{
		*p++=*(*s)++;
		count--;
	}
	*p=0;
	return ret;
}
char *checknchar(char *s,int count)
{
	return getnchar(&s,count);
}
char * dealtag(char *s, stack tagstack,tag *res,int *flag)    //这就相当于一个词法分析，词法分析的一个重要原则就是明确规则，考虑尽可能多的因素，并把所有不符合规则的全部返回错误
{
#define EXIT {*flag=mod; free(t->c) ; free(t); return NULL;}
	int mod,use;
	tag t=(tag)malloc(sizeof(tagnode));
	t->c=(content)malloc(sizeof(contentnode));
	t->c->fistattr=NULL;
	t->neighbor=NULL;
	t->firstchild=NULL;
	t->parent=NULL;
begin:
	s=filterspace_xml(s);
	if(*s!='/')//这里处理的是开始标签
	{
		if(strcmp(checknchar(s,3),"!--")==0)   //若是!--则需要一段过滤,这时注释部分
		{
			getnchar(&s,3);
			while(strcmp(checknchar(s,3),"-->")!=0) s++;
			getnchar(&s,3);
			while(*s++ != '<');
			goto begin;
		}
		mod=0; //表示是获取标签的name
		t->name=getword(&s,&mod);//先获得标签的名字
		if(mod==1 || mod==2)
			EXIT;
		while(*s!='>' && *s!='/')//检测到结束标志(分为单标签结束和双标签结束)为止，来分别填充标签的属性信息
		{
		//这里需要获得属性的值
			attr a=(attr)malloc(sizeof(attrnode));   //注意这里的节点的定义必须放在循环体中。
			a->key=a->value=NULL;
			mod=3;
			a->key=getword(&s,&mod);
			if(mod==1 || mod==4)
				EXIT;

			if(*s != '=')   //
			{
				mod = 4;
				EXIT;
			}
			else
				s++;

			//这里需要获得属性中的值
			mod=6;
			a->value=getword(&s,&mod);   //这里有待完善
			if(mod==1 || mod==7)
				EXIT;

			addattr(t,a); //填充属性信息
			//printf("键为：%s\n",a->key);
			//printf("值为：%s\n",a->value);
		}
		if(*s=='/')//说明该标签是单标签
		{
			addtagchildren(gettop(tagstack),t);
			t->c->text=NULL;
		}
		else    //说明是正常的双标签
		{
			s++;//过滤>符号
			//获得元素里的文本内容
			if(type==1 && strcmp(t->name,"script")==0)  //这是处理html代码中的script标签
			{
				int len=maxsize;
				char *text=(char *)malloc(len),*p;
				p=text;
				while(*s)
				{
					mod=0;
					if(*s=='<')
					{
						char *end=checkword(s+2,&mod);
						if(mod==1)
							EXIT;
						if(*(s+1)=='/' && strcmp(end,"script")==0)
							break;
					}
					*p++=*s++;
					CHECKMEM(p,text);
				}
				*p=0;
				if(!*s)
					EXIT;
				t->c->text=text;
			}
			else
			{
				mod=5;
				t->c->text=getword(&s,&mod);//printf("标签名为：%s\n",t->name);
				if(mod==1)
					EXIT;
			}
			//printf("元素内容为：%s\n",t->c->text);
			if(!stackempty(tagstack)) 
				addtagchildren(gettop(tagstack),t);//设置元素的父子关系
			push(tagstack,t);
		}
	}
	else  //若是结束标签则做以下操作
	{  
		char *text;
		if(!stackempty(tagstack))
		{
			char *end;
			mod=0;
			end=checkword(s+1,&mod);
			if(mod==1 ||mod==2)
				EXIT;
			if(strcmp(gettop(tagstack)->name,end)!=0)  //这说明有些标签的书写不够规范，这是为了兼容
			{
				tag t=gettop(tagstack);
				t->neighbor=t->firstchild;  //将孩子关系调整为兄弟关系
				if(t->firstchild)
					t->firstchild->parent=t->parent;
				t->firstchild=NULL;
				s--;
			}
		}
		else
		{
			mod=8;
			EXIT;
		}
		t=pop(tagstack);  //这个操作在文件规不规范的情况下都是要做的
	}
	*res=t;
	return s;
}
int strcmp_uplow(char *c,char *std)//非大小写敏感
{
	while(*c)
	{
		if(*c-*std==32 || *c-*std==-32  || *c==*std)  //表示此时两字符在过滤大小写之后相同
		{
			c++;
			std++;
		}
		else
			return 0;
	}
	return 1;
}
char * checkdoctype(char *s)
{
	int mod;
	s=filterspace_xml(s);  //过滤掉前面的空格
	if(*s!='<')
		return NULL;
	mod=0;
	char *p=checkword(s+1,&mod);
	if(mod==1 || mod==2)
		return NULL;
	if(strcmp_uplow(p,"html"))
		type=1;
	else if(strcmp_uplow(p,"!doctype"))
	{
		s++; //过滤掉<符号。
		mod=0;
		getword(&s,&mod);
		p=getword(&s,&mod);
		if(mod==1 || mod==2)
			return NULL;
		if(strcmp_uplow(p,"html")) //过滤掉html声明头文件
			type=1; //表示为HTML文档。
		else
			type=0;
		s++;//过滤掉>符号
	}
	else if(strcmp_uplow(p,"?xml"))
	{
		type=2;//表示xml文件
		while(*s!='>') s++;
	}
	free(p);
	return s;
}
xmlres  xmlanalysis(char *buffer,int *ret)
{
	char *p;
	stack s=(stack)malloc(sizeof(stacknode));
	xmlres result=(xmlres)malloc(sizeof(xmlresnode));
	s->top=-1;
	p=checkdoctype(buffer);//设置扫描指针
	switch(type)
	{
	case 0:
		result->type = "xml";
		break ;
	case 1:
		result->type = "html";
		break ;
	case 2:
		result->type = "xml";
		break ;
	}
	int error=-1;
	if(!p)
		goto end;
	while(*p)   //核心的驱动,他只处理从<开始的标签处理
	{
		if(*p++=='<')
			p=dealtag(p,s,&result->res,&error);   //指向<下一个字符
		if(error!=-1)
		{
			result->res=NULL;
			goto end;
		}	
	}
	if(stackempty(s))
		goto end;
	else
	{
		result=NULL;
		*ret=8;
		goto end;
	}
end:
	free(s);
	free(buffer);
	*ret=error;
	return result;
}
xml initxml()
{
	xml x=(xml)malloc(sizeof(xmlnode));
	x->len=0;
	x->alloc_size=xmlsize;
	x->xmltag=(tag *)calloc(x->alloc_size,sizeof(tag));
	return x;
}
void addxml(xml x,tag t)
{
	if(x->len>=x->alloc_size)
	{
		x->alloc_size*=2;
		x->xmltag=(tag *)realloc(x->xmltag,x->alloc_size*sizeof(tag));
	}
	x->xmltag[x->len++]=t;
}
xml findallbyname_s(tag t,char *name,xml x)  //树的深度优先遍历中的先根遍历
{
	tag p;
	if(strcmp(t->name,name)==0)
		addxml(x,t);
	for(p=t->firstchild;p;p=p->neighbor)
		findallbyname_s(p,name,x);
	return x;
}
xml findallbyname(tag t,char *name)
{
	xml x=initxml();
	return findallbyname_s(t,name,x);
}
xml findallbytext_s(tag t,char *text,xml x)  //树的深度优先遍历中的先根遍历
{
	//static int index=0;  //静态变量只要定义了就永久存在,态变量只要定义了就永久存在（包括他的值）即它的声明周期一直存在，这就是它恐怖之所在,多以在下一次调用这个函数前要将内容清空，最简单的办法就是设置count为0
	
	tag p;
	if(t->c->text && strcmp(t->c->text,text)==0)
		addxml(x,t);
	for(p=t->firstchild;p;p=p->neighbor)
		findallbytext_s(p,text,x);
	return x;
}
xml findallbytext(tag t,char *text)
{
	xml x=initxml();
	
	return findallbytext_s(t,text,x);
}
xml  findallbyattr_s(tag t,char *key,char *value,xml x)
{
	tag p;
	char *getvalue;
	if(getattr(t,key,&getvalue))
	{
		if(strcmp(getvalue,value)==0)
			addxml(x,t);
	}
	for(p=t->firstchild;p;p=p->neighbor)
		findallbyattr_s(p,key,value,x);
	return x;
}
xml findallbyattr(tag t,char *key, char *value)
{
	xml x=initxml();
	
	return findallbyattr_s(t,key,value,x);
}
xml findallbyname_b(tag t,char *name)  //树的广度优先遍历
{
	
	 xml x=initxml();
	int index=0;
	tag p=t;      //设置扫描指针
	queue q=(queue)malloc(sizeof(queuenode));
	q->front=q->rear=0;
	inqueue(q,p);
	while(!queueempty(q))
	{
		p=outqueue(q);
		if(strcmp(p->name,name)==0)
			addxml(x,p);
		for(p=p->firstchild; p; p=p->neighbor)
			inqueue(q,p);
	}
	return x;
}
xml getchildren(tag t)
{
	xml x=initxml();
	
	tag p;
	for(p=t->firstchild; p; p=p->neighbor)
		addxml(x,p);
	return x;
}
int getattr(tag t, char *key,char **value)
{
	attr p;
	if(!t)
		return 0;
	for(p=t->c->fistattr;p;p=p->next)
		if(strcmp(p->key,key)==0) //注意在比较字符串变量的内容是要用strcmp 
		{
			*value=p->value;
			return 1;
		}
	return 0;
}





int tonum(char *a)
{
	int total=0,i=1;
	char *p=a;
	while(*p) p++;//获得的是最后的0
	while(p--!=a)
	{
		total=total+(*p-'0')*i;
		i*=10;
	}
	return total;
}
