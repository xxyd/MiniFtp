#ifndef _MINIFTPCLIENT_H
#define _MINIFTPCLIENT_H

#include "GlobalDefine.h"
#include <winsock2.h>
#include <fstream>
using namespace std;

class FtpClient{
private:
	SOCKET sClient;
	SOCKET sdListen, sdClient;
	CmdPacket cmd;
	RspnsPacket rspns;
	
	bool SendCmd();
	bool ReceiveRspns();
	bool InitialDataSocket();
	bool DoCD();
	void DoLCD();
	bool DoPWD();
	bool DoPUT();
	bool DoGET();
	bool DoLS();
	void DoQUIT();
	bool FileExist();
	void ShowErr();
public:
	FtpClient();
	bool Initial(char *IP);
	void Work();
};

#endif