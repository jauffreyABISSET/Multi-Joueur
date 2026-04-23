#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <errno.h>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")

class UDPSocket
{
	SOCKET mSocket;
	sockaddr_in mAddrIn;

	bool mHasSocket = false;
	bool mIsBinded = false;
public:
	UDPSocket();
	~UDPSocket();
	void Close();
	bool BindTo(PCSTR ip, unsigned int port = 1888);

	SOCKET& GetSOCKET() { return mSocket; }

	bool SetAddrIn(int sinfamily, const char* hostIP, int hostPort);

	int SendTo(char* buffer, unsigned int len, sockaddr* to, int tolen);
	int ReceiveFrom(char* buffer, unsigned int len, sockaddr_in& from);

	sockaddr_in& GetSockAddr() { return mAddrIn; }
};

