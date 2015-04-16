#ifndef _GLOBALDEFINE_H
#define _GLOBALDEFINE_H

//服务器侦听控制连接请求的端口
#define CMD_PORT 3333
//客户端侦听数据连接请求的端口
#define DATA_PORT 3343
//命令报文参数缓存的大小
#define CMD_ARG_SIZE 256
//回复报文消息缓存的大小
#define RSPNS_TEXT_SIZE 256
#define BUFFER_SIZE 4096

//命令类型
enum CmdType{
	CD, PWD, PUT, GET, QUIT, LS
};

//回复报文类型
enum RspnsType{
	DONE,
	ERR_CD,
	ERR_CD1,
	ERR_PWD,
	ERR_PUT,
	ERR_PUT1,
	ERR_GET,
	ERR_TYPE
};

//命令报文，从客户端发往服务器
// typedef struct _CmdPacket{
// 	CmdType type;
// 	char arg[CMD_ARG_SIZE];
// } CmdPacket;
struct CmdPacket{
	CmdType type;
	char arg[CMD_ARG_SIZE];
};

//回复报文，从服务器发往客户端
// typedef struct _RspnsPacket{
// 	RspnsType type;
// 	char text[RSPNS_TEXT_SIZE];
// } RspnsPacket;
struct RspnsPacket{
	RspnsType type;
	char text[RSPNS_TEXT_SIZE];
};

#endif
