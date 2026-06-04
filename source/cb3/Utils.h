#pragma once

#include "Systems.h"


namespace cb3
{

typedef std::uint32_t uint;
typedef std::uint8_t byte;

using std::thread;
using std::mutex;
using std::atomic_flag;

using std::string;
using std::vector;

using std::min;
using std::to_string;


//Platform dependent
#define ErrorMessage(msg, caption) MessageBoxW(NULL, msg, caption, MB_ICONERROR | MB_OK);


struct Color
{
	short c[3];
	char change_vector[3];

	void operator/=(int);
	void operator+=(Color);

	void Set(int r, int g, int b);
	void SetRandom();
	void RandomChange(const int strMin, const int strMax);
	void NormalizeChangeVector();


	static Color GetRandomColor();
};


struct Point
{
	int x, y;

	Point(int X, int Y);
	Point();

	void Shift(int X, int Y);	
	void Set(int X, int Y);

	Point operator+(Point toAdd);	
};

struct Rect final: public SDL_Rect
{
	bool IsInBounds(Point p);	
};


std::uint64_t MixRandomSeed(std::uint64_t value);
void SetRandomState(std::uint64_t state);
std::uint64_t GetRandomState();
void SetDeterministicRandom(std::uint64_t seed, std::uint64_t tick, std::uint64_t stream);
std::uint32_t RandomNext();

//Get random number (0 to max-1)
int RandomVal(std::uint64_t max);
int RandomValRange(int min, int max);
float RandomFloatInRange(float min, float max);

//Roll (chance in percents)
bool RandomPercent(int val);

//Roll (chance in 1/10 percent)
bool RandomPercentX10(int val);

//Simple loop
#define repeat(times) for(int i=0;i<times;++i)




//High precision tick counter
typedef std::chrono::steady_clock Clock;
typedef Clock::time_point TimePoint;
typedef std::chrono::duration<int, std::milli> Duration;

#define TimeMSBetween(t2, t1) duration_cast<Duration>(t2 - t1).count() 


}
