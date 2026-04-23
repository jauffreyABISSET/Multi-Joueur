#include "pch.h"
#include "main.h"
#include "Manager.h"

DWORD WINAPI ThreadGameplay(_In_ LPVOID lpParameter)
{
    return 0;
};

int main() // SERVER
{
    srand(time(nullptr));

    WinSockHandler::Start();

    MANAGER->Run();

    WinSockHandler::Clean();
    //return 0;
}