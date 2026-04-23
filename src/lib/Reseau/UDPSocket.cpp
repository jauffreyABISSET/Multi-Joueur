#include "pch.h"
#include "UDPSocket.h"
#include <string>

UDPSocket::UDPSocket()
{
	mSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (mSocket == INVALID_SOCKET)
	{
		mHasSocket = false;
		std::cerr << "INVALID SOCKET\n";
		return;
	}

	mHasSocket = true;
}

UDPSocket::~UDPSocket()
{
	Close();
}

void UDPSocket::Close()
{
	if (mHasSocket == true)
	{
		mHasSocket = false;
		mIsBinded = false;

		closesocket(mSocket);
		mSocket = INVALID_SOCKET;
	}
}

bool UDPSocket::BindTo(PCSTR ip, unsigned int port)
{
	mIsBinded = false;

	if (mHasSocket == false)
		return false;

	mAddrIn.sin_addr.s_addr = htonl(INADDR_ANY);
	mAddrIn.sin_family = AF_INET;
	mAddrIn.sin_port = htons(port);

	if (inet_pton(AF_INET, ip, &mAddrIn.sin_addr) <= 0)
	{
		std::cerr << "INVALID IP\n";
		return false;
	}

	if (bind(mSocket, (sockaddr*)&mAddrIn, sizeof(mAddrIn)) != 0)
	{
		std::cerr << "BIND FAILED\n";
		return false;
	}

	mIsBinded = true;
	return true;
}

bool UDPSocket::SetAddrIn(int sinfamily, const char* hostIP, int hostPort)
{
	mAddrIn = { 0 };

	if (inet_pton(AF_INET, hostIP, &mAddrIn.sin_addr.s_addr) == -1)
		return false;

	mAddrIn.sin_family = sinfamily;
	mAddrIn.sin_port = htons(hostPort);

	return true;
}

int UDPSocket::SendTo(char* buffer, unsigned int len, sockaddr* to, int tolen)
{
	return sendto(mSocket, buffer, len, 0, to, tolen);
}

int UDPSocket::ReceiveFrom(char* buffer, unsigned int len, sockaddr_in& from)
{
	int fromlen = sizeof(from);

	int byteCount = recvfrom(mSocket, buffer, len, 0, reinterpret_cast<sockaddr*>(&from), &fromlen);


	return byteCount;
}
