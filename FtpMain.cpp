#include "MiniFtpServer.h"
#include "MiniFtpClient.h"
#include <iostream>
using namespace std;

int main(){
	cout << "Welcome to MiniFtp" << endl;
	cout << "Press 1 to use as a server!" << endl;
	cout << "Press 2 to use as a client!" << endl;
	char choice;
	cin >> choice;
	switch(choice){
	case '1':
		FtpServer server;
		if(!server.Initial())
			break;
		cout << "MiniFtp is using as a server!" << endl;
		server.Serve();
		break;

	case '2':
		FtpClient client;
		cout << "Please input the server IP: ";
		char serverIP[16];
		cin >> serverIP;
		client.Initial(serverIP);
		cout << "MiniFtp is using as a client!" << endl;
		client.work();
		break;

	default:
		cout << "Illegal input!" << endl;
		break;
	}
	return 0;
}