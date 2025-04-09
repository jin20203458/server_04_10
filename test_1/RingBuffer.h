#pragma once
#include <windows.h>
#include <vector>
#include "Packet.h"

class RingBuffer
{
public:
    explicit RingBuffer(size_t capacity);
    ~RingBuffer();

    size_t available() const;
    size_t freeSpace() const;

    void CommitWrite(size_t bytesWritten);
    bool enqueuePacket(const Packet& packet);
    bool dequeuePacket(Packet& packet);

    size_t getCapacity();
    size_t getHead();
    size_t getTail();

    char* getWritePtr();
    const char* getReadPtr() const;
    void clear();

private:
    bool enqueue(const char* data, size_t len, bool isPartialEnqueueAllowed = false);
    bool dequeue(char* outData, size_t len, bool isPartialDequeueAllowed = false, bool isPeekMode = false);

    char* buffer_;
    size_t capacity_;
    size_t head_;
    size_t tail_;
    size_t dequeueCount_;
    mutable CRITICAL_SECTION ringBufferCS_;
};

