#include "data.h"
#include "lib.h"
#include "amem.h"
#include "execute.h"

object_range initrange(long b,long e,long step)
{
	object_range res = (object_range)angel_alloc_block(APPLYSIZE(sizeof(object_rangenode)));
	res->type = RANGE;
	res->begin = b;
	res->step = b <= e ? step : -step;  //公差
	//初始化项数
	res->end = e;
	res->n = (e-b)/res->step+1;
	return res;
}
object_iterator inititerator(object base)
{
	ASCREF(base);
	object_iterator res = (object_iterator)angel_alloc_block(APPLYSIZE(sizeof(object_iteratornode)));
	res->type = ITERATOR;
	res->base = base;
	res->pointer = 0;
	return res;
}