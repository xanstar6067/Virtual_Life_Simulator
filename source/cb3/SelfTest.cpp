#include "SelfTest.h"

#include "Field.h"
#include "ObjectSaver.h"

#include <filesystem>
#include <sstream>

namespace cb3
{

class MoveTestObject final : public Object
{
public:
    Point destination;

    MoveTestObject(int X, int Y, Point target) :
        Object(X, Y),
        destination(target)
    {
    }

    constexpr ObjectTypes type() override
    {
        return rock;
    }

    int tick() override
    {
        int result = Object::tick();
        if (result == 0)
        {
            pField->MoveObject(this, destination.x, destination.y);
        }
        return result;
    }
};

class AttackTestBot final : public Bot
{
public:
    Point target;

    AttackTestBot(int X, int Y, Point attackTarget) :
        Bot(X, Y, 100),
        target(attackTarget)
    {
    }

    int tick() override
    {
        int result = Object::tick();
        if (result == 0)
        {
            pField->QueueAttack(this, target.x, target.y, false);
        }
        return result;
    }
};

class BirthTestBot final : public Bot
{
public:
    Point target;

    BirthTestBot(int X, int Y, Point birthTarget) :
        Bot(X, Y, 100),
        target(birthTarget)
    {
    }

    int tick() override
    {
        int result = Object::tick();
        if (result == 0)
        {
            pField->QueueBirth(this, new Rock(target.x, target.y), target.x, target.y, 0);
        }
        return result;
    }
};

class DeleteTestObject final : public Object
{
public:
    Point target;

    DeleteTestObject(int X, int Y, Point deleteTarget) :
        Object(X, Y),
        target(deleteTarget)
    {
    }

    constexpr ObjectTypes type() override
    {
        return rock;
    }

    int tick() override
    {
        int result = Object::tick();
        if (result == 0)
        {
            pField->RemoveObject(target.x, target.y);
        }
        return result;
    }
};

static std::uint64_t RunDeterministicWorld(std::uint32_t workerLimit)
{
    WorldConfig config;
    config.width = 53;
    config.height = 17;
    config.maxWorkerThreads = workerLimit;

    Field world(config, 0x123456789abcdef0ULL);
    world.SpawnControlGroup();
    for (uint tick = 1; tick <= 25; ++tick)
    {
        world.tick(tick);
    }
    return world.CalculateStateHash();
}

bool RunSelfTests(std::string& report)
{
    std::ostringstream output;
    bool success = true;
    auto check = [&](bool condition, const char* name)
    {
        output << (condition ? "PASS " : "FAIL ") << name << '\n';
        success = success && condition;
    };

    WorldConfig invalid;
    invalid.width = WorldConfig::MaxAxisSize + 1;
    check(!invalid.Validate(), "reject invalid axis size");

    WorldConfig unavailable;
    unavailable.width = WorldConfig::MaxAxisSize;
    unavailable.height = WorldConfig::MaxAxisSize;
    check(!unavailable.Validate(), "reject world larger than available memory");

    {
        WorldConfig config;
        config.width = 1;
        config.height = 1;
        config.maxWorkerThreads = 4;
        Field world(config, 1);
        check(world.GetWidth() == 1 && world.GetHeight() == 1 && world.GetWorkerCount() == 1,
            "minimum world and work-volume worker limit");
    }

    {
        WorldConfig standard;
        standard.maxWorkerThreads = 2;
        Field world(standard, 7);
        check(world.GetWidth() == WorldConfig::DefaultWidth && world.GetHeight() == WorldConfig::DefaultHeight,
            "standard dynamic world");
    }

    {
        WorldConfig large;
        large.width = 2049;
        large.height = 257;
        large.maxWorkerThreads = 4;
        Field world(large, 8);
        check(world.GetWidth() == 2049 && world.GetHeight() == 257, "large non-divisible dynamic world");
    }

    {
        WorldConfig config;
        config.width = 7;
        config.height = 3;
        config.maxWorkerThreads = 4;
        Field world(config, 2);
        MoveTestObject* first = new MoveTestObject(0, 1, {1, 1});
        MoveTestObject* second = new MoveTestObject(2, 1, {1, 1});
        world.AddObject(first);
        world.AddObject(second);
        world.tick(1);
        check(world.GetObjectLocalCoords(1, 1) == first && world.GetObjectLocalCoords(2, 1) == second,
            "stable first-wins move conflict");
    }

    {
        WorldConfig config;
        config.width = 5;
        config.height = 2;
        config.maxWorkerThreads = 2;
        Field world(config, 3);
        MoveTestObject* mover = new MoveTestObject(4, 0, {world.ValidateX(5), 0});
        world.AddObject(mover);
        world.tick(1);
        check(world.GetObjectLocalCoords(0, 0) == mover, "horizontal boundary movement");
    }

    {
        WorldConfig config;
        config.width = 4;
        config.height = 2;
        config.maxWorkerThreads = 2;
        Field world(config, 4);
        AttackTestBot* first = new AttackTestBot(0, 0, {1, 0});
        AttackTestBot* second = new AttackTestBot(1, 0, {0, 0});
        world.AddObject(first);
        world.AddObject(second);
        world.tick(1);
        check(world.GetObjectLocalCoords(0, 0) == first && world.GetObjectLocalCoords(1, 0) == NULL,
            "stable mutual attack");
    }

    {
        WorldConfig config;
        config.width = 4;
        config.height = 2;
        config.maxWorkerThreads = 2;
        Field world(config, 5);
        BirthTestBot* first = new BirthTestBot(0, 0, {1, 0});
        BirthTestBot* second = new BirthTestBot(2, 0, {1, 0});
        world.AddObject(first);
        world.AddObject(second);
        world.tick(1);
        check(world.GetObjectTypeAt(1, 0) == rock && world.ValidateObjectExistance(first) &&
            world.ValidateObjectExistance(second), "stable first-wins birth conflict");
    }

    {
        WorldConfig config;
        config.width = 4;
        config.height = 2;
        config.maxWorkerThreads = 2;
        Field world(config, 6);
        DeleteTestObject* remover = new DeleteTestObject(0, 0, {1, 0});
        MoveTestObject* mover = new MoveTestObject(1, 0, {2, 0});
        world.AddObject(remover);
        world.AddObject(mover);
        world.tick(1);
        check(world.GetObjectLocalCoords(1, 0) == NULL && world.GetObjectLocalCoords(2, 0) == NULL,
            "delete invalidates later movement");
    }

    const std::uint64_t oneWorker = RunDeterministicWorld(1);
    const std::uint64_t twoWorkers = RunDeterministicWorld(2);
    const std::uint64_t fourWorkers = RunDeterministicWorld(4);
    const std::uint64_t automaticWorkers = RunDeterministicWorld(0);
    output << "HASH 1=" << oneWorker << " 2=" << twoWorkers << " 4=" << fourWorkers << " auto=" << automaticWorkers << '\n';
    check(oneWorker == twoWorkers && oneWorker == fourWorkers && oneWorker == automaticWorkers,
        "deterministic state across 1/2/4/auto workers");

    {
        WorldConfig config;
        config.width = 31;
        config.height = 11;
        config.maxWorkerThreads = 3;
        Field source(config, 0xfeedbeefULL);
        source.AddObject(new Rock(30, 10));
        source.AddObject(new Organics(4, 2, 77));
        source.tick(1);
        const std::uint64_t expectedHash = source.CalculateStateHash();

        std::filesystem::create_directories("SavedObjects/CyberBiology3");
        std::filesystem::path path = "SavedObjects/CyberBiology3/__cb3_self_test_world.tmp";
        std::string filename = path.string();
        ObjectSaver saver;
        bool saved = saver.SaveWorld(&source, filename.data(), 42, 1);

        ObjectSaver::WorldParams header;
        bool headerRead = saver.ReadWorldHeader(filename.data(), header);
        WorldConfig loadedConfig;
        loadedConfig.width = static_cast<std::uint32_t>(header.width);
        loadedConfig.height = static_cast<std::uint32_t>(header.height);
        loadedConfig.maxWorkerThreads = static_cast<std::uint32_t>(header.maxWorkerThreads);
        Field loaded(loadedConfig, header.seed);
        ObjectSaver::WorldParams result = saver.LoadWorld(&loaded, filename.data());
        check(saved && headerRead && result.id == 42 && loaded.CalculateStateHash() == expectedHash,
            "save header creates and restores dynamic world");
        std::filesystem::remove(path);
    }

    for (int i = 0; i < 8; ++i)
    {
        WorldConfig config;
        config.width = 19 + i;
        config.height = 7;
        config.maxWorkerThreads = 4;
        Field world(config, i + 10);
        world.tick(1);
    }
    check(true, "worker pool repeated construction and destruction");

    report = output.str();
    return success;
}

}
