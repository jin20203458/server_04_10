#pragma once
#include <vector>
#include <stdexcept>
#include <string>
#include <print>
enum class PacketType : uint8_t
{
    Unknown = 0x00,
    PlayerInit = 0x01,
    PlayerUpdate = 0x02,
    MonsterUpdate = 0x03,
    WorldUpdate = 0x04
};

enum class AnimType : uint8_t
{
    Idle = 0x00,
    Run = 0x01,
    jump = 0x02,
    Attack = 0x03,
    Roll = 0x04,
    Hit = 0x05,
    Die = 0x06
};

struct PacketHeader
{
    PacketType type;     // 패킷의 타입 (예: 메시지, 명령 등)
    uint8_t playerCount; // 플레이어 수 (예: 월드 업데이트 시)
    uint8_t bossActed;      // 보스 행동 여부
    uint8_t padding;     // reserved for future use
    uint32_t length;     // 패킷의 총 데이터 길이


    PacketHeader()
    {
        type = PacketType::Unknown;
        playerCount = 0;
        bossActed = false;
        padding = 0;
        length = 0;
    }
};

struct Packet
{
    PacketHeader header;
    std::vector<uint8_t> data; // 패킷 데이터
    uint32_t readPos;

    Packet() = default;
    Packet(const PacketHeader& header, std::vector<uint8_t>&& data)
        : header(header), data(std::move(data))
    {
        readPos = 0;
    }


    template <typename T>
    void write(const T& value)
    {
        size_t dataSize = sizeof(T);
        const uint8_t* rawData = reinterpret_cast<const uint8_t*>(&value);
        data.insert(data.end(), rawData, rawData + dataSize);
        header.length = static_cast<uint32_t>(data.size() + sizeof(PacketHeader));
    }

    void writeString(const std::string& value)
    {
        uint16_t strLength = static_cast<uint16_t>(value.size());
        write(strLength);
        data.insert(data.end(), value.begin(), value.end());
        header.length = static_cast<uint32_t>(data.size() + sizeof(PacketHeader));
    }

    template <typename T>
    T read()
    {
        if (readPos + sizeof(T) > data.size())
            throw std::runtime_error("Lack of packet data!");

        T value;
        std::memcpy(&value, data.data() + readPos, sizeof(T));
        readPos += sizeof(T);
        return value;
    }

    std::string readString()
    {
        uint16_t strLength = read<uint16_t>();
        if (readPos + strLength > data.size())
            throw std::runtime_error("Lack of packet data!");

        std::string value(reinterpret_cast<char*>(data.data() + readPos), strLength);
        readPos += strLength;
        return value;
    }



    // Use this when ring buffer is not in use
    std::vector<uint8_t> Serialize() const
    {
        std::vector<uint8_t> tmpBuffer;
        tmpBuffer.resize(sizeof(PacketHeader) + data.size());

        // header
        std::memcpy(tmpBuffer.data(), &header, sizeof(PacketHeader));
        // data
        if (!data.empty())
            std::memcpy(tmpBuffer.data() + sizeof(PacketHeader), data.data(), data.size());

        return tmpBuffer;
    }
    static Packet Deserialize(const std::vector<uint8_t>& buffer)
    {
        if (buffer.size() < sizeof(PacketHeader))
            throw std::runtime_error("Buffer too small!");

        PacketHeader header;
        std::memcpy(&header, buffer.data(), sizeof(PacketHeader));

        std::vector<uint8_t> data(buffer.begin() + sizeof(PacketHeader), buffer.end());

        return Packet(header, std::move(data));
    }

};

