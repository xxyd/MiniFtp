#ifndef _GLOBALDEFINE_H
#define _GLOBALDEFINE_H

//����������������������Ķ˿�
#define CMD_PORT 3333
//�ͻ�������������������Ķ˿�
#define DATA_PORT 3343
//����Ĳ�������Ĵ�С
#define CMD_ARG_SIZE 256
//�ظ�������Ϣ����Ĵ�С
#define RSPNS_TEXT_SIZE 256
#define BUFFER_SIZE 4096

//��������
enum CmdType{
	CD, PWD, PUT, GET, QUIT, LS
};

//�ظ���������
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

//����ģ��ӿͻ��˷���������
// typedef struct _CmdPacket{
// 	CmdType type;
// 	char arg[CMD_ARG_SIZE];
// } CmdPacket;
struct CmdPacket{
	CmdType type;
	char arg[CMD_ARG_SIZE];
};

//�ظ����ģ��ӷ����������ͻ���
// typedef struct _RspnsPacket{
// 	RspnsType type;
// 	char text[RSPNS_TEXT_SIZE];
// } RspnsPacket;
struct RspnsPacket{
	RspnsType type;
	char text[RSPNS_TEXT_SIZE];
};

#endif
