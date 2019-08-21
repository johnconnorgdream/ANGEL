#ifndef arange_def
#define arange_def
#ifdef _cplusplus
extern "c"{
#endif



typedef struct object_rangenode{
	BASEDEF;
	int begin,end,step,n;
}*object_range;
typedef struct object_slicenode{
	BASEDEF;
	object_range range;
	object base;
}*object_slice;

object_slice initslice(object base,object_range range);

#ifdef _cplusplus
}
#endif
#endif