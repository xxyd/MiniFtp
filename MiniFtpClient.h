#ifndef _MINIFTPCLIENT_H
#define _MINIFTPCLIENT_H

#include "GlobalDefine.h"
#include <winsock2.h>
#include <fstream>
using namespace std;

//MiniFtp客户端类
class FtpClient{
private:
	SOCKET sClient; //客户端控制连接连接套接字
	SOCKET sdListen, sdClient; //客户端数据连接侦听套接字和连接套接字
	CmdPacket cmd; //命令数据包
	RspnsPacket rspns; //应答数据包

	bool SendCmd(); //发送命令数据包
	bool ReceiveRspns(); //接收应答数据包
	bool InitialDataSocket(); //初始化数据连接套接字的侦听套接字
	bool DoCD(); //执行cd指令
	void DoLCD(); //执行lcd指令
	bool DoPWD(); //执行pwd指令
	bool DoPUT(); //执行put指令
	bool DoGET(); //执行get指令
	bool DoLS(); //执行ls指令
	void DoLLS(); //执行lls指令
	void DoQUIT(); //执行quit指令
	bool FileExist(); //判断文件是否已存在
	void ShowErr(); //显示错误信息
public:
	FtpClient(); //构造函数
	bool Initial(char *IP); //使用服务器的IP地址初始化控制连接连接套接字
	void Work(); //循环执行用户输入的指令
};

#endif