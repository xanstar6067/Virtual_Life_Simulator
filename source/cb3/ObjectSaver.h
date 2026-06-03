#pragma once

#include "Field.h"
#include "../SimulationMode.h"



namespace cb3
{

class BotNeuralNet;

class MyOutStream final: public std::ofstream
{
public:

    void WriteInt(int);
    void WriteBool(bool);
    void WriteByte(byte);
    void WriteUShort(unsigned short);

    MyOutStream(char* filename, int flags);
};

class MyInputStream final : public std::ifstream
{
public:

    int ReadInt();
    bool ReadBool();
    byte ReadByte();
    unsigned short ReadUShort();

    MyInputStream(char* filename, int flags);
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
    Object* LoadObject(char* filename);

    bool SaveWorld(Field* world, char* filename, int id, int ticknum);

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

};

}
