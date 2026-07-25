#pragma once

#include "Field.h"
#include "SimulationMode.h"


class BotNeuralNet;


class MyOutStream final: public std::ofstream
{
public:

    void WriteInt(int);
    void WriteBool(bool);
    void WriteByte(byte);
    void WriteUShort(unsigned short);
    void WriteFloat(float);

    MyOutStream(char* filename, int flags);
    MyOutStream(const std::filesystem::path& filename, int flags);
};

class MyInputStream final : public std::ifstream
{
public:

    int ReadInt();
    bool ReadBool();
    byte ReadByte();
    unsigned short ReadUShort();
    float ReadFloat();

    MyInputStream(char* filename, int flags);
    MyInputStream(const std::filesystem::path& filename, int flags);
};


class ObjectSaver final
{
public:

    struct WorldParams
    {
        int id, seed, tick, width;
    };

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
    WorldParams LoadWorldLegacy(Field* world, MyInputStream& file);
    WorldParams LoadWorldCompact(Field* world, MyInputStream& file);

public:
    
    bool SaveObject(Object* obj, char* filename);
    bool SaveObject(Object* obj, const std::filesystem::path& filename);
    Object* LoadObject(char* filename);
    Object* LoadObject(const std::filesystem::path& filename);

    bool SaveWorld(Field* world, char* filename, int id, int ticknum);
    bool SaveWorld(Field* world, const std::filesystem::path& filename, int id, int ticknum);

    WorldParams LoadWorld(Field* world, char* filename);
    WorldParams LoadWorld(Field* world, const std::filesystem::path& filename);

};
