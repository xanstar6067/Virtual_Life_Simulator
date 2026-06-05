#include "Field.h"


namespace cb3
{

static constexpr int SaveModeId = static_cast<int>(SimulationMode::CyberBiology3);

Bot* ObjectSaver::LoadBotFromFile(MyInputStream& file)
{
    int lifetime = file.ReadInt();

    if (file.ReadInt() != NumberOfMutationMarkers)
        return NULL;
    if (file.ReadInt() != NumNeuronLayers)
        return NULL;
    if (file.ReadInt() != NumNeuronsInLayerMax)
        return NULL;
    if (file.ReadInt() != sizeof(Neuron))
        return NULL;

    Bot* toRet = new Bot(0, 0);

    toRet->SetLifetime(lifetime);

    Color c;

    repeat(3)
    {
        c.c[i] = file.ReadInt();
        c.change_vector[i] = file.ReadInt();
    }

    toRet->SetColor( c );

    repeat(NumberOfMutationMarkers)
    {
        toRet->GetMarkers()[i] = file.ReadInt();
    }

    toRet->energy = file.ReadInt();
    toRet->SetDirection(file.ReadInt());

    file.read((char*)toRet->GetActiveBrain()->allNeurons, NumNeuronLayers * NumNeuronsInLayerMax * sizeof(Neuron));
    file.read((char*)toRet->GetInitialBrain()->allNeurons, NumNeuronLayers * NumNeuronsInLayerMax * sizeof(Neuron));
    file.read((char*)toRet->GetActiveBrain()->allMemory, NumNeuronLayers * NumNeuronsInLayerMax);

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

ObjectSaver::WorldParams ObjectSaver::LoadWorld(Field* world, char* filename, bool clearWorld, bool loadParams, bool loadLandscape, bool loadBots)
{
    return LoadWorld(world, std::filesystem::path(filename), clearWorld, loadParams, loadLandscape, loadBots);
}

ObjectSaver::WorldParams ObjectSaver::LoadWorld(Field* world, const std::filesystem::path& filename, bool clearWorld, bool loadParams, bool loadLandscape, bool loadBots)
{
    WorldParams toRet = {-1, -1, -1, -1, -1};
    Object* tmpObj;

    //Open file for reading, binary type
    MyInputStream file(filename, std::ios::in | std::ios::binary | std::ios::beg);

    if (file.is_open())
    {
        //Check magic number
        if (file.ReadInt() != MagicNumber_WorldFile)
            goto NoSuccess;

        if (file.ReadInt() != SaveModeId)
            goto NoSuccess;

        //Read world size
        toRet.width = file.ReadInt();
        toRet.height = file.ReadInt();

        if (toRet.height != FieldCellsHeight)
            goto NoSuccess;
                
        if(loadParams)
        {
            //Load world params
            toRet.id = file.ReadInt();
            toRet.seed = file.ReadInt();
            toRet.tick = file.ReadInt();

            if (file.ReadInt() != sizeof world->params)
                goto NoSuccess;

            file.read((char*)&world->params, sizeof world->params);

            Field::PersistentState worldState;
            worldState.spawnApplesCounter = (uint)file.ReadInt();
            worldState.season = (Season)file.ReadByte();
            worldState.changeSeasonCounter = (uint)file.ReadInt();
            world->SetPersistentState(worldState);
        }
        else
        {
            //Skip world params
            file.ignore(4 * 3);

            if (file.ReadInt() != sizeof world->params)
                goto NoSuccess;
            
            file.ignore(sizeof world->params);
            file.ignore(4 + 1 + 4);
        }

        if (file.ReadInt() != NumberOfMutationMarkers)
            goto NoSuccess;

        if (file.ReadInt() != NumNeuronLayers)
            goto NoSuccess;

        if (file.ReadInt() != NumNeuronsInLayerMax)
            goto NoSuccess;

        int objectCount = file.ReadInt();
        if (objectCount < 0)
            goto NoSuccess;

        //Clear world after the header has been validated.
        if(clearWorld)
            world->RemoveAllObjects();

        //Load objects
        for (int i = 0; i < objectCount; ++i)
        {
            int x = file.ReadUShort();
            int y = file.ReadUShort();

            tmpObj = LoadObjectCompact(file);

            if (!tmpObj)
                goto NoSuccess;

            if ((x >= FieldCellsWidth) || (y >= FieldCellsHeight))
            {
                delete tmpObj;
                continue;
            }

            switch (tmpObj->type())
            {
            case rock:
                if (!loadLandscape)
                {
                    delete tmpObj;
                    continue;
                }
                break;

            case bot:
                if (!loadBots)
                {
                    delete tmpObj;
                    continue;
                }
                break;
            }

            tmpObj->x = x;
            tmpObj->y = y;

            if (!world->AddObject(tmpObj))
            {
                delete tmpObj;
            }
        }

        file.close();

        return toRet;
    }

    NoSuccess:
    return {-1, -1};
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
    //Open file for writing, binary type
    MyOutStream file(filename, std::ios::in | std::ios::binary | std::ios::trunc);
    Object* tmpObj;

    if (file.is_open())
    {
        //Magic number
        file.WriteInt(MagicNumber_WorldFile);
        file.WriteInt(SaveModeId);

        //World size
        file.WriteInt(FieldCellsWidth);
        file.WriteInt(FieldCellsHeight);

        //World params
        file.WriteInt(id);
        file.WriteInt(world->seed);
        file.WriteInt(ticknum);

        file.WriteInt(sizeof world->params);
        file.write((char*)&world->params, sizeof world->params);

        const Field::PersistentState worldState = world->GetPersistentState();
        file.WriteInt((int)worldState.spawnApplesCounter);
        file.WriteByte((byte)worldState.season);
        file.WriteInt((int)worldState.changeSeasonCounter);

        file.WriteInt(NumberOfMutationMarkers);
        file.WriteInt(NumNeuronLayers);
        file.WriteInt(NumNeuronsInLayerMax);

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

        //All objects
        for (int x = 0; x < FieldCellsWidth; ++x)
        {
            for (int y = 0; y < FieldCellsHeight; ++y)
            {
                tmpObj = world->GetObjectLocalCoords(x, y);

                if (tmpObj)
                {
                    file.WriteUShort((unsigned short)x);
                    file.WriteUShort((unsigned short)y);
                    WriteObjectCompact(file, tmpObj);
                }
            }
        }

        file.close();

        return true;
    }

    return false;
}

void ObjectSaver::WriteBrainCompact(MyOutStream& file, BotNeuralNet* brain, bool includeMemory)
{
    for (uint layer = 0; layer < NumNeuronLayers; ++layer)
    {
        for (uint neuronIndex = 0; neuronIndex < neuronsInLayer[layer]; ++neuronIndex)
        {
            Neuron* neuron = &brain->allNeurons[layer][neuronIndex];

            file.WriteByte((byte)neuron->type);
            file.WriteByte((byte)neuron->bias);
            file.WriteByte((byte)neuron->numConnections);

            for (uint connectionIndex = 0; connectionIndex < neuron->numConnections; ++connectionIndex)
            {
                NeuronConnection* connection = &neuron->allConnections[connectionIndex];

                file.WriteByte(connection->dest_layer);
                file.WriteByte(connection->dest_neuron);
                file.WriteByte((byte)connection->weight);
            }
        }
    }

    if (includeMemory)
    {
        for (uint layer = 0; layer < NumNeuronLayers; ++layer)
            file.write((char*)brain->allMemory[layer], neuronsInLayer[layer]);
    }
}

bool ObjectSaver::LoadBrainCompact(MyInputStream& file, BotNeuralNet* brain, bool includeMemory)
{
    for (uint layer = 0; layer < NumNeuronLayers; ++layer)
    {
        for (uint neuronIndex = neuronsInLayer[layer]; neuronIndex < NumNeuronsInLayerMax; ++neuronIndex)
        {
            Neuron* neuron = &brain->allNeurons[layer][neuronIndex];
            neuron->type = basic;
            neuron->layer = (byte)layer;
            neuron->SetZero();
        }
    }

    for (uint layer = 0; layer < NumNeuronLayers; ++layer)
    {
        for (uint neuronIndex = 0; neuronIndex < neuronsInLayer[layer]; ++neuronIndex)
        {
            Neuron* neuron = &brain->allNeurons[layer][neuronIndex];

            neuron->type = (NeuronType)file.ReadByte();
            neuron->layer = (byte)layer;
            neuron->bias = (int8_t)file.ReadByte();
            neuron->numConnections = file.ReadByte();

            if (neuron->numConnections > NumNeuronsInLayerMax)
                return false;

            for (uint connectionIndex = 0; connectionIndex < neuron->numConnections; ++connectionIndex)
            {
                NeuronConnection* connection = &neuron->allConnections[connectionIndex];

                connection->dest_layer = file.ReadByte();
                connection->dest_neuron = file.ReadByte();
                connection->weight = (int8_t)file.ReadByte();

                if ((connection->dest_layer >= NumNeuronLayers) || (connection->dest_neuron >= neuronsInLayer[connection->dest_layer]))
                    return false;
            }
        }
    }

    if (includeMemory)
    {
        memset(brain->allMemory, 0, sizeof brain->allMemory);

        for (uint layer = 0; layer < NumNeuronLayers; ++layer)
            file.read((char*)brain->allMemory[layer], neuronsInLayer[layer]);
    }

    return true;
}

void ObjectSaver::WriteBotCompact(MyOutStream& file, Bot* obj)
{
    file.WriteByte((byte)obj->type());
    file.WriteInt(obj->GetLifetime());

    repeat(3)
    {
        file.WriteUShort((unsigned short)obj->GetColor()->c[i]);
        file.WriteByte((byte)obj->GetColor()->change_vector[i]);
    }

    repeat(NumberOfMutationMarkers)
    {
        file.WriteInt(obj->GetMarkers()[i]);
    }

    file.WriteInt(obj->energy);

    const Bot::PersistentState state = obj->GetPersistentState();
    file.WriteByte((byte)state.direction);
    file.WriteInt((int)state.stunned);
    file.WriteInt((int)state.fertilityDelay);
    file.WriteInt(state.energyFromPS);
    file.WriteInt(state.energyFromPredation);
    file.WriteInt(state.energyFromOrganics);
    file.WriteByte((byte)state.nextMarker);
    file.WriteInt(state.addaptation_birthX);
    file.WriteInt((int)state.numAttacks);
    file.WriteInt((int)state.numMovesX);
    file.WriteInt((int)state.numMovesY);
    file.WriteInt((int)state.numPSonLand);

    WriteBrainCompact(file, obj->GetActiveBrain(), true);
    WriteBrainCompact(file, obj->GetInitialBrain(), false);
}

Bot* ObjectSaver::LoadBotCompact(MyInputStream& file)
{
    Bot* toRet = new Bot(0, 0);

    toRet->SetLifetime(file.ReadInt());

    Color c;

    repeat(3)
    {
        c.c[i] = (short)file.ReadUShort();
        c.change_vector[i] = (char)file.ReadByte();
    }

    toRet->SetColor(c);

    repeat(NumberOfMutationMarkers)
    {
        toRet->GetMarkers()[i] = file.ReadInt();
    }

    toRet->energy = file.ReadInt();

    Bot::PersistentState state;
    state.direction = file.ReadByte();
    state.stunned = (uint)file.ReadInt();
    state.fertilityDelay = (uint)file.ReadInt();
    state.energyFromPS = file.ReadInt();
    state.energyFromPredation = file.ReadInt();
    state.energyFromOrganics = file.ReadInt();
    state.nextMarker = file.ReadByte();
    state.addaptation_birthX = file.ReadInt();
    state.numAttacks = (uint)file.ReadInt();
    state.numMovesX = (uint)file.ReadInt();
    state.numMovesY = (uint)file.ReadInt();
    state.numPSonLand = (uint)file.ReadInt();
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
    switch (obj->type())
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
        throw;
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
    file.WriteInt(obj->type());
    file.WriteInt(obj->GetLifetime());

    file.WriteInt(NumberOfMutationMarkers);

    file.WriteInt(NumNeuronLayers);
    file.WriteInt(NumNeuronsInLayerMax);
    file.WriteInt(sizeof(Neuron));

    repeat(3)
    {
        file.WriteInt(obj->GetColor()->c[i]);
        file.WriteInt(obj->GetColor()->change_vector[i]);
    }

    repeat(NumberOfMutationMarkers)
    {
        file.WriteInt(obj->GetMarkers()[i]);
    }

    file.WriteInt(obj->energy);
    file.WriteInt(obj->GetDirection());

    file.write((char*)(obj)->GetActiveBrain()->allNeurons, NumNeuronLayers * NumNeuronsInLayerMax * sizeof(Neuron));
    file.write((char*)(obj)->GetInitialBrain()->allNeurons, NumNeuronLayers * NumNeuronsInLayerMax * sizeof(Neuron));
    file.write((char*)(obj)->GetActiveBrain()->allMemory, NumNeuronLayers * NumNeuronsInLayerMax);
}

void ObjectSaver::WriteObjectToFile(MyOutStream& file, Object* obj)
{
    switch (obj->type())
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
        throw;
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
    Object* toRet;

    //Open file for reading, binary type
    MyInputStream file(filename, std::ios::in | std::ios::binary | std::ios::beg);

    if (file.is_open())
    {
        if (file.ReadInt() != MagicNumber_ObjectFile)
            return NULL;

        if (file.ReadInt() != SaveModeId)
            return NULL;
        
        toRet = LoadObjectCompact(file);

        file.close();

        return toRet;

    }

    return NULL;

}

void MyOutStream::WriteInt(int data)
{
    write((char*)&data, 4);
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

MyOutStream::MyOutStream(char* filename, int flags) :std::ofstream(filename, flags) 
{}

MyOutStream::MyOutStream(const std::filesystem::path& filename, int flags) :std::ofstream(filename, flags)
{}


int MyInputStream::ReadInt()
{
    int toRet;

    read((char*)&toRet, 4);

    return toRet;
}

bool MyInputStream::ReadBool()
{
    bool toRet;

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

MyInputStream::MyInputStream(char* filename, int flags) :std::ifstream(filename, flags) 
{}

MyInputStream::MyInputStream(const std::filesystem::path& filename, int flags) :std::ifstream(filename, flags)
{}

}
