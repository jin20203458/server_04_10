#include "RingBuffer.h"
#include <cstring>
#include <algorithm>
#include <iostream>
#include <print>

RingBuffer::RingBuffer(size_t capacity)
    : buffer_(new char[capacity]), capacity_(capacity), head_(0), tail_(0), dequeueCount_(0)
{
    InitializeCriticalSection(&ringBufferCS_);
}

RingBuffer::~RingBuffer()
{
    delete[] buffer_;  // 메모리 해제
    DeleteCriticalSection(&ringBufferCS_);
}

size_t RingBuffer::available() const
{
    if (tail_ >= head_)
        return tail_ - head_;
    else
        return capacity_ - head_ + tail_;
}

size_t RingBuffer::freeSpace() const
{
    return capacity_ - available() - 1;
}

bool RingBuffer::enqueue(const char* data, size_t len, bool isPartialEnqueueAllowed)
{
    EnterCriticalSection(&ringBufferCS_);

    if (freeSpace() < len)
    {
        if (!isPartialEnqueueAllowed || freeSpace() == 0)
        {
            LeaveCriticalSection(&ringBufferCS_);
            return false;
        }
        len = freeSpace();
    }

    size_t firstPart = min(len, capacity_ - tail_);
    std::memcpy(&buffer_[tail_], data, firstPart);
    size_t remaining = len - firstPart;

    if (remaining > 0)
        std::memcpy(&buffer_[0], data + firstPart, remaining);
    tail_ = (tail_ + len) % capacity_;

    LeaveCriticalSection(&ringBufferCS_);
    return true;
}

void RingBuffer::CommitWrite(size_t bytesWritten)
{
    EnterCriticalSection(&ringBufferCS_);
    tail_ = (tail_ + bytesWritten) % capacity_;
    LeaveCriticalSection(&ringBufferCS_);
}

bool RingBuffer::dequeue(char* outData, size_t len, bool isPartialDequeueAllowed, bool isPeekMode)
{
    EnterCriticalSection(&ringBufferCS_);

    if (available() < len) {
        if (!isPartialDequeueAllowed || available() == 0)
        {
            LeaveCriticalSection(&ringBufferCS_);
            return false;
        }
        len = available();
    }

    size_t firstPart = min(len, capacity_ - head_);
    std::memcpy(outData, &buffer_[head_], firstPart);
    size_t remaining = len - firstPart;

    if (remaining > 0)
        std::memcpy(outData + firstPart, &buffer_[0], remaining);
    if (!isPeekMode)
        head_ = (head_ + len) % capacity_;

    LeaveCriticalSection(&ringBufferCS_);
    return true;
}

bool RingBuffer::enqueuePacket(const Packet& packet)
{
    size_t totalSize =  packet.header.length;
    EnterCriticalSection(&ringBufferCS_);

    if (freeSpace() < totalSize)
    {
        LeaveCriticalSection(&ringBufferCS_);
        return false;
    }

    enqueue(reinterpret_cast<const char*>(&packet.header), sizeof(packet.header));
    enqueue(reinterpret_cast<const char*>(packet.data.data()), packet.data.size());

    LeaveCriticalSection(&ringBufferCS_);
    return true;
}

bool RingBuffer::dequeuePacket(Packet& packet)
{
    bool result = false;
    EnterCriticalSection(&ringBufferCS_);
    do {
        if (available() < sizeof(PacketHeader))
            break;

        PacketHeader header; // Peek
        if (!dequeue(reinterpret_cast<char*>(&header), sizeof(header), 0, 1))
            break;

        size_t totalPacketSize = header.length;
        size_t dataSize = header.length - sizeof(PacketHeader);

        if (available() < totalPacketSize)
            break;

        std::vector<uint8_t> packetData(dataSize);
        dequeue(reinterpret_cast<char*>(&header), sizeof(header));
        dequeue(reinterpret_cast<char*>(packetData.data()), dataSize);

        packet = Packet(header, std::move(packetData));

        result = true;
      //  std::print("dequeudPacket called {}\n", dequeueCount_++);
    } while (false);

    LeaveCriticalSection(&ringBufferCS_);
    return result;
}

size_t RingBuffer::getCapacity() { return capacity_; }
size_t RingBuffer::getHead() { return head_; }
size_t RingBuffer::getTail() { return tail_; }

char* RingBuffer::getWritePtr()
{
    EnterCriticalSection(&ringBufferCS_);
    char* writePtr = &buffer_[tail_];
    LeaveCriticalSection(&ringBufferCS_);
    return writePtr;
}

const char* RingBuffer::getReadPtr() const
{
    EnterCriticalSection(&ringBufferCS_);
    const char* readPtr = &buffer_[head_];
    LeaveCriticalSection(&ringBufferCS_);
    return readPtr;
}

void RingBuffer::clear()
{
    EnterCriticalSection(&ringBufferCS_);
    head_ = tail_ = 0;
    LeaveCriticalSection(&ringBufferCS_);
}

