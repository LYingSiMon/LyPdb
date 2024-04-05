#pragma once

extern "C"
{
#include "ksocket.h"
#include "berkeley.h"
}

namespace helper {
	
	//����������,д�����
	bool writeToDisk(const wchar_t* path, const wchar_t* file_name, char* buf, size_t buf_len);
	void unloadinit(PUNICODE_STRING reg_path);
	void unloadSelf();
}


namespace ksocket {

	constexpr auto packet_magic = 0x12345568;
	constexpr auto server_ip = 0x7F000001; // 127.0.0.1
	enum class PacketStatus{
		send,
		success,
		failure
	};
	struct PacketHeader
	{
		uint32_t   magic;
		PacketStatus status;
	};

	struct Packet
	{

		PacketHeader header;
		void* data;//r3��������
	};

	using acceptCallback_t = bool(*)(void* recvBuf);//����ͨ�ŵĻص� �����Ҫ�û��Լ�ȷ����С
	size_t getContentLength(const char* url, const char* port = "80");//��ȡ���ݳ���
	bool getHttpContent(const char* url, char* buf, size_t bufSize,const char* port="80");//��ȡhttp get����
	bool createServer(short port, acceptCallback_t accept_callback);//����һ������TCP�׽��ֵı��ط���������ͨ��
	void destoryServer();//�رշ�����
	bool init();
	void destory();
}

typedef int SOCKET;

#define INVALID_SOCKET  (SOCKET)(-1)
#define SOCKET_ERROR            (-1)

