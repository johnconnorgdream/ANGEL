//����osize��ʾ�����ֽ���������Ǵ�ϵͳ����ģ�����65535��ֱ�ӷ����б��н��й���
#ifndef number_def
#define number_def
#ifdef _cplusplus
extern "c"{
#endif


#include "typeconfig.h"

typedef struct object_intnode{
	BASEDEF
	int64_t val;
}*object_int;  //8���ֽ�Ϊ�ڴ�������С��λ
typedef struct object_floatnode{
	BASEDEF
	float64_t val;
}*object_float;  //8���ֽ�Ϊ�ڴ�������С��λ

object _stacktoheap(object i);
object_int initinteger(int64_t val);
object_float initfloat(float64_t val);

#ifdef _cplusplus
}
#endif
#endif