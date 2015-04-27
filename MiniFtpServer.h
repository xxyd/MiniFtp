#ifndef _MINIFTPSERVER_H
#define _MINIFTPSERVER_H

#include "GlobalDefine.h"
#include <winsock2.h>
using namespace std;

//�̺߳����Ĳ���
struct threadArg{
	SOCKET	sServer; //�������������׽���
	sockaddr_in saClient; //�ͻ��˵ĵ�ַ��Ϣ
};

//�̺߳��������ڴ���ͻ��˵ĸ�������
DWORD WINAPI ThreadFunc(LPVOID pTArg);

//MiniFtp��������
class FtpServer{
private:
	SOCKET sListen; //�������������������׽���
	threadArg *pTArg; //�̲߳���

public:
	FtpServer(); 
	bool Initial(); //��ʼ���������������׽���
	void Serve(); //��������
};

//�������߳���
class ServerThread{
private:
	SOCKET sServer; //�������������������׽���
	sockaddr_in saClient; //�ͻ��˵ĵ�ַ
	SOCKET sData; //�������������������׽���
	CmdPacket cmd; //�������ݰ�
	RspnsPacket rspns; //Ӧ�����ݰ�

	bool SendRspns(); //����Ӧ�����ݰ�
	bool ReceiveCmd(); //�����������ݰ�
	bool InitialDataSocket(); //��ʼ���������������׽���
	bool ExecCD(); //ִ��cdָ��
	bool ExecPWD(); //ִ��pwdָ��
	bool ExecPUT(); //ִ��putָ��
	bool ExecGET(); //ִ��getָ��
	bool ExecLS(); //ִ��lsָ��
	void ExecQUIT(); //ִ��quitָ��
	bool FileExist(); //�ж��ļ��Ƿ��Ѵ����뵱ǰĿ¼��
public:
	ServerThread(SOCKET s, sockaddr_in a); //���캯��
	void DoServer(); //ѭ��ִ�пͻ��˵�����
};

#endif