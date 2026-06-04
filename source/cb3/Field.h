#pragma once

namespace cb3
{
class Object;
class Bot;
class Apple;
class Rock;
class Organics;
class ObjectSaver;
}

#include "Settings.h"
#include "Utils.h"
#include "WorldConfig.h"

#include "Object.h"
#include "Bot.h"
#include "Apple.h"
#include "Rock.h"
#include "Organics.h"

#include "ObjectSaver.h"
#include "Chart.h"
#include "ImageFactory.h"

namespace cb3
{

enum RenderTypes
{
    natural,
    predators,
    energy,
    noRender
};

enum Season
{
    summer,
    autumn,
    winter,
    spring
};

constexpr const char* SeasonNames[] =
{
    "summer",
    "autumn",
    "winter",
    "spring"
};

struct FieldDynamicParams
{
    int botMaxLifetime;
    int botMaxEnergy;
    int fertility_delay;

    int oceanLevel;
    int mudLevel;

    bool spawnApples;
    int appleEnergy;

    int adaptation_StepsNumToDivide_Winds;

    int adaptation_landBirthBlock;
    int adaptation_seaBirthBlock;
    int adaptation_PSInOceanBlock;
    int adaptation_PSInMudBlock;
    int adaptation_botShouldDoPSOnLandOnceToMultiply;

    int adaptation_organicSpawnRate;

    int adaptation_forceBotMovementsY;
    int adaptation_forceBotMovementsX;

    bool noPredators;
    bool noMutations;

    int PSreward;

    bool useSeasons;
    int seasonInterval;

    int reserved[31];

    void Reset();
    FieldDynamicParams();
};

class Field final
{
private:
    struct CellSnapshot
    {
        Object* object = NULL;
        ObjectTypes type = abstract;
        int energy = 0;
        int direction = 0;
        int markers[NumberOfMutationMarkers] = {};
    };

    enum class CommandType
    {
        move,
        attack,
        digest,
        birth,
        remove
    };

    struct Command
    {
        CommandType type = CommandType::move;
        Object* actor = NULL;
        Object* target = NULL;
        Object* created = NULL;
        std::uint64_t actorId = 0;
        std::uint64_t targetId = 0;
        ObjectTypes targetType = abstract;
        int fromX = 0;
        int fromY = 0;
        int toX = 0;
        int toY = 0;
        int value = 0;
        std::size_t sourceIndex = 0;
        std::uint32_t sequence = 0;
        bool spawnOrganics = false;
    };

    WorldConfig config;
    std::size_t cellCount = 0;
    std::vector<Object*> allCells;
    std::unordered_set<Object*> liveObjects;
    std::vector<CellSnapshot> snapshotCells;
    std::vector<Object*> stableObjects;

    uint objectsTotal = 0;
    uint botsTotal = 0;
    uint applesTotal = 0;
    uint organicsTotal = 0;
    uint predatorsTotal = 0;
    uint averageLifetime = 0;

    uint spawnApplesCounter = 0;
    Season season = summer;
    uint changeSeasonCounter = 0;

    std::vector<std::thread> workers;
    std::vector<std::vector<Command>> workerCommands;
    std::mutex workerMutex;
    std::condition_variable workerStartCondition;
    std::condition_variable workerDoneCondition;
    std::uint64_t workerGeneration = 0;
    std::size_t workersDone = 0;
    std::size_t activeWorkers = 0;
    bool terminateWorkers = false;
    std::atomic<bool> planningPhase = false;
    uint planningTick = 0;

    static thread_local std::size_t currentWorker;
    static thread_local Object* currentObject;
    static thread_local std::uint32_t currentSequence;

    std::uint64_t nextObjectId = 1;
    std::uint64_t generatorState = 0;

    std::size_t Index(int X, int Y) const;
    void StartWorkers();
    void StopWorkers();
    void WorkerLoop(std::size_t index);
    void ProcessObjectRange(std::size_t workerIndex, std::size_t begin, std::size_t end);
    void BuildSnapshot();
    void QueueCommand(Command command);
    void ApplyCommands();
    void ApplyCommand(Command& command);
    void RecalculateStatistics();
    void ObjectTick(Object* tmpObj);
    bool IsCurrentObjectAt(Object* object, int X, int Y) const;
    bool IsLiveIdentity(Object* object, std::uint64_t id) const;
    int MoveObjectDirect(Object* obj, int toX, int toY);
    bool AddObjectDirect(Object* obj);
    void RemoveObjectDirect(int X, int Y);
    void BeginGeneratorTask(std::uint64_t stream);
    void EndGeneratorTask();
    int GetVisibleColumns() const;
    int GetFieldPixelWidth() const;
    int GetFieldPixelHeight() const;
    void ClampDynamicParams();
    void SeasonTick();
    void ChangeSeason();

public:
    struct PersistentState
    {
        uint spawnApplesCounter;
        Season season;
        uint changeSeasonCounter;
        std::uint64_t nextObjectId;
        std::uint64_t generatorState;
    };

    FieldDynamicParams params;

    void shiftRenderPoint(int cx);
    void jumpToFirstBot();
    void mutateWorld();
    void placeWall(uint width = 2);

    int MoveObject(int fromX, int fromY, int toX, int toY);
    int MoveObject(Object* obj, int toX, int toY);

    bool AddObject(Object* obj);
    void ObjectAddOrReplace(Object* obj);
    bool QueueBirth(Bot* parent, Object* child, int X, int Y, int energyCost);
    bool QueueAttack(Bot* attacker, int X, int Y, bool digestOrganics);

    void RemoveObject(int X, int Y);
    void RemoveAllObjects();
    void RemoveBot(int X, int Y, int energyVal = 0);
    void RepaintBot(Bot* b, Color newColor, int differs = 1);

    void tick(uint thisFrame);
    void draw(RenderTypes render = natural);

    void ClampViewOffset();
    SDL_Rect GetViewportRect();
    int GetMaxViewX();
    int GetMaxViewY();
    bool NeedHorizontalScrollbar();
    bool NeedVerticalScrollbar();
    double GetViewScale() const;
    int GetScaledFieldWidth() const;
    int GetScaledFieldHeight() const;
    void PanView(int deltaX, int deltaY);
    void ZoomAtScreenPoint(int X, int Y, int wheelDelta);

    bool IsInBounds(int X, int Y) const;
    bool IsInBounds(Point p) const;
    bool IsInWater(int Y) const;
    bool IsInMud(int Y) const;

    Point FindFreeNeighbourCell(int X, int Y);
    Point FindRandomNeighbourBot(int X, int Y);
    int FindHowManyFreeCellsAround(int X, int Y);

    int ValidateX(int X) const;
    int FindDistanceX(int X1, int X2) const;
    bool IsInBoundsScreenCoords(int X, int Y);
    Point ScreenCoordsToLocal(int X, int Y);

    Object* GetObjectLocalCoords(int X, int Y);
    Object* GetCell(int X, int Y) const;
    ObjectTypes GetObjectTypeAt(int X, int Y) const;
    int GetSnapshotEnergy(int X, int Y) const;
    int GetSnapshotDirection(int X, int Y) const;
    int GetSnapshotKinship(const Bot* observer, int X, int Y) const;
    bool ValidateObjectExistance(Object* obj);

    std::uint32_t GetWidth() const;
    std::uint32_t GetHeight() const;
    std::uint32_t GetWorkerCount() const;
    const WorldConfig& GetConfig() const;
    std::uint64_t GetNextObjectId() const;
    std::uint64_t GetGeneratorState() const;
    std::uint64_t CalculateStateHash() const;
    void SetIdentityState(std::uint64_t nextId, std::uint64_t randomState);
    int RandomWorldValue(int max, std::uint64_t stream = 0);

    uint GetNumObjects();
    uint GetNumBots();
    uint GetNumApples();
    uint GetNumOrganics();
    uint GetNumPredators();
    uint GetAverageLifetime();
    PersistentState GetPersistentState() const;
    void SetPersistentState(const PersistentState& state);

    void SpawnControlGroup();
    void SpawnApples();

    Season GetSeason();
    uint GetSeasonCounter();

    Field(const WorldConfig& config = WorldConfig(), std::uint64_t seed = 0);
    ~Field();

    std::uint64_t seed;
    static int renderX;
    static int viewX;
    static int viewY;
    static double zoom;
};

}
