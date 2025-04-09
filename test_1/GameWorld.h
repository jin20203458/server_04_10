#pragma once
#include <unordered_map>
#include <winsock2.h>
#include <thread>
#include "PlayerData.h"
#include  "MAP.h"
#include "BOSS.h"
constexpr int PORT = 5000;
constexpr int SEND_BUFFER_SIZE = 4096;

class GameWorld {
public:
    GameWorld();
    ~GameWorld();
    void start();
    void stop();

private:
    void acceptConnections();
    void addPlayer(SOCKET socket, PlayerData* player);
    void removePlayer(SOCKET socket);
    PlayerData* getPlayer(SOCKET socket);
    void lockPlayers();
    void unlockPlayers();

    void processPlayerInit(PlayerData* player, Packet& packet);
    void processPlayerUpdate(PlayerData* player, Packet& packet);
    void processMonsterUpdate(Packet& packet);
    
    void updateBossLoop();
    void workerThread();
    void sendWorldData();

    Map* mapPtr;
    BOSS boss;
    CRITICAL_SECTION bossCriticalSection;

    SOCKET listenSock;
    HANDLE iocp;
    CRITICAL_SECTION playersCriticalSection;
    std::unordered_map<SOCKET, PlayerData*> players;
    char WorldBuf[SEND_BUFFER_SIZE];
    bool running;

    std::vector<std::thread> workers;
    std::thread sendThread;
    std::thread bossThread;
};
