#include "data.h"
#include "lib.h"
#include "execute.h"
#include "amem.h"

object_bytes initbytes(int len)
{
	//这是一个底层的内核函数，len代表的是字节数而非字符串的字符数
	object_bytes res;
	char *bs;
	int alloc = len + 1;
	if(alloc <= PAGEALLOCMIN)
	{
		res = (object_bytes)angel_alloc_block(sizeof(object_bytesnode)+alloc);
		bs = (char *)res+sizeof(object_bytesnode);
	}
	else
	{
		alloc *= 2;
		res = (object_bytes)angel_alloc_block(sizeof(object_bytesnode));
		bs = (char *)angel_alloc_page((object)res,alloc);  //多出一个0为为了后面的转化为字符串
		angel_sys_memset(bs,0,sizeof(char)*alloc);
	}
	res->bytes = bs;
	res->type = BYTES;
	res->len  = len;
	res->alloc_size = alloc;
	return res;
}
object_bytes initbytes(char *byte,int len)
{
	object_bytes bts = initbytes(len);
	angel_sys_memcpy(bts->bytes,byte,len);
	return bts;
}
void resizebytes(object_bytes ob)
{
	int oldalloc = ob->alloc_size;
	ob->alloc_size = ob->alloc_size * 3 / 2;
	char *newaddr = (char *)angel_alloc_page((object)ob,ob->alloc_size*sizeof(char));
	char *addr = ob->bytes;
	angel_sys_memset(newaddr,0,ob->alloc_size*sizeof(char));
	angel_sys_memcpy(newaddr,addr,oldalloc*sizeof(char));
	if(oldalloc <= PAGEALLOCMIN)  //表示在block中申请的
	{
		*ob->bytes = oldalloc;
		ob->osize -= oldalloc;
	}
	else
		angel_free_page(addr);
	ob->bytes = newaddr;
}




int appendbytes(object_bytes b,object_bytes b1)
{
	if(b->alloc_size < b->len + b1->len)
		resizebytes(b);
	angel_sys_memcpy(b->bytes,b1->bytes,b1->len);
	b->len += b1->len;
	return 1;
}
object_bytes concatbytes(object_bytes b,object_bytes b1)
{
	object_bytes res = initbytes(GETBYTES(b)->len+GETBYTES(b1)->len);
	angel_sys_memcpy(res->bytes,b->bytes,b->len);
	angel_sys_memcpy(res->bytes+b->len,b1->bytes,b1->len);
	return res;
}
object_bytes insertbytes(object_bytes b,object_bytes b1,int pos)
{
	char *s=b1->bytes;
	int i,j;
	int copynum;
	object_bytes res = initbytes(GETBYTES(b)->len+GETBYTES(b1)->len);
	angel_sys_memcpy(res->bytes,b->bytes,pos+1);
	angel_sys_memcpy(res->bytes+pos+1,b1->bytes,b1->len);
	angel_sys_memcpy(res->bytes+pos+1+b1->len,b->bytes+pos+1,b->len-pos-1);
	return res;
}
object_bytes bytesrepeat(object_bytes b,int count)
{
	int i,copynum;
	int eachlen = b->len;
	object_bytes res = initbytes(b->len*count);
	for(i=0; i<count; i++)
	{
		angel_sys_memcpy(res->bytes+eachlen*i,b->bytes,eachlen);
	}
	*(uint16_t *)&res->bytes[b->len*count] = 0;
	return res;
}
object_bytes copybytes(object_bytes b)
{
	object_bytes res = initbytes(b->len);
	angel_sys_memcpy(res->bytes,b->bytes,b->len);
	return res;
}
object_bytes slicebytes(object_bytes by,object_range range)
{
	int e,b,n,step;
	RANGETOSLICE(range,by->len-1,step,b,e,n);
	int byteslen = n;
	object_bytes res = initbytes(byteslen);
	char *in = res->bytes;
	char *out = (char *)by->bytes;
	for(int i = 0; i < n; i++)
	{
		int index = b + i*step;
		*in++ = out[index];
	}
	return res;
}
/*
字节数组库
*/
object syssize_bytes(object o)
{
	return (object)initinteger(GETBYTES(o)->len);
}
object sysdecode_bytes(object format, object o)
{
	if(ISDEFAULT(format))
	{

	}
	return GETNULL;
}
object sysstr_bytes(object o)
{
	return (object)initstring(GETBYTES(o)->bytes);
}