#include "PlayerData.h"
#include <print>
PlayerData::PlayerData(const std::string& name, SOCKET socket)
    : name(name), posX(0), posY(0), session(socket) {}


void PlayerData::PlayerCommitWrite(size_t bytesTransferred)
{
    session.getRecvRingBuffer().CommitWrite(bytesTransferred);
}

void PlayerData::PlayerPostRecv()
{
    session.PostRecv();
}

bool PlayerData::PlayerExtractPacket(Packet& packet)
{
    return session.ExtractPacket(packet);
}

void PlayerData::processInit(Packet& packet)
{
    std::string name = packet.readString();
    setName(name);

    float x = packet.read<float>();
    float y = packet.read<float>();
    updatePosition(x, y);

    std::print("Player {} initialized at ({}, {})\n", name, x, y);
}

void PlayerData::processUpdate(Packet& packet)
{
    float x = packet.read<float>();
    float y = packet.read<float>();
    updatePosition(x, y);
    
    AnimType anieType = static_cast<AnimType>(packet.read<uint8_t>());
    setAnimType(anieType);

   // std::print("Player {} updated position to ({}, {})\n", getName(), x, y);
}