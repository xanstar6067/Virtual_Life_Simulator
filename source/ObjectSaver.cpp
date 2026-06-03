
#include "Field.h"


Bot* ObjectSaver::LoadBotFromFile(MyInputStream& file)
{
    int lifetime = file.ReadInt();

    if (file.ReadInt() != NumberOfMutationMarkers)
        return NULL;

    if (file.ReadInt() != NumNeuronLayers)
        return NULL;
    if (file.ReadInt() != NeuronsInLayer)
        return NULL;
    if (file.ReadInt() != sizeof Neuron)
        return NULL;

    Bot* toRet = new Bot(0, 0);

    toRet->SetLifetime(lifetime);

    toRet->SetColor( file.ReadInt(), file.ReadInt(), file.ReadInt() );

    repeat(NumberOfMutationMarkers)
    {
        toRet->GetMarkers()[i] = file.ReadInt();
    }

    toRet->energy = file.ReadInt();

    file.read((char*)(toRet->GetActiveBrain()->allNeurons), NumNeuronLayers * NeuronsInLayer * sizeof(Neuron));

    file.read((char*)(toRet->GetInitialBrain()->allNeurons), NumNeuronLayers* NeuronsInLayer * sizeof(Neuron));

    file.read((char*)toRet->GetActiveBrain()->allMemory, NumNeuronLayers* NeuronsInLayer * sizeof(float));

    return toRet;
}

Object* ObjectSaver::LoadObjectFromFile(MyInputStream& file)
{
    Object* toRet;

    switch ((ObjectTypes)file.ReadInt())
    {
    case bot:
        return LoadBotFromFile(file);

    case rock:
        toRet = new Rock(0, 0);

        toRet->SetLifetime(file.ReadInt());

        return toRet;

    case apple:
        toRet = new Apple(0, 0);

        toRet->SetLifetime(file.ReadInt());
        toRet->energy = file.ReadInt();

        return toRet;

    case organic_waste:
        toRet = new Organics(0, 0, 0);

        toRet->SetLifetime(file.ReadInt());
        toRet->energy = file.ReadInt();

        return toRet;
    }

    return NULL;
}

ObjectSaver::WorldParams ObjectSaver::LoadWorld(Field* world, char* filename)
{
    return LoadWorld(world, std::filesystem::path(filename));
}


ObjectSaver::WorldParams ObjectSaver::LoadWorld(Field* world, const std::filesystem::path& filename)
{
    //Open file for reading, binary type
    MyInputStream file(filename, std::ios::in | std::ios::binary | std::ios::beg);

    if (file.is_open())
    {
        int magicNumber = file.ReadInt();

        if (magicNumber == MagicNumber_WorldFileV2)
        {
            WorldParams result = LoadWorldCompact(world, file);
            file.close();
            return result;
        }

        if (magicNumber == MagicNumber_WorldFile)
        {
            WorldParams result = LoadWorldLegacy(world, file);
            file.close();
            return result;
        }

        file.close();
    }

    return {-1, -1};
}

ObjectSaver::WorldParams ObjectSaver::LoadWorldLegacy(Field* world, MyInputStream& file)
{
    WorldParams toRet = {-1, -1, -1, -1};
    Object* tmpObj;

    world->RemoveAllObjects();

    int loadWidth = file.ReadInt();
    toRet.width = loadWidth;

    if (file.ReadInt() != FieldCellsHeight)
        return {-1, -1};

    toRet.id = file.ReadInt();
    toRet.seed = file.ReadInt();
    toRet.tick = file.ReadInt();

    if (file.ReadInt() != sizeof world->params)
        return {-1, -1};

    file.read((char*)&world->params, sizeof world->params);

    for (int x = 0; x < loadWidth; ++x)
    {
        for (int y = 0; y < FieldCellsHeight; ++y)
        {
            tmpObj = LoadObjectFromFile(file);

            if (x >= FieldCellsWidth)
            {
                delete tmpObj;
                tmpObj = NULL;

                continue;
            }
            else if (tmpObj)
            {
                tmpObj->x = x;
                tmpObj->y = y;

                world->AddObject(tmpObj);
            }
        }
    }

    return toRet;
}

ObjectSaver::WorldParams ObjectSaver::LoadWorldCompact(Field* world, MyInputStream& file)
{
    WorldParams toRet = {-1, -1, -1, -1};
    Object* tmpObj;

    int loadWidth = file.ReadInt();
    toRet.width = loadWidth;

    if (file.ReadInt() != FieldCellsHeight)
        return {-1, -1};

    toRet.id = file.ReadInt();
    toRet.seed = file.ReadInt();
    toRet.tick = file.ReadInt();

    if (file.ReadInt() != sizeof world->params)
        return {-1, -1};

    file.read((char*)&world->params, sizeof world->params);

    if (file.ReadInt() != NumberOfMutationMarkers)
        return {-1, -1};

    if (file.ReadInt() != NumNeuronLayers)
        return {-1, -1};

    if (file.ReadInt() != NeuronsInLayer)
        return {-1, -1};

    int objectCount = file.ReadInt();
    if (objectCount < 0)
        return {-1, -1};

    world->RemoveAllObjects();

    for (int i = 0; i < objectCount; ++i)
    {
        int x = file.ReadUShort();
        int y = file.ReadUShort();

        tmpObj = LoadObjectCompact(file);

        if (!tmpObj)
            return {-1, -1};

        if ((x >= FieldCellsWidth) || (y >= FieldCellsHeight))
        {
            delete tmpObj;
            continue;
        }

        tmpObj->x = x;
        tmpObj->y = y;

        world->AddObject(tmpObj);
    }

    return toRet;
}


/*World file format
    4b - magic number
    4b - Field width
    4b - Field height
    4b - sim id
    4b - seed
    4b - ticknum
    4b - FieldDynamicParams size
    FieldDynamicParams params

    following all objects
*/
bool ObjectSaver::SaveWorld(Field* world, char* filename, int id, int ticknum)
{
    return SaveWorld(world, std::filesystem::path(filename), id, ticknum);
}


bool ObjectSaver::SaveWorld(Field* world, const std::filesystem::path& filename, int id, int ticknum)
{
    MyOutStream file(filename, std::ios::in | std::ios::binary | std::ios::trunc);
    Object* tmpObj;

    if (file.is_open())
    {
        file.WriteInt(MagicNumber_WorldFileV2);

        file.WriteInt(FieldCellsWidth);
        file.WriteInt(FieldCellsHeight);

        file.WriteInt(id);

        file.WriteInt(world->seed);

        file.WriteInt(ticknum);

        file.WriteInt(sizeof world->params);
        file.write((char*)&world->params, sizeof world->params);

        file.WriteInt(NumberOfMutationMarkers);
        file.WriteInt(NumNeuronLayers);
        file.WriteInt(NeuronsInLayer);

        int objectCount = 0;

        for (int x = 0; x < FieldCellsWidth; ++x)
        {
            for (int y = 0; y < FieldCellsHeight; ++y)
            {
                if (world->GetObjectLocalCoords(x, y))
                    ++objectCount;
            }
        }

        file.WriteInt(objectCount);

        for (int x = 0; x < FieldCellsWidth; ++x)
        {
            for (int y = 0; y < FieldCellsHeight; ++y)
            {
                tmpObj = world->GetObjectLocalCoords(x, y);

                if (!tmpObj)
                    continue;

                file.WriteUShort((unsigned short)x);
                file.WriteUShort((unsigned short)y);
                WriteObjectCompact(file, tmpObj);
            }
        }

        file.close();

        return true;
    }

    return false;
}

void ObjectSaver::WriteBrainCompact(MyOutStream& file, BotNeuralNet* brain, bool includeMemory)
{
    for (int layer = 0; layer < NumNeuronLayers; ++layer)
    {
        for (int neuronIndex = 0; neuronIndex < NeuronsInLayer; ++neuronIndex)
        {
            Neuron* neuron = &brain->allNeurons[layer][neuronIndex];

            file.WriteByte((byte)neuron->type);
            file.WriteFloat(neuron->bias);
            file.WriteByte((byte)neuron->numConnections);

            for (uint connectionIndex = 0; connectionIndex < neuron->numConnections; ++connectionIndex)
            {
                NeuronConnection* connection = &neuron->allConnections[connectionIndex];

                file.WriteByte(connection->dest_layer);
                file.WriteByte(connection->dest_neuron);
                file.WriteFloat(connection->weight);
            }
        }
    }

    if (includeMemory)
        file.write((char*)brain->allMemory, NumNeuronLayers * NeuronsInLayer * sizeof(float));
}

bool ObjectSaver::LoadBrainCompact(MyInputStream& file, BotNeuralNet* brain, bool includeMemory)
{
    for (int layer = 0; layer < NumNeuronLayers; ++layer)
    {
        for (int neuronIndex = 0; neuronIndex < NeuronsInLayer; ++neuronIndex)
        {
            Neuron* neuron = &brain->allNeurons[layer][neuronIndex];

            neuron->type = (NeuronType)file.ReadByte();
            neuron->bias = file.ReadFloat();
            neuron->numConnections = file.ReadByte();
            neuron->layer = layer;

            if (neuron->numConnections > NeuronsInLayer)
                return false;

            for (uint connectionIndex = 0; connectionIndex < neuron->numConnections; ++connectionIndex)
            {
                NeuronConnection* connection = &neuron->allConnections[connectionIndex];

                connection->dest_layer = file.ReadByte();
                connection->dest_neuron = file.ReadByte();
                connection->weight = file.ReadFloat();

                if ((connection->dest_layer >= NumNeuronLayers) || (connection->dest_neuron >= NeuronsInLayer))
                    return false;
            }
        }
    }

    if (includeMemory)
        file.read((char*)brain->allMemory, NumNeuronLayers * NeuronsInLayer * sizeof(float));

    return true;
}

void ObjectSaver::WriteBotCompact(MyOutStream& file, Bot* obj)
{
    file.WriteByte((byte)obj->type);
    file.WriteInt(obj->GetLifetime());

    file.WriteByte((byte)obj->GetColor()->r);
    file.WriteByte((byte)obj->GetColor()->g);
    file.WriteByte((byte)obj->GetColor()->b);

    repeat(NumberOfMutationMarkers)
    {
        file.WriteInt(obj->GetMarkers()[i]);
    }

    file.WriteInt(obj->energy);

    WriteBrainCompact(file, obj->GetActiveBrain(), true);
    WriteBrainCompact(file, obj->GetInitialBrain(), false);
}

Bot* ObjectSaver::LoadBotCompact(MyInputStream& file)
{
    Bot* toRet = new Bot(0, 0);

    toRet->SetLifetime(file.ReadInt());
    toRet->SetColor(file.ReadByte(), file.ReadByte(), file.ReadByte());

    repeat(NumberOfMutationMarkers)
    {
        toRet->GetMarkers()[i] = file.ReadInt();
    }

    toRet->energy = file.ReadInt();

    if (!LoadBrainCompact(file, toRet->GetActiveBrain(), true))
    {
        delete toRet;
        return NULL;
    }

    if (!LoadBrainCompact(file, toRet->GetInitialBrain(), false))
    {
        delete toRet;
        return NULL;
    }

    return toRet;
}

void ObjectSaver::WriteObjectCompact(MyOutStream& file, Object* obj)
{
    switch (obj->type)
    {
    case bot:
        WriteBotCompact(file, (Bot*)obj);
        break;

    case rock:
        file.WriteByte((byte)ObjectTypes::rock);
        file.WriteInt(obj->GetLifetime());
        break;

    case apple:
        file.WriteByte((byte)ObjectTypes::apple);
        file.WriteInt(obj->GetLifetime());
        file.WriteInt(obj->energy);
        break;

    case organic_waste:
        file.WriteByte((byte)ObjectTypes::organic_waste);
        file.WriteInt(obj->GetLifetime());
        file.WriteInt(obj->energy);
        break;

    default:
        throw("TODO save object");
    }
}

Object* ObjectSaver::LoadObjectCompact(MyInputStream& file)
{
    Object* toRet;

    switch ((ObjectTypes)file.ReadByte())
    {
    case bot:
        return LoadBotCompact(file);

    case rock:
        toRet = new Rock(0, 0);

        toRet->SetLifetime(file.ReadInt());

        return toRet;

    case apple:
        toRet = new Apple(0, 0);

        toRet->SetLifetime(file.ReadInt());
        toRet->energy = file.ReadInt();

        return toRet;

    case organic_waste:
        toRet = new Organics(0, 0, 0);

        toRet->SetLifetime(file.ReadInt());
        toRet->energy = file.ReadInt();

        return toRet;
    }

    return NULL;
}


void ObjectSaver::WriteBotToFile(MyOutStream& file, Bot* obj)
{
    file.WriteInt(obj->type);
    file.WriteInt(obj->GetLifetime());

    file.WriteInt(NumberOfMutationMarkers);

    file.WriteInt(NumNeuronLayers);
    file.WriteInt(NeuronsInLayer);
    file.WriteInt(sizeof Neuron);

    file.WriteInt((obj)->GetColor()->r);
    file.WriteInt((obj)->GetColor()->g);
    file.WriteInt((obj)->GetColor()->b);

    repeat(NumberOfMutationMarkers)
    {
        file.WriteInt((obj)->GetMarkers()[i]);
    }

    file.WriteInt((obj)->energy);

    file.write((char*)(obj)->GetActiveBrain()->allNeurons, NumNeuronLayers * NeuronsInLayer * sizeof(Neuron));
    file.write((char*)(obj)->GetInitialBrain()->allNeurons, NumNeuronLayers * NeuronsInLayer * sizeof(Neuron));

    file.write((char*)(obj)->GetActiveBrain()->allMemory, NumNeuronLayers* NeuronsInLayer * sizeof(float));
}

void ObjectSaver::WriteObjectToFile(MyOutStream& file, Object* obj)
{
    switch (obj->type)
    {
    case bot:
        WriteBotToFile(file, (Bot*)obj);
        break;

    case rock:
        file.WriteInt(ObjectTypes::rock);
        file.WriteInt(obj->GetLifetime());
        break;

    case apple:
        file.WriteInt(ObjectTypes::apple);
        file.WriteInt(obj->GetLifetime());
        file.WriteInt(obj->energy);
        break;

    case organic_waste:
        file.WriteInt(ObjectTypes::organic_waste);
        file.WriteInt(obj->GetLifetime());
        file.WriteInt(obj->energy);
        break;

    default:
        throw("TODO save object");
    }
}



/*Save / load

File format:
    4b - magic number
    4b - object type (bot = 1)
    4b - lifetime
    4b - uint dest layers
    4b - uint neurons in layer
    4b - sizeof (Neuron)
    12b - bot color
    4b - num mutation markers
    4b - energy
    following all neurons from first to last layer by layer
    and then memory data
*/
bool ObjectSaver::SaveObject(Object* obj, char* filename)
{
    return SaveObject(obj, std::filesystem::path(filename));
}


bool ObjectSaver::SaveObject(Object* obj, const std::filesystem::path& filename)
{

    //Open file for writing, binary type, all contents to be deleted
    MyOutStream file(filename, std::ios::in | std::ios::binary | std::ios::trunc);
    
    if (file.is_open())
    {
        file.WriteInt(MagicNumber_ObjectFile);

        WriteObjectToFile(file, obj);

        file.close();

        return true;
    }

    return false;

}

Object* ObjectSaver::LoadObject(char* filename)
{
    return LoadObject(std::filesystem::path(filename));
}


Object* ObjectSaver::LoadObject(const std::filesystem::path& filename)
{    
    Object* toRet;

    //Open file for reading, binary type
    MyInputStream file(filename, std::ios::in | std::ios::binary | std::ios::beg);

    if (file.is_open())
    {
        if (file.ReadInt() != MagicNumber_ObjectFile)
            return NULL;
        
        toRet = LoadObjectFromFile(file);

        file.close();

        return toRet;

    }

    return NULL;

}

void MyOutStream::WriteInt(int data)
{
    write((char*)&data, sizeof(int));
}

void MyOutStream::WriteBool(bool data)
{
    write((char*)&data, 1);
}

void MyOutStream::WriteByte(byte data)
{
    write((char*)&data, sizeof(byte));
}

void MyOutStream::WriteUShort(unsigned short data)
{
    write((char*)&data, sizeof(unsigned short));
}

void MyOutStream::WriteFloat(float data)
{
    write((char*)&data, sizeof(float));
}

MyOutStream::MyOutStream(char* filename, int flags) :std::ofstream(filename, flags) {}
MyOutStream::MyOutStream(const std::filesystem::path& filename, int flags) :std::ofstream(filename, flags) {}


int MyInputStream::ReadInt()
{
    int toRet;

    read((char*)&toRet, sizeof(int));

    return toRet;
}

bool MyInputStream::ReadBool()
{
    bool toRet = false;

    read((char*)&toRet, 1);

    return toRet;
}

byte MyInputStream::ReadByte()
{
    byte toRet = 0;

    read((char*)&toRet, sizeof(byte));

    return toRet;
}

unsigned short MyInputStream::ReadUShort()
{
    unsigned short toRet = 0;

    read((char*)&toRet, sizeof(unsigned short));

    return toRet;
}

float MyInputStream::ReadFloat()
{
    float toRet = 0.0f;

    read((char*)&toRet, sizeof(float));

    return toRet;
}

MyInputStream::MyInputStream(char* filename, int flags) :std::ifstream(filename, flags) {}
MyInputStream::MyInputStream(const std::filesystem::path& filename, int flags) :std::ifstream(filename, flags) {}
