#include "MiniFtpServer.h"
#include <iostream>
#include <fstream>
using namespace std;

//FtpServer构造函数
FtpServer::FtpServer(){}

 //初始化控制连接侦听套接字
bool FtpServer::Initial(){

	WORD wVersionRequested;
	WSADATA wsaData;
	sockaddr_in saServer;
	int ret;
	//winsock 初始化
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
	//创建数据连接侦听套接字，使用TCP协议
	sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sListen == INVALID_SOCKET){
		cout << "Create listen socket failed!" << endl;
		WSACleanup();
		return false;
	}
	//构建本地地址信息
	saServer.sin_family = AF_INET;
	saServer.sin_port = htons(CMD_PORT);
	saServer.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	//绑定
	ret = bind(sListen, (struct sockaddr *)&saServer, sizeof(saServer));
	if (ret == SOCKET_ERROR){
		cout << "Bind failed! Error code: " << WSAGetLastError() << endl;
		closesocket(sListen);
		WSACleanup();
		return false;
	}
	//侦听连接请求
	ret = listen(sListen, 5);
	if (ret == SOCKET_ERROR){
		cout << "Listen failed! Error code: " << WSAGetLastError() << endl;
		closesocket(sListen);
		WSACleanup();
		return false;
	}
	return true;
}

//启动服务
void FtpServer::Serve(){
	//循环接受客户端的连接请求，接受连接请求后生成线程来处理
	while(true){
		pTArg = NULL;
		pTArg = new threadArg;
		if (pTArg == NULL){
			cout << "Attribute memory failed!" << endl;
			continue;
		}

		//阻塞等待客户端的连接请求
		int length = sizeof(pTArg->saClient);
		pTArg->sServer = accept(sListen, (struct sockaddr *)&pTArg->saClient, &length);
		if (pTArg->sServer == INVALID_SOCKET){
			cout << "Accept failed! Error code: " << WSAGetLastError() << endl;
			delete pTArg;
			continue;
		}

		//接受客户端的线程连接后生成线程处理客户端的请求
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

//线程函数
DWORD WINAPI ThreadFunc(LPVOID pTArg) {
	//生成服务器线程类的对象来处理客户端的请求
	ServerThread serverThread(((threadArg *)pTArg)->sServer, ((threadArg *)pTArg)->saClient);
	serverThread.DoServer();

	//线程推出前删除传入参数分配的内存来释放内存
	delete pTArg;
	return 0;
}

//服务器线程类构造函数
ServerThread::ServerThread(SOCKET s, sockaddr_in a){
	//初始化控制连接连接套接字和客户端地址信息
	sServer = s;
	saClient = a;
	
	cout << "Client address is " << inet_ntoa(saClient.sin_addr) << ":" << ntohs(saClient.sin_port) << endl; //显示客户端的IP地址和端口
	saClient.sin_port = htons(DATA_PORT);//修改客户端地址的端口值，用于后面建立数据连接
	//发送回复数据包给客户端，回复内容包含欢迎信息和可执行的命令
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

//执行客户端的请求
void ServerThread::DoServer(){
	//循环获取客户端的命令数据包并且根据命令类型做出相应的处理
	bool exec = true; //表示是否继续循环执行下一条命令
	while (exec){
		//接收客户端发送的数据包
		if (!ReceiveCmd())
			break;
		//根据命令的类型执行相应的操作，如果执行出错则设置exec为false
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
	//结束前关闭控制连接连接套接字
	closesocket(sServer);
}

//发送应答数据包给客户端
bool ServerThread::SendRspns(){
	char *pRspns = (char *)&rspns;
	int ret = send(sServer, pRspns, sizeof(RspnsPacket), 0);
	if (ret == SOCKET_ERROR){
		cout << "Send response failed! Error code: " << WSAGetLastError() << endl;
		return false;
	}
	return true;
}

//从客户端接收命令数据包
bool ServerThread::ReceiveCmd(){
	int ret;
	int nLeft = sizeof(CmdPacket); //剩余字节数，初始为命令数据包的大小
	char *pCmd = (char *)&cmd;
	//循环接收直到剩余字节数为零
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

//初始化数据连接连接套接字
bool ServerThread::InitialDataSocket(){
	//创建数据连接连接套接字
	sData = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sData == INVALID_SOCKET){
		cout << "Create data socket failed!" << endl;
		return false;
	}

	//请求连接客户端
	int ret = connect(sData, (struct sockaddr *)&saClient, sizeof(saClient));
	if (ret == SOCKET_ERROR){
		cout << "Connect to client failed! Error code: " << WSAGetLastError() << endl;
		closesocket(sData);
		return false;
	}
	return true;
}

//执行cd指令
bool ServerThread::ExecCD(){
	//设置当前目录，使用win32 API接口函数
	if (SetCurrentDirectory(cmd.arg)){
		rspns.type = DONE;
		if (!GetCurrentDirectory(RSPNS_TEXT_SIZE, rspns.text)) 
			rspns.type = ERR_CD1; //无法获取当前目录，错误类型ERR_CD1
	}
	else //设置当前目录失败，错误类型ERR_CD
		rspns.type = ERR_CD;
	//发送应答数据包
	if (!SendRspns())
		return false;
	return true;
}

//执行pwd指令
bool ServerThread::ExecPWD(){
	rspns.type = DONE;
	//获取当前目录，将结果放在应答数据包的内容中
	if (!GetCurrentDirectory(RSPNS_TEXT_SIZE, rspns.text))
		rspns.type = ERR_PWD; //无法获取当前目录，错误类型ERR_PWD
	//发送应答数据包
	if (!SendRspns())
		return false;
	return true;
}

//执行put指令
bool ServerThread::ExecPUT(){
	//查找服务器的当前目录下有无重名文件
	if (FileExist()){
		rspns.type = ERR_PUT; //有重名文件，错误类型ERR_PUT
		if (!SendRspns())
			return false;
	}
	else{ //无重名文件
		fstream fout;
		fout.open(cmd.arg, ios::out | ios::binary); //二进制方式写文件
		if (fout.is_open()){ //文件是否成功打开
			rspns.type = DONE;
			//发送应答数据包并建立数据连接连接套接字连接客户端
			if (!SendRspns()) {
				fout.close();
				return false;
			}
			if (!InitialDataSocket()) {
				fout.close();
				return false;
			}
			//循环接收客户端发送的数据并且写入到文件 
			char buf[BUFFER_SIZE];
			cout << "Receiving data..." << endl;
			while (true){
				int ret = recv(sData, buf, BUFFER_SIZE, 0); //接收数据
				if (ret == SOCKET_ERROR){
					cout << "Receive data failed because some error occurs during data transmiting!" << endl;
					fout.close();
					closesocket(sData);
					return false;
				}

				if (ret == 0) //当接收到得字节数为0时说明数据传送已经结束了
					break;

				fout.write(buf, ret); //将接收到的数据写入文件
			}
			//接收文件成功后关闭文件和数据连接连接套接字并显示接收成功信息
			fout.close();
			closesocket(sData);
			cout << "Receive data succeed!" << endl;
		}
		else{ //文件打开失败
			cout << "Can't open the file to write data!" << endl;
			rspns.type = ERR_PUT1; //无法打开文件写入数据，错误类型ERR_PUT1
			if (!SendRspns())
				return false;
			//关闭文件和数据连接连接套接字
			fout.close();
			closesocket(sData);
		}
	}
	return true;
}

//执行get指令
bool ServerThread::ExecGET(){
	fstream fin;
	fin.open(cmd.arg, ios::in | ios::binary); //二进制方式读文件
	if (fin.is_open()){ //文件是否打开成功
		rspns.type = DONE;
		//发送应答数据包并建立数据连接连接套接字连接客户端
		if (!SendRspns()){
			fin.close();
			return false;
		}
		if (!InitialDataSocket()){
			fin.close();
			return false;
		}

		//循环读取文件数据并发送给客户端
		char buf[BUFFER_SIZE];
		cout << "Sending data..." << endl;
		while (true){
			fin.read(buf, BUFFER_SIZE); //读取文件内容，每次BUFFER_SIZE字节
			int length = fin.gcount();
			int ret = send(sData, buf, length, 0); //发送数据
			if (ret == SOCKET_ERROR){
				cout << "Send data failed because some error occurs during data transmiting!" << endl;
				fin.close();
				closesocket(sData);
				return false;
			}
			if (length < BUFFER_SIZE) //当读取到的数据字节数小于BUFFER_SIZE字节时说明已经读取到文件末尾了
				break;
		}
		//发送文件成功后关闭文件和数据连接连接套接字并显示发送成功信息
		fin.close();
		closesocket(sData);
		cout << "Send data succeed!" << endl;
	}
	else{ //文件打开失败
		rspns.type = ERR_GET; //无法打开文件读取数据，错误类型ERR_GET
		if (!SendRspns())
			return false;
	}
	return true;
}

//执行ls指令
bool ServerThread::ExecLS(){
	rspns.type = DONE;
	//发送应答数据包并建立数据连接连接套接字连接客户端
	if (!SendRspns())
		return false;
	if (!InitialDataSocket())
		return false;

	HANDLE hff;
	WIN32_FIND_DATA fd;
	hff = FindFirstFile("*", &fd); //搜索文件,匹配任何名字
	if (hff == INVALID_HANDLE_VALUE){ //检测是否出现错误
		 //出错则发送错误信息
		cout << "List file failed!" << endl;
		char *dataStr = "Can't list files!\n";
		int ret = send(sData, dataStr, strlen(dataStr), 0);
		if (ret == SOCKET_ERROR){
			cout << "Send file list failed!" << endl;
			closesocket(sData);
			return false;
		}
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
		int ret = send(sData, filerecord, strlen(filerecord), 0); //发送文件信息
		if (ret == SOCKET_ERROR){
			cout << "Send file list failed!" << endl;
			closesocket(sData);
			return false;
		}
		find = FindNextFile(hff, &fd); //搜索下一个文件，没找到则结束数据传输
	}
	closesocket(sData); //关闭数据连接连接套接字
	return true;

}

//执行quit指令
void ServerThread::ExecQUIT(){
	cout << "Client(" << inet_ntoa(saClient.sin_addr) << ") exit!" << endl; //输出客户端退出的信息
	//发送应答数据包
	rspns.type = DONE;
	strcpy_s(rspns.text, "Goodbye!\n");
	SendRspns();
}

//判断文件是否已存在与当前目录下
bool ServerThread::FileExist(){
	WIN32_FIND_DATA fd;
	if (FindFirstFile(cmd.arg, &fd) == INVALID_HANDLE_VALUE) //文件名为命令数据包的参数
		return false;
	return true;
}
