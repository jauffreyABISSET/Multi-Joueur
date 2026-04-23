#include "pch.h"
#include <iostream>

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR cmdLine, int cmdShow) // CLIENT
{
    WinSockHandler::Start();

    CPU_RUN(WINDOW_WIDTH, WINDOW_HEIGHT);

    WinSockHandler::Clean();
    return 0;
}