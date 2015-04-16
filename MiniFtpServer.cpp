#include "MiniFtpServer.h"

FtpServer::FtpServer(){}

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

	sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sListen == INVALID_SOCKET){
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

void FtpServer::Serve(){

	//循环接受客户端连接请求，并生成线程去处理：
	while(true){
		pTArg = NULL;
		pTArg = new pTArg;
		if(pTArg == NULL){
			cout << "Attribute memory failed!" << endl;
			continue;
		}
		
		int length = sizeof(pTArg->saClient);
		//等待接受客户端控制连接请求
		pTArg->sServer = accept(sListen, (struct sockaddr *)&pTArg->clientaddr, &length);
		if(pTArg->sServer == INVALID_SOCKET){
			cout << "Accept failed! Error code: " << WSAGetLastError() << endl;
			delete pTArg;
			continue; 
		}

		//创建一个线程来处理相应客户端的请求：
		DWORD dwThreadId;//, dwThrdParam = 1; 
		HANDLE hThread; 
		
		hThread = CreateThread( 
			NULL,                        // no security attributes 
			0,                           // use default stack size  
			ThreadFunc,                  // thread function 
			pTArg,					// argument to thread function 
			0,                           // use default creation flags 
			&dwThreadId);                // returns the thread identifier 
		
		// Check the return value for success. 
		
		if (hThread == NULL){
			cout << "Create thread failed!" << endl;
			closesocket(pTArg->sServer);
			delete pTArg;
		}
	}
}

//线程函数，参数包括相应控制连接的套接字：
DWORD WINAPI ThreadFunc( LPVOID pTArg ) { 
	ServerThread serverThread(((threadArg *)pTArg)->sServer,  ((threadArg *)pTArg)->saClient);
	serverThread.DoServer(); 

	delete pTArg;
	return 0;
} 

ServerThread::ServerThread(SOCKET s, sockaddr_in a){
	sServer = s;
	saClient = a;

	//发送回复报文给客户端，内含命令使用说明：
	cout << "Client address is " << inet_ntoa(saClient.sin_addr) << ntohs(saClient.sin_port) << endl;
	saClient.sin_port = htons(DATA_PORT);//修改客户端地址的端口值，用于后面建立数据连接

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
		"quit\t<no param>\n";
	strcpy_s(rspns.text, welcome);
	SendRspns();
}

void ServerThread::DoServer(){
	//循环获取客户端命令报文并进行处理
	while(true){
		if(!ReceiveCmd())
			break;
		if(!ExecCmd())
			break;
	}
	
	//线程结束前关闭控制连接套接字：
	closesocket(sServer);
}

bool ServerThread::SendRspns(){
	char *pRspns = (char *)&rspns;
	int ret = send(sServer, pRspns, sizeof(RspnsPacket), 0);
	if(ret == SOCKET_ERROR){
		cout << "Send response failed! Error code: " << WSAGetLastError() << endl;
		return false;
	}
	return true;
}

bool ServerThread::ReceiveCmd(){
	int ret;
	int nLeft = sizeof(CmdPacket);
	char *pCmd = (char *)&cmd; 
	
	//从控制连接中读取数据，大小为sizeof(CmdPacket)：
	while(nLeft > 0){
		ret = recv(sServer, pCmd, nLeft, 0);
		if(ret == SOCKET_ERROR){
			cout << "Receive command failed! Error code: " << WSAGetLastError() << endl;
			return false;
		}
		if(ret == 0){
			cout << "Client has closed the connection!" << endl;
			return false;
		}
		
		nLeft -= ret;
		pCmd += ret;
	}
	
	return true; //成功获取命令报文
}

bool ExecCmd(){
	//根据命令类型分派执行：
	switch(cmd.type)
	{
	case CD:
		//设置当前目录，使用win32 API接口函数
		if(SetCurrentDirectory(cmd.arg)){
			rspns.type = DONE;
			if(!GetCurrentDirectory(RSPNS_TEXT_SIZE, rspns.text))
				rspns.type = ERR_CD1;
		}
		else
			rspns.type = ERR_CD;
		if(!SendRspns()) //发送回复报文
			return false;
		break;

	case PWD:
		rspns.type = DONE;
		//获取当前目录，并放至回复报文中
		if(!GetCurrentDirectory(RSPNS_TEXT_SIZE, rspns.text))
			rspns.type = ERR_PWD;
		if(!SendRspns()) 
			return false;
		break;

	case PUT:
		//首先发送回复报文
		//verify no file with same name exits to make sure that no file will be overwritten
		if(FileExists(cmd.arg)){
			rspns.type = ERR_PUT;
			if(!SendRspns()) 
				return false;
		}
		else{
			rspns.type = DONE;
			if(!SendRspns()) 
				return false;
			//另建一个数据连接来接收数据：
			if(!InitialDataSocket()) 
				return false;
			if(!ReceiveData()) 
				return false;
		}
		break;

	case GET:
		fin.open(cmd.arg, ios::in|ios::binary);
		if(fin.is_open()){
			rspns.type = DONE;
			if(!SendRspns()){
				fin.close();
				return false;
			}
			else{
				//创建额外的数据连接来传送数据：
				if(!InitialDataSocket()){
					fin.close();
					return false;
				}
				if(!SendData())
					return false;
			}
		}
		else{ //打开文件失败：
			rspns.type = ERR_GET;
			if(!SendRspns()) 
				return false;
		}
		break;

	case LS:
		rspns.type = DONE;
		if(!SendRspns())
			return false;
		//首先建立数据连接：
		if(!InitialDataSocket()) 
			return false;
		//发送文件列表信息：
		if(!SendFileList())
			return false;
		break;

	case QUIT:
		cout << "Client exit!" << endl;
		rspns.type = DONE;
		SendRspns();
		return false;

	default:
		rspns.type = ERR_TYPE;
		if(!SendRspns())
			return false;
	}
	return true;
}

bool ServerThread::InitialDataSocket(){
	//创建socket
	sData = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sData == INVALID_SOCKET){
		cout << "Create data socket failed!" << endl;
		return false;
	}

	//请求连接客户端
	int ret = connect(sData, (struct sockaddr *)&saClient, sizeof(saClient));
	if (ret == SOCKET_ERROR){
		cout << "Connect to client failed!" << endl;
		closesocket(sData);
		return false;
	}
	return true;
}

bool ServerThread::ReceiveData(){
	char buf[BUFFER_SIZE];
	fstream fout;
	fout.open(cmd.arg, ios::out|ios::binary);
	if(fout.is_open()){
		//循环接收所有数据，并同时写往文件：
		cout << "Receiving data..." << endl;
		while(true){
			int ret = recv(sData, buf, BUFFER_SIZE, 0);
			if(ret == SOCKET_ERROR){
				cout << "Receive data failed because some error occurs during data transmiting!" << endl;
				fout.close();
				closesocket(sData);
				return false;
			}

			if(ret == 0) //数据传送结束
				break;

			fout.write(buf, ret);
		}
		fout.close();
		closesocket(sData);
		cout << "Receive data succeed!" << endl;
		return true;
	}
	else{
		cout << "Can't open the file to write data!" << endl;
		fout.close();
		closesocket(sData);
		return false;
	}
}

bool ServerThread::SendData(){
	char buf[BUFFER_SIZE];
	cout << "Sending data..." << endl;
	while(true){
	//从文件中循环读取数据并发往客户端
		fin.read(buf, BUFFER_SIZE);
		int length = fin.gcount();
		int ret = send(sData, buf, length, 0);
		if(ret == SOCKET_ERROR){
			cout << "Send data failed because some error occurs during data transmiting!" << endl;
			fin.close();
			closesocket(sData);
			return false;
		}
		if(length < BUFFER_SIZE)
			break;
	}
	fin.close();
	closesocket(sData);
	cout << "Send data succeed!" << endl;
	return true;
}

bool ServerThread::FileExist(){
	WIN32_FIND_DATA fd;
	if(FindFirstFile(cmd.arg, &fd) == INVALID_HANDLE_VALUE)
		return false;
	return true;
}

bool ServerThread::SendFileList(){
	HANDLE hff;
	WIN32_FIND_DATA fd;

	//搜索文件
	hff = FindFirstFile("*", &fd);
	if(hff == INVALID_HANDLE_VALUE){ //发生错误
		cout << "List file failed!" << endl;
		char *dataStr="Can't list files!\n";
		int ret = send(sData, dataStr, strlen(dataStr), 0);
		if(ret == SOCKET_ERROR)
			cout << "Send file list failed!" << endl;
		closesocket(sData);
		return false;
	}

	bool find = true;
	while(find){
		//发送此项文件信息：
		if(!SendFileRecord(&fd)){
			closesocket(datatcps);
			return false;
		}
		//搜索下一个文件：
		find = FindNextFile(hff, &fd);
	}

	closesocket(sData);
	return true;
}

bool ServerThread::SendFileRecord(WIN32_FIND_DATA* pfd){
	char filerecord[MAX_PATH+32];
	FILETIME ft;
	FileTimeToLocalFileTime(&pfd->ftLastWriteTime, &ft);
	SYSTEMTIME lastwtime;
	FileTimeToSystemTime(&ft, &lastwtime);
	char *dir = pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? "<DIR>" : "";
	sprintf(filerecord,"%04d-%02d-%02d %02d:%02d    %5s  %10d  %-20s\n",
		lastwtime.wYear,
		lastwtime.wMonth,
		lastwtime.wDay,
		lastwtime.wHour,
		lastwtime.wMinute,
		dir,
		pfd->nFileSizeLow,
		pfd->cFileName);
	int ret = send(sData, filerecord, strlen(filerecord), 0);
	if(ret == SOCKET_ERROR){
		cout << "Send file list failed!" << endl;
		return false;
	}
	return true;
}
