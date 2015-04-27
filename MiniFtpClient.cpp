#include "MiniFtpClient.h"
#include <iostream>
using namespace std;

FtpClient::FtpClient(){}

//使用服务器的IP地址初始化控制连接连接套接字
bool FtpClient::Initial(char *IP){
	struct sockaddr_in saServer;
	WORD wVersionRequested;
	WSADATA wsaData;
	int ret;
	//Winsock初始化
	wVersionRequested = MAKEWORD(2, 2);
	ret = WSAStartup(wVersionRequested, &wsaData);
	if (ret != 0){
		cout << "WSAStartup failed!" << endl;
		return false;
	}
	//确认Winsock支持2.2版本
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2){
		cout << "Invalid Winsock version!" << endl;
		WSACleanup();
		return false;
	}
	//创建控制连接连接套接字，使用TCP协议
	sClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sClient == INVALID_SOCKET){
		cout << "Create control socket failed!" << endl;
		WSACleanup();
		return false;
	}
	//构建服务器地址信息
	saServer.sin_family = AF_INET;
	saServer.sin_port = htons(CMD_PORT);
	saServer.sin_addr.S_un.S_addr = inet_addr(IP);
	// 请求连接服务器
	ret = connect(sClient, (struct sockaddr *)&saServer, sizeof(saServer));
	if (ret == SOCKET_ERROR){
		cout << "Connect failed! Error code: " << WSAGetLastError() << endl;
		closesocket(sClient);
		WSACleanup();
		return false;
	}
	//连接成功后接收服务器的应答数据包并显示应答数据包的内容
	if (!ReceiveRspns())
		return false;
	cout << rspns.text;
	return true;
}

//执行用户的指令
void FtpClient::Work(){
	char input[20];

	// 循环读取用户的输入并根据输入执行相应的成员函数直到用户输入quit指令或者连接出错
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
	//结束前关闭控制连接连接套接字
	closesocket(sClient);
	WSACleanup();
}

//发送命令数据包给服务器
bool FtpClient::SendCmd(){
	char *pCmd = (char *)&cmd;
	int ret = send(sClient, pCmd, sizeof(CmdPacket), 0);
	if (ret == SOCKET_ERROR){
		cout << "Send command failed!" << WSAGetLastError() << endl;
		return false;
	}
	return true;
}

//从服务器接收应答数据包
bool FtpClient::ReceiveRspns(){
	int ret;
	int nLeft = sizeof(RspnsPacket); //剩余字节数，初始为应答数据包的大小
	char *pRspns = (char *)&rspns;
	while (nLeft > 0){ //循环接收直到剩余字节为0
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

//初始化数据连接侦听套接字
bool FtpClient::InitialDataSocket(){
	struct sockaddr_in saClient;
	//创建数据连接侦听套接字，使用TCP协议
	sdListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sdListen == INVALID_SOCKET){
		cout << "Create data listen socket failed!" << endl;
		return false;
	}
	//构建本地地址信息
	saClient.sin_family = AF_INET;
	saClient.sin_port = htons(DATA_PORT);
	saClient.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	//绑定
	int ret = bind(sdListen, (struct sockaddr *)&saClient, sizeof(saClient));
	if (ret == SOCKET_ERROR){
		cout << "Bind data listen socket failed! Error code: " << WSAGetLastError() << endl;
		closesocket(sdListen);
		return false;
	}
	//侦听连接请求
	ret = listen(sdListen, 1);
	if (ret == SOCKET_ERROR){
		cout << "Data socket listen failed! Error code: " << WSAGetLastError() << endl;
		closesocket(sdListen);
		return false;
	}
	return true;
}

//执行cd指令
bool FtpClient::DoCD(){
	cmd.type = CD;
	cin >> cmd.arg;
	//发送命令数据包并接收应答数据包
	if (!SendCmd())
		return false;
	if (!ReceiveRspns())
		return false;
	if (rspns.type != DONE) //判断cd指令是否执行成功
		ShowErr(); //如果应答类型不是DONE则显示详细错误信息
	else
		cout << rspns.text << endl; //成功则显示应答数据包的内容
	return true;
}

//执行lcd指令
void FtpClient::DoLCD(){
	char LCDstr[256];
	cin >> LCDstr;
	//设置当前目录，使用win32 API接口函数,并根据结果显示相应的信息
	if (SetCurrentDirectory(LCDstr)) 
		cout << "Local dir is " << LCDstr << " now!" << endl;
	else
		cout << "LCD can't change to that dir!" << endl;
}

//执行pwd指令
bool FtpClient::DoPWD(){
	cmd.type = PWD;
	//发送命令数据包并接收应答数据包
	if (!SendCmd())
		return false;
	if (!ReceiveRspns())
		return false;
	if (rspns.type != DONE) //判断pwd指令是否执行成功
		ShowErr(); //如果应答类型不是DONE则显示详细错误信息
	else
		cout << rspns.text << endl; //成功则显示应答数据包的内容
	return true;
}

//执行put指令
bool FtpClient::DoPUT(){
	fstream fin;

	cmd.type = PUT;
	cin >> cmd.arg;
	fin.open(cmd.arg, ios::in | ios::binary); //二进制方式读文件
	if (!fin.is_open()){ //文件是否打开成功
		cout << "Can't open file " << cmd.arg << endl;
		return true; //返回true使得报告错误后客户端可以继续执行
	}

	//创建数据连接侦听套接字并侦听服务器的连接请求
	if (!InitialDataSocket()){
		fin.close();
		return false;
	}
	//发送命令数据包并接收应答数据包
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
	if (rspns.type != DONE){ //判断put指令能否继续执行
		ShowErr(); //如果应答类型不是DONE则显示详细错误信息
		fin.close(); 
		closesocket(sdListen);
		return true; //应答类型不为DONE说明服务器下已存在同名文件或文件无法打开但是连接没有断开，可以继续执行其他指令
	}

	//接受服务器的数据连接请求
	struct sockaddr_in saServer;
	int length = sizeof(saServer);
	sdClient = accept(sdListen, (struct sockaddr *)&saServer, &length);
	if (sdClient == INVALID_SOCKET){
		cout << "Data socket accept failed!" << endl;
		fin.close();
		closesocket(sdListen);
		return false;
	}

	//循环读取文件数据并发送给服务器
	char buf[BUFFER_SIZE];
	cout << "Sending data..." << endl;
	while (true){
		fin.read(buf, BUFFER_SIZE); //读取文件内容，每次读取BUFFER_SIZE字节
		length = fin.gcount();
		int ret = send(sdClient, buf, length, 0); //发送数据
		if (ret == SOCKET_ERROR){
			cout << "Send data failed because some error occurs during data transmiting!" << endl;
			fin.close();
			closesocket(sdClient);
			closesocket(sdListen);
			return false;
		}
		if (length < BUFFER_SIZE)  //当读取到的数据字节数小于BUFFER_SIZE字节时说明已经读取到文件末尾了
			break;
	}
	//发送文件成功后关闭文件和数据连接侦听套接字、连接套接字并显示发送成功信息
	fin.close();
	closesocket(sdClient);
	closesocket(sdListen);
	cout << "Send data succeed!" << endl;
	return true;
}

//执行get指令
bool FtpClient::DoGET(){
	fstream fout;

	cmd.type = GET;
	cin >> cmd.arg;
	if (FileExist()){ //查看本地是否存在同名文件
		cout << "There is already exists a file named " << cmd.arg << endl;
		return true; //返回true使得报告错误后客户端可以继续执行
	}
	fout.open(cmd.arg, ios::out | ios::binary); //创建本地文件以二进制的方式写
	if (!fout.is_open()){ //文件是否打开成功
		cout << "Can't open file to write data!" << endl;
		return true; //返回true使得报告错误后客户端可以继续执行
	}

	//创建数据连接侦听套接字并侦听服务器的连接请求
	if (!InitialDataSocket()){
		fout.close();
		DeleteFile(cmd.arg);
		return false;
	}
	//发送命令数据包并接收应答数据包
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
	if (rspns.type != DONE){ //判断put指令能否继续执行
		ShowErr(); //如果应答类型不是DONE则显示详细错误信息
		fout.close();
		closesocket(sdListen);
		DeleteFile(cmd.arg);
		return true;  //应答类型不为DONE说明服务器下已存在同名文件或文件无法打开但是连接没有断开，可以继续执行其他指令
	}

	//接受服务器的数据连接请求
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

	//循环接收服务器发送的数据并且写入到文件 
	char buf[BUFFER_SIZE];
	cout << "Receiving data..." << endl;
	while (true){
		int ret = recv(sdClient, buf, BUFFER_SIZE, 0); //接收数据
		if (ret == SOCKET_ERROR){
			cout << "Receive data failed because some error occurs during data transmiting!" << endl;
			fout.close();
			closesocket(sdClient);
			closesocket(sdListen);
			DeleteFile(cmd.arg);
			return false;
		}
		if (ret == 0) //当接收到得字节数为0时说明数据传送已经结束了
			break;

		fout.write(buf, ret); //将接收到的数据写入到文件
	}
	//接收文件成功后关闭文件和数据连接侦听套接字、连接套接字并显示接收成功信息
	fout.close();
	closesocket(sdClient);
	closesocket(sdListen);
	cout << "Receive data succeed!" << endl;
	return true;
}

//执行ls指令
bool FtpClient::DoLS(){
	cmd.type = LS;

	//创建数据连接侦听套接字并侦听服务器的连接请求
	if (!InitialDataSocket())
		return false;
	//发送命令数据包并接收应答数据包
	if (!SendCmd()){
		closesocket(sdListen);
		return false;
	}
	if (!ReceiveRspns()){
		closesocket(sdListen);
		return false;
	}
	//接受服务器的数据连接请求
	struct sockaddr_in saServer;
	int length = sizeof(saServer);
	sdClient = accept(sdListen, (struct sockaddr *)&saServer, &length);
	if (sdClient == INVALID_SOCKET){
		cout << "Data socket accept failed!" << endl;
		closesocket(sdListen);
		return false;
	}

	//接收服务器发来的数据，每次接收的大小不固定，接收后全部显示
	char buf[BUFFER_SIZE];
	while (true){
		int ret = recv(sdClient, buf, BUFFER_SIZE - 1, 0); //接收数据
		if (ret == SOCKET_ERROR){
			cout << "Receive file list failed because some error occurs during data transmiting!" << endl;
			closesocket(sdClient);
			closesocket(sdListen);
			return false;
		}
		if (ret == 0) //当接收到得字节数为0时说明数据传送已经结束了
			break;

		buf[ret] = 0;
		cout << buf;
	}
	//显示完毕后关闭数据连接侦听套接字和连接套接字
	closesocket(sdListen);
	closesocket(sdClient);
	return true;
}

//执行lls指令
void FtpClient::DoLLS(){
	HANDLE hff;
	WIN32_FIND_DATA fd;

	hff = FindFirstFile("*", &fd); //搜索文件,匹配任何名字
	if (hff == INVALID_HANDLE_VALUE){ //检测是否出现错误
		cout << "List file failed!" << endl;
		return;
	}

	//没有错误则循环发送找到的每一个文件的信息
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
		find = FindNextFile(hff, &fd); //搜索下一个文件
	}
}

//执行quit指令
void FtpClient::DoQUIT(){
	cmd.type = QUIT;
	//发送命令数据包并接收应答数据包
	if (!SendCmd())
		return;
	if (ReceiveRspns())
		cout << rspns.text;
}

//判断文件是否已存在于本地目录中
bool FtpClient::FileExist(){
	WIN32_FIND_DATA fd;
	if (FindFirstFile(cmd.arg, &fd) == INVALID_HANDLE_VALUE)
		return false;
	return true;
}

//根据应答报文的应答类型显示错误信息
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
