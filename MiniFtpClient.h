#ifndef _MINIFTPCLIENT_H
#define _MINIFTPCLIENT_H

#include "GlobalDefine.h"
#include <winsock2.h>
#include <fstream>
using namespace std;

//MiniFtp�ͻ�����
class FtpClient{
private:
	SOCKET sClient; //�ͻ��˿������������׽���
	SOCKET sdListen, sdClient; //�ͻ����������������׽��ֺ������׽���
	CmdPacket cmd; //�������ݰ�
	RspnsPacket rspns; //Ӧ�����ݰ�

	bool SendCmd(); //�����������ݰ�
	bool ReceiveRspns(); //����Ӧ�����ݰ�
	bool InitialDataSocket(); //��ʼ�����������׽��ֵ������׽���
	bool DoCD(); //ִ��cdָ��
	void DoLCD(); //ִ��lcdָ��
	bool DoPWD(); //ִ��pwdָ��
	bool DoPUT(); //ִ��putָ��
	bool DoGET(); //ִ��getָ��
	bool DoLS(); //ִ��lsָ��
	void DoLLS(); //ִ��llsָ��
	void DoQUIT(); //ִ��quitָ��
	bool FileExist(); //�ж��ļ��Ƿ��Ѵ���
	void ShowErr(); //��ʾ������Ϣ
public:
	FtpClient(); //���캯��
	bool Initial(char *IP); //ʹ�÷�������IP��ַ��ʼ���������������׽���
	void Work(); //ѭ��ִ���û������ָ��
};

#endif