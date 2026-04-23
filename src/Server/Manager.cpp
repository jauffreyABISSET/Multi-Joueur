#include "pch.h"
#include "Manager.h"

#include <random>
#include <chrono>

#include "../src/lib/Reseau/TThread.h"
#include "ThreadReceiveClients.h"

#include "../lib/Reseau/MessagePack.h"
#include "../lib/Reseau/InputPack.h"
#include "../lib/Reseau/PingPongPack.h"

#include "../lib/Maths.h"
#include "../lib/ServerEvent.h"

Manager* Manager::m_pInstance = nullptr;

void Manager::CalculateDeltaTime()
{
	using clock = std::chrono::high_resolution_clock;

	static auto lastTime = clock::now();
	auto currentTime = clock::now();

	std::chrono::duration<float> delta = currentTime - lastTime;
	lastTime = currentTime;

	mDT = std::min(delta.count(), 0.05f);
}

bool Manager::ConfigSocket()
{
#ifdef INPUT_IP
	std::cin.clear();

	std::string ip = "";
	std::cout << "Set the IP to Bind : ";
	std::cin >> ip;

	if (!mSock.BindTo(ip.c_str(), HOST_PORT))
	{
		std::cout << "Erreur Server Bind !\n\n";
		return false;
	}
	else
	{
		std::cout << "Socket Connected !\n";
		return true;
	}
#else
	if (!mSock.BindTo(HOST_IP, HOST_PORT))
	{
		std::cout << "Erreur Server Bind !\n\n";
		return false;
	}
	else
	{
		std::cout << "Socket Connected !\n";
		return true;
	}
#endif
}

//Get
Manager* Manager::Get()
{
	if (m_pInstance == nullptr)
	{
		m_pInstance = new Manager();
	}

	return m_pInstance;
}
Client* Manager::GetClient(sockaddr_in addr)
{
	for (auto it = mAllClients.begin(); it != mAllClients.end(); ++it)
	{
		Client* pClient = it->second;

		if (pClient == nullptr)
			continue;

		sockaddr_in clientAddr = pClient->addr;

		if (clientAddr.sin_addr.s_addr == addr.sin_addr.s_addr)
		{
			return pClient;
		}
	}

	return nullptr;
}
void Manager::GetDefaultPlayerStateConfig(PlayerState& target)
{
	static std::mt19937 rng(std::random_device{}());

	float marginX = ARENA_SCALE.x * 0.5f;
	float marginY = ARENA_SCALE.y * 0.5f;

	float x = (-marginX * 0.5f) + std::generate_canonical< double, 128 >(rng) * marginX;
	float y = (-marginY * 0.5f) + std::generate_canonical< double, 128 >(rng) * marginY;
	target.dat.SetData(XMFLOAT3(x, y, 0), { 0, 0, 0 }, mPlayerMaxHp, true);
}

//Main Loop
void Manager::Run()
{
	Init();

	while (mIsOpen)
	{
		CalculateDeltaTime();
		Update();
	}

	DeleteCriticalSection(&mCS_clientAccess);
	DeleteCriticalSection(&mCS_Pack_ID);
	DeleteCriticalSection(&mCS_AddPacksToClientQueue);

	DeleteCriticalSection(&mCS_clientToAddAccess);
	DeleteCriticalSection(&mCS_clientToEditAccess);
	DeleteCriticalSection(&mCS_clientToRemoveAccess);
}
void Manager::Init()
{
	bool begin = false;

	while (begin == false)
	{
		begin = ConfigSocket();
	}

	InitializeCriticalSection(&mCS_Pack_ID);
	InitializeCriticalSection(&mCS_clientAccess);
	InitializeCriticalSection(&mCS_AddPacksToClientQueue);

	InitializeCriticalSection(&mCS_clientToAddAccess);
	InitializeCriticalSection(&mCS_clientToEditAccess);
	InitializeCriticalSection(&mCS_clientToRemoveAccess);

	mpThreadReceiveClients = new ThreadReceiveClients;
	mpThreadReceiveClients->InitThread(false);

	mUpdateTimer.Init(FIXED_DT);
	mDisplayClientTimer.Init(1.f);

	mEventTimer.Init(15.f, true);
	mCurrentEvent = ServerEvent::NONE;
	mIsEventActive = false;

	mTime.Init(0.3);
	mTime.Pause();
}
void Manager::Update()
{
	AddNewClients();
	EditClients();
	RemoveClients();

	HandleClientPacks();

	mDisplayClientTimer.Update(mDT);
	if (mDisplayClientTimer.IsTimeOut())
	{
		mDisplayClientTimer.ResetTime();
		DisplayClients();
	}

	//Ticks of Fixed DT
	mUpdateTimer.Update(mDT);
	if (mUpdateTimer.IsTimeOut())
	{
		mUpdateTimer.ResetTime();

		ApplyInputs();
		UpdateGame();
		HandleCollisions();

		mTick++;
		SendClientPacks();
	}

	mEventTimer.Update(mDT);
	if (mEventTimer.IsTimeOut())
	{
		std::string oldEvent = mCurrentEvent;

		if (mIsEventActive == false)
		{
			int randomTime = std::rand() % 15 + 10;
			mEventTimer.SetTargetTime(randomTime);
			ManageEvent();
		}
		else
		{
			mEventTimer.SetTargetTime(10.f);
			mCurrentEvent = ServerEvent::NONE;
		}

		HandleCurrentEvent(oldEvent);
		mEventTimer.ResetTime();
	}
}
void Manager::UpdateGame()
{
	if (mAllClients.empty())
		return;

	for (auto it = mAllClients.begin(); it != mAllClients.end(); ++it)
	{
		Client* pClient = it->second;

		if (pClient == nullptr)
			continue;

		//Check if Client is Alive
		Timer& lastPingClientTimer = pClient->lastPingTimer;
		PlayerState& playerState = pClient->playerState;

		lastPingClientTimer.Update(FIXED_DT);
		if (lastPingClientTimer.IsTimeOut())
		{
			lastPingClientTimer.ResetTime();
			pClient->isAFK = true;
			playerState.dat.hp = 0;
			playerState.dat.isAlive = 0;
		}

		Timer& playerRespawnTimer = playerState.respawnTime;
		playerRespawnTimer.Update(FIXED_DT);

		if (playerState.dat.isAlive == 0 && pClient->isAFK == false)
		{
			playerRespawnTimer.Resume();
			if (playerRespawnTimer.IsTimeOut())
			{
				playerRespawnTimer.ResetTime();
				playerRespawnTimer.Pause();
				GetDefaultPlayerStateConfig(playerState);
			}
		}

		//GAME LOGIC
		XMFLOAT3& pos1 = playerState.dat.pos;
		Maths::LockSphereInsideCube(pos1, PLAYER_RADIUS, { 0, 0, 0 }, ARENA_SCALE);

		playerState.reloadTimer.Update(FIXED_DT);

		//Players Repulse
		for (auto it2 = mAllClients.begin(); it2 != mAllClients.end(); ++it2)
		{
			Client* pClient2 = it2->second;

			XMFLOAT3& pos2 = pClient2->playerState.dat.pos;

			if (pClient == pClient2)
				continue;

			if (pClient2->isAFK || pClient2->playerState.dat.isAlive == false)
				continue;

			Maths::RepulseSphereSphere(pos1, PLAYER_RADIUS, pos2, PLAYER_RADIUS);
		}
	}

	for (auto it = mBallStates.begin(); it != mBallStates.end();)
	{
		BallState& ballState = *it;
		BallDataToSend& dat = it->dat;

		XMFLOAT3 velocity = { ballState.dir.x * ballState.speed, ballState.dir.y * ballState.speed, ballState.dir.z * ballState.speed };

		dat.pos.x += velocity.x * FIXED_DT;
		dat.pos.y += velocity.y * FIXED_DT;
		dat.pos.z += velocity.z * FIXED_DT;

		Timer& timer = it->lifeTime;
		timer.Update(FIXED_DT);

		if (timer.IsTimeOut())
		{
			it = mBallStates.erase(it);
		}
		else
			++it;
	}
	if (mChaosMode == true)
	{
		HandleChaosMode();
	}
	else
	{
		mTime.ResetTime();
	}
}
void Manager::HandleChaosMode()
{
	mTime.Update(FIXED_DT);
	if (mTime.IsTimeOut())
	{
		mTime.ResetTime();
		for (auto& [name, client] : mAllClients)
		{
			client->playerState.dat.hp -= 1;
		}
	}
}
void Manager::ApplyInputs()
{
	auto localQueue = mInputsQueue;
	mInputsQueue.clear();

	for (auto& [client, pInput] : localQueue)
	{
		if (client->playerState.dat.isAlive == false)
			continue;

		PlayerState& playerState = client->playerState;
		XMFLOAT3& playerRot = playerState.dat.rot;
		XMFLOAT3& playerPos = playerState.dat.pos;

		uint8_t currentInput = pInput->input;

		float boost = 1;
		if (currentInput & (uint8_t)Input::SHIFT)
			boost = mPlayerSpeedMultiplier;

		ApplyMovement(playerPos, currentInput, mPlayerSpeed * boost);

		if (mSpinningMode == false)
			ApplyCannonRot(playerRot, pInput->dir);
		else
			playerRot.z -= mSpinningSpeed * FIXED_DT * boost;

		XMFLOAT3 forward = Maths::GetWorldForwardFromYPR({ 0, 1, 0 }, playerRot);

		if (currentInput & (uint8_t)Input::MOUSELEFT)
		{
			Timer& reloadTimer = client->playerState.reloadTimer;

			if (reloadTimer.IsTimeOut())
			{
				reloadTimer.ResetTime();

				float speedBoost = 1.f;

				if (mCurrentEvent == ServerEvent::SNIPER_MODE)
				{
					if (boost == 0)
						speedBoost = 2.f;

					CreateBall(playerPos, client->id, forward, mBallSpeed * speedBoost);
				}
				else
				{
					if (boost == 1)
						CreateBall(playerPos, client->id, forward, mBallSpeed * speedBoost);
				}

				if (mPoisonMode == true)
					playerState.dat.hp -= 5;
			}
		}

		if (playerState.dat.hp <= 0)
			playerState.dat.isAlive = 0;

		delete pInput;
	}
}
void Manager::ApplyCannonRot(XMFLOAT3& entityRot, XMFLOAT3 dir)
{
	if (mDrunkMode == false)
	{
		float angleRad = std::atan2(dir.x, dir.y);
		entityRot.z = -angleRad;
	}
	else
	{
		float angleRad = std::atan2(dir.x, -dir.y);
		entityRot.z = angleRad;
	}
}
void Manager::ApplyMovement(XMFLOAT3& entityPos, uint8_t currentInput, float speed)
{
	XMFLOAT3 moveDir = { 0, 0, 0 };

	if (currentInput & (uint8_t)Input::RIGHT)
		moveDir.x += 1;
	if (currentInput & (uint8_t)Input::LEFT)
		moveDir.x -= 1;

	if (currentInput & (uint8_t)Input::FORWARD)
		moveDir.y += 1;
	if (currentInput & (uint8_t)Input::BACKWARD)
		moveDir.y -= 1;

	if (moveDir.x == 0 && moveDir.y == 0)
		return;

	if (mDrunkMode)
	{
		moveDir.x = -moveDir.x;
		moveDir.y = -moveDir.y;
	}

	Maths::SelfNormalize(moveDir);

	entityPos.x += moveDir.x * speed * FIXED_DT;
	entityPos.y += moveDir.y * speed * FIXED_DT;
	entityPos.z += moveDir.z * speed * FIXED_DT;
}
void Manager::HandleCollisions()
{
	for (auto itBall = mBallStates.begin(); itBall != mBallStates.end();)
	{
		bool destroyBall = false;
		BallDataToSend& ballDat = itBall->dat;

		for (auto itClient = mAllClients.begin(); itClient != mAllClients.end(); ++itClient)
		{
			if (destroyBall)
				continue;

			Client* pClient = itClient->second;

			if (pClient->playerState.dat.isAlive == false)
			{
				continue;
			}

			XMFLOAT3& pos1 = pClient->playerState.dat.pos;
			PlayerDataToSend& targetDatas = pClient->playerState.dat;

			if (itBall->ownerID == pClient->id)
				continue;

			//BALL COLLISIONS
			if (Maths::IsCollisionSphereSphere(ballDat.pos, ballDat.radius, pos1, PLAYER_RADIUS))
			{
				uint8_t& targetHp = targetDatas.hp;
				targetHp = std::max(0, targetHp - mBallDamage);

				for (auto& [name, killerClient] : mAllClients)
				{
					if (killerClient->id == itBall->ownerID)
					{
						uint8_t& killerHp = killerClient->playerState.dat.hp;

						if (targetHp <= 0)
						{
							killerClient->playerState.dat.killCount++;

							if (mHealthMode == true)
							{
								killerHp = PLAYER_MAX_HP;
							}
							else
							{
								if (mOneShotMode == false)
								{
									killerHp += KILL_HP_REWARD;

									if (killerHp > PLAYER_MAX_HP)
									{
										killerHp = PLAYER_MAX_HP;
									}
								}
							}
							break;
						}
						else
						{
							if (mChaosMode)
							{
								killerHp += mBallDamage;
							}
						}
					}
				}

				if (targetHp <= 0)
				{
					targetDatas.isAlive = false;
				}

				destroyBall = true;
				itBall = mBallStates.erase(itBall);
			}
		}

		if (destroyBall == false)
			++itBall;
	}
}

//Handle Packs
void Manager::HandleClientPacks()
{
	EnterCriticalSection(&MANAGER->mCS_clientAccess);
	auto localClients = MANAGER->mAllClients;
	LeaveCriticalSection(&MANAGER->mCS_clientAccess);

	for (auto it = localClients.begin(); it != localClients.end(); ++it)
	{
		Client* c = it->second;

		if (c == nullptr)
			continue;

		std::list<char*> localQueue = {};

		EnterCriticalSection(&MANAGER->mCS_AddPacksToClientQueue);
		std::swap(localQueue, c->mPackQueue); // empty the client queue and stock the all content in localQueue
		LeaveCriticalSection(&MANAGER->mCS_AddPacksToClientQueue);

		for (char* buffer : localQueue)
		{
			Pack* pPack = reinterpret_cast<Pack*>(buffer);
			uint16_t id = pPack->id;

			switch (pPack->header)
			{
			case PackType::MESSAGE_PACK:
			{
				MessagePack* pMessage = reinterpret_cast<MessagePack*>(buffer);
				std::cout << "[" << id << "] | " << c->name << " : " << pMessage->message << std::endl;

				break;
			}
			case PackType::CS_INPUT_PACK:
				if (IsPackMoreRecent(id, &c->lastPackIDSeen[(int)PackType::CS_INPUT_PACK]))
				{
					InputPack* pInput = reinterpret_cast<InputPack*>(buffer);
					HandleInputPacks(c, pInput);
				}
				break;
			}

			delete buffer;
		}

		localQueue.clear();
	}
}
void Manager::HandleInputPacks(Client* c, InputPack* pInput)
{
	InputPack* pQueueInput = new InputPack();
	memcpy(pQueueInput, pInput, sizeof(InputPack));

	mInputsQueue[c] = pQueueInput;
}

//Send Packs
void Manager::SendClientPacks()
{
	EnterCriticalSection(&MANAGER->mCS_clientAccess);
	auto localClients = MANAGER->mAllClients;
	LeaveCriticalSection(&MANAGER->mCS_clientAccess);

	if (localClients.empty())
		return;

	char* pingBuffer = MakePingPack();
	std::vector<char*> playersBuffers = MakePlayerDataPack();
	std::vector<char*> ballsBuffers = MakeBallDataPacks();
	char* eventBuffer = MakeEventMessagePack();

	int amount = 0;

	for (auto it = localClients.begin(); it != localClients.end(); ++it)
	{
		Client* pCurrent = it->second;

		if (pCurrent->isAFK)
			continue;

		amount++;

		MANAGER->mSock.SendTo(pingBuffer, sizeof(PingPack), (sockaddr*)&pCurrent->addr, sizeof(pCurrent->addr));
		MANAGER->mSock.SendTo(eventBuffer, sizeof(MessagePack), (sockaddr*)&pCurrent->addr, sizeof(pCurrent->addr));

		for (char* buffer : playersBuffers)
		{
			MANAGER->mSock.SendTo(buffer, sizeof(PlayerGamePack), (sockaddr*)&pCurrent->addr, sizeof(pCurrent->addr));
		}

		for (char* buffer : ballsBuffers)
		{
			MANAGER->mSock.SendTo(buffer, sizeof(BallGameFragmentPack), (sockaddr*)&pCurrent->addr, sizeof(pCurrent->addr));
		}
	}

	for (char*& buffer : playersBuffers)
	{
		delete buffer;
	}

	for (char*& buffer : ballsBuffers)
	{
		delete buffer;
	}

	delete pingBuffer;
	delete eventBuffer;
}
std::vector<char*> Manager::MakePlayerDataPack()
{
	EnterCriticalSection(&MANAGER->mCS_clientAccess);
	auto& localClients = MANAGER->mAllClients;
	LeaveCriticalSection(&MANAGER->mCS_clientAccess);
	std::vector<Client*> clients = {};
	for (auto& [name, client] : localClients)
	{
		clients.push_back(client);
	}

	std::vector<char*> buffers = {};

	int dataAmount = localClients.size();
	if (dataAmount <= 0)
		return buffers;

	int fullPacks = dataAmount / MAX_PLAYERS;
	int rest = dataAmount % MAX_PLAYERS;

	EnterCriticalSection(&MANAGER->mCS_Pack_ID);
	uint16_t fullPackID = GetNewPackID();
	LeaveCriticalSection(&MANAGER->mCS_Pack_ID);

	for (size_t packIndex = 0; packIndex < fullPacks; packIndex++)
	{
		EnterCriticalSection(&MANAGER->mCS_Pack_ID);
		uint16_t id = GetNewPackID();
		LeaveCriticalSection(&MANAGER->mCS_Pack_ID);
		const int packSize = sizeof(PlayerGamePack);
		PlayerGamePack pgp;
		pgp.Set(id, PackType::SC_PLAYERGAME_FRAGMENT_PACK, packSize);
		pgp.SetFragment(fullPackID, packIndex, fullPacks + (rest > 0 ? 1 : 0), mTick);
		pgp.count = MAX_PLAYERS;

		size_t index = 0;

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			pgp.playerData[i] = clients[packIndex * MAX_PLAYERS + i]->playerState.dat;
		}

		char* buffer = new char[packSize];
		memcpy(buffer, &pgp, packSize);

		buffers.push_back(buffer);
	}

	if (rest > 0)
	{
		EnterCriticalSection(&MANAGER->mCS_Pack_ID);
		uint16_t id = GetNewPackID();
		LeaveCriticalSection(&MANAGER->mCS_Pack_ID);

		const int packSize = sizeof(PlayerGamePack);
		PlayerGamePack pgp;
		pgp.Set(id, PackType::SC_PLAYERGAME_FRAGMENT_PACK, packSize);
		pgp.SetFragment(fullPackID, fullPacks, fullPacks + 1, mTick);
		pgp.count = rest;

		int startIndex = fullPacks * MAX_PLAYERS;
		for (int i = 0; i < rest; ++i)
		{
			pgp.playerData[i] = clients[startIndex + i]->playerState.dat;
		}

		char* buffer = new char[packSize];
		memcpy(buffer, &pgp, packSize);

		buffers.push_back(buffer);
	}

	return buffers;
}
std::vector<char*> Manager::MakeBallDataPacks()
{
	std::vector<char*> buffers = {};

	int dataAmount = MANAGER->mBallStates.size();

	if (dataAmount <= 0)
		return buffers;

	int fullPacks = dataAmount / MAX_BALLS;
	int rest = dataAmount % MAX_BALLS;

	EnterCriticalSection(&MANAGER->mCS_Pack_ID);
	uint16_t fullPackID = GetNewPackID();
	LeaveCriticalSection(&MANAGER->mCS_Pack_ID);

	for (size_t packIndex = 0; packIndex < fullPacks; packIndex++)
	{
		EnterCriticalSection(&MANAGER->mCS_Pack_ID);
		uint16_t id = GetNewPackID();
		LeaveCriticalSection(&MANAGER->mCS_Pack_ID);

		const int packSize = sizeof(BallGameFragmentPack);
		BallGameFragmentPack bgp;
		bgp.Set(id, PackType::SC_BALLGAME_FRAGMENTS_PACK, packSize);
		bgp.SetFragment(fullPackID, packIndex, fullPacks + (rest > 0 ? 1 : 0), mTick);
		bgp.count = MAX_BALLS;

		for (int i = 0; i < MAX_BALLS; ++i)
		{
			bgp.ballData[i] = MANAGER->mBallStates[packIndex * MAX_BALLS + i].dat;
		}

		char* buffer = new char[packSize];
		memcpy(buffer, &bgp, packSize);

		buffers.push_back(buffer);
	}

	if (rest > 0)
	{
		EnterCriticalSection(&MANAGER->mCS_Pack_ID);
		uint16_t id = GetNewPackID();
		LeaveCriticalSection(&MANAGER->mCS_Pack_ID);

		const int packSize = sizeof(BallGameFragmentPack);
		BallGameFragmentPack bgp;
		bgp.Set(id, PackType::SC_BALLGAME_FRAGMENTS_PACK, packSize);
		bgp.SetFragment(fullPackID, fullPacks, fullPacks + 1, mTick);
		bgp.count = rest;

		int startIndex = fullPacks * MAX_BALLS;
		for (int i = 0; i < rest; ++i)
		{
			bgp.ballData[i] =
				MANAGER->mBallStates[startIndex + i].dat;
		}

		char* buffer = new char[packSize];
		memcpy(buffer, &bgp, packSize);

		buffers.push_back(buffer);
	}

	return buffers;
}
char* Manager::MakePingPack()
{
	EnterCriticalSection(&MANAGER->mCS_Pack_ID);
	uint16_t id = GetNewPackID();
	LeaveCriticalSection(&MANAGER->mCS_Pack_ID);
	const int packSize = sizeof(PingPack);
	PingPack pingPack;
	pingPack.Set(id, PackType::SC_PING_PACK, packSize);

	char* buffer = new char[packSize];
	memcpy(buffer, &pingPack, packSize);

	return buffer;
}
char* Manager::MakeEventMessagePack()
{
	EnterCriticalSection(&MANAGER->mCS_Pack_ID);
	uint16_t id = GetNewPackID();
	LeaveCriticalSection(&MANAGER->mCS_Pack_ID);
	const int packSize = sizeof(MessagePack);
	MessagePack eventPack;
	eventPack.Set(id, PackType::SC_EVENT_MESSAGE_PACK, packSize);
	eventPack.SetMessage(mCurrentEvent + " | " + std::to_string((int)mEventTimer.GetTime() + 1) + "s -> Next");

	char* buffer = new char[packSize];
	memcpy(buffer, &eventPack, packSize);

	return buffer;
}

//Client Handler
void Manager::AddNewClients()
{
	EnterCriticalSection(&mCS_clientToAddAccess);
	auto localClientsToAdd = mClientsToAdd;
	mClientsToAdd = {};
	LeaveCriticalSection(&mCS_clientToAddAccess);

	while (localClientsToAdd.empty() == false)
	{
		Client* pClient = localClientsToAdd.front();
		localClientsToAdd.pop();

		if (pClient == nullptr)
			continue;

		std::string name = pClient->name;

		if (mAllClients.count(name) == false)
		{
			mAllClients[name] = pClient;

			PlayerState playerState;

			GetDefaultPlayerStateConfig(playerState);

			playerState.dat.SetUsername(name);
			playerState.reloadTimer.Init(mPlayerReloadSpeed, true);
			playerState.respawnTime.Init(PLAYER_RESPAWN_TIME, true, true);
			playerState.dat.killCount = 0;

			pClient->playerState = playerState;
		}
	}
}
void Manager::EditClients()
{
	EnterCriticalSection(&mCS_clientToEditAccess);
	auto localClientsToEdit = mClientsToEdit;
	mClientsToEdit = {};
	LeaveCriticalSection(&mCS_clientToEditAccess);

	while (localClientsToEdit.empty() == false)
	{
		Client* pClient = localClientsToEdit.front().first;
		sockaddr_in newAddr = localClientsToEdit.front().second;

		localClientsToEdit.pop();

		auto it = mAllClients.find(pClient->name);

		if (it != mAllClients.end())
		{
			Client* pClientToEdit = it->second;

			for (size_t i = 0; i < (int)PackType::Count; i++)
			{
				pClientToEdit->lastPackIDSeen[i] = 0;
			}
			
			pClientToEdit->lastPingTimer.ResetTime();
			pClientToEdit->isAFK = false;
			pClientToEdit->addr = newAddr;

			GetDefaultPlayerStateConfig(it->second->playerState);
		}
	}
}
void Manager::RemoveClients()
{
	EnterCriticalSection(&mCS_clientToRemoveAccess);
	auto localClientsToRemove = mClientsToRemove;
	mClientsToRemove = {};
	LeaveCriticalSection(&mCS_clientToRemoveAccess);

	while (localClientsToRemove.empty() == false)
	{
		Client* pClient = localClientsToRemove.front();
		localClientsToRemove.pop();

		if (pClient == nullptr)
			continue;

		std::string name = pClient->name;

		if (mAllClients.count(name))
		{
			mAllClients.erase(name);

			delete pClient;
		}

		DisplayClients();
	}
}

//Misc
void Manager::DisplayClients()
{
	system("cls");

	std::cout << "All Clients : \n";
	std::cout << "Number = " << mAllClients.size() << std::endl;

	for (auto it = mAllClients.begin(); it != mAllClients.end(); ++it)
	{
		Client* c = it->second;

		std::string status = "Playing";

		if (c->isAFK)
			status = "Deconnected";

		std::cout << "[" << c->id << "] " << c->name << " | Status : " << status << " | ip : " << c->addr.sin_addr.s_addr << " | port : " << c->addr.sin_port << std::endl;
	}

	std::cout << "\n\n\n";
}
void Manager::CreateBall(const XMFLOAT3& pos, int clientID, XMFLOAT3 dir, float speed)
{
	BallState ballState;
	ballState.speed = speed;
	ballState.dir = Maths::Normalize(dir);;
	ballState.ownerID = clientID;

	BallDataToSend& b = ballState.dat;
	b.pos = pos;
	b.radius = mBallRadius;
	b.lifeTime = mBallLifeTime;

	static uint32_t newBallID = -1;
	b.id = newBallID++;

	ballState.lifeTime.Init(mBallLifeTime, true);
	mBallStates.push_back(ballState);
}
void Manager::UpdatePlayersReloadSpeed()
{
	for (auto& [name, client] : mAllClients)
	{
		client->playerState.reloadTimer.SetTargetTime(mPlayerReloadSpeed);
	}
}

void Manager::ManageEvent()
{
	int rdm = GenerateNumber();

	switch (rdm)
	{
	case 1:
		mCurrentEvent = ServerEvent::X2BALL;
		break;
	case 2:
		mCurrentEvent = ServerEvent::BIG_BALL;
		break;
	case 3:
		mCurrentEvent = ServerEvent::SNIPER_MODE;
		break;
	case 4:
		mCurrentEvent = ServerEvent::SPINNING_MODE;
		break;
	case 5:
		mCurrentEvent = ServerEvent::DRUNK_MODE;
		break;
	case 6:
		mCurrentEvent = ServerEvent::HEALTH_MODE;
		break;
	case 7:
		mCurrentEvent = ServerEvent::SPEEDY_GONZALES_MODE;
		break;
	case 8:
		mCurrentEvent = ServerEvent::ONE_SHOT_MODE;
		break;
	case 9:
		mCurrentEvent = ServerEvent::CHAOS_MODE;
		break;
	case 10:
		mCurrentEvent = ServerEvent::POISON_MODE;
	}
}
void Manager::HandleCurrentEvent(std::string oldEvent)
{
	if (mCurrentEvent == ServerEvent::NONE)
	{
		mIsEventActive = false;

		if (oldEvent == ServerEvent::X2BALL)
		{
			mPlayerReloadSpeed = PLAYER_RELOAD_SPEED;
			UpdatePlayersReloadSpeed();
		}
		else if (oldEvent == ServerEvent::BIG_BALL)
		{
			mBallRadius = BALL_RADIUS;
		}
		else if (oldEvent == ServerEvent::SNIPER_MODE)
		{
			mBallRadius = BALL_RADIUS;
			mPlayerSpeed = PLAYER_SPEED;
			mPlayerSpeedMultiplier = PLAYER_SPEED_MULTIPLIER;
		}
		else if (oldEvent == ServerEvent::SPINNING_MODE)
		{
			mSpinningMode = false;
			mPlayerReloadSpeed = PLAYER_RELOAD_SPEED;
			mBallLifeTime = BALL_LIFETIME;
			UpdatePlayersReloadSpeed();
		}
		else if (oldEvent == ServerEvent::DRUNK_MODE)
		{
			mDrunkMode = false;
		}
		else if (oldEvent == ServerEvent::HEALTH_MODE)
		{
			mHealthMode = false;
		}
		else if (oldEvent == ServerEvent::SPEEDY_GONZALES_MODE)
		{
			mPlayerSpeed = PLAYER_SPEED;
			mBallSpeed = BALL_SPEED;
		}
		else if (oldEvent == ServerEvent::ONE_SHOT_MODE)
		{
			mOneShotMode = false;
			mPlayerMaxHp = PLAYER_MAX_HP;
			for (auto& [name, client] : mAllClients)
			{
				client->playerState.dat.hp = mPlayerMaxHp;
			}
		}
		else if (oldEvent == ServerEvent::CHAOS_MODE)
		{
			mChaosMode = false;
			mTime.Pause();
		}
		else if (oldEvent == ServerEvent::POISON_MODE)
		{
			mPoisonMode = false;
			mBallDamage = BALL_DAMAGE;
		}
	}
	else
	{
		mIsEventActive = true;

		if (mCurrentEvent == ServerEvent::X2BALL)
		{
			mPlayerReloadSpeed = PLAYER_RELOAD_SPEED * 0.5f;
			UpdatePlayersReloadSpeed();
		}
		else if (mCurrentEvent == ServerEvent::BIG_BALL)
		{
			mBallRadius = BALL_RADIUS * 2.f;
		}
		else if (mCurrentEvent == ServerEvent::SNIPER_MODE)
		{
			mBallRadius = BALL_RADIUS * 0.75f;
			mPlayerSpeed = PLAYER_SPEED * 1.5f;
			mPlayerSpeedMultiplier = 0;
		}
		else if (mCurrentEvent == ServerEvent::SPINNING_MODE)
		{
			mSpinningMode = true;
			mPlayerReloadSpeed = PLAYER_RELOAD_SPEED * 0.15f;
			mBallLifeTime = 1.5f;
			UpdatePlayersReloadSpeed();
		}
		else if (mCurrentEvent == ServerEvent::DRUNK_MODE)
		{
			mDrunkMode = true;
		}
		else if (mCurrentEvent == ServerEvent::HEALTH_MODE)
		{
			mHealthMode = true;
		}
		else if (mCurrentEvent == ServerEvent::SPEEDY_GONZALES_MODE)
		{
			mPlayerSpeed = PLAYER_SPEED * 3.f;
			mBallSpeed = BALL_SPEED * 3.f;
		}
		else if (mCurrentEvent == ServerEvent::ONE_SHOT_MODE)
		{
			mOneShotMode = true;
			mPlayerMaxHp = 10;
			for (auto& [name, client] : mAllClients)
			{
				client->playerState.dat.hp = mPlayerMaxHp;
			}
		}
		else if (mCurrentEvent == ServerEvent::CHAOS_MODE)
		{
			mChaosMode = true;
			mTime.Resume();
		}
		else if (mCurrentEvent == ServerEvent::POISON_MODE)
		{
			mPoisonMode = true;
			mBallDamage = BALL_DAMAGE * 2.f;
		}
	}
}
int Manager::GenerateNumber()
{
	int rdm = 0;
	int value = -1;

	if (mRandomNumbers.empty())
	{
		for (int i = 0; i < ServerEvent::eventCount; ++i)
			mRandomNumbers.push_back(i + 1);
	}

	rdm = rand() % mRandomNumbers.size();

	value = mRandomNumbers[rdm];
	auto it = mRandomNumbers.begin() + rdm;
	mRandomNumbers.erase(it);

	return value;
}