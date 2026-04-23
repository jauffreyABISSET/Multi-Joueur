#include "pch.h"
#include "TCPSocket.h"
#include <string>

TCPSocket::TCPSocket()
{
	mSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (mSocket == INVALID_SOCKET)
	{
		mHasSocket = false;
		std::cerr << "INVALID SOCKET\n";
		return;
	}

	mHasSocket = true;
}

TCPSocket::~TCPSocket()
{
	Close();
}

void TCPSocket::Close()
{
	if (mHasSocket == true)
	{
		mHasSocket = false;
		mIsConnected = false;

		closesocket(mSocket);
		mSocket = INVALID_SOCKET;
	}
}

bool TCPSocket::ConnectTo(PCSTR ip, unsigned int port)
{
	mIsConnected = false;

	if (mHasSocket == false)
		return false;

	mAddrIn.sin_family = AF_INET;
	mAddrIn.sin_port = htons(port);
	if (inet_pton(AF_INET, ip, &mAddrIn.sin_addr) <= 0)
	{
		std::cerr << "INVALID IP\n";
		return false;
	}

	if (connect(mSocket, (sockaddr*)&mAddrIn, sizeof(mAddrIn)) == SOCKET_ERROR)
	{
		std::cout << "CONNECTION ERROR : " << WSAGetLastError() << std::endl;
		return false;
	}

	mIsConnected = true;
	return true;
}

int TCPSocket::Send(const char* buffer)
{
	if (mIsConnected == false)
	{
		std::cerr << "NOT CONNECTED\n";
		return -1;
	}

	return send(mSocket, buffer, strlen(buffer), 0);
}

int TCPSocket::Receive(char* buffer, unsigned int len)
{
	int byteCount = recv(mSocket, buffer, len - 1, 0);

	if (byteCount == SOCKET_ERROR)
	{
		std::cerr << "RECV ERROR : " << WSAGetLastError() << std::endl;
		return -1;
	}

	if(byteCount > 0)
		buffer[byteCount] = '\0'; // end of the chars

	return byteCount;
}
