#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "_xmlcompile.h"
int type=0; //Ϊ1��xml�ļ���Ϊ2��HTML�ļ�
char *buffer;
tag activetag;  //��ǰ��ѯ���Ļ�ڵ㣬ֻ����һ��
tag roottag;  //�ĵ����ĸ��ڵ�

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
		printf("�������ˣ�");
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
		printf("���п��ˣ�");
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
	result=ftell(f);   //��ȡ
	fseek(f,0,cur);  //��ԭ
	return result;
}

void dealcode(char *filename)   //�淶����Ĺ���
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
		//���Ͼ��Ǵ������������飬��䣬������
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
char* filterspace_xml(char *s)  //�����˵��ִ��ĵ�һ���ַ������ǿո񡣼������ո���Ҫʱ���øù��ˡ�
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
	char *temp=(char *)malloc(maxsize);//����һ��������,���������õĻ������Ĵ�С̫���ˣ�Ӧ�ö�̬����
	char *p=temp;
	int use;
	*s=filterspace_xml(*s);
	//��Ӧ�ó��ֵ�����г�����Ȼ�����������ȫ����Ϊ���Ϸ���
	int len = maxsize; //��ʼ����Ŀռ�
	switch(*flag)
	{
	case 0:
		while((!ISDEIVE(**s)) && **s!='>' && **s!='/')
		{
			if(**s=='\0')
			{
				*flag=1;  //��ʾ�쳣�˳�
				break ;
			}
			else if(**s=='=' || **s=='<')
			{
				*flag=2; //Ԫ��������
				break ;
			}
			else
				*p++=*(*s)++;
			CHECKMEM(p,temp);
		}
		break ;
	case 3:  //��ʾ��ʱ��
		while((!ISDEIVE(**s)) && **s!='=')
		{
			if(**s=='\0')
			{
				*flag=1;
				break ;
			}
			else if(**s=='>' || **s=='<' || **s=='/')
			{
				*flag=4; //Ԫ������������
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
		else   //���Ե���д���ڿ���һ�¡�
		{
			if(**s=='"')//ע������ı��ʽд�������˵���һ�����ţ�
			{
				(*s)++;
				while(**s!='"')
				{
					if(!**s)
						s=s;
					*p++=*(*s)++;
					CHECKMEM(p,temp);
				}
				(*s)++;             //���˵��ڶ�������
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
	//������β�巨����Ϻã��ܹ���֤Ԫ�ص��Ⱥ�λ�á�
	tag p;
	if(!parent->firstchild)  //��ֹparent->firstchildΪ�յ���forѭ���е�p->neighbor���ʲ���
	{
		child->neighbor=NULL;
		parent->firstchild=child;
	}
	else
	{
		for(p=parent->firstchild;p->neighbor; p=p->neighbor); //��ø�Ԫ�����һ���ӽڵ㣬��������
		child->neighbor=NULL;
		p->neighbor=child;
	}
	//ע�����������neighbor�൱��next��ֻ�и�Ԫ�صĵ�һ����Ԫ�ز��С�
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
char * dealtag(char *s, stack tagstack,tag *res,int *flag)    //����൱��һ���ʷ��������ʷ�������һ����Ҫԭ�������ȷ���򣬿��Ǿ����ܶ�����أ��������в����Ϲ����ȫ�����ش���
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
	if(*s!='/')//���ﴦ����ǿ�ʼ��ǩ
	{
		if(strcmp(checknchar(s,3),"!--")==0)   //����!--����Ҫһ�ι���,��ʱע�Ͳ���
		{
			getnchar(&s,3);
			while(strcmp(checknchar(s,3),"-->")!=0) s++;
			getnchar(&s,3);
			while(*s++ != '<');
			goto begin;
		}
		mod=0; //��ʾ�ǻ�ȡ��ǩ��name
		t->name=getword(&s,&mod);//�Ȼ�ñ�ǩ������
		if(mod==1 || mod==2)
			EXIT;
		while(*s!='>' && *s!='/')//��⵽������־(��Ϊ����ǩ������˫��ǩ����)Ϊֹ�����ֱ�����ǩ��������Ϣ
		{
		//������Ҫ������Ե�ֵ
			attr a=(attr)malloc(sizeof(attrnode));   //ע������Ľڵ�Ķ���������ѭ�����С�
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

			//������Ҫ��������е�ֵ
			mod=6;
			a->value=getword(&s,&mod);   //�����д�����
			if(mod==1 || mod==7)
				EXIT;

			addattr(t,a); //���������Ϣ
			//printf("��Ϊ��%s\n",a->key);
			//printf("ֵΪ��%s\n",a->value);
		}
		if(*s=='/')//˵���ñ�ǩ�ǵ���ǩ
		{
			addtagchildren(gettop(tagstack),t);
			t->c->text=NULL;
		}
		else    //˵����������˫��ǩ
		{
			s++;//����>����
			//���Ԫ������ı�����
			if(type==1 && strcmp(t->name,"script")==0)  //���Ǵ���html�����е�script��ǩ
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
				t->c->text=getword(&s,&mod);//printf("��ǩ��Ϊ��%s\n",t->name);
				if(mod==1)
					EXIT;
			}
			//printf("Ԫ������Ϊ��%s\n",t->c->text);
			if(!stackempty(tagstack)) 
				addtagchildren(gettop(tagstack),t);//����Ԫ�صĸ��ӹ�ϵ
			push(tagstack,t);
		}
	}
	else  //���ǽ�����ǩ�������²���
	{  
		char *text;
		if(!stackempty(tagstack))
		{
			char *end;
			mod=0;
			end=checkword(s+1,&mod);
			if(mod==1 ||mod==2)
				EXIT;
			if(strcmp(gettop(tagstack)->name,end)!=0)  //��˵����Щ��ǩ����д�����淶������Ϊ�˼���
			{
				tag t=gettop(tagstack);
				t->neighbor=t->firstchild;  //�����ӹ�ϵ����Ϊ�ֵܹ�ϵ
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
		t=pop(tagstack);  //����������ļ��治�淶������¶���Ҫ����
	}
	*res=t;
	return s;
}
int strcmp_uplow(char *c,char *std)//�Ǵ�Сд����
{
	while(*c)
	{
		if(*c-*std==32 || *c-*std==-32  || *c==*std)  //��ʾ��ʱ���ַ��ڹ��˴�Сд֮����ͬ
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
	s=filterspace_xml(s);  //���˵�ǰ��Ŀո�
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
		s++; //���˵�<���š�
		mod=0;
		getword(&s,&mod);
		p=getword(&s,&mod);
		if(mod==1 || mod==2)
			return NULL;
		if(strcmp_uplow(p,"html")) //���˵�html����ͷ�ļ�
			type=1; //��ʾΪHTML�ĵ���
		else
			type=0;
		s++;//���˵�>����
	}
	else if(strcmp_uplow(p,"?xml"))
	{
		type=2;//��ʾxml�ļ�
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
	p=checkdoctype(buffer);//����ɨ��ָ��
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
	while(*p)   //���ĵ�����,��ֻ�����<��ʼ�ı�ǩ����
	{
		if(*p++=='<')
			p=dealtag(p,s,&result->res,&error);   //ָ��<��һ���ַ�
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
xml findallbyname_s(tag t,char *name,xml x)  //����������ȱ����е��ȸ�����
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
xml findallbytext_s(tag t,char *text,xml x)  //����������ȱ����е��ȸ�����
{
	//static int index=0;  //��̬����ֻҪ�����˾����ô���,̬����ֻҪ�����˾����ô��ڣ���������ֵ����������������һֱ���ڣ���������ֲ�֮����,��������һ�ε����������ǰҪ��������գ���򵥵İ취��������countΪ0
	
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
xml findallbyname_b(tag t,char *name)  //���Ĺ�����ȱ���
{
	
	 xml x=initxml();
	int index=0;
	tag p=t;      //����ɨ��ָ��
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
		if(strcmp(p->key,key)==0) //ע���ڱȽ��ַ���������������Ҫ��strcmp 
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
	while(*p) p++;//��õ�������0
	while(p--!=a)
	{
		total=total+(*p-'0')*i;
		i*=10;
	}
	return total;
}
