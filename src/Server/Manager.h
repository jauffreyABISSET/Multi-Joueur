#pragma once
#include "main.h"
#include "../lib/CPU/Ball.h"
#include "../src/lib/Reseau/Client.h"
#include "../lib/CPU/Player.h"
#include "../lib/Timer.h"
#include <queue>

#define MANAGER Manager::Get()

class ThreadReceiveClients;

class InputPack;

class Manager
{
private:
	uint32_t mTick = 0;

	static Manager* m_pInstance;

	//Delta
	const float mFixedDT = FIXED_DT; //Constant
	float mDT = FIXED_DT; //Variable
	std::string mCurrentEvent;
	bool mIsEventActive = false;

	//Timers
	Timer mEventTimer;
	Timer mUpdateTimer;
	Timer mDisplayClientTimer;
	Timer mTime;

	//Game Variables
	float mPlayerSpeedMultiplier = PLAYER_SPEED_MULTIPLIER;
	float mBallRadius = BALL_RADIUS;
	float mBallSpeed = BALL_SPEED;
	float mBallLifeTime = BALL_LIFETIME;
	int mBallDamage = BALL_DAMAGE;
	float mPlayerSpeed = PLAYER_SPEED;
	float mPlayerReloadSpeed = PLAYER_RELOAD_SPEED;
	int mPlayerMaxHp = PLAYER_MAX_HP;

	bool mChaosMode = false;
	float mPoisonMode;
	bool mSpinningMode = false;
	float mSpinningSpeed = 15.f;
	bool mDrunkMode = false;
	bool mOneShotMode = false;
	bool mHealthMode = false;
public:
	bool mIsOpen = true;
	UDPSocket mSock;

	//THREADS
	ThreadReceiveClients* mpThreadReceiveClients = nullptr;

	//DATAS
	std::vector<BallState> mBallStates;
	std::unordered_map<Client*, InputPack*> mInputsQueue;
	std::vector<int> mRandomNumbers;

	//CS
	CRITICAL_SECTION mCS_clientToAddAccess;
	CRITICAL_SECTION mCS_clientToEditAccess;
	CRITICAL_SECTION mCS_clientToRemoveAccess;
	CRITICAL_SECTION mCS_clientAccess;
	CRITICAL_SECTION mCS_Pack_ID;
	CRITICAL_SECTION mCS_AddPacksToClientQueue;

	//CLIENT DATABASE
	std::unordered_map<std::string, Client*> mAllClients;
	std::queue<Client*> mClientsToAdd;
	std::queue<std::pair<Client*, sockaddr_in>> mClientsToEdit;
	std::queue<Client*> mClientsToRemove;

public:
	Manager() = default;
	~Manager() { m_pInstance = nullptr; }

	bool ConfigSocket();

	//Get
	static Manager* Get();
	Client* GetClient(sockaddr_in addr);
	void GetDefaultPlayerStateConfig(PlayerState& target); // Datas when a new player is coming
	const float& GetDeltaTime() const { return mDT; }

	//Main Loop
	void Init();
	void Run();
	void Update();
	void UpdateGame();
	void HandleChaosMode();
	void ApplyInputs();
	void ApplyCannonRot(XMFLOAT3& entityRot, XMFLOAT3 dir);
	void ApplyMovement(XMFLOAT3& entityPos, uint8_t pInput, float speed);
	void HandleCollisions();

	//Handle Packs
	void HandleClientPacks();
	void HandleInputPacks(Client* c, InputPack* pInput);

	//Send Packs
	void SendClientPacks();
	std::vector<char*> MakePlayerDataPack();
	std::vector<char*> MakeBallDataPacks();
	char* MakePingPack();
	char* MakeEventMessagePack();

	//Client Handler
	void AddNewClients();
	void EditClients();
	void RemoveClients();

	//Misc
	void DisplayClients();
	void CreateBall(const XMFLOAT3& pos, int clientID, XMFLOAT3 dir, float speed);
	void UpdatePlayersReloadSpeed();
	void ManageEvent();
	void HandleCurrentEvent(std::string oldEvent); // Gameplay
	int GenerateNumber();

private:
	void CalculateDeltaTime();
};

