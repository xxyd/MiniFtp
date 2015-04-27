#include "MiniFtpServer.h"
#include <iostream>
#include <fstream>
using namespace std;

//FtpServer���캯��
FtpServer::FtpServer(){}

 //��ʼ���������������׽���
bool FtpServer::Initial(){

	WORD wVersionRequested;
	WSADATA wsaData;
	sockaddr_in saServer;
	int ret;
	//winsock ��ʼ��
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
	sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sListen == INVALID_SOCKET){
		cout << "Create listen socket failed!" << endl;
		WSACleanup();
		return false;
	}
	//�������ص�ַ��Ϣ
	saServer.sin_family = AF_INET;
	saServer.sin_port = htons(CMD_PORT);
	saServer.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	//��
	ret = bind(sListen, (struct sockaddr *)&saServer, sizeof(saServer));
	if (ret == SOCKET_ERROR){
		cout << "Bind failed! Error code: " << WSAGetLastError() << endl;
		closesocket(sListen);
		WSACleanup();
		return false;
	}
	//������������
	ret = listen(sListen, 5);
	if (ret == SOCKET_ERROR){
		cout << "Listen failed! Error code: " << WSAGetLastError() << endl;
		closesocket(sListen);
		WSACleanup();
		return false;
	}
	return true;
}

//��������
void FtpServer::Serve(){
	//ѭ�����ܿͻ��˵��������󣬽�����������������߳�������
	while(true){
		pTArg = NULL;
		pTArg = new threadArg;
		if (pTArg == NULL){
			cout << "Attribute memory failed!" << endl;
			continue;
		}

		//�����ȴ��ͻ��˵���������
		int length = sizeof(pTArg->saClient);
		pTArg->sServer = accept(sListen, (struct sockaddr *)&pTArg->saClient, &length);
		if (pTArg->sServer == INVALID_SOCKET){
			cout << "Accept failed! Error code: " << WSAGetLastError() << endl;
			delete pTArg;
			continue;
		}

		//���ܿͻ��˵��߳����Ӻ������̴߳���ͻ��˵�����
		DWORD dwThreadId, dwThrdParam = 1;
		HANDLE hThread;
		hThread = CreateThread(NULL, 0, ThreadFunc, pTArg, 0, &dwThreadId);
		if (hThread == NULL){
			cout << "Create thread failed!" << endl;
			closesocket(pTArg->sServer);
			delete pTArg;
		}
	}
}

//�̺߳���
DWORD WINAPI ThreadFunc(LPVOID pTArg) {
	//���ɷ������߳���Ķ���������ͻ��˵�����
	ServerThread serverThread(((threadArg *)pTArg)->sServer, ((threadArg *)pTArg)->saClient);
	serverThread.DoServer();

	//�߳��Ƴ�ǰɾ���������������ڴ����ͷ��ڴ�
	delete pTArg;
	return 0;
}

//�������߳��๹�캯��
ServerThread::ServerThread(SOCKET s, sockaddr_in a){
	//��ʼ���������������׽��ֺͿͻ��˵�ַ��Ϣ
	sServer = s;
	saClient = a;
	
	cout << "Client address is " << inet_ntoa(saClient.sin_addr) << ":" << ntohs(saClient.sin_port) << endl; //��ʾ�ͻ��˵�IP��ַ�Ͷ˿�
	saClient.sin_port = htons(DATA_PORT);//�޸Ŀͻ��˵�ַ�Ķ˿�ֵ�����ں��潨����������
	//���ͻظ����ݰ����ͻ��ˣ��ظ����ݰ�����ӭ��Ϣ�Ϳ�ִ�е�����
	rspns.type = DONE;
	char welcome[RSPNS_TEXT_SIZE] =
		"Welcome to Mini FTP Server!\n"
		"Avilable commands:\n"
		"cd\t<path>\n"
		"lcd\t<path>\n"
		"pwd\t<no param>\n"
		"put\t<file>\n"
		"get\t<file>\n"
		"ls\t<no param>\n"
		"lls\t<no param>\n"
		"quit\t<no param>\n";
	strcpy_s(rspns.text, welcome);
	SendRspns();
}

//ִ�пͻ��˵�����
void ServerThread::DoServer(){
	//ѭ����ȡ�ͻ��˵��������ݰ����Ҹ�����������������Ӧ�Ĵ���
	bool exec = true; //��ʾ�Ƿ����ѭ��ִ����һ������
	while (exec){
		//���տͻ��˷��͵����ݰ�
		if (!ReceiveCmd())
			break;
		//�������������ִ����Ӧ�Ĳ��������ִ�г���������execΪfalse
		switch (cmd.type)
		{
		case CD:
			if (!ExecCD())
				exec = false;
			break;

		case PWD:
			if (!ExecPWD())
				exec = false;
			break;

		case PUT:
			if (!ExecPUT())
				exec = false;
			break;

		case GET:
			if (!ExecGET())
				exec = false;
			break;

		case LS:
			if (!ExecLS())
				exec = false;
			break;

		case QUIT:
			ExecQUIT();
			exec = false;
			break;

		default:
			rspns.type = ERR_TYPE;
			if (!SendRspns())
				exec = false;
			break;
		}
	}
	//����ǰ�رտ������������׽���
	closesocket(sServer);
}

//����Ӧ�����ݰ����ͻ���
bool ServerThread::SendRspns(){
	char *pRspns = (char *)&rspns;
	int ret = send(sServer, pRspns, sizeof(RspnsPacket), 0);
	if (ret == SOCKET_ERROR){
		cout << "Send response failed! Error code: " << WSAGetLastError() << endl;
		return false;
	}
	return true;
}

//�ӿͻ��˽����������ݰ�
bool ServerThread::ReceiveCmd(){
	int ret;
	int nLeft = sizeof(CmdPacket); //ʣ���ֽ�������ʼΪ�������ݰ��Ĵ�С
	char *pCmd = (char *)&cmd;
	//ѭ������ֱ��ʣ���ֽ���Ϊ��
	while (nLeft > 0){
		ret = recv(sServer, pCmd, nLeft, 0);
		if (ret == SOCKET_ERROR){
			cout << "Receive command failed! Error code: " << WSAGetLastError() << endl;
			return false;
		}
		if (ret == 0){
			cout << "Client has closed the connection!" << endl;
			return false;
		}

		nLeft -= ret;
		pCmd += ret;
	}
	return true;
}

//��ʼ���������������׽���
bool ServerThread::InitialDataSocket(){
	//�����������������׽���
	sData = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sData == INVALID_SOCKET){
		cout << "Create data socket failed!" << endl;
		return false;
	}

	//�������ӿͻ���
	int ret = connect(sData, (struct sockaddr *)&saClient, sizeof(saClient));
	if (ret == SOCKET_ERROR){
		cout << "Connect to client failed! Error code: " << WSAGetLastError() << endl;
		closesocket(sData);
		return false;
	}
	return true;
}

//ִ��cdָ��
bool ServerThread::ExecCD(){
	//���õ�ǰĿ¼��ʹ��win32 API�ӿں���
	if (SetCurrentDirectory(cmd.arg)){
		rspns.type = DONE;
		if (!GetCurrentDirectory(RSPNS_TEXT_SIZE, rspns.text)) 
			rspns.type = ERR_CD1; //�޷���ȡ��ǰĿ¼����������ERR_CD1
	}
	else //���õ�ǰĿ¼ʧ�ܣ���������ERR_CD
		rspns.type = ERR_CD;
	//����Ӧ�����ݰ�
	if (!SendRspns())
		return false;
	return true;
}

//ִ��pwdָ��
bool ServerThread::ExecPWD(){
	rspns.type = DONE;
	//��ȡ��ǰĿ¼�����������Ӧ�����ݰ���������
	if (!GetCurrentDirectory(RSPNS_TEXT_SIZE, rspns.text))
		rspns.type = ERR_PWD; //�޷���ȡ��ǰĿ¼����������ERR_PWD
	//����Ӧ�����ݰ�
	if (!SendRspns())
		return false;
	return true;
}

//ִ��putָ��
bool ServerThread::ExecPUT(){
	//���ҷ������ĵ�ǰĿ¼�����������ļ�
	if (FileExist()){
		rspns.type = ERR_PUT; //�������ļ�����������ERR_PUT
		if (!SendRspns())
			return false;
	}
	else{ //�������ļ�
		fstream fout;
		fout.open(cmd.arg, ios::out | ios::binary); //�����Ʒ�ʽд�ļ�
		if (fout.is_open()){ //�ļ��Ƿ�ɹ���
			rspns.type = DONE;
			//����Ӧ�����ݰ��������������������׽������ӿͻ���
			if (!SendRspns()) {
				fout.close();
				return false;
			}
			if (!InitialDataSocket()) {
				fout.close();
				return false;
			}
			//ѭ�����տͻ��˷��͵����ݲ���д�뵽�ļ� 
			char buf[BUFFER_SIZE];
			cout << "Receiving data..." << endl;
			while (true){
				int ret = recv(sData, buf, BUFFER_SIZE, 0); //��������
				if (ret == SOCKET_ERROR){
					cout << "Receive data failed because some error occurs during data transmiting!" << endl;
					fout.close();
					closesocket(sData);
					return false;
				}

				if (ret == 0) //�����յ����ֽ���Ϊ0ʱ˵�����ݴ����Ѿ�������
					break;

				fout.write(buf, ret); //�����յ�������д���ļ�
			}
			//�����ļ��ɹ���ر��ļ����������������׽��ֲ���ʾ���ճɹ���Ϣ
			fout.close();
			closesocket(sData);
			cout << "Receive data succeed!" << endl;
		}
		else{ //�ļ���ʧ��
			cout << "Can't open the file to write data!" << endl;
			rspns.type = ERR_PUT1; //�޷����ļ�д�����ݣ���������ERR_PUT1
			if (!SendRspns())
				return false;
			//�ر��ļ����������������׽���
			fout.close();
			closesocket(sData);
		}
	}
	return true;
}

//ִ��getָ��
bool ServerThread::ExecGET(){
	fstream fin;
	fin.open(cmd.arg, ios::in | ios::binary); //�����Ʒ�ʽ���ļ�
	if (fin.is_open()){ //�ļ��Ƿ�򿪳ɹ�
		rspns.type = DONE;
		//����Ӧ�����ݰ��������������������׽������ӿͻ���
		if (!SendRspns()){
			fin.close();
			return false;
		}
		if (!InitialDataSocket()){
			fin.close();
			return false;
		}

		//ѭ����ȡ�ļ����ݲ����͸��ͻ���
		char buf[BUFFER_SIZE];
		cout << "Sending data..." << endl;
		while (true){
			fin.read(buf, BUFFER_SIZE); //��ȡ�ļ����ݣ�ÿ��BUFFER_SIZE�ֽ�
			int length = fin.gcount();
			int ret = send(sData, buf, length, 0); //��������
			if (ret == SOCKET_ERROR){
				cout << "Send data failed because some error occurs during data transmiting!" << endl;
				fin.close();
				closesocket(sData);
				return false;
			}
			if (length < BUFFER_SIZE) //����ȡ���������ֽ���С��BUFFER_SIZE�ֽ�ʱ˵���Ѿ���ȡ���ļ�ĩβ��
				break;
		}
		//�����ļ��ɹ���ر��ļ����������������׽��ֲ���ʾ���ͳɹ���Ϣ
		fin.close();
		closesocket(sData);
		cout << "Send data succeed!" << endl;
	}
	else{ //�ļ���ʧ��
		rspns.type = ERR_GET; //�޷����ļ���ȡ���ݣ���������ERR_GET
		if (!SendRspns())
			return false;
	}
	return true;
}

//ִ��lsָ��
bool ServerThread::ExecLS(){
	rspns.type = DONE;
	//����Ӧ�����ݰ��������������������׽������ӿͻ���
	if (!SendRspns())
		return false;
	if (!InitialDataSocket())
		return false;

	HANDLE hff;
	WIN32_FIND_DATA fd;
	hff = FindFirstFile("*", &fd); //�����ļ�,ƥ���κ�����
	if (hff == INVALID_HANDLE_VALUE){ //����Ƿ���ִ���
		 //�������ʹ�����Ϣ
		cout << "List file failed!" << endl;
		char *dataStr = "Can't list files!\n";
		int ret = send(sData, dataStr, strlen(dataStr), 0);
		if (ret == SOCKET_ERROR){
			cout << "Send file list failed!" << endl;
			closesocket(sData);
			return false;
		}
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
		int ret = send(sData, filerecord, strlen(filerecord), 0); //�����ļ���Ϣ
		if (ret == SOCKET_ERROR){
			cout << "Send file list failed!" << endl;
			closesocket(sData);
			return false;
		}
		find = FindNextFile(hff, &fd); //������һ���ļ���û�ҵ���������ݴ���
	}
	closesocket(sData); //�ر��������������׽���
	return true;

}

//ִ��quitָ��
void ServerThread::ExecQUIT(){
	cout << "Client(" << inet_ntoa(saClient.sin_addr) << ") exit!" << endl; //����ͻ����˳�����Ϣ
	//����Ӧ�����ݰ�
	rspns.type = DONE;
	strcpy_s(rspns.text, "Goodbye!\n");
	SendRspns();
}

//�ж��ļ��Ƿ��Ѵ����뵱ǰĿ¼��
bool ServerThread::FileExist(){
	WIN32_FIND_DATA fd;
	if (FindFirstFile(cmd.arg, &fd) == INVALID_HANDLE_VALUE) //�ļ���Ϊ�������ݰ��Ĳ���
		return false;
	return true;
}
