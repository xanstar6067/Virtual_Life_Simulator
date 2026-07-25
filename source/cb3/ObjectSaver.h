#pragma once

#include "Field.h"
#include "../SimulationMode.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>


namespace cb3
{

class BotNeuralNet;

// The compact CB3 format consists mostly of one-to-four-byte fields. Calling
// std::fstream::read/write for every field is extremely expensive for dense
// worlds, so these wrappers keep a large user-space buffer and make the hot
// primitive operations simple memory copies.
class MyOutStream final
{
private:
    static constexpr size_t BufferCapacity = 1024 * 1024;

    std::ofstream stream;
    std::unique_ptr<char[]> buffer;
    size_t bufferedBytes = 0;
    bool failed = false;
    bool closed = false;

    void FlushBuffer();
    void WriteBytesSlow(const char* data, size_t size);

    void WriteBytes(const void* data, size_t size)
    {
        if (failed || !stream.is_open())
        {
            failed = true;
            return;
        }

        if (size <= BufferCapacity - bufferedBytes)
        {
            std::memcpy(buffer.get() + bufferedBytes, data, size);
            bufferedBytes += size;
            return;
        }

        WriteBytesSlow(static_cast<const char*>(data), size);
    }

public:
    void WriteInt(int data) { WriteBytes(&data, sizeof(data)); }
    void WriteBool(bool data) { WriteBytes(&data, sizeof(data)); }
    void WriteByte(byte data) { WriteBytes(&data, sizeof(data)); }
    void WriteUShort(unsigned short data) { WriteBytes(&data, sizeof(data)); }

    MyOutStream& write(const char* data, std::streamsize size)
    {
        if (size < 0)
            failed = true;
        else
            WriteBytes(data, static_cast<size_t>(size));
        return *this;
    }

    MyOutStream(char* filename, int flags);
    MyOutStream(const std::filesystem::path& filename, int flags);
    ~MyOutStream();

    bool is_open() const { return stream.is_open(); }
    bool good() const { return !failed; }
    bool close();
};

class MyInputStream final
{
private:
    static constexpr size_t BufferCapacity = 1024 * 1024;

    std::ifstream stream;
    std::unique_ptr<char[]> buffer;
    size_t bufferPosition = 0;
    size_t bufferedBytes = 0;
    bool failed = false;

    bool RefillBuffer();
    void ReadBytesSlow(char* destination, size_t size);
    void IgnoreSlow(size_t size);

    void ReadBytes(void* destination, size_t size)
    {
        if (size <= bufferedBytes - bufferPosition)
        {
            std::memcpy(destination, buffer.get() + bufferPosition, size);
            bufferPosition += size;
            return;
        }

        ReadBytesSlow(static_cast<char*>(destination), size);
    }

public:
    int ReadInt()
    {
        int value = 0;
        ReadBytes(&value, sizeof(value));
        return value;
    }

    bool ReadBool()
    {
        bool value = false;
        ReadBytes(&value, sizeof(value));
        return value;
    }

    byte ReadByte()
    {
        if (bufferPosition < bufferedBytes)
            return static_cast<byte>(buffer[bufferPosition++]);

        byte value = 0;
        ReadBytesSlow(reinterpret_cast<char*>(&value), sizeof(value));
        return value;
    }

    unsigned short ReadUShort()
    {
        unsigned short value = 0;
        ReadBytes(&value, sizeof(value));
        return value;
    }

    MyInputStream& read(char* destination, std::streamsize size)
    {
        if (size < 0)
            failed = true;
        else
            ReadBytes(destination, static_cast<size_t>(size));
        return *this;
    }

    MyInputStream& ignore(std::streamsize size)
    {
        if (size < 0)
        {
            failed = true;
        }
        else
        {
            const size_t bytesToIgnore = static_cast<size_t>(size);
            const size_t available = bufferedBytes - bufferPosition;
            if (bytesToIgnore <= available)
                bufferPosition += bytesToIgnore;
            else
                IgnoreSlow(bytesToIgnore);
        }
        return *this;
    }

    MyInputStream(char* filename, int flags);
    MyInputStream(const std::filesystem::path& filename, int flags);

    bool is_open() const { return stream.is_open(); }
    bool good() const { return !failed; }
    void close() { stream.close(); }
};


class ObjectSaver final
{
private:

    void WriteBotToFile(MyOutStream& file, Bot* obj);
    Bot* LoadBotFromFile(MyInputStream& file);

    void WriteObjectToFile(MyOutStream& file, Object* obj);
    Object* LoadObjectFromFile(MyInputStream& file);

    void WriteBrainCompact(MyOutStream& file, BotNeuralNet* brain, bool includeMemory);
    bool LoadBrainCompact(MyInputStream& file, BotNeuralNet* brain, bool includeMemory);
    void WriteBotCompact(MyOutStream& file, Bot* obj);
    Bot* LoadBotCompact(MyInputStream& file);
    void WriteObjectCompact(MyOutStream& file, Object* obj);
    Object* LoadObjectCompact(MyInputStream& file);

public:
    
    bool SaveObject(Object* obj, char* filename);
    bool SaveObject(Object* obj, const std::filesystem::path& filename);
    Object* LoadObject(char* filename);
    Object* LoadObject(const std::filesystem::path& filename);

    bool SaveWorld(Field* world, char* filename, int id, int ticknum);
    bool SaveWorld(Field* world, const std::filesystem::path& filename, int id, int ticknum);

    struct WorldParams
    {
        int
        id,
        seed,
        tick,
        width,
        height;
    };

    WorldParams LoadWorld(Field* world, char* filename, bool clearWorld = true, bool loadParams = true, bool loadLandscape = true, bool loadBots = true);
    WorldParams LoadWorld(Field* world, const std::filesystem::path& filename, bool clearWorld = true, bool loadParams = true, bool loadLandscape = true, bool loadBots = true);

};

}
