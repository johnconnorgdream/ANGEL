#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib,"Ws2_32.lib")
#endif

#include "_common.h"
#include "_socket.h"


int initsock();

extern ext_type socket_type;
void socketdealloc(object o)
{
	object_socket os = GETSOCKET(o);
#ifdef WIN32
	closesocket(os->s);
#else
#endif
}


int initsock()
{
#ifdef WIN32
	int ret;
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested=MAKEWORD(2, 2); //希望使用的WinSock DLL 的版本  
    ret=WSAStartup(wVersionRequested, &wsaData);
	return ret;
#else
#endif
}
void uninitsocket()
{
#ifdef WIN32
	WSACleanup();
#else
#endif
}
int getsockaddr(sockaddr_in *addr,object host)
{
	object_string ip;
	object_int port;
	if(ISSTR(GETLIST(host)->item[0]))
	{
		ip = GETSTR(GETLIST(host)->item[0]);
	}
	else
	{
		angel_error("主机ip必须是字符串类型！");
		return 0;
	}
	if(ISINT(GETLIST(host)->item[1]))
	{
		port = (object_int)GETLIST(host)->item[1];
	}
	else
	{
		angel_error("主机port必须是整形！");
		return 0;
	}
	char *s_ip = tomult(ip);

	addr->sin_addr.S_un.S_addr=inet_addr(s_ip);
	free(s_ip);
	addr->sin_port=htons(port->val);
	addr->sin_family=AF_INET;

	return 1;
}
object syssocket(object protocol)
{
	//可扩展创建非/阻塞模式
	int type;
	object_socket res = (object_socket)initext(APPLYSIZE(sizeof(object_socketnode)));
	res->_type = socket_type;
	
	ARG_CHECK(protocol,STR,"socket",1);

	if(comparestring(GETSTR(protocol),CONST("tcp"))==0)
		type=SOCK_STREAM;
	else if(comparestring(GETSTR(protocol),CONST("tcp"))==0)
		type=SOCK_DGRAM;
	else
	{
		angel_error("protocol should be either tcp or udp");
		return GETNULL;
	}
	SOCKET s=socket(AF_INET,type,0);
	if(s == INVALID_SOCKET)
	{
		return GETNULL;
	}
	res->s = s;
	return (object)res;
}
object sysbind(object s,object host)
{
	ARG_CHECK(s,SOCKET,"bind",1);
	ARG_CHECK(host,LIST,"bind",2);

	sockaddr_in addr;
	if(!getsockaddr(&addr,host))
		return GETFALSE;
	
	if(-1 == bind(GETSOCKET(s)->s,(sockaddr *)&addr,sizeof(sockaddr_in)))
		return GETFALSE;
	return GETTRUE;
}
object syslisten(object s,object blog)
{
	int bcount = 10;

	ARG_CHECK(s,SOCKET,"listen",1);
	if(!ISDEFAULT(blog))
	{
		ARG_CHECK(blog,INT,"listen",2);
		bcount = GETINT(blog);
	}
	//每个平台
	if(listen(GETSOCKET(s)->s,bcount)==-1)
		return GETFALSE;
	else
		return GETTRUE;
}
object sysaccept(object s,object from)
{

	ARG_CHECK(s,SOCKET,"accept",1);
	ARG_CHECK(from,LIST,"accept",2);

	int addrlen=sizeof(sockaddr);
	sockaddr_in addr;

	SOCKET saccept=accept(GETSOCKET(s)->s,(sockaddr *)&addr,&addrlen);

	clearlist(GETLIST(from));
	_addlist(GETLIST(from),(object)initstring(inet_ntoa(addr.sin_addr)));
	_addlist(GETLIST(from),(object)initinteger(addr.sin_port));
	
	if(saccept==INVALID_SOCKET)
	{
		return GETNULL;
	}

	object_socket res = (object_socket)initext(APPLYSIZE(sizeof(object_socketnode)));
	res->_type = socket_type;
	res->s = saccept;

	return (object)res;
}
object sysconnect(object s,object to)
{
	int ret;
	sockaddr_in addr;
	ARG_CHECK(s,SOCKET,"connect",1);
	ARG_CHECK(to,LIST,"connect",2);


	if(!getsockaddr(&addr,to))
		return GETFALSE;

	ret = connect(GETSOCKET(s)->s,(sockaddr *)&addr,sizeof(sockaddr));
	if(ret < 0)
		return GETFALSE;
	else
		return GETTRUE;
}
object sysrecv(object s)
{
#define RECV_BUF_SIZE 128
	int ret;
	ARG_CHECK(s,SOCKET,"recv",1);

	char recvs[RECV_BUF_SIZE];
	object_list recv_set = initarray();
	int recvflag = 1;
	object_string res;
	int totalsize = 0;
	while(recvflag)
	{
		ret = recv(GETSOCKET(s)->s,recvs,RECV_BUF_SIZE,0);
		res = copystring_str(recvs,ret);
		if(ret <= 0)
		{
			DECREF(recv_set);
			return GETNULL;
		}
		totalsize += ret;
		if(ret < RECV_BUF_SIZE) //表示已经取完了
		{
			recvflag = 0;
		}
		_addlist(recv_set,(object)res);
	}
	res = initstring(totalsize);
	res->len = 0;
	joinstring(res,(object)recv_set);
	res->len = totalsize;
	DECREF(recv_set);
	return (object)res;
}
object syssend(object s,object content)
{
	int ret,sendsize;
	ARG_CHECK(s,SOCKET,"send",1);
	ARG_CHECK(content,STR,"send",2);
	int totallen = GETSTR(content)->len;
	while(1){
		sendsize = send(GETSOCKET(s)->s,GETSTR(content)->s,GETSTR(content)->len,0);
		if(sendsize < 0)
			return GETFALSE;
		else
		{
			ret += sendsize;
			if(ret >= totallen)
				break ;
		}
	}
	return GETTRUE;
}
object syssclose(object s)
{
	int ret ;
	ARG_CHECK(s,SOCKET,"sclose",1);

	ret = closesocket(GETSOCKET(s)->s);
	if(ret < 0)
		return GETFALSE;
	else
		return GETTRUE;
}
object sysnetaddr(object hostname,object servername)
{
#define HOST_LEN 100
	char *_hostname = NULL;
	char *_servername = NULL;
	if(!ISDEFAULT(hostname))
	{
		ARG_CHECK(hostname,STR,"netaddr",1);
		if(comparestring(GETSTR(hostname),CONST("localhost")) == 0) 
		{
			goto localaddr;
		}
		else
		{
			_hostname = tomult(GETSTR(hostname));
		}
		if(!ISDEFAULT(servername))
		{
			ARG_CHECK(servername,STR,"netaddr",2);
			_hostname = tomult(GETSTR(hostname));
			_servername = tomult(GETSTR(servername));
		}
	}
	else
	{
localaddr:
		_hostname = (char *)calloc(HOST_LEN,sizeof(char));
		gethostname(_hostname,HOST_LEN);
	}
	addrinfo hints, *res;
	memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;     /* Allow IPv4 */
    hints.ai_flags = AI_PASSIVE;/* For wildcard IP address */
    hints.ai_protocol = 0;         /* Any protocol */
    hints.ai_socktype = SOCK_STREAM;

	
	object_list ret = (object_list)GETNULL;
	if(getaddrinfo(_hostname,_servername,&hints,&res) != 0)
	{
		goto ret;
	}
	
	ret = initarray();
	for(addrinfo * p = res; p; p=p->ai_next)
	{
		object_list item = initarray(2);
		_addlist(ret,(object)item);
		SOCKADDR_IN *addr=(SOCKADDR_IN *)p->ai_addr;
		_addlist(item,(object)initstring(inet_ntoa(addr->sin_addr)));
		_addlist(item,(object)initinteger(ntohs(addr->sin_port)));
	}
ret:
	free(_hostname);
	if(_servername) free(_servername);
	return (object)ret;
}
object sysnetname(object ip)
{
#define HOST_LEN 100
	char *hostname = NULL;

	if(!ISDEFAULT(ip))
	{
		ARG_CHECK(ip,STR,"netname",1);
		if(comparestring(GETSTR(ip),CONST("localhost")) == 0) 
		{
			goto localname;
		}
		else
		{
			char *_ip = tomult(GETSTR(ip));
			in_addr ina;
			ina.S_un.S_addr = inet_addr(_ip);
			free(_ip);
			hostent *h = gethostbyaddr((char *)&ina,4,AF_INET);
			if(h)
				return (object)initstring(h->h_name);
			else
				return GETNULL;
		}
	}
	else
	{
localname:
		hostname = (char *)calloc(HOST_LEN,sizeof(char));
		gethostname(hostname,HOST_LEN);
		object_string ret = initstring(hostname);
		free(hostname);
		return (object)ret;
	}
}
object syssocketopt(object sock,object opt)
{

	return GETNULL;
}

/*

char **_getnameinfo(hostinfo info)
{
	SOCKADDR_IN addr=getnetaddr(info.ip,info.port);
	char *hostname=(char *)calloc(NI_MAXHOST,sizeof(char));
	char *servername=(char *)calloc(NI_MAXSERV,sizeof(char));
	if(getnameinfo((SOCKADDR *)&addr,sizeof(SOCKADDR),hostname,NI_MAXHOST,servername,
		NI_MAXSERV,0)!=0)
		return NULL;
	char **ret=(char **)calloc(2,sizeof(char *));
	ret[0]=hostname;
	ret[1]=servername;
	return ret;

}
int _recvfrom(SOCKET s,char *buf,int size,hostinfo *info)
{
	int ret,recvlen;
	sockaddr_in from;
	recvlen=sizeof(sockaddr_in);
	ret = recvfrom(s,buf,size,0,(sockaddr *)&from,&recvlen);
	info->ip=inet_ntoa(from.sin_addr);
	info->port=ntohs(from.sin_port);


	if(ret<=0)
		return WSAGetLastError();
	else
		return 0;
}
int _sendto(SOCKET s,char *buf,hostinfo info)
{
	int ret;
	sockaddr_in to;
	to=getnetaddr(info.ip,info.port);
	ret = sendto(s,buf,sizeof(buf)+1,0,(sockaddr *)&to,sizeof(to));
	if(ret<=0)
		return WSAGetLastError();
	else
		return 0;
}

int socket_close(SOCKET s,char *type)
{
	int ret=closesocket(s);
	if(ret==-1)
		return WSAGetLastError();
	else
		return 0;
}
int _getsocktype(SOCKET s,int *error)
{
	int optval,optlen=4;
	int ret=getsockopt(s,SOL_SOCKET,SO_TYPE,(char *)&optval,&optlen);

	if(ret==-1)
		*error = WSAGetLastError();
	else
		*error = 0;
	return optval;
}
char *_gethostname()
{
	char *name=(char *)calloc(NI_MAXHOST,sizeof(char));
	int namelen=NI_MAXHOST;
	if(initwinsock()!=0)
		goto error;
	int flag=gethostname(name,namelen);
	if(flag!=0)
		goto error;
	else
		return name;
error:
	free(name);
	return NULL;
}
*/