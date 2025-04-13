#pragma once
#include <unordered_map>
#include <winsock2.h>
#include <thread>
#include "PlayerData.h"
#include "MAP.h"
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

    void updateMapLoop();
    void processMonsterUpdate(Packet& packet);
    void updateBossLoop();

    void workerThread();
    void sendWorldData();
   
    bool running;
    Map* mapPtr;
    BOSS boss;

    SOCKET listenSock;
    HANDLE iocp;
    CRITICAL_SECTION playersCriticalSection;
    std::unordered_map<SOCKET, PlayerData*> players;

    std::vector<std::thread> workers;
    std::thread sendThread;

    std::thread mapThread;
    std::thread bossThread;
};
