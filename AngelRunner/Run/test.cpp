#include <locale.h>
#include <tchar.h>
#include "angel.h"
#include "stringutil.h"
//��ϵͳ����ģ���������ģ��͸�ֵ����������ģ����������������߼����㡢����ȡֵ���������㡢�����㡢����ȡ����������ǰ���ַ���ֵ���Զ����������ã��������Ƿ��ر����ĵ�ַ��Ϊ�˸��㷺�Ĳ��������б���ȡֵ�����Ӿ��������������һ�Ŷ��ۡ�
//��ϵͳ�������ΪһԪ����Ͷ�Ԫ���㣬���ж�Ԫ������Ҫ�Ƚ������������ӷֱ���һԪ���㻯Ϊͳһ���������ٽ��ж�Ԫ���㡣����һ�㵥Ŀ�����б������Ա������ȡֵ������������ǲ�ͳһ�ģ���Ҫ�������ۡ�
//��ϵͳ��һԪ�������������ִ�У����ݡ���������Ա�����������м�����ȡֵ�����п����͡�
#include <stdio.h>
#include <stdlib.h>



int main(int argc ,char **argv)
{
	//int len;
	//char *pattern = towide("(a+?)",&len);
	//regelement reg = are_compile((wchar *)pattern);
	setlocale(LC_ALL,"");
	//wchar_t wstr[] = L"���Ǿ���ɶ";
	//wchar_t fomat[] = L"%s";
	//printf("%ls",wstr);
	shell(argc,argv);
	return 1;
}

