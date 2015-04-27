#ifndef _MINIFTPSERVER_H
#define _MINIFTPSERVER_H

#include "GlobalDefine.h"
#include <winsock2.h>
using namespace std;

//线程函数的参数
struct threadArg{
	SOCKET	sServer; //控制连接连接套接字
	sockaddr_in saClient; //客户端的地址信息
};

//线程函数，用于处理客户端的各种请求
DWORD WINAPI ThreadFunc(LPVOID pTArg);

//MiniFtp服务器类
class FtpServer{
private:
	SOCKET sListen; //服务器控制连接侦听套接字
	threadArg *pTArg; //线程参数

public:
	FtpServer(); 
	bool Initial(); //初始化控制连接侦听套接字
	void Serve(); //启动服务
};

//服务器线程类
class ServerThread{
private:
	SOCKET sServer; //服务器控制连接连接套接字
	sockaddr_in saClient; //客户端的地址
	SOCKET sData; //服务器数据连接连接套接字
	CmdPacket cmd; //命令数据包
	RspnsPacket rspns; //应答数据包

	bool SendRspns(); //发送应答数据包
	bool ReceiveCmd(); //接收命令数据包
	bool InitialDataSocket(); //初始化数据连接连接套接字
	bool ExecCD(); //执行cd指令
	bool ExecPWD(); //执行pwd指令
	bool ExecPUT(); //执行put指令
	bool ExecGET(); //执行get指令
	bool ExecLS(); //执行ls指令
	void ExecQUIT(); //执行quit指令
	bool FileExist(); //判断文件是否已存在与当前目录下
public:
	ServerThread(SOCKET s, sockaddr_in a); //构造函数
	void DoServer(); //循环执行客户端的请求
};

#endif