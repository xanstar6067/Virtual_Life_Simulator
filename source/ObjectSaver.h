#pragma once

#include "Field.h"



class MyOutStream final: public std::ofstream
{
public:

    void WriteInt(int);
    void WriteBool(bool);

    MyOutStream(char* filename, int flags);
    MyOutStream(const std::filesystem::path& filename, int flags);
};

class MyInputStream final : public std::ifstream
{
public:

    int ReadInt();
    bool ReadBool();

    MyInputStream(char* filename, int flags);
    MyInputStream(const std::filesystem::path& filename, int flags);
};


class ObjectSaver final
{
private:

    void WriteBotToFile(MyOutStream& file, Bot* obj);
    Bot* LoadBotFromFile(MyInputStream& file);

    void WriteObjectToFile(MyOutStream& file, Object* obj);
    Object* LoadObjectFromFile(MyInputStream& file);

public:
    
    bool SaveObject(Object* obj, char* filename);
    bool SaveObject(Object* obj, const std::filesystem::path& filename);
    Object* LoadObject(char* filename);
    Object* LoadObject(const std::filesystem::path& filename);

    bool SaveWorld(Field* world, char* filename, int id, int ticknum);
    bool SaveWorld(Field* world, const std::filesystem::path& filename, int id, int ticknum);

    struct WorldParams
    {
        int id, seed, tick, width;
    };

    WorldParams LoadWorld(Field* world, char* filename);
    WorldParams LoadWorld(Field* world, const std::filesystem::path& filename);

};
