//其中osize表示的是字节数，如果是从系统申请的（大于65535这直接放在列表中进行管理）
#ifndef number_def
#define number_def
#ifdef _cplusplus
extern "c"{
#endif


#include "typeconfig.h"

typedef struct object_intnode{
	BASEDEF
	int64_t val;
}*object_int;  //8个字节为内存管理的最小单位
typedef struct object_floatnode{
	BASEDEF
	float64_t val;
}*object_float;  //8个字节为内存管理的最小单位

object _stacktoheap(object i);
object_int initinteger(int64_t val);
object_float initfloat(float64_t val);

#ifdef _cplusplus
}
#endif
#endif