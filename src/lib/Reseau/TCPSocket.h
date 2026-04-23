#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <errno.h>
#include <iostream>
#pragma comment(lib, "Ws2_32.lib")

class TCPSocket
{
	SOCKET mSocket;
	sockaddr_in mAddrIn;

	bool mHasSocket = false;
	bool mIsConnected = false;
public:
	TCPSocket();
	~TCPSocket();
	void Close();
	bool ConnectTo(PCSTR ip, unsigned int port = 80);

	int Send(const char* buffer);
	int Receive(char* buffer, unsigned int len);
};

