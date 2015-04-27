#include "MiniFtpServer.h"
#include "MiniFtpClient.h"
#include <iostream>
using namespace std;

int main(){
	// 显示提示信息，按1作为服务器运行，按2作为客户端运行
	cout << "Welcome to MiniFtp" << endl;
	cout << "Press 1 to use as a server!" << endl;
	cout << "Press 2 to use as a client!" << endl;
	char choice;
	cin >> choice;
	if (choice == '1'){
		//选择1则显示MiniFtp正在作为服务器运行，并生成服务器类的对象
		cout << "MiniFtp is using as a server!" << endl;
		FtpServer server;
		if (!server.Initial()) //初始化
			return 0;
		server.Serve(); //启动服务
	}
	else if (choice == '2'){
		//选择2则显示MiniFtp正在作为客户端运行，并且生成客户端类的对象
		cout << "MiniFtp is using as a client!" << endl;
		FtpClient client;
		cout << "Please input the server IP: "; //提示用户输入服务器的IP地址
		char serverIP[16];
		cin >> serverIP;
		client.Initial(serverIP); //初始化
		client.Work(); //执行用户输入的命令
	}
	else
		cout << "Illegal input!" << endl;
	return 0;
}