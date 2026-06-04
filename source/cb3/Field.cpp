#include "Field.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace cb3
{

int Field::renderX = 0;
int Field::viewX = 0;
int Field::viewY = 0;
double Field::zoom = 1.0;

thread_local std::size_t Field::currentWorker = 0;
thread_local Object* Field::currentObject = NULL;
thread_local std::uint32_t Field::currentSequence = 0;

static int GetSidePanelXForField()
{
    int fixedX = 2 * FieldX + FieldWidth;
    int maxVisibleX = windowWidth - GUISidePanelWidth - InterfaceBorder;

    if (maxVisibleX < InterfaceBorder)
    {
        maxVisibleX = InterfaceBorder;
    }

    return (fixedX < maxVisibleX) ? fixedX : maxVisibleX;
}

static double ClampFieldZoom(double value)
{
    if (value < FieldZoomMin)
    {
        return FieldZoomMin;
    }

    if (value > FieldZoomMax)
    {
        return FieldZoomMax;
    }

    return value;
}

std::size_t Field::Index(int X, int Y) const
{
    return static_cast<std::size_t>(Y) * config.width + static_cast<std::size_t>(X);
}

std::uint32_t Field::GetWidth() const
{
    return config.width;
}

std::uint32_t Field::GetHeight() const
{
    return config.height;
}

std::uint32_t Field::GetWorkerCount() const
{
    return static_cast<std::uint32_t>(workers.size());
}

const WorldConfig& Field::GetConfig() const
{
    return config;
}

std::uint64_t Field::GetNextObjectId() const
{
    return nextObjectId;
}

std::uint64_t Field::GetGeneratorState() const
{
    return generatorState;
}

std::uint64_t Field::CalculateStateHash() const
{
    std::uint64_t hash = 1469598103934665603ULL;
    auto addBytes = [&](const void* data, std::size_t size)
    {
        const byte* bytes = static_cast<const byte*>(data);
        for (std::size_t i = 0; i < size; ++i)
        {
            hash ^= bytes[i];
            hash *= 1099511628211ULL;
        }
    };
    auto addBrain = [&](BotNeuralNet* brain, bool includeMemory)
    {
        for (uint layer = 0; layer < NumNeuronLayers; ++layer)
        {
            for (uint neuronIndex = 0; neuronIndex < neuronsInLayer[layer]; ++neuronIndex)
            {
                Neuron& neuron = brain->allNeurons[layer][neuronIndex];
                byte neuronType = static_cast<byte>(neuron.type);
                int8_t bias = neuron.bias;
                byte connections = neuron.numConnections;
                addBytes(&neuronType, sizeof(neuronType));
                addBytes(&bias, sizeof(bias));
                addBytes(&connections, sizeof(connections));
                for (uint connection = 0; connection < neuron.numConnections; ++connection)
                {
                    addBytes(&neuron.allConnections[connection], sizeof(NeuronConnection));
                }
            }

            if (includeMemory)
            {
                addBytes(brain->allMemory[layer], neuronsInLayer[layer]);
            }
        }
    };

    addBytes(&seed, sizeof(seed));
    addBytes(&nextObjectId, sizeof(nextObjectId));
    addBytes(&generatorState, sizeof(generatorState));
    addBytes(&spawnApplesCounter, sizeof(spawnApplesCounter));
    addBytes(&season, sizeof(season));
    addBytes(&changeSeasonCounter, sizeof(changeSeasonCounter));
    addBytes(&params, sizeof(params));

    for (Object* object : allCells)
    {
        byte occupied = object ? 1 : 0;
        addBytes(&occupied, sizeof(occupied));
        if (!object)
        {
            continue;
        }

        ObjectTypes type = object->type();
        std::uint64_t id = object->GetStableId();
        uint lifetime = object->GetLifetime();
        addBytes(&type, sizeof(type));
        addBytes(&id, sizeof(id));
        addBytes(&object->x, sizeof(object->x));
        addBytes(&object->y, sizeof(object->y));
        addBytes(&object->energy, sizeof(object->energy));
        addBytes(&lifetime, sizeof(lifetime));

        if (type == bot)
        {
            Bot* creature = static_cast<Bot*>(object);
            Bot::PersistentState state = creature->GetPersistentState();
            addBytes(&state.direction, sizeof(state.direction));
            addBytes(&state.stunned, sizeof(state.stunned));
            addBytes(&state.fertilityDelay, sizeof(state.fertilityDelay));
            addBytes(&state.energyFromPS, sizeof(state.energyFromPS));
            addBytes(&state.energyFromPredation, sizeof(state.energyFromPredation));
            addBytes(&state.energyFromOrganics, sizeof(state.energyFromOrganics));
            addBytes(&state.nextMarker, sizeof(state.nextMarker));
            addBytes(&state.addaptation_birthX, sizeof(state.addaptation_birthX));
            addBytes(&state.numAttacks, sizeof(state.numAttacks));
            addBytes(&state.numMovesX, sizeof(state.numMovesX));
            addBytes(&state.numMovesY, sizeof(state.numMovesY));
            addBytes(&state.numPSonLand, sizeof(state.numPSonLand));
            addBytes(creature->GetMarkers(), sizeof(int) * NumberOfMutationMarkers);
            addBytes(creature->GetColor()->c, sizeof(creature->GetColor()->c));
            addBytes(creature->GetColor()->change_vector, sizeof(creature->GetColor()->change_vector));
            addBrain(creature->GetActiveBrain(), true);
            addBrain(creature->GetInitialBrain(), false);
        }
    }

    return hash;
}

void Field::SetIdentityState(std::uint64_t nextId, std::uint64_t randomState)
{
    nextObjectId = (nextId > 0) ? nextId : 1;
    generatorState = randomState ? randomState : MixRandomSeed(seed);
}

int Field::GetVisibleColumns() const
{
    return (std::min)(static_cast<int>(config.width), static_cast<int>(FieldRenderCellsWidth));
}

int Field::GetFieldPixelWidth() const
{
    return GetVisibleColumns() * FieldCellSize;
}

int Field::GetFieldPixelHeight() const
{
    const std::uint64_t height = static_cast<std::uint64_t>(config.height) * FieldCellSize;
    return static_cast<int>((std::min)(height, static_cast<std::uint64_t>(INT_MAX)));
}

static void GetFieldViewportLayout(const Field& field, SDL_Rect& viewport, bool& needHorizontal, bool& needVertical)
{
    int availableWidth = GetSidePanelXForField() - FieldX - InterfaceBorder;
    int availableHeight = windowHeight - FieldY;

    availableWidth = (std::max)(availableWidth, 1);
    availableHeight = (std::max)(availableHeight, 1);
    needHorizontal = false;
    needVertical = false;

    int scaledFieldWidth = field.GetScaledFieldWidth();
    int scaledFieldHeight = field.GetScaledFieldHeight();

    for (int i = 0; i < 2; ++i)
    {
        needHorizontal = scaledFieldWidth > (availableWidth - (needVertical ? FieldScrollbarSize : 0));
        needVertical = scaledFieldHeight > (availableHeight - (needHorizontal ? FieldScrollbarSize : 0));
    }

    if (needVertical)
    {
        availableWidth -= FieldScrollbarSize;
    }

    if (needHorizontal)
    {
        availableHeight -= FieldScrollbarSize;
    }

    viewport = {FieldX, FieldY, (std::max)(availableWidth, 1), (std::max)(availableHeight, 1)};
}

double Field::GetViewScale() const
{
    int availableWidth = (std::max)(GetSidePanelXForField() - FieldX - InterfaceBorder, 1);
    int availableHeight = (std::max)(windowHeight - FieldY, 1);
    double scaleX = availableWidth / (GetFieldPixelWidth() * 1.0);
    double scaleY = availableHeight / (GetFieldPixelHeight() * 1.0);
    double scale = (std::min)(scaleX, scaleY);
    return (std::max)(scale, 0.05) * ClampFieldZoom(zoom);
}

int Field::GetScaledFieldWidth() const
{
    return (std::max)(1, static_cast<int>(std::ceil(GetFieldPixelWidth() * GetViewScale())));
}

int Field::GetScaledFieldHeight() const
{
    const double value = GetFieldPixelHeight() * GetViewScale();
    return (std::max)(1, static_cast<int>((std::min)(value, static_cast<double>(INT_MAX))));
}

void Field::ClampViewOffset()
{
    viewX = (std::max)(0, (std::min)(viewX, GetMaxViewX()));
    viewY = (std::max)(0, (std::min)(viewY, GetMaxViewY()));
}

SDL_Rect Field::GetViewportRect()
{
    SDL_Rect viewport;
    bool needHorizontal;
    bool needVertical;
    GetFieldViewportLayout(*this, viewport, needHorizontal, needVertical);
    return viewport;
}

int Field::GetMaxViewX()
{
    SDL_Rect viewport;
    bool needHorizontal;
    bool needVertical;
    GetFieldViewportLayout(*this, viewport, needHorizontal, needVertical);
    return (std::max)(GetScaledFieldWidth() - viewport.w, 0);
}

int Field::GetMaxViewY()
{
    SDL_Rect viewport;
    bool needHorizontal;
    bool needVertical;
    GetFieldViewportLayout(*this, viewport, needHorizontal, needVertical);
    return (std::max)(GetScaledFieldHeight() - viewport.h, 0);
}

bool Field::NeedHorizontalScrollbar()
{
    SDL_Rect viewport;
    bool needHorizontal;
    bool needVertical;
    GetFieldViewportLayout(*this, viewport, needHorizontal, needVertical);
    return needHorizontal;
}

bool Field::NeedVerticalScrollbar()
{
    SDL_Rect viewport;
    bool needHorizontal;
    bool needVertical;
    GetFieldViewportLayout(*this, viewport, needHorizontal, needVertical);
    return needVertical;
}

void Field::PanView(int deltaX, int deltaY)
{
    viewX -= deltaX;
    viewY -= deltaY;
    ClampViewOffset();
}

void Field::ZoomAtScreenPoint(int X, int Y, int wheelDelta)
{
    if (wheelDelta == 0)
    {
        return;
    }

    ClampViewOffset();
    SDL_Rect viewport = GetViewportRect();
    double oldScale = GetViewScale();
    double fieldX = (viewX + X - viewport.x) / oldScale;
    double fieldY = (viewY + Y - viewport.y) / oldScale;
    double factor = std::pow(FieldZoomStep, std::abs(wheelDelta));
    zoom = ClampFieldZoom((wheelDelta > 0) ? zoom * factor : zoom / factor);

    double newScale = GetViewScale();
    viewX = static_cast<int>(std::round(fieldX * newScale - (X - viewport.x)));
    viewY = static_cast<int>(std::round(fieldY * newScale - (Y - viewport.y)));
    ClampViewOffset();
}

void Field::ChangeSeason()
{
    season = static_cast<Season>(static_cast<int>(season) + 1);
    if (season > spring)
    {
        season = summer;
    }
}

void Field::SeasonTick()
{
    if (++changeSeasonCounter >= static_cast<uint>(params.seasonInterval))
    {
        ChangeSeason();
        changeSeasonCounter = 0;
    }
}

Season Field::GetSeason()
{
    return season;
}

uint Field::GetSeasonCounter()
{
    return changeSeasonCounter;
}

Field::PersistentState Field::GetPersistentState() const
{
    return {spawnApplesCounter, season, changeSeasonCounter, nextObjectId, generatorState};
}

void Field::SetPersistentState(const PersistentState& state)
{
    spawnApplesCounter = state.spawnApplesCounter;
    season = state.season <= spring ? state.season : summer;
    changeSeasonCounter = state.changeSeasonCounter;
    SetIdentityState(state.nextObjectId, state.generatorState);
}

void Field::shiftRenderPoint(int cx)
{
    const int width = static_cast<int>(config.width);
    if (width <= 0)
    {
        renderX = 0;
        return;
    }

    renderX = (renderX + cx) % width;
    if (renderX < 0)
    {
        renderX += width;
    }
}

void Field::jumpToFirstBot()
{
    for (std::size_t index = 0; index < allCells.size(); ++index)
    {
        Object* object = allCells[index];
        if (object && object->type() == bot)
        {
            renderX = object->x;
            return;
        }
    }
}

Object* Field::GetCell(int X, int Y) const
{
    if (!IsInBounds(X, Y))
    {
        return NULL;
    }

    const std::size_t index = Index(X, Y);
    if (planningPhase.load(std::memory_order_relaxed))
    {
        return snapshotCells[index].object;
    }

    return allCells[index];
}

ObjectTypes Field::GetObjectTypeAt(int X, int Y) const
{
    if (!IsInBounds(X, Y))
    {
        return abstract;
    }

    if (planningPhase.load(std::memory_order_relaxed))
    {
        return snapshotCells[Index(X, Y)].type;
    }

    Object* object = allCells[Index(X, Y)];
    return object ? object->type() : abstract;
}

int Field::GetSnapshotEnergy(int X, int Y) const
{
    if (!IsInBounds(X, Y))
    {
        return 0;
    }

    if (planningPhase.load(std::memory_order_relaxed))
    {
        return snapshotCells[Index(X, Y)].energy;
    }

    Object* object = allCells[Index(X, Y)];
    return object ? object->energy : 0;
}

int Field::GetSnapshotDirection(int X, int Y) const
{
    if (!IsInBounds(X, Y))
    {
        return 0;
    }

    if (planningPhase.load(std::memory_order_relaxed))
    {
        return snapshotCells[Index(X, Y)].direction;
    }

    Object* object = allCells[Index(X, Y)];
    return (object && object->type() == bot) ? static_cast<Bot*>(object)->GetDirection() : 0;
}

int Field::GetSnapshotKinship(const Bot* observer, int X, int Y) const
{
    if (!observer || !IsInBounds(X, Y))
    {
        return 0;
    }

    if (!planningPhase.load(std::memory_order_relaxed))
    {
        Object* object = allCells[Index(X, Y)];
        return (object && object->type() == bot) ? const_cast<Bot*>(observer)->FindKinship(static_cast<Bot*>(object)) : 0;
    }

    const CellSnapshot& snapshot = snapshotCells[Index(X, Y)];
    if (!snapshot.object || snapshot.type != bot)
    {
        return 0;
    }

    int matching = 0;
    int* markers = const_cast<Bot*>(observer)->GetMarkers();
    for (uint i = 0; i < NumberOfMutationMarkers; ++i)
    {
        if (markers[i] == snapshot.markers[i])
        {
            ++matching;
        }
    }

    if (matching >= NumberOfMutationMarkers - HowMuchDifferenseCantBeTold)
    {
        matching = NumberOfMutationMarkers;
    }
    return matching;
}

Point Field::FindFreeNeighbourCell(int X, int Y)
{
    if (GetCell(X, Y) == NULL)
    {
        return {X, Y};
    }

    Point freeCells[9];
    int count = 0;
    for (int cx = -1; cx <= 1; ++cx)
    {
        for (int cy = -1; cy <= 1; ++cy)
        {
            int targetX = ValidateX(X + cx);
            int targetY = Y + cy;
            if (IsInBounds(targetX, targetY) && GetCell(targetX, targetY) == NULL)
            {
                freeCells[count++].Set(targetX, targetY);
            }
        }
    }

    return count > 0 ? freeCells[RandomVal(count)] : Point(-1, -1);
}

Point Field::FindRandomNeighbourBot(int X, int Y)
{
    Point bots[9];
    int count = 0;
    for (int cx = -1; cx <= 1; ++cx)
    {
        for (int cy = -1; cy <= 1; ++cy)
        {
            int targetX = ValidateX(X + cx);
            int targetY = Y + cy;
            if (IsInBounds(targetX, targetY) && GetObjectTypeAt(targetX, targetY) == bot)
            {
                bots[count++].Set(targetX, targetY);
            }
        }
    }

    return count > 0 ? bots[RandomVal(count)] : Point(-1, -1);
}

int Field::FindHowManyFreeCellsAround(int X, int Y)
{
    int count = 0;
    for (int cx = -1; cx <= 1; ++cx)
    {
        for (int cy = -1; cy <= 1; ++cy)
        {
            int targetX = ValidateX(X + cx);
            int targetY = Y + cy;
            if (IsInBounds(targetX, targetY) && GetCell(targetX, targetY) == NULL)
            {
                ++count;
            }
        }
    }
    return count;
}

bool Field::IsCurrentObjectAt(Object* object, int X, int Y) const
{
    return object && IsInBounds(X, Y) && allCells[Index(X, Y)] == object;
}

bool Field::IsLiveIdentity(Object* object, std::uint64_t id) const
{
    return object && liveObjects.find(object) != liveObjects.end() && object->GetStableId() == id;
}

int Field::MoveObjectDirect(Object* obj, int toX, int toY)
{
    if (!obj || !IsInBounds(toX, toY))
    {
        return -2;
    }

    if (allCells[Index(toX, toY)])
    {
        return -1;
    }

    if (!IsCurrentObjectAt(obj, obj->x, obj->y))
    {
        return -3;
    }

    allCells[Index(obj->x, obj->y)] = NULL;
    allCells[Index(toX, toY)] = obj;
    obj->x = toX;
    obj->y = toY;
    return 0;
}

void Field::QueueCommand(Command command)
{
    command.actorId = command.actor ? command.actor->GetStableId() : 0;
    command.targetId = command.target ? command.target->GetStableId() : 0;
    command.sourceIndex = currentObject ? Index(currentObject->x, currentObject->y) : 0;
    command.sequence = currentSequence++;
    workerCommands[currentWorker].push_back(command);
}

int Field::MoveObject(int fromX, int fromY, int toX, int toY)
{
    if (!IsInBounds(toX, toY))
    {
        return -2;
    }

    if (planningPhase.load(std::memory_order_relaxed))
    {
        Object* object = GetCell(fromX, fromY);
        if (!object)
        {
            return -3;
        }
        if (GetCell(toX, toY))
        {
            return -1;
        }

        Command command;
        command.type = CommandType::move;
        command.actor = object;
        command.fromX = fromX;
        command.fromY = fromY;
        command.toX = toX;
        command.toY = toY;
        QueueCommand(command);
        return 0;
    }

    Object* object = IsInBounds(fromX, fromY) ? allCells[Index(fromX, fromY)] : NULL;
    return MoveObjectDirect(object, toX, toY);
}

int Field::MoveObject(Object* obj, int toX, int toY)
{
    return obj ? MoveObject(obj->x, obj->y, toX, toY) : -3;
}

bool Field::AddObjectDirect(Object* obj)
{
    if (!obj || !IsInBounds(obj->x, obj->y))
    {
        return false;
    }

    Object*& cell = allCells[Index(obj->x, obj->y)];
    if (cell)
    {
        return false;
    }

    if (obj->GetStableId() == 0)
    {
        obj->SetStableId(nextObjectId++);
    }
    else if (obj->GetStableId() >= nextObjectId)
    {
        nextObjectId = obj->GetStableId() + 1;
    }

    cell = obj;
    liveObjects.insert(obj);
    return true;
}

bool Field::AddObject(Object* obj)
{
    return AddObjectDirect(obj);
}

void Field::ObjectAddOrReplace(Object* obj)
{
    if (!obj || !IsInBounds(obj->x, obj->y))
    {
        delete obj;
        return;
    }

    RemoveObjectDirect(obj->x, obj->y);
    AddObjectDirect(obj);
}

bool Field::QueueBirth(Bot* parent, Object* child, int X, int Y, int energyCost)
{
    if (!planningPhase.load(std::memory_order_relaxed) || !parent || !child || !IsInBounds(X, Y) || GetCell(X, Y))
    {
        delete child;
        return false;
    }

    Command command;
    command.type = CommandType::birth;
    command.actor = parent;
    command.created = child;
    command.toX = X;
    command.toY = Y;
    command.value = energyCost;
    QueueCommand(command);
    return true;
}

bool Field::QueueAttack(Bot* attacker, int X, int Y, bool digestOrganics)
{
    if (!planningPhase.load(std::memory_order_relaxed) || !attacker || !IsInBounds(X, Y))
    {
        return false;
    }

    Object* target = GetCell(X, Y);
    ObjectTypes targetType = GetObjectTypeAt(X, Y);
    if (!target)
    {
        return false;
    }

    if (digestOrganics)
    {
        if (targetType != organic_waste)
        {
            return false;
        }
    }
    else if (targetType != bot && targetType != apple
#ifdef BotCanEatRock
        && targetType != rock
#endif
        )
    {
        return false;
    }

    Command command;
    command.type = digestOrganics ? CommandType::digest : CommandType::attack;
    command.actor = attacker;
    command.target = target;
    command.targetType = targetType;
    command.toX = X;
    command.toY = Y;
    command.value = GetSnapshotEnergy(X, Y);
    QueueCommand(command);
    return true;
}

void Field::RemoveObjectDirect(int X, int Y)
{
    if (!IsInBounds(X, Y))
    {
        return;
    }

    Object*& object = allCells[Index(X, Y)];
    liveObjects.erase(object);
    delete object;
    object = NULL;
}

void Field::RemoveObject(int X, int Y)
{
    if (planningPhase.load(std::memory_order_relaxed))
    {
        Object* object = GetCell(X, Y);
        if (object)
        {
            Command command;
            command.type = CommandType::remove;
            command.actor = object;
            command.fromX = X;
            command.fromY = Y;
            QueueCommand(command);
        }
        return;
    }

    RemoveObjectDirect(X, Y);
}

void Field::RemoveBot(int X, int Y, int energyVal)
{
    if (planningPhase.load(std::memory_order_relaxed))
    {
        Object* object = GetCell(X, Y);
        if (object)
        {
            Command command;
            command.type = CommandType::remove;
            command.actor = object;
            command.fromX = X;
            command.fromY = Y;
            command.value = energyVal;
            command.spawnOrganics = energyVal > 0 && RandomPercentX10(params.adaptation_organicSpawnRate);
            QueueCommand(command);
        }
        return;
    }

    RemoveObjectDirect(X, Y);
    if (energyVal > 0 && RandomPercentX10(params.adaptation_organicSpawnRate))
    {
        AddObjectDirect(new Organics(X, Y, energyVal));
    }
}

void Field::RemoveAllObjects()
{
    for (Object*& object : allCells)
    {
        liveObjects.erase(object);
        delete object;
        object = NULL;
    }
    liveObjects.clear();
}

void Field::mutateWorld()
{
    BeginGeneratorTask(0x6d7574617465ULL);
    for (Object* object : allCells)
    {
        if (object && object->type() == bot)
        {
            static_cast<Bot*>(object)->Mutagen();
        }
    }
    EndGeneratorTask();
}

void Field::placeWall(uint wallWidth)
{
    wallWidth = (std::min)(wallWidth, config.width);
    Object* first = GetCell(0, 0);
    bool remove = first && first->type() == rock;

    for (uint y = 0; y < config.height; ++y)
    {
        for (uint x = 0; x < wallWidth; ++x)
        {
            if (remove)
            {
                if (GetObjectTypeAt(x, y) == rock)
                {
                    RemoveObjectDirect(x, y);
                }
            }
            else
            {
                ObjectAddOrReplace(new Rock(x, y));
            }
        }
    }
}

void Field::RepaintBot(Bot* source, Color newColor, int differs)
{
    for (Object* object : allCells)
    {
        if (object && object->type() == bot && static_cast<Bot*>(object)->FindKinship(source) >= NumberOfMutationMarkers - differs)
        {
            static_cast<Bot*>(object)->SetColor(newColor);
        }
    }
}

void Field::ObjectTick(Object* object)
{
    int result = object->tick();
    if (result == 1)
    {
        if (object->type() == bot)
        {
            RemoveBot(object->x, object->y, object->energy);
        }
        else
        {
            RemoveObject(object->x, object->y);
        }
    }
}

void Field::BuildSnapshot()
{
    stableObjects.clear();
    stableObjects.reserve(objectsTotal > 0 ? objectsTotal : 1024);

    for (std::size_t index = 0; index < allCells.size(); ++index)
    {
        Object* object = allCells[index];
        CellSnapshot& snapshot = snapshotCells[index];
        snapshot.object = object;
        snapshot.type = object ? object->type() : abstract;
        snapshot.energy = object ? object->energy : 0;
        snapshot.direction = 0;
        std::fill(std::begin(snapshot.markers), std::end(snapshot.markers), 0);

        if (object)
        {
            stableObjects.push_back(object);
            if (snapshot.type == bot)
            {
                Bot* creature = static_cast<Bot*>(object);
                snapshot.direction = creature->GetDirection();
                std::copy(creature->GetMarkers(), creature->GetMarkers() + NumberOfMutationMarkers, snapshot.markers);
            }
        }
    }
}

void Field::ProcessObjectRange(std::size_t workerIndex, std::size_t begin, std::size_t end)
{
    currentWorker = workerIndex;
    for (std::size_t index = begin; index < end; ++index)
    {
        Object* object = stableObjects[index];
        currentObject = object;
        currentSequence = 0;
        SetDeterministicRandom(seed, planningTick, object->GetStableId());
        ObjectTick(object);
    }
    currentObject = NULL;
}

void Field::WorkerLoop(std::size_t index)
{
    std::uint64_t observedGeneration = 0;
    for (;;)
    {
        std::size_t begin = 0;
        std::size_t end = 0;
        {
            std::unique_lock<std::mutex> lock(workerMutex);
            workerStartCondition.wait(lock, [&]()
            {
                return terminateWorkers || workerGeneration != observedGeneration;
            });

            if (terminateWorkers)
            {
                return;
            }

            observedGeneration = workerGeneration;
            if (index < activeWorkers)
            {
                begin = stableObjects.size() * index / activeWorkers;
                end = stableObjects.size() * (index + 1) / activeWorkers;
            }
        }

        if (begin < end)
        {
            ProcessObjectRange(index, begin, end);
        }

        {
            std::lock_guard<std::mutex> lock(workerMutex);
            ++workersDone;
        }
        workerDoneCondition.notify_one();
    }
}

void Field::StartWorkers()
{
    std::uint32_t cpuCount = static_cast<std::uint32_t>((std::max)(SDL_GetCPUCount(), 1));
    std::uint32_t limit = config.maxWorkerThreads == 0 ? cpuCount : (std::min)(config.maxWorkerThreads, cpuCount);
    std::size_t desired = (std::min)(static_cast<std::size_t>((std::max)(limit, 1u)), cellCount);
    workerCommands.resize(desired);
    workers.reserve(desired);

    try
    {
        for (std::size_t index = 0; index < desired; ++index)
        {
            workers.emplace_back(&Field::WorkerLoop, this, index);
        }
    }
    catch (...)
    {
        StopWorkers();
        throw;
    }
}

void Field::StopWorkers()
{
    {
        std::lock_guard<std::mutex> lock(workerMutex);
        terminateWorkers = true;
    }
    workerStartCondition.notify_all();

    for (std::thread& worker : workers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }
    workers.clear();
}

void Field::ApplyCommand(Command& command)
{
    switch (command.type)
    {
    case CommandType::move:
        if (IsLiveIdentity(command.actor, command.actorId) && IsCurrentObjectAt(command.actor, command.fromX, command.fromY))
        {
            if (MoveObjectDirect(command.actor, command.toX, command.toY) == 0 && command.actor->type() == bot)
            {
                Bot* creature = static_cast<Bot*>(command.actor);
                creature->numMovesX += command.fromX != command.toX ? 1 : 0;
                creature->numMovesY += command.fromY != command.toY ? 1 : 0;
            }
        }
        break;

    case CommandType::attack:
    case CommandType::digest:
        if (IsLiveIdentity(command.actor, command.actorId) && IsLiveIdentity(command.target, command.targetId) &&
            IsCurrentObjectAt(command.target, command.toX, command.toY) &&
            command.target->type() == command.targetType)
        {
            Bot* attacker = static_cast<Bot*>(command.actor);
            bool valid = command.type == CommandType::digest ? command.targetType == organic_waste :
                (command.targetType == bot || command.targetType == apple
#ifdef BotCanEatRock
                    || command.targetType == rock
#endif
                );

            if (valid)
            {
                RemoveObjectDirect(command.toX, command.toY);
                if (command.targetType == bot)
                {
                    attacker->GiveEnergy(command.value, predation);
                    ++attacker->numAttacks;
                }
                else if (command.targetType == apple || command.targetType == organic_waste)
                {
                    attacker->GiveEnergy(command.value, organics);
                }
            }
        }
        break;

    case CommandType::birth:
        if (IsLiveIdentity(command.actor, command.actorId) && !GetCell(command.toX, command.toY))
        {
            Bot* parent = static_cast<Bot*>(command.actor);
            if (parent->energy > command.value)
            {
                parent->energy -= command.value;
                command.created->x = command.toX;
                command.created->y = command.toY;
                if (AddObjectDirect(command.created))
                {
                    command.created = NULL;
                }
            }
        }
        break;

    case CommandType::remove:
        if (IsLiveIdentity(command.actor, command.actorId))
        {
            int X = command.actor->x;
            int Y = command.actor->y;
            RemoveObjectDirect(X, Y);
            if (command.spawnOrganics && !GetCell(X, Y))
            {
                AddObjectDirect(new Organics(X, Y, command.value));
            }
        }
        break;
    }
}

void Field::ApplyCommands()
{
    std::vector<Command> commands;
    std::size_t total = 0;
    for (const auto& worker : workerCommands)
    {
        total += worker.size();
    }
    commands.reserve(total);

    for (auto& worker : workerCommands)
    {
        std::move(worker.begin(), worker.end(), std::back_inserter(commands));
        worker.clear();
    }

    std::stable_sort(commands.begin(), commands.end(), [](const Command& left, const Command& right)
    {
        if (left.sourceIndex != right.sourceIndex)
        {
            return left.sourceIndex < right.sourceIndex;
        }
        return left.sequence < right.sequence;
    });

    for (Command& command : commands)
    {
        ApplyCommand(command);
        delete command.created;
        command.created = NULL;
    }
}

void Field::RecalculateStatistics()
{
    objectsTotal = 0;
    botsTotal = 0;
    applesTotal = 0;
    organicsTotal = 0;
    predatorsTotal = 0;
    averageLifetime = 0;

    for (Object* object : allCells)
    {
        if (!object)
        {
            continue;
        }

        ++objectsTotal;
        if (object->type() == bot)
        {
            ++botsTotal;
            Bot* creature = static_cast<Bot*>(object);
            predatorsTotal += creature->isPredator() ? 1 : 0;
            averageLifetime += object->GetLifetime();
        }
        else if (object->type() == apple)
        {
            ++applesTotal;
        }
        else if (object->type() == organic_waste)
        {
            ++organicsTotal;
        }
    }

    if (botsTotal > 0)
    {
        averageLifetime /= botsTotal;
    }
}

void Field::ClampDynamicParams()
{
    params.oceanLevel = (std::max)(0, (std::min)(params.oceanLevel, static_cast<int>(config.height)));
    params.mudLevel = (std::max)(0, (std::min)(params.mudLevel, static_cast<int>(config.height)));
}

void Field::tick(uint thisFrame)
{
    ClampDynamicParams();

    if (params.useSeasons)
    {
        SeasonTick();
    }

    Object::currentFrame = thisFrame;
    planningTick = thisFrame;

    if (params.spawnApples && spawnApplesCounter++ == AppleSpawnInterval)
    {
        SpawnApples();
        spawnApplesCounter = 0;
    }

    BuildSnapshot();
    planningPhase.store(true, std::memory_order_release);

    activeWorkers = (std::min)(workers.size(), stableObjects.size());
    if (activeWorkers > 0)
    {
        {
            std::lock_guard<std::mutex> lock(workerMutex);
            workersDone = 0;
            ++workerGeneration;
        }
        workerStartCondition.notify_all();

        std::unique_lock<std::mutex> lock(workerMutex);
        workerDoneCondition.wait(lock, [&]()
        {
            return workersDone == workers.size();
        });
    }

    planningPhase.store(false, std::memory_order_release);
    ApplyCommands();
    RecalculateStatistics();
}

void Field::draw(RenderTypes render)
{
    ClampDynamicParams();
    ClampViewOffset();
    SDL_Rect viewport = GetViewportRect();
    int scaledFieldWidth = GetScaledFieldWidth();
    int scaledFieldHeight = GetScaledFieldHeight();
    double scale = GetViewScale();
    SDL_Rect fieldRect = {FieldX - viewX, FieldY - viewY, scaledFieldWidth, scaledFieldHeight};

    SDL_RenderSetClipRect(renderer, &viewport);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &viewport);
    SDL_SetRenderDrawColor(renderer, FieldBackgroundColor);
    SDL_RenderFillRect(renderer, &fieldRect);

    int mudHeight = static_cast<int>(std::ceil(params.mudLevel * FieldCellSize * scale));
    SDL_SetRenderDrawColor(renderer, MudColor);
    SDL_Rect mud = {FieldX - viewX, FieldY - viewY + scaledFieldHeight - mudHeight, scaledFieldWidth, mudHeight};
    SDL_RenderFillRect(renderer, &mud);

    int oceanHeight = static_cast<int>(std::ceil(params.oceanLevel * FieldCellSize * scale));
    SDL_SetRenderDrawColor(renderer, OceanColor);
    SDL_Rect ocean = {FieldX - viewX, FieldY - viewY + scaledFieldHeight - oceanHeight, scaledFieldWidth, (std::max)(oceanHeight - mud.h, 0)};
    SDL_RenderFillRect(renderer, &ocean);

    double scaledCell = FieldCellSize * scale;
    int firstY = (std::max)(0, static_cast<int>(std::floor(viewY / scaledCell)) - 1);
    int lastY = (std::min)(static_cast<int>(config.height), static_cast<int>(std::ceil((viewY + viewport.h) / scaledCell)) + 1);
    int X = renderX;

    for (int visibleX = 0; visibleX < GetVisibleColumns(); ++visibleX)
    {
        if (X >= static_cast<int>(config.width))
        {
            X -= static_cast<int>(config.width);
        }

        for (int Y = firstY; Y < lastY; ++Y)
        {
            Object* object = allCells[Index(X, Y)];
            if (object)
            {
                switch (render)
                {
                case natural: object->draw(); break;
                case predators: object->drawPredators(); break;
                case energy: object->drawEnergy(); break;
                }
            }
        }
        ++X;
    }

#ifdef DrawUnderwaterMask
    SDL_SetRenderDrawColor(renderer, UnderwaterMaskColor);
    SDL_Rect underwater = {FieldX - viewX, FieldY - viewY + scaledFieldHeight - oceanHeight, scaledFieldWidth, oceanHeight};
    SDL_RenderFillRect(renderer, &underwater);
#endif

    SDL_RenderSetClipRect(renderer, NULL);
}

bool Field::IsInBounds(int X, int Y) const
{
    return X >= 0 && Y >= 0 && X < static_cast<int>(config.width) && Y < static_cast<int>(config.height);
}

bool Field::IsInBounds(Point point) const
{
    return IsInBounds(point.x, point.y);
}

bool Field::IsInWater(int Y) const
{
    return Y >= static_cast<int>(config.height) - params.oceanLevel;
}

bool Field::IsInMud(int Y) const
{
    return Y >= static_cast<int>(config.height) - params.mudLevel;
}

int Field::ValidateX(int X) const
{
    const int width = static_cast<int>(config.width);
    X %= width;
    return X < 0 ? X + width : X;
}

int Field::FindDistanceX(int X1, int X2) const
{
    int direct = std::abs(X2 - X1);
    return (std::min)(direct, static_cast<int>(config.width) - direct);
}

bool Field::IsInBoundsScreenCoords(int X, int Y)
{
    ClampViewOffset();
    SDL_Rect viewport = GetViewportRect();
    SDL_Rect fieldRect = {FieldX - viewX, FieldY - viewY, GetScaledFieldWidth(), GetScaledFieldHeight()};
    return X >= viewport.x && X < viewport.x + viewport.w && Y >= viewport.y && Y < viewport.y + viewport.h &&
        X >= fieldRect.x && X < fieldRect.x + fieldRect.w && Y >= fieldRect.y && Y < fieldRect.y + fieldRect.h;
}

Point Field::ScreenCoordsToLocal(int X, int Y)
{
    ClampViewOffset();
    SDL_Rect viewport = GetViewportRect();
    double scale = GetViewScale();
    X = static_cast<int>(std::floor(((X - viewport.x + viewX) / scale) / FieldCellSize));
    Y = static_cast<int>(std::floor(((Y - viewport.y + viewY) / scale) / FieldCellSize));
    return {ValidateX(X + renderX), Y};
}

Object* Field::GetObjectLocalCoords(int X, int Y)
{
    return IsInBounds(X, Y) ? allCells[Index(X, Y)] : NULL;
}

bool Field::ValidateObjectExistance(Object* object)
{
    return object && liveObjects.find(object) != liveObjects.end();
}

uint Field::GetNumObjects() { return objectsTotal; }
uint Field::GetNumBots() { return botsTotal; }
uint Field::GetNumApples() { return applesTotal; }
uint Field::GetNumOrganics() { return organicsTotal; }
uint Field::GetNumPredators() { return predatorsTotal; }
uint Field::GetAverageLifetime() { return averageLifetime; }

void Field::BeginGeneratorTask(std::uint64_t stream)
{
    SetRandomState(MixRandomSeed(generatorState ^ MixRandomSeed(stream)));
}

void Field::EndGeneratorTask()
{
    generatorState = GetRandomState();
}

int Field::RandomWorldValue(int max, std::uint64_t stream)
{
    BeginGeneratorTask(stream);
    int value = RandomVal(max);
    EndGeneratorTask();
    return value;
}

void Field::SpawnControlGroup()
{
    BeginGeneratorTask(0x636f6e74726f6cULL);
    for (int i = 0; i < ControlGroupSize; ++i)
    {
        Bot* creature = new Bot(RandomVal(config.width), RandomVal(config.height), params.botMaxEnergy);
        if (!AddObjectDirect(creature))
        {
            delete creature;
        }
    }
    EndGeneratorTask();
}

void Field::SpawnApples()
{
    BeginGeneratorTask(0x6170706c6573ULL ^ planningTick);
    const uint landHeight = config.height > static_cast<uint>(params.oceanLevel) ? config.height - params.oceanLevel : 0;
    for (uint Y = 0; Y < landHeight; ++Y)
    {
        for (uint X = 0; X < config.width; ++X)
        {
            if (!allCells[Index(X, Y)] && RandomPercentX10(SpawnAppleInCellChance))
            {
                AddObjectDirect(new Apple(X, Y));
            }
        }
    }
    EndGeneratorTask();
}

Field::Field(const WorldConfig& worldConfig, std::uint64_t seedValue) :
    config(worldConfig),
    seed(seedValue)
{
    std::wstring error;
    if (!config.Validate(&error))
    {
        throw std::invalid_argument("Invalid CB3 world configuration");
    }

    cellCount = static_cast<std::size_t>(config.width) * config.height;
    allCells.assign(cellCount, NULL);
    snapshotCells.resize(cellCount);
    generatorState = MixRandomSeed(seed);
    params.oceanLevel = (std::min)(params.oceanLevel, static_cast<int>(config.height));
    params.mudLevel = (std::min)(params.mudLevel, static_cast<int>(config.height));
    Object::SetField(this);
    StartWorkers();
}

Field::~Field()
{
    StopWorkers();
    RemoveAllObjects();
}

void FieldDynamicParams::Reset()
{
    memset(this, 0, sizeof(*this));
    oceanLevel = InitialOceanHeight;
    mudLevel = InitialMudLayerHeight;
    appleEnergy = DefaultAppleEnergy;
    spawnApples = false;
    botMaxLifetime = MaxBotLifetimeInitial;
    botMaxEnergy = BotMaxEnergyInitial;
    adaptation_StepsNumToDivide_Winds = 0;
    adaptation_landBirthBlock = 0;
    adaptation_seaBirthBlock = 0;
    adaptation_PSInOceanBlock = 0;
    adaptation_PSInMudBlock = 0;
    adaptation_botShouldDoPSOnLandOnceToMultiply = 0;
    adaptation_forceBotMovementsY = 0;
    adaptation_organicSpawnRate = 0;
    adaptation_forceBotMovementsX = 0;
    noPredators = false;
    noMutations = false;
    fertility_delay = FertilityDelayInitial;
    PSreward = PSRewardInitial;
    useSeasons = false;
    seasonInterval = 2000;
}

FieldDynamicParams::FieldDynamicParams()
{
    Reset();
}

}
