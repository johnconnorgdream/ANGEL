#include <time.h>
#include <Windows.h>
#include "_common.h"

void _sleepe(int count,wchar_t *unit)
{
	double multisecond;
	if(wcscmp(unit,L"s")==0)
		multisecond=count*1000;
	else if(wcscmp(unit,L"us")==0)
		multisecond=count/1000;
	else if(wcscmp(unit,L"ns")==0)
		multisecond=count/1000000;
	else
		multisecond=count;
	Sleep(multisecond);
}
double _clock(wchar_t *unit)
{
	double dida=(double)clock()/(double)CLOCKS_PER_SEC;
	if(wcscmp(unit,L"ms")==0)
		dida*=1000;
	else if(wcscmp(unit,L"us")==0)
		dida*=1000000;
	else if(wcscmp(unit,L"ns")==0)
		dida*=1000000000;
	return dida;
}

object sysclock(object unit)
{
	wchar_t * _unit;
	if(ISDEFAULT(unit))
	{
		_unit = L"ms";
	}
	else
	{
		ARG_CHECK(unit,STR,"clock",1);
		_unit = (wchar_t *)GETSTR(unit)->s;
	}
	int ms = _clock(_unit);
	return (object)initinteger(ms);
}
object syssleep(object count,object unit)
{
	wchar_t * _unit;
	ARG_CHECK(count,INT,"sleep",1);
	if(ISDEFAULT(unit))
	{
		_unit = L"ms";
	}
	else
	{
		ARG_CHECK(unit,STR,"sleep",2);
		_unit = (wchar_t *)GETSTR(unit)->s;
	}
	_sleepe(GETINT(count),_unit);
	return GETNULL;
}