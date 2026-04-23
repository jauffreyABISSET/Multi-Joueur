#pragma once
#include <errno.h>

//cpu engine
#include "../../lib/cpulib/include/cpu-engine/cpu-engine.h"
#ifdef _DEBUG
	#pragma comment(lib, "../../lib/cpulib/lib/cpu-core-debug.lib")
	#pragma comment(lib, "../../lib/cpulib/lib/cpu-render-debug.lib")
	#pragma comment(lib, "../../lib/cpulib/lib/cpu-engine-debug.lib")
#else
	#pragma comment(lib, "../../lib/cpulib/lib/cpu-core.lib")
	#pragma comment(lib, "../../lib/cpulib/lib/cpu-render.lib")
	#pragma comment(lib, "../../lib/cpulib/lib/cpu-engine.lib")
#endif
#include <SDKDDKVer.h>
#define CPU cpuEngine

// lib
#pragma comment(lib, "lib.lib")
#include "../../src/lib/Reseau/TCPSocket.h"
#include "../../src/lib/Reseau/TThread.h"
#include "../../src/lib/Reseau/UDPSocket.h"
#include "../../src/lib/Reseau/Pack.h"

static WSADATA data;
struct WinSockHandler
{
	static int Start()
	{
		return WSAStartup(MAKEWORD(2, 2), &data);
	}
	static void Clean()
	{
		WSACleanup();
	}
};

//INFOS 
constexpr int WINDOW_WIDTH = 640; // 512 | 1024
constexpr int WINDOW_HEIGHT = 360; // 256 | 576

#define INPUT_IP // if not defined, the Host IP will be used

#define MAX_BUFFER_SIZE 1024
#define FIXED_DT 0.016f
#define MAX_WAITING_TIME 4.f
constexpr const char* HOST_IP = "10.10.137.47"; //"127.0.0.1" "10.10.137.56" "10.10.137.47"

constexpr int HOST_PORT = 1888;

// GAMEDATA CONSTANTS
inline float Z_ZOOM_OUT = -20.f;

constexpr int KILL_HP_REWARD = 20;

constexpr float PLAYER_RADIUS = 1.f;
constexpr int PLAYER_MAX_HP = 100;
constexpr float PLAYER_SPEED = 8.f;
constexpr float PLAYER_RELOAD_SPEED = 0.4f;
constexpr float PLAYER_SPEED_MULTIPLIER = 2.f;
constexpr float PLAYER_RESPAWN_TIME = 3.f;

constexpr float BALL_SPEED = 30.f;
constexpr float BALL_RADIUS = 0.25f;
constexpr float BALL_LIFETIME = 2.f;
constexpr int BALL_DAMAGE = 10;

constexpr XMFLOAT3 ARENA_SCALE = { 120, 120, PLAYER_RADIUS * 2 };
constexpr float OBSTACLE_THICKNESS = ARENA_SCALE.y * 0.02f;
