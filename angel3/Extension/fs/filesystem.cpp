#ifdef WIN32
#include <Windows.h>
#else
#endif

#include "_common.h"

#define lhtolonglong(low,high) ((high << 32) | low)

int WCHARLEN(WCHAR *ws)
{
	int count = 0;
	while(*ws){
		count++;
		ws++;
	}
	return count;
}
object_dict _getfileinfo(char* pathname)
{
	object_dict ret = initdictionary();
#ifdef WIN32
	WIN32_FILE_ATTRIBUTE_DATA fAttrData;
	WCHAR *win_pathname = (WCHAR *)pathname;
	GetFileAttributesEx(win_pathname,GetFileExInfoStandard,&fAttrData);
	if(fAttrData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		_adddict(ret,(object)CONST("attr"),(object)CONST("dir"));
	}
	else
	{
		_adddict(ret,(object)CONST("attr"),(object)CONST("file"));
	}
	_adddict(ret,(object)CONST("a_time"),(object)initinteger(lhtolonglong(fAttrData.ftLastAccessTime.dwLowDateTime,fAttrData.ftLastAccessTime.dwLowDateTime)));
	_adddict(ret,(object)CONST("c_time"),(object)initinteger(lhtolonglong(fAttrData.ftCreationTime.dwLowDateTime,fAttrData.ftCreationTime.dwLowDateTime)));
	_adddict(ret,(object)CONST("w_time"),(object)initinteger(lhtolonglong(fAttrData.ftLastWriteTime.dwLowDateTime,fAttrData.ftLastWriteTime.dwLowDateTime)));
	_adddict(ret,(object)CONST("size"),(object)initinteger(lhtolonglong(fAttrData.nFileSizeLow,fAttrData.nFileSizeHigh)));
#endif
	return ret;
}
object_list _getls(char* pathname)
{
	object_list ret = initarray();
#ifdef WIN32
	WIN32_FIND_DATA FindFileData;
	WCHAR *win_pathname = (WCHAR *)pathname;
	HANDLE  hListFile = FindFirstFile(win_pathname, &FindFileData);
	if (INVALID_HANDLE_VALUE == hListFile)
	{
		return ret;
	}
	do
	{
		object_dict item = initdictionary();
		
		if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			_adddict(item,(object)CONST("attr"),(object)CONST("dir"));
		}
		else
		{
			_adddict(item,(object)CONST("attr"),(object)CONST("file"));
		}
		_adddict(item,(object)CONST("name"),(object)copystring_str((char *)FindFileData.cFileName,WCHARLEN(FindFileData.cFileName)*2));
		_adddict(item,(object)CONST("a_time"),(object)initinteger(lhtolonglong(FindFileData.ftLastAccessTime.dwLowDateTime,FindFileData.ftLastAccessTime.dwLowDateTime)));
		_adddict(item,(object)CONST("w_time"),(object)initinteger(lhtolonglong(FindFileData.ftLastWriteTime.dwLowDateTime,FindFileData.ftLastWriteTime.dwLowDateTime)));
		_adddict(item,(object)CONST("c_time"),(object)initinteger(lhtolonglong(FindFileData.ftCreationTime.dwLowDateTime,FindFileData.ftCreationTime.dwLowDateTime)));
		_adddict(item,(object)CONST("size"),(object)initinteger(lhtolonglong(FindFileData.nFileSizeLow,FindFileData.nFileSizeHigh)));
		_addlist(ret,(object)item);
	}
	while (FindNextFile(hListFile, &FindFileData));
	FindClose(hListFile);
#else

#endif
	return ret;
}