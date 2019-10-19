#ifndef socket_def
#define socket_def

#ifndef COMMON_LIB

//这里面定义与具体领域相关的
typedef struct object_socketnode{
	EXT_HEAD
	int errorno;
#ifdef WIN32
	SOCKET s;
#else
#endif
}*object_socket;

#define GETSOCKET(o) ((object_socket)o)

#endif

#define ISSOCKET(o) (GETEXT(o)->_type == socket_type)
#define SOCKETNAME "resource<socket>"


int initsock();
void uninitsocket();
void socketdealloc(object o);
object syssocket(object protocol);
object sysbind(object s,object host);
object syslisten(object s,object blog);
object sysaccept(object s,object from);
object sysconnect(object s,object to);
object sysrecv(object s);
object syssend(object s,object content);
object syssclose(object s);
object sysnetaddr(object hostname,object servername);
object sysnetname(object ip);
object syssocketopt(object sock,object opt);



#endif