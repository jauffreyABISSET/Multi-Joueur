#pragma once
#include "../lib/CPU/Ball.h"
#include "../lib/Reseau/InputPack.h"
#include "../lib/CPU/Player.h"
#include <list>

#include "Timer.h"

#define APP App::GetInstance()

class ThreadGameplayMessages;

class Arena;

class App
{
public:
	App();
	virtual ~App();

	//Get
	static App& GetInstance() { return *s_pApp; }
	UDPSocket* GetSocket() { return mpSock; }

	//Main Methods
	void OnStart();
	void OnUpdate();
	void OnExit();
	void OnRender(int pass);
	static void MyPixelShader(cpu_ps_io& io);

	//Login
	bool ConfigSocket();
	void LoginState();
	std::string GetInputName();
	void TryLogin();

	//Game Logic
	void MainLoopState();
	void HandleInputs();
	void CalculateMouseDirection();
	void HandleEntities();

	//UI
	void DrawTexts();
	void DrawMap();

	//Handle Packs
	void HandlePacks();
	void HandlePlayerGameFragmentPacks(char* buffer);
	void HandlePlayerGamePack();
	void HandleBallGameFragmentPacks(char* buffer);
	void HandleBallGamePack();

	void HandleEventMessagePack(char* buffer);

	//Send Packs
	void SendPacks();
	char* MakePongPack();
	char* MakeInputPack();

	//Misc
	void CameraFollow(XMFLOAT3 pos);
	bool IsEntityVisisble(XMFLOAT3 entityPos);
	void ResetAllClientDatas();
	void InitializClientDatas();
	void ToggleConsole(bool state);
	void ToggleWindow(bool state);

private:
	inline static App* s_pApp = nullptr;
	UDPSocket* mpSock = nullptr;

	//Input values to send
	bool mSendPong = false;

	XMFLOAT3 mDirection = { 0, 0, 0 };
	uint8_t mInput = (uint8_t)Input::NONE;

	//Game Datas to display
	std::string mCurrentServerEvent;
	std::unordered_map<std::string, Player*> mPlayers;
	std::unordered_map<std::string, XMFLOAT2> mPlayersPos;
	std::unordered_map<uint32_t, Ball*> mBalls;	

	//Minimap
	XMFLOAT2 myCurrentPos = { 0, 0 };
	int mMapPixelsScale = 60;

	//Fragmentation packs Testing
	std::vector<BallGameFragmentPack> mBallFragments;
	std::vector<bool> mBallFragmentsReceived;

	std::vector<PlayerGamePack> mPlayerFragments;
	std::vector<bool> mPlayerFragmentsReceived;
public:
	//Datas
	bool mIsRunning = true;
	bool mIsLogged = false;
	std::string mUsername = "";
	Timer mTimeSinceLastPing;
	Arena* mpArena = nullptr;
	cpu_font* mpFont_Score = nullptr;
	cpu_font* mpFont_Score_1 = nullptr;
	cpu_font* mpFont_Score_2 = nullptr;
	cpu_font* mpFont_Score_3 = nullptr;
	cpu_font* mpFont_Event = nullptr;
	cpu_font* mpFont_Kills = nullptr;

	//Packs
	std::list<char*> mPacksQueue;
	uint16_t mLastPackIDSeen[(int)PackType::Count];

	//CS
	CRITICAL_SECTION mCS_SocketAccess;
	CRITICAL_SECTION mCS_PacksQueue;
	CRITICAL_SECTION mCS_Pack_ID;

	//Threads
	ThreadGameplayMessages* mpThreadGameplayMessages = nullptr;
};

