
#include "Field.h"

#include <algorithm>
#include <cmath>


int Field::seed;
int Field::renderX=0;
int Field::viewX = 0;
int Field::viewY = 0;
double Field::zoom = 1.0;

Season season;

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

static void GetFieldViewportLayout(SDL_Rect& viewport, bool& needHorizontal, bool& needVertical)
{
    int availableWidth = GetSidePanelXForField() - FieldX - InterfaceBorder;
    int availableHeight = windowHeight - FieldY;

    if (availableWidth < 1)
    {
        availableWidth = 1;
    }

    if (availableHeight < 1)
    {
        availableHeight = 1;
    }

    needHorizontal = false;
    needVertical = false;

    int scaledFieldWidth = Field::GetScaledFieldWidth();
    int scaledFieldHeight = Field::GetScaledFieldHeight();

    for (int i = 0; i < 2; ++i)
    {
        bool horizontal = scaledFieldWidth > (availableWidth - (needVertical ? FieldScrollbarSize : 0));
        bool vertical = scaledFieldHeight > (availableHeight - (horizontal ? FieldScrollbarSize : 0));

        needHorizontal = horizontal;
        needVertical = vertical;
    }

    if (needVertical)
    {
        availableWidth -= FieldScrollbarSize;
    }

    if (needHorizontal)
    {
        availableHeight -= FieldScrollbarSize;
    }

    if (availableWidth < 1)
    {
        availableWidth = 1;
    }

    if (availableHeight < 1)
    {
        availableHeight = 1;
    }

    viewport = {
        FieldX,
        FieldY,
        availableWidth,
        availableHeight
    };
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

static double GetFitScale()
{
    int availableWidth = GetSidePanelXForField() - FieldX - InterfaceBorder;
    int availableHeight = windowHeight - FieldY;

    if (availableWidth < 1)
    {
        availableWidth = 1;
    }

    if (availableHeight < 1)
    {
        availableHeight = 1;
    }

    double scaleX = availableWidth / (FieldWidth * 1.0);
    double scaleY = availableHeight / (FieldHeight * 1.0);
    double scale = (scaleX < scaleY) ? scaleX : scaleY;

    return (scale > 0.05) ? scale : 0.05;
}

double Field::GetViewScale()
{
    return GetFitScale() * ClampFieldZoom(zoom);
}

int Field::GetScaledFieldWidth()
{
    return std::max(1, (int)std::ceil(FieldWidth * GetViewScale()));
}

int Field::GetScaledFieldHeight()
{
    return std::max(1, (int)std::ceil(FieldHeight * GetViewScale()));
}

void Field::ClampViewOffset()
{
    int maxX = GetMaxViewX();
    int maxY = GetMaxViewY();

    if (viewX < 0)
    {
        viewX = 0;
    }
    else if (viewX > maxX)
    {
        viewX = maxX;
    }

    if (viewY < 0)
    {
        viewY = 0;
    }
    else if (viewY > maxY)
    {
        viewY = maxY;
    }
}

SDL_Rect Field::GetViewportRect()
{
    SDL_Rect viewport;
    bool needHorizontal;
    bool needVertical;

    GetFieldViewportLayout(viewport, needHorizontal, needVertical);

    return viewport;
}

int Field::GetMaxViewX()
{
    SDL_Rect viewport;
    bool needHorizontal;
    bool needVertical;

    GetFieldViewportLayout(viewport, needHorizontal, needVertical);

    int scaledFieldWidth = GetScaledFieldWidth();

    return (scaledFieldWidth > viewport.w) ? scaledFieldWidth - viewport.w : 0;
}

int Field::GetMaxViewY()
{
    SDL_Rect viewport;
    bool needHorizontal;
    bool needVertical;

    GetFieldViewportLayout(viewport, needHorizontal, needVertical);

    int scaledFieldHeight = GetScaledFieldHeight();

    return (scaledFieldHeight > viewport.h) ? scaledFieldHeight - viewport.h : 0;
}

bool Field::NeedHorizontalScrollbar()
{
    SDL_Rect viewport;
    bool needHorizontal;
    bool needVertical;

    GetFieldViewportLayout(viewport, needHorizontal, needVertical);

    return needHorizontal;
}

bool Field::NeedVerticalScrollbar()
{
    SDL_Rect viewport;
    bool needHorizontal;
    bool needVertical;

    GetFieldViewportLayout(viewport, needHorizontal, needVertical);

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

    if (wheelDelta > 0)
    {
        zoom *= factor;
    }
    else
    {
        zoom /= factor;
    }

    zoom = ClampFieldZoom(zoom);

    double newScale = GetViewScale();
    viewX = (int)std::round(fieldX * newScale - (X - viewport.x));
    viewY = (int)std::round(fieldY * newScale - (Y - viewport.y));

    ClampViewOffset();
}


void Field::shiftRenderPoint(int cx)
{
    renderX += cx;

    if (renderX < 0)
    {
        renderX = FieldCellsWidth - 1;
    }
    else if (renderX >= FieldCellsWidth)
    {
        renderX = 0;
    }
}

void Field::jumpToFirstBot()
{
    Object* obj;

    for (int X = 0; X < FieldCellsWidth; ++X)
    {
        for (int Y = 0; Y < FieldCellsHeight; ++Y)
        {
            obj = allCells[X][Y];

            if (obj)
            {
                if (obj->type == bot)
                {
                    renderX = X;

                    return;
                }
            }
        }
    }
}


Point Field::FindFreeNeighbourCell(int X, int Y)
{
    //If this cell is empty
    if (allCells[X][Y] == NULL)
    {
        return { X,Y };
    }

    //Form an array of nearby free cells

    int tx;
    Point tmpArray[9];
    int i = 0;

    for (int cx = -1; cx < 2; ++cx)
    {
        for (int cy = -1; cy < 2; ++cy)
        {
            tx = ValidateX(X + cx);

            if (IsInBounds(tx, Y + cy))
            {
                if (allCells[tx][Y + cy] == NULL)
                {
                    tmpArray[i++].Set(tx, Y + cy);
                }
            }
        }
    }

    //Get random free cell from array
    if (i > 0)
    {
        return tmpArray[RandomVal(i)];
    }

    //No free cells nearby
    return { -1, -1 };
}

Point Field::FindRandomNeighbourBot(int X, int Y)
{
    int tx;
    Point tmpArray[9];
    int i = 0;

    for (int cx = -1; cx < 2; ++cx)
    {
        for (int cy = -1; cy < 2; ++cy)
        {
            tx = ValidateX(X + cx);

            if (IsInBounds(tx, Y + cy))
            {
                if (allCells[tx][Y + cy] != NULL)
                {
                    if(allCells[tx][Y + cy]->type == bot)
                        tmpArray[i++].Set(tx, Y + cy);
                }
            }
        }
    }

    //Get random bot from array
    if (i > 0)
    {
        return tmpArray[RandomVal(i)];
    }

    //No free cells nearby
    return { -1, -1 };
}

int Field::FindHowManyFreeCellsAround(int X, int Y)
{
    int toRet = 0;

    //If cell itself is empty
    if (allCells[X][Y] == NULL)
    {
        ++toRet;
    }

    //Parse all cells
    int tx;

    for (int cx = -1; cx < 2; ++cx)
    {
        for (int cy = -1; cy < 2; ++cy)
        {

            tx = ValidateX(X + cx);

            if (IsInBounds(tx, Y + cy))
            {
                if (allCells[tx][Y + cy] == NULL)
                {
                    ++toRet;
                }
            }
        }
    }

    return toRet;
}


int Field::MoveObject(int fromX, int fromY, int toX, int toY)
{
    if (!IsInBounds(toX, toY))
        return -2;

    if (allCells[toX][toY])
        return -1;

    Object* tmpObj = allCells[fromX][fromY];

    if (tmpObj)
    {
        allCells[toX][toY] = tmpObj;
        allCells[fromX][fromY] = NULL;

        tmpObj->x = toX;
        tmpObj->y = toY;

        return 0;
    }

    return -3;
}

int Field::MoveObject(Object* obj, int toX, int toY)
{
    return MoveObject(obj->x, obj->y, toX, toY);
}

bool Field::AddObject(Object* obj)
{
    if (allCells[obj->x][obj->y])
        return false;

    allCells[obj->x][obj->y] = obj;

    return true;
}

void Field::RemoveObject(int X, int Y)
{
    Object* tmpO = allCells[X][Y];

    if (tmpO)
    {
        if (trackedObject == tmpO)
            trackedObjectRemoved = true;

        allCells[X][Y] = NULL;
        delete tmpO;
    }
}

void Field::RemoveAllObjects()
{
    for (int cx = 0; cx < FieldCellsWidth; ++cx)
    {
        for (int cy = 0; cy < FieldCellsHeight; ++cy)
        {
            RemoveObject(cx, cy);
        }
    }
}

void Field::RemoveBot(int X, int Y, int energyVal)
{
    RemoveObject(X, Y);

    if(RandomPercentX10(params.adaptation_organicSpawnRate))
    {
        if (energyVal > 0)
            AddObject(new Organics(X, Y, energyVal));
    }
}

void Field::RepaintBot(Bot* b, Color newColor, int differs)
{
    Object* tmpObj;

    for (uint ix = 0; ix < FieldCellsWidth; ++ix)
    {
        for (uint iy = 0; iy < FieldCellsHeight; ++iy)
        {
            tmpObj = allCells[ix][iy];

            if (tmpObj)
            {
                if (tmpObj->type == bot)
                {
                    if (((Bot*)tmpObj)->FindKinship(b) >= (NumberOfMutationMarkers - differs))
                    {
                        ((Bot*)tmpObj)->SetColor(newColor);
                    }
                }
            }
        }

    }
}


void Field::ObjectTick(Object* tmpObj)
{
    int t = tmpObj->tick();

    if (t == 1)
    {
        //Object destroyed
        if (tmpObj->type == bot)
            RemoveBot(tmpObj->x, tmpObj->y, tmpObj->energy);
        else
            RemoveObject(tmpObj->x, tmpObj->y);

        return;
    }
}

void ThreadCounters::Clear()
{
    objects = 0;
    bots = 0;
    apples = 0;
    organics = 0;
}

//tick function for single threaded build
inline void Field::tick_single_thread()
{
    Object* tmpObj;

    objectsTotal = 0;
    botsTotal = 0;
    applesTotal = 0;
    organicsTotal = 0;

    for (uint ix = 0; ix < FieldCellsWidth; ++ix)
    {
        for (uint iy = 0; iy < FieldCellsHeight; ++iy)
        {
            tmpObj = allCells[ix][iy];

            if (tmpObj)
            {
                ++objectsTotal;

                if (tmpObj->type == bot)
                    ++botsTotal;
                else if (tmpObj->type == apple)
                    ++applesTotal;
                else if (tmpObj->type == organic_waste)
                    ++organicsTotal;

                ObjectTick(tmpObj);
            }
        }

    }
}

//Wait for a signal 
inline bool Field::ThreadWait(const uint index)
{
    std::unique_lock<std::mutex> lock(threadMutex);
    threadStartCondition.wait(lock, [&]()
    {
        return threadGoMarker[index] || terminateThreads;
    });

    return !terminateThreads;
}

//Process function for multithreaded simulation
void Field::ProcessPart_MultipleThreads(const uint firstX1, const uint firstX2, const uint secondX1, const uint secondX2, const uint index)
{

    srand(seed + index);

    auto obj_cals = [&](Object* tmpObj)
    {
        if (tmpObj == NULL)
            return;

        ++counters[index].objects;

        if (tmpObj->type == bot)
            ++counters[index].bots;
        else if (tmpObj->type == apple)
            ++counters[index].apples;
        else if (tmpObj->type == organic_waste)
            ++counters[index].organics;

        ObjectTick(tmpObj);
    };

    for(;;)
    {
        
        if (!ThreadWait(index))
            return;

        for (uint X = firstX1; X < firstX2; ++X)
        {
            for (uint Y = 0; Y < FieldCellsHeight; ++Y)
            {
                obj_cals(allCells[X][Y]);
            }
        }

        {
            std::lock_guard<std::mutex> lock(threadMutex);
            threadGoMarker[index] = false;
            ++threadsReady;
        }
        threadDoneCondition.notify_one();

        if (!ThreadWait(index))
            return;

        for (uint X = secondX1; X < secondX2; ++X)
        {
            for (uint Y = 0; Y < FieldCellsHeight; ++Y)
            {
                obj_cals(allCells[X][Y]);
            }
        }

        {
            std::lock_guard<std::mutex> lock(threadMutex);
            threadGoMarker[index] = false;
            ++threadsReady;
        }
        threadDoneCondition.notify_one();
    }

}

//Start all threads
void Field::StartThreads()
{
    {
        std::lock_guard<std::mutex> lock(threadMutex);
        threadsReady = 0;

        repeat(numThreads)
        {
            threadGoMarker[i] = true;
        }
    }

    threadStartCondition.notify_all();
}

//Wait for all threads to finish their calculations
void Field::WaitForThreads()
{
    std::unique_lock<std::mutex> lock(threadMutex);
    threadDoneCondition.wait(lock, [&]()
    {
        return (threadsReady >= numThreads) || terminateThreads;
    });
}

//Multithreaded tick function
inline void Field::tick_multiple_threads()
{
    auto clear_counters = [&]()
    {
        repeat(numThreads)
        {
            counters[i].Clear();
        }
    };

    objectsTotal = 0;
    botsTotal = 0;
    applesTotal = 0;
    organicsTotal = 0;

    auto addToCounters = [&]()
    {
        repeat(numThreads)
        {
            objectsTotal += counters[i].objects;
            botsTotal += counters[i].bots;
            applesTotal += counters[i].apples;
            organicsTotal += counters[i].organics;
        }
    };

    //Clear object counters
    clear_counters();

    //Starting signal for all threads
    StartThreads();

    //Wait for threads to synchronize first time
    WaitForThreads();

    //Add object counters
    addToCounters();

    //Clear object counters
    clear_counters();

    //Starting signal for all threads
    StartThreads();

    //Wait for threads to synchronize second time
    WaitForThreads();

    //Add object counters
    addToCounters();

}

//Tick function
void Field::tick(uint thisFrame)
{
    Object::currentFrame = thisFrame;

    if(params.spawnApples)
    {
        if (spawnApplesInterval++ == AppleSpawnInterval)
        {
            SpawnApples();

            spawnApplesInterval = 0;
        }
    }

    #ifdef UseOneThread
        tick_single_thread();
    #else
        tick_multiple_threads();
    #endif
}

Field::PersistentState Field::GetPersistentState() const
{
    return {spawnApplesInterval};
}

void Field::SetPersistentState(const PersistentState& state)
{
    spawnApplesInterval = state.spawnApplesInterval;
}



void Field::draw(RenderTypes render)
{
    ClampViewOffset();

    SDL_Rect viewport = GetViewportRect();
    int scaledFieldWidth = GetScaledFieldWidth();
    int scaledFieldHeight = GetScaledFieldHeight();
    double scale = GetViewScale();
    SDL_Rect fieldRect = { FieldX - viewX, FieldY - viewY, scaledFieldWidth, scaledFieldHeight };

    SDL_RenderSetClipRect(renderer, &viewport);

    //Clear the whole viewport before drawing the scaled field.
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &viewport);

    //Background
    SDL_SetRenderDrawColor(renderer, FieldBackgroundColor);
    SDL_RenderFillRect(renderer, &fieldRect);
    
    //Ocean
#ifdef DrawOcean
    SDL_SetRenderDrawColor(renderer, OceanColor);
    int oceanHeight = (int)std::ceil(params.oceanLevel * FieldCellSize * scale);
    SDL_Rect ocean = { FieldX - viewX, FieldY - viewY + scaledFieldHeight - oceanHeight, scaledFieldWidth, oceanHeight };
    SDL_RenderFillRect(renderer, &ocean);
#endif

    //Mud layer
#ifdef DrawMudLayer
    SDL_SetRenderDrawColor(renderer, MudColor);
    int mudHeight = (int)std::ceil(params.mudLevel * FieldCellSize * scale);
    SDL_Rect mud = { FieldX - viewX, FieldY - viewY + scaledFieldHeight - mudHeight, scaledFieldWidth, mudHeight };
    SDL_RenderFillRect(renderer, &mud);
#endif

    //Objects
    Object* tmpObj;
    int ix = renderX;

    for (uint i = 0; i < FieldRenderCellsWidth; ++i)
    {
        for (uint iy = 0; iy < FieldCellsHeight; ++iy)
        {

            if (ix >= FieldCellsWidth)
                ix -= FieldCellsWidth;

            tmpObj = allCells[ix][iy];

            if (tmpObj)
            {
                //Draw function switch, based on selected render type
                switch (render)
                {
                case natural:
                    tmpObj->draw();
                    break;
                case predators:
                    tmpObj->drawPredators();
                    break;
                case energy:
                    tmpObj->drawEnergy();
                    break;
                }
            }
        }

        ++ix;
    }

    SDL_RenderSetClipRect(renderer, NULL);
}

//Is cell out if bounds?
bool Field::IsInBounds(int X, int Y)
{
    return ((X >= 0) && (Y >= 0) && (X < FieldCellsWidth) && (Y < FieldCellsHeight));
}

bool Field::IsInBounds(Point p)
{
    return IsInBounds(p.x, p.y);
}

bool Field::IsInWater(int Y)
{
    return (Y >= (FieldCellsHeight - params.oceanLevel));
}

bool Field::IsInMud(int Y)
{
    return (Y >= (FieldCellsHeight - params.mudLevel));
}



int Field::ValidateX(int X)
{
    if (X < 0)
    {
        return X + FieldCellsWidth;
    }
    else if (X >= FieldCellsWidth)
    {
        return (X - FieldCellsWidth);
    }

    return X;
}


bool Field::IsInBoundsScreenCoords(int X, int Y)
{
    ClampViewOffset();

    SDL_Rect viewport = GetViewportRect();
    SDL_Rect fieldRect = { FieldX - viewX, FieldY - viewY, GetScaledFieldWidth(), GetScaledFieldHeight() };

    return ((X >= viewport.x) && (X < viewport.x + viewport.w) && (Y >= viewport.y) && (Y < viewport.y + viewport.h) &&
        (X >= fieldRect.x) && (X < fieldRect.x + fieldRect.w) && (Y >= fieldRect.y) && (Y < fieldRect.y + fieldRect.h));
}


Point Field::ScreenCoordsToLocal(int X, int Y)
{
    ClampViewOffset();

    SDL_Rect viewport = GetViewportRect();
    double scale = GetViewScale();

    X -= viewport.x;
    Y -= viewport.y;

    X += viewX;
    Y += viewY;

    X = (int)std::floor((X / scale) / FieldCellSize);
    Y = (int)std::floor((Y / scale) / FieldCellSize);

    X += renderX;

    X = ValidateX(X);

    return { X, Y };
}


Object* Field::GetObjectLocalCoords(int X, int Y)
{
    return allCells[X][Y];
}


void Field::TrackObject(Object* obj)
{
    trackedObjectRemoved = false;
    trackedObject = obj;
}

bool Field::ValidateObjectExistance(Object* obj)
{
    return obj && trackedObject == obj && !trackedObjectRemoved;
}


uint Field::GetNumObjects()
{
    return objectsTotal;
}

uint Field::GetNumBots()
{
    return botsTotal;
}

uint Field::GetNumApples()
{
    return applesTotal;
}

uint Field::GetNumOrganics()
{
    return organicsTotal;
}

uint Field::GetNumThreads()
{
#ifdef UseOneThread
    return 1;
#else
    return (uint)numThreads;
#endif
}




void Field::SpawnControlGroup()
{
    for (int i = 0; i < ControlGroupSize; ++i)
    {
        Bot* tmpBot = new Bot(RandomVal(FieldCellsWidth), RandomVal(FieldCellsHeight), MaxPossibleEnergyForABot);

        if (!AddObject(tmpBot))
            delete tmpBot;
    }
}


void Field::SpawnApples()
{
    Object* tmpObj;
    const int landHeight = std::clamp(FieldCellsHeight - params.oceanLevel, 0, FieldCellsHeight);

    for (uint ix = 0; ix < FieldCellsWidth; ++ix)
    {
        for (int iy = 0; iy < landHeight; ++iy)
        {

            tmpObj = allCells[ix][iy];

            if (tmpObj == NULL)
            {
                //Take a chance to spawn an apple
                if (RandomPercentX10(SpawnAppleInCellChance))
                {
                    AddObject(new Apple(ix, iy));
                }
            }
        }
    }
}

void Field::PauseThreads()
{
    pauseThreads = true;
}

void Field::UnpauseThreads()
{
    pauseThreads = false;
}


//Create field
Field::Field()
{
    int cpuCount = SDL_GetCPUCount();
    numThreads = (cpuCount > 0) ? cpuCount : 1;
    if ((MaxSimulationThreads > 0) && (numThreads > MaxSimulationThreads))
        numThreads = MaxSimulationThreads;
    if (numThreads > (FieldCellsWidth / 2))
        numThreads = FieldCellsWidth / 2;

    //Clear array
    memset(allCells, 0, sizeof(Point*) * FieldCellsWidth * FieldCellsHeight);

    //Spawn objects
    #ifdef SpawnControlGroupAtStart
        SpawnControlGroup();
    #endif

    #ifdef SpawnOneAtStart
        Bot* tmpBot = new Bot(80, 60, MaxPossibleEnergyForABot);

        AddObject(tmpBot);
    #endif

    //Start threads
#ifndef UseOneThread
    threadGoMarker = std::vector<abool>(numThreads);
    counters = std::vector<ThreadCounters>(numThreads);
    threads.reserve(numThreads);

    repeat(numThreads)
    {
        threadGoMarker[i] = false;

        uint firstChunk = (uint)i * 2;
        uint secondChunk = firstChunk + 1;
        uint totalChunks = (uint)numThreads * 2;

        uint firstX1 = (FieldCellsWidth * firstChunk) / totalChunks;
        uint firstX2 = (FieldCellsWidth * (firstChunk + 1)) / totalChunks;
        uint secondX1 = (FieldCellsWidth * secondChunk) / totalChunks;
        uint secondX2 = (FieldCellsWidth * (secondChunk + 1)) / totalChunks;

        threads.emplace_back(&Field::ProcessPart_MultipleThreads, this, firstX1, firstX2, secondX1, secondX2, i);
    }
#endif

    Object::SetPointers(this, (Object***)allCells);

}

Field::~Field()
{
#ifndef UseOneThread
    {
        std::lock_guard<std::mutex> lock(threadMutex);
        terminateThreads = true;
        pauseThreads = false;

        repeat(numThreads)
        {
            threadGoMarker[i] = true;
        }
    }
    threadStartCondition.notify_all();

    repeat(numThreads)
    {
        if (threads[i].joinable())
            threads[i].join();
    }
#endif

    RemoveAllObjects();
}

void FieldDynamicParams::Reset()
{
    oceanLevel = InitialOceanHeight;
    mudLevel = InitialMudLayerHeight;
    appleEnergy = DefaultAppleEnergy;

    adaptation_DeathChance_Winds = 0;
    adaptation_StepsNum_Winds = 2;

    adaptation_landBirthBlock = 0;
    adaptation_seaBirthBlock = 0;
    adaptation_PSInOceanBlock = 0;
    adaptation_PSInMudBlock = 0;
    adaptation_botShouldBeOnLandOnceToMultiply = 0;
    adaptation_botShouldDoPSOnLandOnceToMultiply = 0;
    adaptation_forceBotMovements = 0;

    adaptation_organicSpawnRate = 0;

    memset(reserved, 0, sizeof(reserved));
}

FieldDynamicParams::FieldDynamicParams()
{
    Reset();
}
