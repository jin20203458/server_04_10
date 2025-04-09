#include "ClientSession.h"
#include <iostream>
#include <algorithm>

#define BUFFER_SIZE 1024

ClientSession::ClientSession(SOCKET socket) : clientSock_(socket), recvRingBuf_(BUFFER_SIZE)
{
    ZeroMemory(&over_, sizeof(over_));
}

void ClientSession::PostRecv()
{
    size_t freeSpace = recvRingBuf_.freeSpace();
    if (freeSpace == 0) return;

    DWORD flags = 0;
    DWORD bytesReceived = 0;
    WSABUF wsaBuf;

    size_t firstPart = min(freeSpace, recvRingBuf_.getCapacity() - recvRingBuf_.getTail());
    wsaBuf.buf = recvRingBuf_.getWritePtr();
    wsaBuf.len = static_cast<ULONG>(firstPart);
    ZeroMemory(&over_, sizeof(over_));

    if (WSARecv(clientSock_, &wsaBuf, 1, &bytesReceived, &flags, &over_, NULL) == SOCKET_ERROR)
    {
        if (WSAGetLastError() != WSA_IO_PENDING) std::cerr << "WSARecv failed!\n";
    }
}

bool ClientSession::ExtractPacket(Packet& packet)
{
    return recvRingBuf_.dequeuePacket(packet);
}

// 접근자 메서드 정의
WSAOVERLAPPED& ClientSession::getOverlapped()
{
    return over_;
}

SOCKET ClientSession::getClientSocket() const
{
    return clientSock_;
}

RingBuffer& ClientSession::getRecvRingBuffer()
{
    return recvRingBuf_;
}


