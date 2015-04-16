#ifndef _MINIFTPSERVER_H
#define _MINIFTPSERVER_H

#include "GlobalDefine.h"
#include <winsock2.h>
#include <fstream>
using namespace std;

//创建线程时传递的数据结构，内含控制连接套接字和客户端地址信息：
// struct _threadArg {
// 	SOCKET	sServer;
// 	sockaddr_in saClient;
// } threadArg;
struct threadArg{
	SOCKET	sServer;
	sockaddr_in saClient;
};

DWORD WINAPI ThreadFunc(LPVOID pTArg);

class FtpServer{
private:
	SOCKET sListen;
	threadArg *pTArg;
	
public:
	FtpServer();
	bool Initial();
	void Serve();
};

class ServerThread{
private:
	SOCKET sServer;
	sockaddr_in saClient;
	SOCKET sData;
	CmdPacket cmd;
	RspnsPacket rspns;

	bool SendRspns();
	bool ReceiveCmd();
	bool ExecCmd();
	bool InitialDataSocket();
	bool ExecCD();
	bool ExecPWD();
	bool ExecPUT();
	bool ExecGET();
	bool ExecLS();
	void ExecQUIT();
	bool FileExist();
public:
	ServerThread(SOCKET s, sockaddr_in a);
	void DoServer();
};

#endif