#include "Field.h"

#include <algorithm>
#include <cmath>


namespace cb3
{

int Field::seed;
int Field::renderX=0;
int Field::viewX = 0;
int Field::viewY = 0;
double Field::zoom = 1.0;

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

    viewport = { FieldX, FieldY, availableWidth, availableHeight };
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
    return (std::max)(1, (int)std::ceil(FieldWidth * GetViewScale()));
}

int Field::GetScaledFieldHeight()
{
    return (std::max)(1, (int)std::ceil(FieldHeight * GetViewScale()));
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


void Field::ChangeSeason()
{
    season = (Season)((int)season + 1);

    if (season > spring)
    {
        season = summer;
    }
}

void Field::SeasonTick()
{
    if (++changeSeasonCounter >= (uint)params.seasonInterval)
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
    return {spawnApplesCounter, season, changeSeasonCounter};
}

void Field::SetPersistentState(const PersistentState& state)
{
    spawnApplesCounter = state.spawnApplesCounter;
    season = state.season <= spring ? state.season : summer;
    changeSeasonCounter = state.changeSeasonCounter;
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
                if (obj->type() == bot)
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
                    if(allCells[tx][Y + cy]->type() == bot)
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
    Object** cell = &allCells[obj->x][obj->y];

    if (*cell)
        return false;

    *cell = obj;

    return true;
}

void Field::ObjectAddOrReplace(Object* obj)
{
    Object** cell = &allCells[obj->x][obj->y];

    if (*cell)
        RemoveObject(obj->x, obj->y);

    *cell = obj;
}

void Field::mutateWorld()
{
    for (int cx = 0; cx < FieldCellsWidth; ++cx)
    {
        for (int cy = 0; cy < FieldCellsHeight; ++cy)
        {
            Object* o = allCells[cx][cy];

            if(o)
            {
                if (o->type() == bot)
                {
                    ((Bot*)o)->Mutagen();
                }
            }
        }
    }
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

void Field::placeWall(uint width)
{
    Object* o = allCells[0][0];

    //If there is a wall
    if (o)
    {
        if (o->type() == rock)
        {
            repeat(FieldCellsHeight)
            {
                for (uint b = 0; b < width; ++b)
                {
                    o = allCells[b][i];

                    if (o)
                    {
                        if (o->type() == rock)
                        {
                            RemoveObject(b, i);
                        }
                    }
                }
            }

            return;
        }
    }

    //Otherwise create a new one
    repeat(FieldCellsHeight)
    {
        for(uint b = 0; b < width; ++b)
        {
            ObjectAddOrReplace(new Rock(b, i));
        }
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
                if (tmpObj->type() == bot)
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
        if (tmpObj->type() == bot)
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
    predators = 0;
    lifetime = 0;
}


inline void Field::tick_single_thread()
{
    Object* tmpObj;

    objectsTotal = 0;
    botsTotal = 0;
    applesTotal = 0;
    organicsTotal = 0;
    predatorsTotal = 0;
    averageLifetime = 0;

    for (uint ix = 0; ix < FieldCellsWidth; ++ix)
    {
        for (uint iy = 0; iy < FieldCellsHeight; ++iy)
        {
            tmpObj = allCells[ix][iy];

            if (tmpObj)
            {
                ++objectsTotal;

                if (tmpObj->type() == bot)
                {
                    ++botsTotal;

                    if (((Bot*)tmpObj)->isPredator())
                    {
                        ++predatorsTotal;
                    }

                    averageLifetime += tmpObj->GetLifetime();
                }
                else if (tmpObj->type() == apple)
                    ++applesTotal;
                else if (tmpObj->type() == organic_waste)
                    ++organicsTotal;

                ObjectTick(tmpObj);
            }
        }

    }

    if(botsTotal > 0)
        averageLifetime /= botsTotal;
}


inline bool Field::ThreadWait(const uint index)
{
    std::unique_lock<std::mutex> lock(threadMutex);
    threadStartCondition.wait(lock, [&]()
    {
        return threadGoMarker[index] || terminateThreads;
    });

    return !terminateThreads;
}


void Field::ProcessPart_MultipleThreads(const uint X1, const uint X2, const uint index)
{
    srand(seed + index);    

    auto obj_calc = [&](Object* tmpObj)
    {
        if (tmpObj == NULL)
            return;

        ++objectCounters[index].objects;

        if (tmpObj->type() == bot)
        {
            ++objectCounters[index].bots;

            if (((Bot*)tmpObj)->isPredator())
            {
                ++objectCounters[index].predators;
            }

            objectCounters[index].lifetime += tmpObj->GetLifetime();
        }
        else if (tmpObj->type() == apple)
            ++objectCounters[index].apples;
        else if (tmpObj->type() == organic_waste)
            ++objectCounters[index].organics;

        ObjectTick(tmpObj);
    };

    const uint middleX = X1 + ((X2 - X1) / 2);
    const uint passStart[2] = {X1, middleX};
    const uint passEnd[2] = {middleX, X2};

    for(;;)
    {
        for(uint pass = 0; pass < 2; ++pass)
        {
            if (!ThreadWait(index))
                return;

            //Calculate chunk
            for (uint X = passStart[pass]; X < passEnd[pass]; ++X)
            {
                for (uint Y = 0; Y < FieldCellsHeight; ++Y)
                {
                    obj_calc(allCells[X][Y]);
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
}

void Field::StartThreads()
{
    repeat(numThreads)
    {
        uint X1 = (FieldCellsWidth * (uint)i) / (uint)numThreads;
        uint X2 = (FieldCellsWidth * (uint)(i + 1)) / (uint)numThreads;

        threads[i] = std::thread(&Field::ProcessPart_MultipleThreads, this, X1, X2, i);
    }
}

void Field::SignalThreads()
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

void Field::WaitForThreads()
{
    std::unique_lock<std::mutex> lock(threadMutex);
    threadDoneCondition.wait(lock, [&]()
    {
        return (threadsReady >= numThreads) || terminateThreads;
    });
}


inline void Field::tick_multiple_threads()
{
    auto clearCounters = [&]()
    {
        repeat(numThreads)
        {
            objectCounters[i].Clear();
        }
    };

    auto addCounters = [&]()
    {
        repeat(numThreads)
        {
            objectsTotal += objectCounters[i].objects;
            botsTotal += objectCounters[i].bots;
            applesTotal += objectCounters[i].apples;
            organicsTotal += objectCounters[i].organics;
            predatorsTotal += objectCounters[i].predators;
            averageLifetime += objectCounters[i].lifetime;
        }
    };

    objectsTotal = 0;
    botsTotal = 0;
    applesTotal = 0;
    organicsTotal = 0;
    predatorsTotal = 0;
    averageLifetime = 0;

    //2 passes
    repeat(2)
    {
        //Clear object counters
        clearCounters();

        //Starting signal for all threads
        SignalThreads();

        //Wait for threads to synchronize
        WaitForThreads();

        //Add object counters
        addCounters();
    }

    if (botsTotal > 0)
        averageLifetime /= botsTotal;
}


void Field::tick(uint thisFrame)
{
    //Change season
    if (params.useSeasons)
        SeasonTick();

    //Memorize frame number
    Object::currentFrame = thisFrame;

    //Spawn apples
    if(params.spawnApples)
    {
        if (spawnApplesCounter++ == AppleSpawnInterval)
        {
            SpawnApples();

            spawnApplesCounter = 0;
        }
    }

    //Make simulation step
    if (numThreads == 1)
    {
        tick_single_thread();
    }
    else
    {
        tick_multiple_threads();
    }
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

    //Mud layer
    SDL_SetRenderDrawColor(renderer, MudColor);
    int mudHeight = (int)std::ceil(params.mudLevel * FieldCellSize * scale);
    SDL_Rect mud = { FieldX - viewX, FieldY - viewY + scaledFieldHeight - mudHeight, scaledFieldWidth, mudHeight };
    SDL_RenderFillRect(renderer, &mud);

    //Ocean
    SDL_SetRenderDrawColor(renderer, OceanColor);
    int oceanHeight = (int)std::ceil(params.oceanLevel * FieldCellSize * scale);
    SDL_Rect ocean = { FieldX - viewX, FieldY - viewY + scaledFieldHeight - oceanHeight, scaledFieldWidth, oceanHeight - mud.h };
    SDL_RenderFillRect(renderer, &ocean);

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

    //Underwater mask
    #ifdef DrawUnderwaterMask
    {
        SDL_SetRenderDrawColor(renderer, UnderwaterMaskColor);
        SDL_Rect underwater = { FieldX - viewX, FieldY - viewY + scaledFieldHeight - oceanHeight, scaledFieldWidth, oceanHeight };
        SDL_RenderFillRect(renderer, &underwater);
    }
    #endif

    SDL_RenderSetClipRect(renderer, NULL);

}

bool Field::IsInBounds(int X, int Y)
{
    return ((X >= 0) and (Y >= 0) and (X < FieldCellsWidth) and (Y < FieldCellsHeight));
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

int Field::FindDistanceX(int X1, int X2)
{
    uint minDistX1 = min(X1, (FieldCellsWidth - X1));
    uint minDistX2 = min(X2, (FieldCellsWidth - X2));
    uint crossDist = abs(X2 - X1);

    return min(crossDist, minDistX1 + minDistX2);
}

bool Field::IsInBoundsScreenCoords(int X, int Y)
{
    ClampViewOffset();

    SDL_Rect viewport = GetViewportRect();
    SDL_Rect fieldRect = { FieldX - viewX, FieldY - viewY, GetScaledFieldWidth(), GetScaledFieldHeight() };

    return ((X >= viewport.x) and (X < viewport.x + viewport.w) and (Y >= viewport.y) and (Y < viewport.y + viewport.h) and
        (X >= fieldRect.x) and (X < fieldRect.x + fieldRect.w) and (Y >= fieldRect.y) and (Y < fieldRect.y + fieldRect.h));
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

uint Field::GetNumPredators()
{
    return predatorsTotal;
}

uint Field::GetAverageLifetime()
{
    return averageLifetime;
}

uint Field::GetNumThreads()
{
    return (uint)numThreads;
}

void Field::SpawnControlGroup()
{
    for (int i = 0; i < ControlGroupSize; ++i)
    {
        Bot* tmpBot = new Bot(RandomVal(FieldCellsWidth), RandomVal(FieldCellsHeight), params.botMaxEnergy);

        if (!AddObject(tmpBot))
            delete tmpBot;
    }
}


void Field::SpawnApples()
{
    const int landHeight = std::clamp(FieldCellsHeight - params.oceanLevel, 0, FieldCellsHeight);
    if (landHeight == 0)
        return;

    constexpr uint ChanceScale = 1000;
    const uint landCells = FieldCellsWidth * (uint)landHeight;
    const uint chance = std::clamp((uint)SpawnAppleInCellChance, 0u, ChanceScale);
    const uint64_t weightedAttempts = (uint64_t)landCells * chance;
    uint attempts = (uint)(weightedAttempts / ChanceScale);
    const uint remainder = (uint)(weightedAttempts % ChanceScale);

    if (remainder > 0 && (uint)RandomVal(ChanceScale) < remainder)
        ++attempts;

    for (uint i = 0; i < attempts; ++i)
    {
        const uint x = (uint)RandomVal(FieldCellsWidth);
        const uint y = (uint)RandomVal(landHeight);

        if (allCells[x][y] == NULL)
        {
            AddObject(new Apple(x, y));
        }
    }
}


Field::Field()
{
    const int cpuCount = SDL_GetCPUCount();
    numThreads = std::clamp(cpuCount, 1, NumThreads);
    numThreads = (std::min)(numThreads, FieldCellsWidth / 2);

    //Clear array
    memset(allCells, 0, sizeof(Point*) * FieldCellsWidth * FieldCellsHeight);    

    if (numThreads > 1)
    {
        StartThreads();
    }

    Object::SetPointers(this, (Object***)allCells);

}

Field::~Field()
{
    if (numThreads > 1)
    {
        {
            std::lock_guard<std::mutex> lock(threadMutex);
            terminateThreads = true;

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
    }

    RemoveAllObjects();
}

void FieldDynamicParams::Reset()
{
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

    memset(reserved, 0, sizeof(reserved));
}

FieldDynamicParams::FieldDynamicParams()
{
    Reset();
}


}
