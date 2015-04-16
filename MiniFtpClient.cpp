#include "MiniFtpClient.h"

FtpClient::FtpClient(){}

bool FtpClient::Initial(char *IP){
	//struct hostent *he;
	struct sockaddr_in saServer;		
	WORD wVersionRequested;
	WSADATA wsaData;
	int ret;
	//Winsock初始化：
	wVersionRequested = MAKEWORD( 2, 2 );	
	ret = WSAStartup(wVersionRequested, &wsaData);
	if (ret != 0){
		cout << "WSAStartup failed!" << endl;
		return false;
	}

	//确认WinSock DLL的版本是2.2：
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2){
		cout << "Invalid Winsock version!" << endl;
		WSACleanup();
		return false;
	}
	
	//创建用于控制连接的socket：
	sClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sClient == INVALID_SOCKET){
		cout << "Create control socket failed!" << endl;
		WSACleanup();
		return false;
	}

	// if((he = gethostbyname(argv[1])) == NULL)
	// {
	// 	printf("gethostbyname failed!");
	// 	exit(1);
	// }

	saServer.sin_family = AF_INET;
	saServer.sin_port = htons(CMD_PORT);
	//saServer.sin_addr = *(struct in_addr *)he->h_addr_list[0];
	saServer.sin_addr.S_un.S_addr = inet_addr(IP);
	memset(&(saServer.sin_zero), 0, sizeof(saServer.sin_zero));
	
	// 连接服务器：
	ret = connect(sClient, (struct sockaddr *)&saServer, sizeof(saServer));
	if(ret == SOCKET_ERROR){
		cout << "Connect failed! Error code: " << WSAGetLastError() << endl;
		closesocket(sClient);
		WSACleanup();
		return false;
	}

	//连接成功后，首先接收服务器发回的消息：
	if(!ReceiveRspns())
		return false;
	cout << rspns.text;
	return true;
}

void FtpClient::Work(){
	char input[20];

	// 主循环：读取用户输入并分派执行
	while(true){
		cin >> input;
		if(input[0] == 'c' && input[1] == 'd' && input[2] == 0){
			if(!DoCD())
				break;
		}
		else if(input[0] == 'l' && input[1] == 'c' && input[2] == 'd' && input[3] == 0){
			DoLCD();
		}
		else if(input[0] == 'p' && input[1] == 'w' && input[2] == 'd' && input[3] == 0){
			if(!DoPWD())
				break;
		}
		else if(input[0] == 'p' && input[1] == 'u' && input[2] == 't' && input[3] == 0){
			if(!DoPUT())
				break;
		}
		else if(input[0] == 'g' && input[1] == 'e' && input[2] == 't' && input[3] == 0){
			if(!DoGET())
				break;
		}
		else if(input[0] == 'l' && input[1] == 's' && input[2] == 0){
			if(!DoLS())
				break;
		}
		else if(input[0] == 'q' && input[1] == 'u' && input[2] == 'i' && input[3] == 't' && input[4] == 0){
			DoQUIT();
			break;
		}
		else{
			cout << "Unsupported command!" << endl;
		}
	}
	closesocket(sClient);
	WSACleanup();
}

bool FtpClient::SendCmd(){
	char *pCmd = (char *)&cmd；
	int ret = send(sClient, pCmd, sizeof(CmdPacket), 0);
	if(ret == SOCKET_ERROR){
		cout << "Send command failed!" << WSAGetLastError() << endl;
		return false;
	}
	return true;
}

bool FtpClient::ReceiveRspns(){
	int ret;
	int nLeft = sizeof(RspnsPacket);
	char *pRspns = (char *)&rspns;

	//从控制连接中读取数据，大小为sizeof(RspnsPacket)：
	while(nLeft > 0){
		ret = recv(sClient, pRspns, nLeft, 0);
		if(ret == SOCKET_ERROR){
			cout << "Receive response failed! Error code: " << WSAGetLastError() << endl;
			return false;
		}
		if(ret == 0){
			cout << "Receive response failed!" << endl;
			return false;
		}

		nLeft -= ret;
		pRspns += ret;
	}	
	return true; //成功获取回复报文
}

bool FtpClient::InitialDataSocket(){
	struct sockaddr_in saClient;
	
	// 创建用于数据连接的套接字：
	sdListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sdListen == INVALID_SOCKET ){
		cout << "Create data listen socket failed!" << endl;
		return false;
	}
	
	saClient.sin_family = AF_INET;
	saClient.sin_port = htons(DATA_PORT);
	saClient.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	memset(&(saClient.sin_zero), 0, sizeof(saClient.sin_zero));
	
	// 绑定：
	int ret = bind(sdListen, (struct sockaddr *)&saClient, sizeof(saClient));
	if(ret == SOCKET_ERROR){
		cout << "Bind data listen socket failed! Error code: " << WSAGetLastError() << endl;
		closesocket(sdListen);
		return false;
	}
	
	// 侦听数据连接请求：
	ret = listen(sdListen, 1);
	if(ret == SOCKET_ERROR){
		cout << "Data socket listen failed! Error code: " << WSAGetLastError() << endl;
		closesocket(sdListen);
		return false;
	}
	return true;
}

bool FtpClient::DoCD(){
	cmd.type = CD;
	cin >> cmd.arg;
	//发送命令报文并读取回复：
	if(!SendCmd())
		return false;
	if(!ReceiveRspns())
		return false;
	if(rspns.type != DONE)
		ShowErr();
	cout << rspns.text << endl;
	return true;
}

void FtpClient::DoLCD(){
	char LCDstr[256];
	cin >> LCDstr;
	//设置当前目录，使用win32 API接口函数
	if(SetCurrentDirectory(LCDstr))
		cout << "Local dir is " << LCDstr << " now!"<< endl;
	else
		cout << "LCD can't change to that dir!" << endl;
}

bool FtpClient::DoPWD(){
	cmd.type = PWD;
	//发送命令报文并读取回复：
	if(!SendCmd())
		return false;
	if(!ReceiveRspns())
		return false;
	if(rspns.type != DONE)
		ShowErr();
	cout << rspns.text << endl;
	return true;
}

bool FtpClient::DoPUT(){
	fstream fin;
	
	cmd.type = PUT;
	cin >> cmd.arg;
	fin.open(cmd.arg, ios::in|ios::binary);
	if(!fin.is_open()){
		cout << "Can't open file " << cmd.arg << endl;
		return true; //返回true使得报告错误后客户端可以继续执行
	}
	
	//创建数据连接套接字并进入侦听状态：
	if(!InitialDataSocket()){
		fin.close();
		return false;
	}
	
	//发送命令报文并读取回复：
	if(!SendCmd()){
		fin.close();
		closesocket(sdListen);
		return false;
	}
	if(!ReceiveRspns()){
		fin.close();
		closesocket(sdListen);
		return false;
	}
	if(rspns.type != DONE){
		ShowErr();
		fin.close();
		closesocket(sdListen);
		return false;
	}
	
	struct sockaddr_in saServer;
	int length = sizeof(saServer);
	// 准备接受数据连接请求：
	sdClient = accept(sdListen, (struct sockaddr *)&saServer, &length);
	if(sdClient == INVALID_SOCKET){
		cout << "Data socket accept failed!" << endl;
		fin.close();
		closesocket(sdListen);
		return false;
	}

	cout << "Sending data..." << endl;	
	char buf[BUFFER_SIZE];
	// 循环从文件中读取数据并发给服务器：
	while(true){
		fin.read(buf. BUFFER_SIZE);
		length = fin.gcount();
		int ret = send(sdClient, buf, length, 0);
		if(ret == SOCKET_ERROR){
			cout << "Send data failed because some error occurs during data transmiting!" << endl;
			fin.close();
			closesocket(sdClient);
			closesocket(sdListen);
			return false;
		}
		if(length < BUFFER_SIZE)
			break;
	}
	cout << "Sending data succeed!" << endl;
	fin.close();
	closesocket(sdClient);
	closesocket(sdListen);
	return true;
}

bool FtpClient::DoGET(){
	fstream fout;
	
	//设置命令报文：
	cmd.type = GET;
	cin >> cmd.arg;
	//查看本地是否存在同名文件
	if(FileExist(cmd.arg)){
		cout << "There is already exists a file named " << cmd.arg << endl;
		return true; //返回true使得报告错误后客户端可以继续执行
	} 
	//创建本地文件以供写数据：
	fout.open(cmd.arg, ios::out|ios::binary);
	if(!fout.is_open()){
		cout << "Can't open file to write data!" << endl;
		return true; //返回true使得报告错误后客户端可以继续执行
	}

	//创建数据连接套接字并进入侦听状态：
	if(!InitialDataSocket()){
		fout.close();
		DeleteFile(cmd.arg);
		return false;
	}
	
	//发送命令报文并读取回复：
	if(!SendCmd()){
		fout.close();
		closesocket(sdListen);
		DeleteFile(cmd.arg);
		return false;
	}
	if(!ReceiveRspns()){
		fout.close();
		closesocket(sdListen);
		DeleteFile(cmd.arg);
		return false;
	}
	if(rspns.type != DONE){
		ShowErr();
		fout.close();
		closesocket(sdListen);
		DeleteFile(cmd.arg);
		return true; //此时为服务器无法打开要获取的文件，连接没有出错，可以继续执行
	}

	struct sockaddr_in saServer;
	int length = sizeof(saServer);
	// 准备接受数据连接请求：
	sdClient = accept(sdListen, (struct sockaddr *)&saServer, &length);
	if(sdClient == INVALID_SOCKET){
		cout << "Data socket accept failed!" << endl;
		fout.close();
		closesocket(sdListen);
		DeleteFile(cmd.arg);
		return false;
	}

	cout << "Receiving data..." << endl;
	char buf[BUFFER_SIZE];
	// 循环读取网络数据并写入文件：
	while(true){
		int ret = recv(sdClient, buf, BUFFER_SIZE, 0);
		if(ret == SOCKET_ERROR){
			cout << "Receive data failed because some error occurs during data transmiting!" << endl;
			fout.close();
			closesocket(sdClient);
			closesocket(sdListen);
			DeleteFile(cmd.arg);
			return false;
		}

		if(ret == 0) //数据传输结束
			break;

		fout.write(buf, ret);
	}
	fout.close();
	closesocket(sdClient);
	closesocket(sdListen);
	return true;
}

bool FtpClient::DoLS(){
	cmd.type = LS;

	//创建数据连接套接字并进入侦听状态：
	if(!InitialDataSocket())
		return false;
	
	//发送命令报文并读取回复：
	if(!SendCmd()){
		closesocket(sdListen);
		return false;
	}
	if(!ReceiveRspns()){
		closesocket(sdListen);
		return false;
	}

	struct sockaddr_in saServer;
	int length = sizeof(saServer);
	// 准备接受数据连接请求：
	sdClient = accept(sdListen, (struct sockaddr *)&saServer, &length);
	if(sdClient == INVALID_SOCKET){
		cout << "Data socket accept failed!" << endl;
		closesocket(sdListen);
		return false;
	}
	
	char buf[BUFFER_SIZE];
	//每次读到多少数据就显示多少，直到数据连接断开
	while(true){
		int ret = recv(sdClient, buf, BUFFER_SIZE - 1, 0);
		if(ret == SOCKET_ERROR){
			cout << "Receive file list failed because some error occurs during data transmiting!" << endl;
			closesocket(sdClient);
			closesocket(sdListen);
			return false;
		}
		if(ret == 0)
			break;
		
		//显示数据：
		buf[ret] = 0;
		cout << buf;
	}
	closesocket(sdListen);
	closesocket(sdClient);
	return true;
}

void FtpClient::DoQUIT(){
	cmd.type = QUIT;
	SendCmd();
	ReceiveRspns();
	cout << rspns.text;
}

bool FtpClient::FileExist(){
	WIN32_FIND_DATA fd;
	if(FindFirstFile(cmd.arg, &fd) == INVALID_HANDLE_VALUE)
		return false;
	return true;
}

void FtpClient::ShowErr(){
	switch(rspns.type){
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
	}
}
