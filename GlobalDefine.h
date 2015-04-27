#ifndef _GLOBALDEFINE_H
#define _GLOBALDEFINE_H

#define CMD_PORT 3333 //����������������������Ķ˿�
#define DATA_PORT 3343 //�ͻ�������������������Ķ˿�
#define CMD_ARG_SIZE 256 //�������ݰ���������ֽ���
#define RSPNS_TEXT_SIZE 256 //Ӧ�����ݰ���������ֽ���
#define BUFFER_SIZE 4096 //��������ÿ�η��͵����ݵ�����ֽ���

//�������� 
//lcd��llsֻ��Ҫ�ڿͻ��˱���ִ�У�����Ҫͨ���������ݰ�����,������������в�����������
enum CmdType{
	CD, PWD, PUT, GET, QUIT, LS
};

//Ӧ������
enum RspnsType{
	DONE, //����ִ�гɹ�
	ERR_CD, //Ŀ¼�޷�����ΪĿ��Ŀ¼
	ERR_CD1, //Ŀ¼�Ѿ�����ΪĿ��Ŀ¼�������޷���ȡ��ǰĿ¼
	ERR_PWD, //�޷���ȡ��ǰĿ¼
	ERR_PUT, //��ǰĿ¼���Ѿ�����ͬ���ļ�
	ERR_PUT1, //�޷����ļ���д������
	ERR_GET, //�޷����ļ�����ȡ����
	ERR_TYPE //��֧�ֵ�����
};

//�������ݰ��� �����������ͺͲ���
struct CmdPacket{
	CmdType type;
	char arg[CMD_ARG_SIZE];
};

//Ӧ�����ݰ�������Ӧ�����ͺͻظ�����
struct RspnsPacket{
	RspnsType type;
	char text[RSPNS_TEXT_SIZE];
};

#endif
