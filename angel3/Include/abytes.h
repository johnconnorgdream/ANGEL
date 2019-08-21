#ifndef bytes_def
#define bytes_def

#ifdef _cplusplus
extern "c"{
#endif


typedef struct object_bytesnode{
	BASEDEF;
	char *bytes;
	int len,alloc_size;
}*object_bytes;



void resizebytes(object_bytes ob);


object_bytes initbytes(int len);
object_bytes concatbytes(object_bytes b,object_bytes b1);
object_bytes insertbytes(object_bytes b,object_bytes b1,int pos);
object_bytes bytesrepeat(object_bytes b,int count);
object_bytes copybytes(object_bytes b);
object_bytes slicebytes(object_bytes by,object_range range);

object syssize_bytes(object o);



#ifdef _cplusplus
}
#endif
#endif