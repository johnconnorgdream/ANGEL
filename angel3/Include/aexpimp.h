#ifndef aexpimp_def
#define aexpimp_def

#ifdef _cplusplus
extern "c"{
#endif


#ifdef WIN32
	#ifdef ANGEL_BUILDIN
		#define ANGEL_ENV(TYPE) extern __declspec(dllexport) TYPE
		#define ANGEL_API extern __declspec(dllexport)
	#else
		#define ANGEL_ENV(TYPE) extern __declspec(dllimport) TYPE
		#define ANGEL_API extern __declspec(dllimport)
		#define ANGEL_LIB extern __declspec(dllexport)
	#endif
#else

#endif

#ifdef _cplusplus
}
#endif
#endif