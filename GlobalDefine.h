#ifndef _GLOBALDEFINE_H
#define _GLOBALDEFINE_H

#define CMD_PORT 3333 //服务器侦听控制连接请求的端口
#define DATA_PORT 3343 //客户端侦听数据连接请求的端口
#define CMD_ARG_SIZE 256 //命令数据包参数最大字节数
#define RSPNS_TEXT_SIZE 256 //应答数据包内容最大字节数
#define BUFFER_SIZE 4096 //数据连接每次发送的数据的最大字节数

//命令类型 
//lcd和lls只需要在客户端本地执行，不需要通过命令数据包发送,因此命令类型中不包含这两项
enum CmdType{
	CD, PWD, PUT, GET, QUIT, LS
};

//应答类型
enum RspnsType{
	DONE, //命令执行成功
	ERR_CD, //目录无法更改为目标目录
	ERR_CD1, //目录已经更改为目标目录，但是无法获取当前目录
	ERR_PWD, //无法获取当前目录
	ERR_PUT, //当前目录下已经存在同名文件
	ERR_PUT1, //无法打开文件来写入数据
	ERR_GET, //无法打开文件来读取数据
	ERR_TYPE //不支持的命令
};

//命令数据包， 包含命令类型和参数
struct CmdPacket{
	CmdType type;
	char arg[CMD_ARG_SIZE];
};

//应答数据包，包含应答类型和回复内容
struct RspnsPacket{
	RspnsType type;
	char text[RSPNS_TEXT_SIZE];
};

#endif
