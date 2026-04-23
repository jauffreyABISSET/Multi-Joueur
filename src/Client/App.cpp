#include "pch.h"
#include "App.h"
#include <unordered_set>

#include "../lib/CPU/KeyInputs.h"
#include "../lib/Reseau/PingPongPack.h"
#include "../lib/CPU/Arena.h"
#include "../lib/Reseau/LoginPack.h"
#include "../lib/Reseau/MessagePack.h"

#include "ThreadGameplayMessages.h"

#include "../lib/Maths.h"

App::App()
{
	s_pApp = this;
	CPU_CALLBACK_START(OnStart);
	CPU_CALLBACK_UPDATE(OnUpdate);
	CPU_CALLBACK_EXIT(OnExit);
	CPU_CALLBACK_RENDER(OnRender);
}
App::~App()
{
}

//Main Methods
void App::OnStart()
{
	ResetAllClientDatas();
	InitializeCriticalSection(&mCS_PacksQueue);
	InitializeCriticalSection(&mCS_Pack_ID);
	InitializeCriticalSection(&mCS_SocketAccess);

	for (size_t i = 0; i < (int)PackType::Count; i++)
	{
		mLastPackIDSeen[i] = 0;
	}

	mpFont_Score = new cpu_font();
	mpFont_Score->Create(14, CPU_CYAN, "ok");

	mpFont_Score_1 = new cpu_font();
	mpFont_Score_1->Create(13, CPU_GOLD, "ok");

	mpFont_Score_2 = new cpu_font();
	mpFont_Score_2->Create(13, CPU_SILVER, "ok");

	mpFont_Score_3 = new cpu_font();
	mpFont_Score_3->Create(13, CPU_BRONZE, "ok");

	mpFont_Event = new cpu_font();
	mpFont_Event->Create(15, CPU_WHITE, "ok");

	mpFont_Kills = new cpu_font();
	mpFont_Kills->Create(14, CPU_RED, "ok");

	XMFLOAT3 color1 = { 0, 0, 0.1f };
	XMFLOAT3 color2 = { 0.05f, 0, 0.1f };

	CPU.m_groundColor = color1;
	CPU.m_skyColor = color2;
	CPU.m_amigaStyle = true;

	CPU.GetParticleData()->Create(1000000);
}
void App::OnUpdate()
{
	if (mIsRunning == false)
		return;

	if (mIsLogged == false)
		LoginState();
	else
		MainLoopState();
}
void App::OnExit()
{
	ResetAllClientDatas();

	mIsRunning = false;

	HANDLE h = mpThreadGameplayMessages->GetHandle();

	WaitForSingleObject(h, 5000);

	EnterCriticalSection(&mCS_SocketAccess);
	delete mpSock;
	mpSock = nullptr;
	LeaveCriticalSection(&mCS_SocketAccess);

	DeleteCriticalSection(&mCS_PacksQueue);
	DeleteCriticalSection(&mCS_Pack_ID);
	DeleteCriticalSection(&mCS_SocketAccess);

	delete mpFont_Score_1;
}
void App::OnRender(int pass)
{
	DrawTexts();
	DrawMap();
}
void App::MyPixelShader(cpu_ps_io& io)
{
	io.color = io.p.color;
}

//Login
bool App::ConfigSocket()
{
#ifdef INPUT_IP
	std::string ip;
	std::cout << "Set the Host IP : ";

	std::getline(std::cin, ip);

	if (mpSock->SetAddrIn(AF_INET, ip.c_str(), HOST_PORT) == false)
	{
		std::cerr << "Error Client SetAddrIn()";
		return false;
	}

	return true;
#else
	if (mpSock->SetAddrIn(AF_INET, HOST_IP, HOST_PORT) == false)
	{
		std::cerr << "Error Client SetAddrIn()";
		return false;
	}

	return true;
#endif
}
void App::LoginState()
{
	if (mpSock != nullptr)
		return;

	ToggleConsole(true);
	ToggleWindow(false);

	EnterCriticalSection(&mCS_SocketAccess);
	mpSock = new UDPSocket();
	LeaveCriticalSection(&mCS_SocketAccess);

	DWORD time = 500;
	int sizeTime = sizeof(int);
	setsockopt(mpSock->GetSOCKET(), SOL_SOCKET, SO_RCVTIMEO, (char*)&time, sizeTime);

	TryLogin();

	DWORD endTime = -1;
	setsockopt(mpSock->GetSOCKET(), SOL_SOCKET, SO_RCVTIMEO, (char*)&time, endTime);

	mpThreadGameplayMessages = new ThreadGameplayMessages;
	mpThreadGameplayMessages->InitThread(false);

	ToggleConsole(false);
	ToggleWindow(true);
}
std::string App::GetInputName()
{
	std::string name = "";

	while (name.empty())
	{
		std::getline(std::cin, name);
	}

	name.substr(0, USERNAME_MAX_SIZE - 1);

	return name;
}
void App::TryLogin()
{
	int tryCount = 0;
	int tryAmount = 5;

	while (APP.mIsLogged == false && APP.mIsRunning)
	{
		ConfigSocket();

		if (tryCount > tryAmount)
		{
			tryCount = 0;
			std::cerr << "Enter your username again : \n";
		}
		else
		{
			std::cout << "Welcome ! Please enter your username (max " << USERNAME_MAX_SIZE << " chars) : \n";
		}

		sockaddr_in addr = mpSock->GetSockAddr();

		std::string name = GetInputName();

		uint16_t id = -1;

		EnterCriticalSection(&mCS_Pack_ID);
		id = GetNewPackID();
		LeaveCriticalSection(&mCS_Pack_ID);

		const int packSize = sizeof(LoginAskPack);
		LoginAskPack p;
		p.Set(id, PackType::CS_LOGIN_ASK_PACK, packSize);
		p.SetMessage(name.c_str());

		char bufferSend[packSize] = { 0 };
		memcpy(bufferSend, &p, packSize);

		if (mpSock->SendTo(bufferSend, packSize, (sockaddr*)&addr, sizeof(addr)) != -1)
		{
			std::cout << "Message Send to Server !\n";
		}

		while (tryCount <= tryAmount)
		{
			sockaddr_in from;
			char bufferRecv[MAX_BUFFER_SIZE] = { 0 };

			int bytes_received = mpSock->ReceiveFrom(bufferRecv, sizeof(bufferRecv), from);

			std::cout << "Error : " << WSAGetLastError() << std::endl;

			if (bytes_received == sizeof(LoginAnswerPack))
			{
				LoginAnswerPack* pAnswer = reinterpret_cast<LoginAnswerPack*>(bufferRecv);
				std::string content = pAnswer->message;

				std::cout << pAnswer->message << std::endl;

				if (pAnswer->status == VALID_LOGIN)
				{
					mUsername = std::string(p.message);
					mIsLogged = true;
					mTimeSinceLastPing.Init(MAX_WAITING_TIME);
					InitializClientDatas();

					break;
				}
			}

			tryCount++;
		}
	}
}

//Game Logic
void App::MainLoopState()
{
	std::cout << "Client Created Balls : " << mBalls.size() << std::endl;

	mTimeSinceLastPing.Update(cpuTime.delta);

	if (mTimeSinceLastPing.IsTimeOut() || cpuInput.IsKeyDown(VK_ESCAPE))
	{
		mTimeSinceLastPing.ResetTime();
		std::cerr << "Server isn't responding !\n";

		ResetAllClientDatas();

		EnterCriticalSection(&mCS_SocketAccess);
		delete mpSock;
		mpSock = nullptr;
		LeaveCriticalSection(&mCS_SocketAccess);

		WaitForSingleObject(mpThreadGameplayMessages->GetHandle(), INFINITE);

		delete mpThreadGameplayMessages;
		mpThreadGameplayMessages = nullptr;

		return;
	}

	HandlePacks();
	HandleEntities();
	HandleInputs();

	SendPacks();
}
void App::HandleEntities()
{
	for (auto it = mBalls.begin(); it != mBalls.end();)
	{
		if (it->second->mToDestroy)
		{
			delete it->second;
			it = mBalls.erase(it);
		}
		else
		{
			it->second->Update(cpuTime.delta);
			++it;
		}
	}

	for (auto it = mPlayers.begin(); it != mPlayers.end();)
	{
		if (it->second->mToDestroy)
		{
			delete it->second;
			it = mPlayers.erase(it);
		}
		else
		{
			it->second->Update(cpuTime.delta);
			++it;
		}
	}
}
void App::HandleInputs()
{
	CalculateMouseDirection();

	mInput = (uint8_t)Input::NONE;

	if (cpuInput.IsKey(VK_Q))
		mInput |= (uint8_t)Input::LEFT;
	if (cpuInput.IsKey(VK_D))
		mInput |= (uint8_t)Input::RIGHT;
	if (cpuInput.IsKey(VK_Z))
		mInput |= (uint8_t)Input::FORWARD;
	if (cpuInput.IsKey(VK_S))
		mInput |= (uint8_t)Input::BACKWARD;

	if (cpuInput.IsKey(VK_LBUTTON))
		mInput |= (uint8_t)Input::MOUSELEFT;
	if (cpuInput.IsKey(VK_LSHIFT))
		mInput |= (uint8_t)Input::SHIFT;
}
void App::CalculateMouseDirection()
{
	RECT windowSize;
	GetClientRect(cpuWindow.GetHWND(), &windowSize);
	float clientWidth = float(windowSize.right - windowSize.left);
	float clientHeight = float(windowSize.bottom - windowSize.top);

	POINT mousePos;
	GetCursorPos(&mousePos);
	ScreenToClient(cpuWindow.GetHWND(), &mousePos);
	mousePos.x = std::clamp((float)mousePos.x, 0.f, (float)clientWidth);
	mousePos.y = std::clamp((float)mousePos.y, 0.f, (float)clientHeight);

	XMFLOAT3 middleScreen = { clientWidth * 0.5f, clientHeight * 0.5f, 0 };
	XMFLOAT3 v = Maths::GetDirectionVector(middleScreen, XMFLOAT3(mousePos.x, clientHeight - mousePos.y, 0));
	Maths::SelfNormalize(v);

	mDirection = v;
}

//UI
void App::DrawTexts()
{
	auto pDevice = CPU.GetDevice();

	std::string event = "Event : " + mCurrentServerEvent;
	pDevice->DrawText(mpFont_Event, event.c_str(), (float)WINDOW_WIDTH * 0.5f, 10.f, 1);

	auto it = mPlayers.find(mUsername);
	if (it == mPlayers.end())
		return;

	Player* me = it->second;

	std::string text = "Kills\n" + std::to_string(me->GetKillCount());

	CPU.GetDevice()->DrawText(mpFont_Event, mUsername.c_str(), 60, 7, 1);
	CPU.GetDevice()->DrawText(mpFont_Kills, text.c_str(), 60, 25, 1);

	struct Entry
	{
		std::string name;
		uint16_t kills;
	};

	Entry first = { "NONE", 0 };
	Entry second = { "NONE", 0 };
	Entry third = { "NONE", 0 };

	for (auto& [name, player] : mPlayers)
	{
		uint16_t k = player->GetKillCount();

		if (k > first.kills)
		{
			third = second;
			second = first;
			first = { name, k };
		}
		else if (k > second.kills)
		{
			third = second;
			second = { name, k };
		}
		else if (k > third.kills)
		{
			third = { name, k };
		}
	}

	int x = 20;
	int y = 60;
	int line = 22;

	pDevice->DrawText(mpFont_Score, "TOP 3 KILLS", x, y);
	pDevice->DrawText(mpFont_Score_1, ("1. " + first.name + " : " + std::to_string(first.kills)).c_str(), x, y + line);
	pDevice->DrawText(mpFont_Score_2, ("2. " + second.name + " : " + std::to_string(second.kills)).c_str(), x, y + 2 * line);
	pDevice->DrawText(mpFont_Score_3, ("3. " + third.name + " : " + std::to_string(third.kills)).c_str(), x, y + 3 * line);
}
void App::DrawMap()
{
	auto pDevice = CPU.GetDevice();

	XMFLOAT3 mapColor(1, 1, 1);

	XMFLOAT3 playerColor(0, 1, 0);
	XMFLOAT3 otherColor(1, 0, 0);

	if (ARENA_SCALE.y == 0)
		return;

	float offset = 10;
	float ratio = ARENA_SCALE.x / ARENA_SCALE.y;

	XMFLOAT2 pos = {};
	XMFLOAT2 dim = {};

	if (ratio > 1)
	{
		pos = { (float)(WINDOW_WIDTH - mMapPixelsScale) - offset, offset };
		dim = { (float)mMapPixelsScale, (float)(mMapPixelsScale / (float)ratio) };
	}
	else
	{
		pos = { (float)(WINDOW_WIDTH - (float)mMapPixelsScale * ratio - offset), offset };
		dim = { (float)mMapPixelsScale * ratio, (float)mMapPixelsScale };
	}

	pDevice->DrawRectangle(pos.x, pos.y, dim.x, dim.y, mapColor);

	for (auto [name, pPos] : mPlayersPos)
	{
		float nx = (pPos.x + ARENA_SCALE.x * 0.5f) / ARENA_SCALE.x;
		float ny = (pPos.y + ARENA_SCALE.y * 0.5f) / ARENA_SCALE.y;

		float minimapX = pos.x + nx * dim.x;
		float minimapY = pos.y + (1 - ny) * dim.y;

		float size = (float)mMapPixelsScale * 0.035f;
		float playerSize = (float)mMapPixelsScale * 0.05f;

		if(name == mUsername)
			pDevice->DrawRectangle(minimapX - playerSize * 0.5f, minimapY - playerSize * 0.5f, playerSize, playerSize, playerColor);
		else
			pDevice->DrawRectangle(minimapX - size * 0.5f, minimapY - size * 0.5f, size, size, otherColor);
	}
}

//Handle Packs
void App::HandlePacks()
{
	std::list<char*> localQueue = {};

	EnterCriticalSection(&mCS_PacksQueue);
	localQueue = mPacksQueue; // empty the server queue and stock the all content in localQueue
	mPacksQueue.clear();
	LeaveCriticalSection(&mCS_PacksQueue);

	for (char* buffer : localQueue)
	{
		if (buffer == nullptr)
			return;

		Pack* pPack = reinterpret_cast<Pack*>(buffer);

		switch (pPack->header)
		{
		case PackType::SC_PING_PACK:
			mTimeSinceLastPing.ResetTime();
			mSendPong = true;
			break;

		case PackType::SC_PLAYERGAME_FRAGMENT_PACK:
			if (IsPackMoreRecent(pPack->id, &mLastPackIDSeen[(int)PackType::SC_PLAYERGAME_FRAGMENT_PACK]))
				HandlePlayerGameFragmentPacks(buffer);
			break;

		case PackType::SC_BALLGAME_FRAGMENTS_PACK:
			HandleBallGameFragmentPacks(buffer);
			break;

		case PackType::SC_EVENT_MESSAGE_PACK:
			if (IsPackMoreRecent(pPack->id, &mLastPackIDSeen[(int)PackType::SC_EVENT_MESSAGE_PACK]))
				HandleEventMessagePack(buffer);
			break;
		}

		delete buffer;
	}
}
void App::HandlePlayerGameFragmentPacks(char* buffer)
{
	PlayerGamePack* pgpFrag = reinterpret_cast<PlayerGamePack*>(buffer);

	if (pgpFrag == nullptr)
		return;

	if (mPlayerFragments.size() != pgpFrag->fragmentCount)
	{
		mPlayerFragments.resize(pgpFrag->fragmentCount);
		mPlayerFragmentsReceived.resize(pgpFrag->fragmentCount, false);
	}

	mPlayerFragments[pgpFrag->fragmentIndex] = *pgpFrag;
	mPlayerFragmentsReceived[pgpFrag->fragmentIndex] = true;

	bool isFull = true;

	for (bool received : mPlayerFragmentsReceived)
	{
		if (received == false)
		{
			isFull = false;
			break;
		}
	}

	if (isFull == false)
		return;

	HandlePlayerGamePack();
}
void App::HandlePlayerGamePack()
{
	std::unordered_set<std::string> receivedUsernames;

	for (PlayerGamePack& pgp : mPlayerFragments)
	{
		for (size_t i = 0; i < pgp.count; i++)
		{
			PlayerDataToSend dat = pgp.playerData[i];
			std::string currentPlayerUsername = std::string(dat.username);
			receivedUsernames.insert(currentPlayerUsername);

			auto it = mPlayers.find(currentPlayerUsername);

			XMFLOAT3 pos = dat.pos;
			XMFLOAT3 color = (mUsername == currentPlayerUsername) ? XMFLOAT3(0, 0, 1.f) : XMFLOAT3(1.f, 0, 0);

			Player* pPlayer = nullptr;

			if (it != mPlayers.end())
				pPlayer = it->second;
			else
			{
				pPlayer = new Player(PLAYER_RADIUS, color);
				mPlayers[currentPlayerUsername] = pPlayer;
			}

			//Map update
			mPlayersPos[currentPlayerUsername] = { pos.x, pos.y };

			if (dat.isAlive == false)
			{
				mPlayersPos.erase(currentPlayerUsername);
			}

			//Datas
			pPlayer->SetPosition(pos);
			pPlayer->SetHP(dat.hp);
			pPlayer->SetRotation(dat.rot);

			//UI
			pPlayer->UpdateHealthBar();
			pPlayer->ToggleVisibilty(dat.isAlive);
			pPlayer->ToggleBarVisibilty(dat.isAlive);
			pPlayer->SetKillCount(dat.killCount);

			//Particles
			pPlayer->SetEmitterState(dat.isAlive);
			pPlayer->GetEmitter()->pos = pPlayer->GetPosition();

			if (mUsername == currentPlayerUsername)
			{
				myCurrentPos = { pos.x, pos.y };
				CameraFollow({ pos.x, pos.y, Z_ZOOM_OUT });
			}
		}
	}

	//Destroy Obsolete Players
	for (auto& [name, player] : mPlayers)
	{
		if (receivedUsernames.find(name) == receivedUsernames.end())
		{
			player->mToDestroy = true;
			
			mPlayersPos.erase(name);
		}
	}
}
void App::HandleBallGameFragmentPacks(char* buffer)
{
	BallGameFragmentPack* bgpFrag = reinterpret_cast<BallGameFragmentPack*>(buffer);

	if (bgpFrag == nullptr)
		return;

	if (mBallFragments.size() != bgpFrag->fragmentCount)
	{
		mBallFragments.resize(bgpFrag->fragmentCount);
		mBallFragmentsReceived.resize(bgpFrag->fragmentCount, false);
	}

	mBallFragments[bgpFrag->fragmentIndex] = *bgpFrag;
	mBallFragmentsReceived[bgpFrag->fragmentIndex] = true;

	bool isFull = true;

	for (bool received : mBallFragmentsReceived)
	{
		if (received == false)
		{
			isFull = false;
			break;
		}
	}

	if (isFull == false)
		return;

	HandleBallGamePack();
}
void App::HandleBallGamePack()
{
	std::unordered_set<uint32_t> receivedIDs;

	for (BallGameFragmentPack& bgp : mBallFragments)
	{
		for (size_t i = 0; i < bgp.count; i++)
		{
			BallDataToSend dat = bgp.ballData[i];
			Ball* pBall = nullptr;

			XMFLOAT3 color = XMFLOAT3(1.f, 1.f, 0.f);

			receivedIDs.insert(dat.id);

			auto it = mBalls.find(dat.id);

			if (it != mBalls.end())
			{
				pBall = it->second;
			}

			//Add New Balls
			if (pBall == nullptr)
			{
				pBall = new Ball(dat.radius, color, dat.lifeTime);
				mBalls[dat.id] = pBall;
			}

			pBall->SetPosition(dat.pos);
			pBall->GetEmitter()->pos = pBall->GetPosition();
		}
	}

	for (auto it = mBalls.begin(); it != mBalls.end(); ++it)
	{
		if (receivedIDs.find(it->first) == receivedIDs.end())
		{
			it->second->mToDestroy = true;
		}
	}
}
void App::HandleEventMessagePack(char* buffer)
{
	MessagePack* pEvent = reinterpret_cast<MessagePack*>(buffer);

	if (pEvent == nullptr)
		return;

	mCurrentServerEvent = std::string(pEvent->message);
}

//Send Packs
void App::SendPacks()
{
	if (mIsLogged == false)
		return;

	if (mpSock == nullptr)
		return;

	sockaddr_in addr = mpSock->GetSockAddr();
	uint16_t id = -1;

	EnterCriticalSection(&mCS_Pack_ID);
	id = GetNewPackID();
	LeaveCriticalSection(&mCS_Pack_ID);

	if (mSendPong == true)
	{
		char* pongBuffer = MakePongPack();
		mpSock->SendTo(pongBuffer, sizeof(PongPack), (sockaddr*)&addr, sizeof(addr));
		delete pongBuffer;
	}

	char* inputBuffer = MakeInputPack();
	mpSock->SendTo(inputBuffer, sizeof(InputPack), (sockaddr*)&addr, sizeof(addr));
	delete inputBuffer;
}
char* App::MakePongPack()
{
	EnterCriticalSection(&mCS_Pack_ID);
	uint16_t id = GetNewPackID();
	LeaveCriticalSection(&mCS_Pack_ID);

	const int packSize = sizeof(PongPack);
	PongPack pongPack;
	pongPack.Set(id, PackType::CS_PONG_PACK, packSize);

	char* buffer = new char[packSize];
	memcpy(buffer, &pongPack, packSize);

	return buffer;
}
char* App::MakeInputPack()
{
	EnterCriticalSection(&mCS_Pack_ID);
	uint16_t id = GetNewPackID();
	LeaveCriticalSection(&mCS_Pack_ID);

	const int packSize = sizeof(InputPack);
	InputPack p;
	p.Set(id, PackType::CS_INPUT_PACK, packSize);
	p.input = mInput;
	p.dir = mDirection;

	char* buffer = new char[packSize];
	memcpy(buffer, &p, packSize);

	return buffer;
}

//Misc
void App::CameraFollow(XMFLOAT3 pos)
{
	CPU.GetCamera()->transform.SetPosition(pos.x, pos.y, pos.z);
}
bool App::IsEntityVisisble(XMFLOAT3 entityPos)
{
	//TODO
	XMFLOAT3 camPos = CPU.GetCamera()->transform.pos;

	return false;
}
void App::ResetAllClientDatas()
{
	CameraFollow({ 0, 0, 0 });

	mUsername = "";
	mIsLogged = false;

	for (auto it = mBalls.begin(); it != mBalls.end();)
	{
		delete it->second;
		it = mBalls.erase(it);
	}
	mBalls.clear();

	for (auto it = mPlayers.begin(); it != mPlayers.end();)
	{
		delete it->second;
		it = mPlayers.erase(it);
	}
	mPlayers.clear();

	delete mpArena;
	mpArena = nullptr;
}
void App::InitializClientDatas()
{
	mpArena = new Arena();
}
void App::ToggleConsole(bool state)
{
	if (state == true)
	{
		AllocConsole();
		FILE* fp;

		freopen_s(&fp, "CONOUT$", "w", stdout);
		freopen_s(&fp, "CONOUT$", "w", stderr);
		freopen_s(&fp, "CONIN$", "r", stdin);
	}
	else
	{
		FreeConsole();
	}
}
void App::ToggleWindow(bool state)
{
	if (state == true)
		ShowWindow(cpuWindow.GetHWND(), SW_SHOW);
	else
		ShowWindow(cpuWindow.GetHWND(), SW_HIDE);
}
