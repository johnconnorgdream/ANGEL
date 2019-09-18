#include <stdlib.h>
#include "data.h"
#include "stringutil.h"
#include "lib.h"
/*
kmpģʽƥ��
next���������̱���Ҳ��ƥ�����
k = next[i]��ʾ����kʹ��str[0..i-1]��str[0..k-1] == str[i-k..i-1]
Ŀ���Ǽ���str[0..i-1]��next[1..i-1]��֮���Դ�1��ʼ
*/
void calcnext(int16_t *pattern,int *next)  //next�����ʼ����Ϊ0
{
	int i,j;
	i = 1;
	j = 0;
	while(pattern[i])
	{
		if(pattern[i] == pattern[j])
		{
			j++;
			next[i] = j;
			i++;
		}
		else
		{
			if(j > 0)
				j = next[j-1];
			else //j==0
			{
				i++;
			}
		}
	}
}

int *next;
int kmp(wchar * s_s1,wchar *s_s2,int begin,int end,int patternlen)
{
	int i = begin,j = 0;

	while(i <= end)
	{
		if(s_s1[i] == s_s2[j])
		{
			if(j == patternlen-1)
			{
				return i;
				//addlist(ret,(object)initinteger(i - patternlen+1));
				//j = next[j];
			}
			else
				j++;
			i++;
		}
		else
		{
			if(j > 0)
				j = next[j-1];
			else
				i++;
		}
	}
	return i;
}
object strfind(wchar * s,wchar *pattern,int begin,int end,int patternlen)
{
	next = (int *)calloc(patternlen,sizeof(int));
	calcnext(pattern,next);
	int res = kmp(s,pattern,begin,end,patternlen);
	return (object)initinteger(res - patternlen+1);
}
object strfindall(wchar * s,wchar *pattern,int begin,int end,int patternlen)
{
	next = (int *)calloc(patternlen,sizeof(int));
	calcnext(pattern,next);
	object_list ret = initarray();
	int i = begin;
	while(i <= end)
	{
		i = kmp(s,pattern,i,end,patternlen);
		addlist(ret,(object)initinteger(i));
		i++;
	}
	return (object)ret;
}