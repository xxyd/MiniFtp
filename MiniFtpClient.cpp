#include "MiniFtpClient.h"
#include <iostream>
using namespace std;

FtpClient::FtpClient(){}

//ʹ�÷�������IP��ַ��ʼ���������������׽���
bool FtpClient::Initial(char *IP){
	struct sockaddr_in saServer;
	WORD wVersionRequested;
	WSADATA wsaData;
	int ret;
	//Winsock��ʼ��
	wVersionRequested = MAKEWORD(2, 2);
	ret = WSAStartup(wVersionRequested, &wsaData);
	if (ret != 0){
		cout << "WSAStartup failed!" << endl;
		return false;
	}
	//ȷ��Winsock֧��2.2�汾
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2){
		cout << "Invalid Winsock version!" << endl;
		WSACleanup();
		return false;
	}
	//�����������������׽��֣�ʹ��TCPЭ��
	sClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sClient == INVALID_SOCKET){
		cout << "Create control socket failed!" << endl;
		WSACleanup();
		return false;
	}
	//������������ַ��Ϣ
	saServer.sin_family = AF_INET;
	saServer.sin_port = htons(CMD_PORT);
	saServer.sin_addr.S_un.S_addr = inet_addr(IP);
	// �������ӷ�����
	ret = connect(sClient, (struct sockaddr *)&saServer, sizeof(saServer));
	if (ret == SOCKET_ERROR){
		cout << "Connect failed! Error code: " << WSAGetLastError() << endl;
		closesocket(sClient);
		WSACleanup();
		return false;
	}
	//���ӳɹ�����շ�������Ӧ�����ݰ�����ʾӦ�����ݰ�������
	if (!ReceiveRspns())
		return false;
	cout << rspns.text;
	return true;
}

//ִ���û���ָ��
void FtpClient::Work(){
	char input[20];

	// ѭ����ȡ�û������벢��������ִ����Ӧ�ĳ�Ա����ֱ���û�����quitָ��������ӳ���
	while (true){
		cin >> input;
		if (input[0] == 'c' && input[1] == 'd' && input[2] == 0){
			if (!DoCD())
				break;
		}
		else if (input[0] == 'l' && input[1] == 'c' && input[2] == 'd' && input[3] == 0){
			DoLCD();
		}
		else if (input[0] == 'p' && input[1] == 'w' && input[2] == 'd' && input[3] == 0){
			if (!DoPWD())
				break;
		}
		else if (input[0] == 'p' && input[1] == 'u' && input[2] == 't' && input[3] == 0){
			if (!DoPUT())
				break;
		}
		else if (input[0] == 'g' && input[1] == 'e' && input[2] == 't' && input[3] == 0){
			if (!DoGET())
				break;
		}
		else if (input[0] == 'l' && input[1] == 's' && input[2] == 0){
			if (!DoLS())
				break;
		}
		else if (input[0] == 'l' && input[1] == 'l' && input[2] == 's' && input[3] == 0){
			DoLLS();
		}
		else if (input[0] == 'q' && input[1] == 'u' && input[2] == 'i' && input[3] == 't' && input[4] == 0){
			DoQUIT();
			break;
		}
		else{
			cout << "Unsupported command!" << endl;
		}
	}
	//����ǰ�رտ������������׽���
	closesocket(sClient);
	WSACleanup();
}

//�����������ݰ���������
bool FtpClient::SendCmd(){
	char *pCmd = (char *)&cmd;
	int ret = send(sClient, pCmd, sizeof(CmdPacket), 0);
	if (ret == SOCKET_ERROR){
		cout << "Send command failed!" << WSAGetLastError() << endl;
		return false;
	}
	return true;
}

//�ӷ���������Ӧ�����ݰ�
bool FtpClient::ReceiveRspns(){
	int ret;
	int nLeft = sizeof(RspnsPacket); //ʣ���ֽ�������ʼΪӦ�����ݰ��Ĵ�С
	char *pRspns = (char *)&rspns;
	while (nLeft > 0){ //ѭ������ֱ��ʣ���ֽ�Ϊ0
		ret = recv(sClient, pRspns, nLeft, 0);
		if (ret == SOCKET_ERROR){
			cout << "Receive response failed! Error code: " << WSAGetLastError() << endl;
			return false;
		}
		if (ret == 0){
			cout << "Receive response failed!" << endl;
			return false;
		}

		nLeft -= ret;
		pRspns += ret;
	}
	return true;
}

//��ʼ���������������׽���
bool FtpClient::InitialDataSocket(){
	struct sockaddr_in saClient;
	//�����������������׽��֣�ʹ��TCPЭ��
	sdListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sdListen == INVALID_SOCKET){
		cout << "Create data listen socket failed!" << endl;
		return false;
	}
	//�������ص�ַ��Ϣ
	saClient.sin_family = AF_INET;
	saClient.sin_port = htons(DATA_PORT);
	saClient.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	//��
	int ret = bind(sdListen, (struct sockaddr *)&saClient, sizeof(saClient));
	if (ret == SOCKET_ERROR){
		cout << "Bind data listen socket failed! Error code: " << WSAGetLastError() << endl;
		closesocket(sdListen);
		return false;
	}
	//������������
	ret = listen(sdListen, 1);
	if (ret == SOCKET_ERROR){
		cout << "Data socket listen failed! Error code: " << WSAGetLastError() << endl;
		closesocket(sdListen);
		return false;
	}
	return true;
}

//ִ��cdָ��
bool FtpClient::DoCD(){
	cmd.type = CD;
	cin >> cmd.arg;
	//�����������ݰ�������Ӧ�����ݰ�
	if (!SendCmd())
		return false;
	if (!ReceiveRspns())
		return false;
	if (rspns.type != DONE) //�ж�cdָ���Ƿ�ִ�гɹ�
		ShowErr(); //���Ӧ�����Ͳ���DONE����ʾ��ϸ������Ϣ
	else
		cout << rspns.text << endl; //�ɹ�����ʾӦ�����ݰ�������
	return true;
}

//ִ��lcdָ��
void FtpClient::DoLCD(){
	char LCDstr[256];
	cin >> LCDstr;
	//���õ�ǰĿ¼��ʹ��win32 API�ӿں���,�����ݽ����ʾ��Ӧ����Ϣ
	if (SetCurrentDirectory(LCDstr)) 
		cout << "Local dir is " << LCDstr << " now!" << endl;
	else
		cout << "LCD can't change to that dir!" << endl;
}

//ִ��pwdָ��
bool FtpClient::DoPWD(){
	cmd.type = PWD;
	//�����������ݰ�������Ӧ�����ݰ�
	if (!SendCmd())
		return false;
	if (!ReceiveRspns())
		return false;
	if (rspns.type != DONE) //�ж�pwdָ���Ƿ�ִ�гɹ�
		ShowErr(); //���Ӧ�����Ͳ���DONE����ʾ��ϸ������Ϣ
	else
		cout << rspns.text << endl; //�ɹ�����ʾӦ�����ݰ�������
	return true;
}

//ִ��putָ��
bool FtpClient::DoPUT(){
	fstream fin;

	cmd.type = PUT;
	cin >> cmd.arg;
	fin.open(cmd.arg, ios::in | ios::binary); //�����Ʒ�ʽ���ļ�
	if (!fin.is_open()){ //�ļ��Ƿ�򿪳ɹ�
		cout << "Can't open file " << cmd.arg << endl;
		return true; //����trueʹ�ñ�������ͻ��˿��Լ���ִ��
	}

	//�����������������׽��ֲ���������������������
	if (!InitialDataSocket()){
		fin.close();
		return false;
	}
	//�����������ݰ�������Ӧ�����ݰ�
	if (!SendCmd()){
		fin.close();
		closesocket(sdListen);
		return false;
	}
	if (!ReceiveRspns()){
		fin.close();
		closesocket(sdListen);
		return false;
	}
	if (rspns.type != DONE){ //�ж�putָ���ܷ����ִ��
		ShowErr(); //���Ӧ�����Ͳ���DONE����ʾ��ϸ������Ϣ
		fin.close(); 
		closesocket(sdListen);
		return true; //Ӧ�����Ͳ�ΪDONE˵�����������Ѵ���ͬ���ļ����ļ��޷��򿪵�������û�жϿ������Լ���ִ������ָ��
	}

	//���ܷ�������������������
	struct sockaddr_in saServer;
	int length = sizeof(saServer);
	sdClient = accept(sdListen, (struct sockaddr *)&saServer, &length);
	if (sdClient == INVALID_SOCKET){
		cout << "Data socket accept failed!" << endl;
		fin.close();
		closesocket(sdListen);
		return false;
	}

	//ѭ����ȡ�ļ����ݲ����͸�������
	char buf[BUFFER_SIZE];
	cout << "Sending data..." << endl;
	while (true){
		fin.read(buf, BUFFER_SIZE); //��ȡ�ļ����ݣ�ÿ�ζ�ȡBUFFER_SIZE�ֽ�
		length = fin.gcount();
		int ret = send(sdClient, buf, length, 0); //��������
		if (ret == SOCKET_ERROR){
			cout << "Send data failed because some error occurs during data transmiting!" << endl;
			fin.close();
			closesocket(sdClient);
			closesocket(sdListen);
			return false;
		}
		if (length < BUFFER_SIZE)  //����ȡ���������ֽ���С��BUFFER_SIZE�ֽ�ʱ˵���Ѿ���ȡ���ļ�ĩβ��
			break;
	}
	//�����ļ��ɹ���ر��ļ����������������׽��֡������׽��ֲ���ʾ���ͳɹ���Ϣ
	fin.close();
	closesocket(sdClient);
	closesocket(sdListen);
	cout << "Send data succeed!" << endl;
	return true;
}

//ִ��getָ��
bool FtpClient::DoGET(){
	fstream fout;

	cmd.type = GET;
	cin >> cmd.arg;
	if (FileExist()){ //�鿴�����Ƿ����ͬ���ļ�
		cout << "There is already exists a file named " << cmd.arg << endl;
		return true; //����trueʹ�ñ�������ͻ��˿��Լ���ִ��
	}
	fout.open(cmd.arg, ios::out | ios::binary); //���������ļ��Զ����Ƶķ�ʽд
	if (!fout.is_open()){ //�ļ��Ƿ�򿪳ɹ�
		cout << "Can't open file to write data!" << endl;
		return true; //����trueʹ�ñ�������ͻ��˿��Լ���ִ��
	}

	//�����������������׽��ֲ���������������������
	if (!InitialDataSocket()){
		fout.close();
		DeleteFile(cmd.arg);
		return false;
	}
	//�����������ݰ�������Ӧ�����ݰ�
	if (!SendCmd()){
		fout.close();
		closesocket(sdListen);
		DeleteFile(cmd.arg);
		return false;
	}
	if (!ReceiveRspns()){
		fout.close();
		closesocket(sdListen);
		DeleteFile(cmd.arg);
		return false;
	}
	if (rspns.type != DONE){ //�ж�putָ���ܷ����ִ��
		ShowErr(); //���Ӧ�����Ͳ���DONE����ʾ��ϸ������Ϣ
		fout.close();
		closesocket(sdListen);
		DeleteFile(cmd.arg);
		return true;  //Ӧ�����Ͳ�ΪDONE˵�����������Ѵ���ͬ���ļ����ļ��޷��򿪵�������û�жϿ������Լ���ִ������ָ��
	}

	//���ܷ�������������������
	struct sockaddr_in saServer;
	int length = sizeof(saServer);
	sdClient = accept(sdListen, (struct sockaddr *)&saServer, &length);
	if (sdClient == INVALID_SOCKET){
		cout << "Data socket accept failed!" << endl;
		fout.close();
		closesocket(sdListen);
		DeleteFile(cmd.arg);
		return false;
	}

	//ѭ�����շ��������͵����ݲ���д�뵽�ļ� 
	char buf[BUFFER_SIZE];
	cout << "Receiving data..." << endl;
	while (true){
		int ret = recv(sdClient, buf, BUFFER_SIZE, 0); //��������
		if (ret == SOCKET_ERROR){
			cout << "Receive data failed because some error occurs during data transmiting!" << endl;
			fout.close();
			closesocket(sdClient);
			closesocket(sdListen);
			DeleteFile(cmd.arg);
			return false;
		}
		if (ret == 0) //�����յ����ֽ���Ϊ0ʱ˵�����ݴ����Ѿ�������
			break;

		fout.write(buf, ret); //�����յ�������д�뵽�ļ�
	}
	//�����ļ��ɹ���ر��ļ����������������׽��֡������׽��ֲ���ʾ���ճɹ���Ϣ
	fout.close();
	closesocket(sdClient);
	closesocket(sdListen);
	cout << "Receive data succeed!" << endl;
	return true;
}

//ִ��lsָ��
bool FtpClient::DoLS(){
	cmd.type = LS;

	//�����������������׽��ֲ���������������������
	if (!InitialDataSocket())
		return false;
	//�����������ݰ�������Ӧ�����ݰ�
	if (!SendCmd()){
		closesocket(sdListen);
		return false;
	}
	if (!ReceiveRspns()){
		closesocket(sdListen);
		return false;
	}
	//���ܷ�������������������
	struct sockaddr_in saServer;
	int length = sizeof(saServer);
	sdClient = accept(sdListen, (struct sockaddr *)&saServer, &length);
	if (sdClient == INVALID_SOCKET){
		cout << "Data socket accept failed!" << endl;
		closesocket(sdListen);
		return false;
	}

	//���շ��������������ݣ�ÿ�ν��յĴ�С���̶������պ�ȫ����ʾ
	char buf[BUFFER_SIZE];
	while (true){
		int ret = recv(sdClient, buf, BUFFER_SIZE - 1, 0); //��������
		if (ret == SOCKET_ERROR){
			cout << "Receive file list failed because some error occurs during data transmiting!" << endl;
			closesocket(sdClient);
			closesocket(sdListen);
			return false;
		}
		if (ret == 0) //�����յ����ֽ���Ϊ0ʱ˵�����ݴ����Ѿ�������
			break;

		buf[ret] = 0;
		cout << buf;
	}
	//��ʾ��Ϻ�ر��������������׽��ֺ������׽���
	closesocket(sdListen);
	closesocket(sdClient);
	return true;
}

//ִ��llsָ��
void FtpClient::DoLLS(){
	HANDLE hff;
	WIN32_FIND_DATA fd;

	hff = FindFirstFile("*", &fd); //�����ļ�,ƥ���κ�����
	if (hff == INVALID_HANDLE_VALUE){ //����Ƿ���ִ���
		cout << "List file failed!" << endl;
		return;
	}

	//û�д�����ѭ�������ҵ���ÿһ���ļ�����Ϣ
	bool find = true;
	while (find){
		char filerecord[MAX_PATH + 32];
		FILETIME ft;
		FileTimeToLocalFileTime(&fd.ftLastWriteTime, &ft);
		SYSTEMTIME lastwtime;
		FileTimeToSystemTime(&ft, &lastwtime);
		char *dir = fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? "<DIR>" : "";
		sprintf_s(filerecord, "%04d-%02d-%02d %02d:%02d    %5s  %10d  %-20s\n", lastwtime.wYear, lastwtime.wMonth, lastwtime.wDay, lastwtime.wHour, lastwtime.wMinute, dir, fd.nFileSizeLow, fd.cFileName);
		cout << filerecord;
		find = FindNextFile(hff, &fd); //������һ���ļ�
	}
}

//ִ��quitָ��
void FtpClient::DoQUIT(){
	cmd.type = QUIT;
	//�����������ݰ�������Ӧ�����ݰ�
	if (!SendCmd())
		return;
	if (ReceiveRspns())
		cout << rspns.text;
}

//�ж��ļ��Ƿ��Ѵ����ڱ���Ŀ¼��
bool FtpClient::FileExist(){
	WIN32_FIND_DATA fd;
	if (FindFirstFile(cmd.arg, &fd) == INVALID_HANDLE_VALUE)
		return false;
	return true;
}

//����Ӧ���ĵ�Ӧ��������ʾ������Ϣ
void FtpClient::ShowErr(){
	switch (rspns.type){
	case ERR_CD:
		cout << "CD error! Can't change to that dir!" << endl;
		break;
	case ERR_CD1:
		cout << "CD succeed! But can't get current dir!" << endl;
		break;
	case ERR_PWD:
		cout << "Can't get current dir!" << endl;
		break;
	case ERR_PUT:
		cout << "File already exist on server!" << endl;
		break;
	case ERR_PUT1:
		cout << "File not exist! But can't open the file!" << endl;
		break;
	case ERR_GET:
		cout << "Can't open that file!" << endl;
		break;
	case ERR_TYPE:
		cout << "Unsupported command!" << endl;
		break;
	default:
		cout << "Undefined error type!" << endl;
		break;
	}
}
