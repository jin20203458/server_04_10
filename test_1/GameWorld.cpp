#include "GameWorld.h"
#include <iostream>
#include <print>

GameWorld::GameWorld() : listenSock(INVALID_SOCKET), iocp(NULL), running(false)
{
	InitializeCriticalSection(&playersCriticalSection);
	mapPtr = new Map(-32, 2, -2, 10); 

	// MovingTrap 초기화
	traps.emplace_back(0.0f, 0.0f, *mapPtr, "T1");
	traps.emplace_back(3.0f, 0.0f, *mapPtr, "T2");
	traps.emplace_back(3.0f, 0.0f, *mapPtr, "T3");
	traps.emplace_back(3.0f, 0.0f, *mapPtr, "T4");
	traps.emplace_back(3.0f, 0.0f, *mapPtr, "T5");

}

GameWorld::~GameWorld() { 
	stop();
	DeleteCriticalSection(&playersCriticalSection);
}

void GameWorld::start()
{
	WSAData wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {
		std::cerr << "WSAStartup failed!\n";
		exit(-1);
	}

	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSock == INVALID_SOCKET) {
		std::cerr << "Server socket creation failed!\n";
		exit(-1);
	}

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(PORT);

	if (bind(listenSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Bind failed!\n";
		exit(-1);
	}

	if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR) {
		std::cerr << "Listen failed!\n";
		exit(-1);
	}

	iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (iocp == NULL) {
		std::cerr << "CreateIoCompletionPort failed!\n";
		exit(-1);
	}

	running = true;

	for (int i = 0; i < 4; ++i) // 비동기 수신
		workers.emplace_back(&GameWorld::workerThread, this);
	for (auto& worker : workers) worker.detach();
	                       
	// 동기 송신 (주기적)
	sendThread = std::thread(&GameWorld::sendWorldData, this);
	sendThread.detach();

	// 보스 행동 주기적 업데이트
	bossThread = std::thread(&GameWorld::updateBossLoop, this);
	bossThread.detach();
	// 맵 주기적 업데이트
	mapThread = std::thread(&GameWorld::updateMapLoop, this);
	mapThread.detach();

	acceptConnections();
}

void GameWorld::acceptConnections()
{
	sockaddr_in caddr;
	SOCKET csock;
	int addrlen = sizeof(caddr);

	while (running)
	{
		csock = accept(listenSock, (SOCKADDR*)&caddr, &addrlen);
		if (csock == INVALID_SOCKET)
		{
			std::cerr << "accept failed!\n";
			continue;
		}

		// 클라이언트 세션 생성
		PlayerData* newPlayer = new PlayerData("UninitPlayer", csock);

		// IOCP에 클라이언트 소켓 추가
		if (CreateIoCompletionPort(reinterpret_cast<HANDLE>(csock), iocp,
			reinterpret_cast<ULONG_PTR>(newPlayer), 0) == NULL)
		{
			std::cerr << "CreateIoCompletionPort failed!\n";
			closesocket(csock);
			delete newPlayer;
			continue;
		}

		// 클라이언트 세션을 GameWorld에 추가
		addPlayer(csock, newPlayer);

		// 데이터 수신을 시작
		newPlayer->session.PostRecv();
	}
}

void GameWorld::workerThread()
{
	DWORD bytesTransferred;
	ULONG_PTR completionKey;
	LPOVERLAPPED lpOverlapped;

	while (running)
	{
		BOOL result = GetQueuedCompletionStatus
		(iocp, &bytesTransferred, &completionKey, &lpOverlapped, INFINITE);

		PlayerData* player = reinterpret_cast<PlayerData*>(completionKey);

		if (!result)              // 강제종료
		{
			std::cerr << "GetQueuedCompletionStatus failed! Error: " << GetLastError() << '\n';
			std::print("player {} is disconnected!\n", player->name);
			closesocket(player->session.getClientSocket());
			removePlayer(player->session.getClientSocket());
			continue;
		}
		if (bytesTransferred == 0) // 정상종료
		{
			std::print("player {} is disconnected!\n", player->name);
			closesocket(player->session.getClientSocket());
			removePlayer(player->session.getClientSocket());
			continue;
		}
		player->PlayerCommitWrite(bytesTransferred);

		// 클라이언트에서 데이터를 받은 후 처리
		player->PlayerPostRecv();

		// 패킷을 추출하여 처리
		Packet packet;
	
		while (player->PlayerExtractPacket(packet))
		{
			// 패킷 타입을 읽고 처리
			PacketType dataType = packet.header.type;

			if     (dataType == PacketType::PlayerInit)   player->processInit(packet);
			else if(dataType == PacketType::PlayerUpdate) player->processUpdate(packet);
			else if(dataType == PacketType::MonsterUpdate)processMonsterUpdate(packet);
			else    std::cerr << "Invalid packet type\n";
		}
	}
}


void GameWorld::processMonsterUpdate(Packet& packet)
{
	int damage = packet.read<int>();
	boss.takeDamage(damage);
}

void GameWorld::updateBossLoop()
{
	while (running)
	{
		boss.lock();
		boss.update();
		boss.unlock();

		std::this_thread::sleep_for(std::chrono::milliseconds(20));  // 100ms마다 반복
	}
}

void GameWorld::updateMovingTraps()
{
	for (auto& trap : traps) 
	{
		trap.update();
		/*std::print("Trap {} is at ({:.2f}, {:.2f})\n",
			trap.getId(), trap.getX(), trap.getY());*/
	}
}

void GameWorld::updateMapLoop()
{
	while (running)
	{
		updateMovingTraps(); // 맵 업데이트
		std::this_thread::sleep_for(std::chrono::milliseconds(20));  // 100ms마다 반복
	}
}

void GameWorld::sendWorldData() // 보낼 데이터
{
	BossState currentState;

	while (running)
	{
		if (players.empty()) continue;

		// 월드 헤더 설정
		Packet worldPacket;
		worldPacket.header.type = PacketType::WorldUpdate;
		worldPacket.header.playerCount = players.size();
		worldPacket.header.bossActed = false;

		// 플레이어 데이터 직렬화
		lockPlayers();
		for (auto& pair : players)
		{
			PlayerData* player = pair.second;
			worldPacket.writeString(player->getName());   // 플레이어 이름
			worldPacket.write<float>(player->getPosX());  // X 좌표
			worldPacket.write<float>(player->getPosY());  // Y 좌표
			worldPacket.write<uint8_t>(player->getAnimTypeAsByte());
		}
		unlockPlayers();


		// MovingTrap 데이터 직렬화
		worldPacket.write<uint8_t>(traps.size()); // 트랩 개수
		for (const auto& trap : traps) 
		{
			worldPacket.writeString(trap.getId());        // 트랩 ID
			worldPacket.write<float>(trap.getX());        // 트랩 X 좌표
			worldPacket.write<float>(trap.getY());        // 트랩 Y 좌표
		}


		boss.lock();
		currentState = boss.getState();

		// 보스 상태가 변경되었는지 확인
		if (boss.hasStateChanged())
		{
			std::print("BOSS state changed to {}\n", static_cast<int>(currentState));
			worldPacket.header.bossActed = true;
		}
		if (boss.hasHpChanged())
		{
			std::print("BOSS HP changed to {}\n", boss.hp);
			worldPacket.header.bossActed = true;
		}

		// 보스 데이터 직렬화
		if (worldPacket.header.bossActed) // 보스의 변동사항이 있었다면
		{
			worldPacket.write<uint8_t>(static_cast<uint8_t>(currentState)); // 보스 상태
			worldPacket.write<int>(boss.hp); // 보스 HP
		}
		boss.unlock();


		// 월드 패킷 직렬화
		std::vector<uint8_t> serializedPacket = worldPacket.Serialize();  
		const char* sendBuffer = reinterpret_cast<const char*>(serializedPacket.data());

		// 직렬화된 월드 데이터를 모든 플레이어에게 브로드캐스트
		for (auto& pair : players)
		{
			PlayerData* player = pair.second;
			int bytesSent = send(player->getClientSession().getClientSocket(), sendBuffer
				, static_cast<int>(serializedPacket.size()), 0);
		}
		//std::cout << "패킷 사이즈" << serializedPacket.size() << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));  // 보내는 속도 조절(서버에서 클라이언트들로)
	}
}


void GameWorld::addPlayer(SOCKET socket, PlayerData* player)
{
	lockPlayers();
	players[socket] = player;
	unlockPlayers();
}

void GameWorld::removePlayer(SOCKET socket)
{
	lockPlayers();
	auto it = players.find(socket);
	if (it != players.end())
	{
		delete it->second;
		players.erase(it);
	}
	unlockPlayers();
}

PlayerData* GameWorld::getPlayer(SOCKET socket)
{
	lockPlayers();
	auto it = players.find(socket);
	PlayerData* player = nullptr;
	if (it != players.end())
	{
		player = it->second;
	}
	unlockPlayers();
	return player;
}

void GameWorld::lockPlayers()
{
	EnterCriticalSection(&playersCriticalSection);
}

void GameWorld::unlockPlayers()
{
	LeaveCriticalSection(&playersCriticalSection);
}

void GameWorld::stop()
{
	running = false;
	closesocket(listenSock);
	for (auto& pair : players)
	{
		closesocket(pair.first);
		delete pair.second;
	}
	players.clear();
	WSACleanup();
}