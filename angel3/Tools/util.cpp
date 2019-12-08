#include <stdlib.h>
#include <string.h>
#include "data.h"
#include "util.h"
#include "execute.h"
#include "shell.h"
#include "amem.h"
__forceinline int encode_format(char *buffer,char **decode_entry)
{
	//优先判断是否是utf8或unicode，如果不是默认按
	char *p = buffer;
	int encodetype;
	*decode_entry = p;
	if((uchar)*p == 0xfe || (uchar)*p == 0xff)  //带BOM的utf-8
	{
		*decode_entry += 2;
		return UTF8_TYPE; //utf-8 with BOM
	}
	else
	{
		if(*p > 0)  
		{
			if(*p > 0x7f)
			{
				if(((*p) & 0xE0) == 0xC0 || ((*p) & 0xF0) == 0xE0)
					return UTF8_TYPE; //utf-8
				else
					return UNICODE_TYPE; //
			}
			return UTF8_TYPE; //utf-8
		}
		else
		{
			return NATIVE_TYPE; //native编码
		}
	}
}
int64_t digitstimes(int64_t l)
{
	int i=1;
	int64_t base=10;
	while(l/base)
	{
		i++;
		base*=10;
	}
	return base;
}
int digits(int64_t l)
{
	int i=1;
	int64_t base=10;
	while(l/base)
	{
		i++;
		base*=10;
	}
	return i;
}

int64_t toint(char *a)
{
	int64_t total=0,i=1;
	int singal=1;  //singal表示该字符表示的正负
	char *p;
	if(*a=='-')	
	{
		p=++a;
		singal=-1;
	}
	else
		p=a;
	while(*p) p++;//获得的是最后的0
	while(p--!=a)
	{
		total=total+(*p-'0')*i;
		i*=10;
	}
	if(singal==-1)  
		total=-total;	
	return total;
}
double todecimal(char *a)
{
	long temp = toint(a);
	long bits = digitstimes(temp);
	return (double)temp/bits;
}
double tofloat(char *a)
{
	char *p = a;
	while(*p) 
	{
		if(*p == '.')
		{
			*p = 0;
			int64_t integer = toint(a);
			double decimal = todecimal(p+1);
			return integer + decimal;
		}
		p++;//获得的是最后的0
	}
	return toint(a);
}
char* tointchar(int64_t a)
{
	char *ret,*p,*temp,*q;
	int64_t b;
	int digit = digits(a);
	p=(char *)angel_sys_malloc(digit+1);
	ret=(char *)angel_sys_malloc(digit+1);
	q=ret;
	temp=p;
	int radix=10; //以十进制为例
	if(a>=0)
		b=a;
	else
		b=-a;
	if(b==0)
		*p++='0';
	while(b)
	{
		int i;
		i=b%radix;//每次取最后一位
		b/=radix; //每次截去最后一位
		*p++=i+'0';
	}
	if(a<0)
		*p++='-';
	while(p>temp)
		*q++=*--p;
	*q=0;
	free(temp);
	return ret;
}



char *getstrcat(char *des,char *src)
{
	char *res=(char *)angel_sys_malloc(strlen(des)+strlen(src)+1),*p;   //字符串加法之所以慢是这里每次都要计算strlen()
	p=res;
	while(*des) *p++=*des++;
	while(*src) *p++=*src++;
	*p=0;
	return res;
}
char *tonative(object_string s)  //UNICODE转化为NATIVE
{
	char *res = (char *)angel_sys_calloc(s->len+1,sizeof(char));
	int gbksize = UnicodeToGbk((uint16_t *)s->s,res,s->len);
	return res;
}
char *tomult(object_string s,int *size)  //将Unicode转化为utf8
{
	char *res = (char *)angel_sys_calloc(s->len*3/2+1,sizeof(char));
	int len = UnicodeToUtf8(s->s,res);
	if(size)
		*size = len;
	return res;
}
char *towide_n(char *s,int *reslen) //NATIVE->UNICODE
{
	int len = *reslen;
	char *test = (char *)angel_sys_calloc(len*2+2,sizeof(char));
	*reslen = GbkToUnicode(s,(uint16_t *)test,len);
	return test;
}
char *towide(char *s,int *reslen)  //UTF8->UNICODE
{
	int len = *reslen;
	char *test = (char *)angel_sys_calloc(len*2+2,sizeof(char));
	int widelen = Utf8ToUnicode(s,test);
	if(widelen == -1)
		widelen = GbkToUnicode(s,(uint16_t *)test,len);
	*reslen = widelen;
	return test;
}
