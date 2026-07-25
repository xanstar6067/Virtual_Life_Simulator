
#include "Field.h"

#include <cmath>
#include <memory>

static constexpr int SaveModeId = static_cast<int>(SimulationMode::Classic);


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

    const int red = file.ReadInt();
    const int green = file.ReadInt();
    const int blue = file.ReadInt();
    toRet->SetColor(red, green, blue);

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
            int modeId = file.ReadInt();

            if (modeId != SaveModeId)
            {
                file.close();
                return {-1, -1};
            }

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
    const WorldParams loadFailure = {-1, -1, -1, -1};
    WorldParams toRet = loadFailure;

    const int loadWidth = file.ReadInt();
    toRet.width = loadWidth;

    constexpr int MaxLegacyWidth = FieldCellsWidth * 4;
    if (!file || loadWidth <= 0 || loadWidth > MaxLegacyWidth || file.ReadInt() != FieldCellsHeight)
        return loadFailure;

    toRet.id = file.ReadInt();
    toRet.seed = file.ReadInt();
    toRet.tick = file.ReadInt();

    if (!file || file.ReadInt() != sizeof world->params)
        return loadFailure;

    FieldDynamicParams loadedParams;
    file.read((char*)&loadedParams, sizeof loadedParams);
    if (!file)
        return loadFailure;

    std::vector<std::unique_ptr<Object>> loadedObjects;

    for (int x = 0; x < loadWidth; ++x)
    {
        for (int y = 0; y < FieldCellsHeight; ++y)
        {
            const int objectType = file.ReadInt();
            if (!file)
                return loadFailure;

            if (objectType == ObjectTypes::abstract)
                continue;

            if (objectType < ObjectTypes::bot || objectType > ObjectTypes::apple)
                return loadFailure;

            file.seekg(-static_cast<std::streamoff>(sizeof(int)), std::ios::cur);
            if (!file)
                return loadFailure;

            std::unique_ptr<Object> object(LoadObjectFromFile(file));
            if (!file || !object)
                return loadFailure;

            if (x >= FieldCellsWidth)
                continue;

            object->x = x;
            object->y = y;
            loadedObjects.push_back(std::move(object));
        }
    }

    if (!file)
        return loadFailure;

    // Старый формат хранит все клетки подряд. Мир заменяется только после
    // полного чтения и проверки файла, чтобы поврежденное сохранение не
    // уничтожило текущую симуляцию.
    world->RemoveAllObjects();
    world->params = loadedParams;

    for (std::unique_ptr<Object>& object : loadedObjects)
    {
        if (world->AddObject(object.get()))
            object.release();
    }

    return toRet;
}

ObjectSaver::WorldParams ObjectSaver::LoadWorldCompact(Field* world, MyInputStream& file)
{
    const WorldParams loadFailure = {-1, -1, -1, -1};
    WorldParams toRet = loadFailure;

    int loadWidth = file.ReadInt();
    toRet.width = loadWidth;

    if (!file || loadWidth <= 0 || file.ReadInt() != FieldCellsHeight)
        return loadFailure;

    toRet.id = file.ReadInt();
    toRet.seed = file.ReadInt();
    toRet.tick = file.ReadInt();

    if (!file || file.ReadInt() != sizeof world->params)
        return loadFailure;

    FieldDynamicParams loadedParams;
    file.read((char*)&loadedParams, sizeof loadedParams);

    Field::PersistentState loadedState;
    loadedState.spawnApplesInterval = (uint)file.ReadInt();

    if (!file || file.ReadInt() != NumberOfMutationMarkers)
        return loadFailure;

    if (file.ReadInt() != NumNeuronLayers)
        return loadFailure;

    if (file.ReadInt() != NeuronsInLayer)
        return loadFailure;

    int objectCount = file.ReadInt();
    constexpr int MaxObjectCount = FieldCellsWidth * FieldCellsHeight;
    if (!file || objectCount < 0 || objectCount > MaxObjectCount)
        return loadFailure;

    std::vector<std::unique_ptr<Object>> loadedObjects;
    loadedObjects.reserve((size_t)objectCount);
    std::vector<byte> occupiedCells((size_t)MaxObjectCount, 0);

    for (int i = 0; i < objectCount; ++i)
    {
        int x = file.ReadUShort();
        int y = file.ReadUShort();

        std::unique_ptr<Object> tmpObj(LoadObjectCompact(file));

        if (!file || !tmpObj)
            return loadFailure;

        if ((x >= FieldCellsWidth) || (y >= FieldCellsHeight))
        {
            continue;
        }

        const size_t cellIndex = (size_t)x * FieldCellsHeight + (size_t)y;
        if (occupiedCells[cellIndex])
            return loadFailure;

        tmpObj->x = x;
        tmpObj->y = y;
        occupiedCells[cellIndex] = 1;
        loadedObjects.push_back(std::move(tmpObj));
    }

    if (!file)
        return loadFailure;

    //Commit only after the complete file has been parsed and validated.
    world->RemoveAllObjects();
    world->params = loadedParams;
    world->SetPersistentState(loadedState);

    for (std::unique_ptr<Object>& object : loadedObjects)
    {
        if (world->AddObject(object.get()))
            object.release();
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
        file.WriteInt(SaveModeId);

        file.WriteInt(FieldCellsWidth);
        file.WriteInt(FieldCellsHeight);

        file.WriteInt(id);

        file.WriteInt(world->seed);

        file.WriteInt(ticknum);

        file.WriteInt(sizeof world->params);
        file.write((char*)&world->params, sizeof world->params);

        const Field::PersistentState worldState = world->GetPersistentState();
        file.WriteInt((int)worldState.spawnApplesInterval);

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

            const byte type = file.ReadByte();
            neuron->bias = file.ReadFloat();
            neuron->numConnections = file.ReadByte();
            neuron->layer = layer;

            if (!file || type > (byte)NeuronType::memory || !std::isfinite(neuron->bias) ||
                neuron->numConnections > NeuronsInLayer)
                return false;

            neuron->type = (NeuronType)type;

            for (uint connectionIndex = 0; connectionIndex < neuron->numConnections; ++connectionIndex)
            {
                NeuronConnection* connection = &neuron->allConnections[connectionIndex];

                connection->dest_layer = file.ReadByte();
                connection->dest_neuron = file.ReadByte();
                connection->weight = file.ReadFloat();

                if (!file || !std::isfinite(connection->weight) ||
                    (connection->dest_layer >= NumNeuronLayers) ||
                    (connection->dest_neuron >= NeuronsInLayer))
                    return false;
            }
        }
    }

    if (includeMemory)
    {
        file.read((char*)brain->allMemory, NumNeuronLayers * NeuronsInLayer * sizeof(float));
        if (!file)
            return false;
    }

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

    const Bot::PersistentState state = obj->GetPersistentState();
    file.WriteByte((byte)state.direction);
    file.WriteInt(state.stunned);
    file.WriteInt(state.fertilityDelay);
    file.WriteInt(state.energyFromPS);
    file.WriteInt(state.energyFromPredation);
    file.WriteInt(state.energyFromOrganics);
    file.WriteByte((byte)state.nextMarker);
    file.WriteInt(state.adaptation_numTicks);
    file.WriteInt(state.adaptation_numRightSteps);
    file.WriteInt(state.addaptation_lastX);
    file.WriteInt(state.adaptationCounter);
    file.WriteInt((int)state.numAttacks);
    file.WriteInt((int)state.numMovesY);
    file.WriteInt((int)state.numPSonLand);
    file.WriteBool(state.wasOnLand);

    WriteBrainCompact(file, obj->GetActiveBrain(), true);
    WriteBrainCompact(file, obj->GetInitialBrain(), false);
}

Bot* ObjectSaver::LoadBotCompact(MyInputStream& file)
{
    Bot* toRet = new Bot(0, 0);

    toRet->SetLifetime(file.ReadInt());
    const byte red = file.ReadByte();
    const byte green = file.ReadByte();
    const byte blue = file.ReadByte();
    toRet->SetColor(red, green, blue);

    repeat(NumberOfMutationMarkers)
    {
        toRet->GetMarkers()[i] = file.ReadInt();
    }

    toRet->energy = file.ReadInt();

    Bot::PersistentState state;
    state.direction = file.ReadByte();
    state.stunned = file.ReadInt();
    state.fertilityDelay = file.ReadInt();
    state.energyFromPS = file.ReadInt();
    state.energyFromPredation = file.ReadInt();
    state.energyFromOrganics = file.ReadInt();
    state.nextMarker = file.ReadByte();
    state.adaptation_numTicks = file.ReadInt();
    state.adaptation_numRightSteps = file.ReadInt();
    state.addaptation_lastX = file.ReadInt();
    state.adaptationCounter = file.ReadInt();
    state.numAttacks = (uint)file.ReadInt();
    state.numMovesY = (uint)file.ReadInt();
    state.numPSonLand = (uint)file.ReadInt();
    state.wasOnLand = file.ReadBool();

    if (!file)
    {
        delete toRet;
        return NULL;
    }

    toRet->SetPersistentState(state);

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
        file.WriteInt(SaveModeId);

        WriteObjectCompact(file, obj);

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
    //Open file for reading, binary type
    MyInputStream file(filename, std::ios::in | std::ios::binary | std::ios::beg);

    if (file.is_open())
    {
        const int magicNumber = file.ReadInt();

        Object* toRet = NULL;

        if (magicNumber == MagicNumber_ObjectFile)
        {
            if (file.ReadInt() != SaveModeId)
                return NULL;

            toRet = LoadObjectCompact(file);
        }
        else if (magicNumber == MagicNumber_ObjectFileLegacy)
        {
            toRet = LoadObjectFromFile(file);
        }
        else
        {
            return NULL;
        }

        if (!file || !toRet)
        {
            delete toRet;
            return NULL;
        }

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
    int toRet = 0;

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
