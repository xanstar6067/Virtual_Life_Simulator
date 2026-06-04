
#include "Utils.h"



namespace cb3
{

thread_local std::uint64_t randomState = 0x9e3779b97f4a7c15ULL;

std::uint64_t MixRandomSeed(std::uint64_t value)
{
	value += 0x9e3779b97f4a7c15ULL;
	value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
	value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
	return value ^ (value >> 31);
}

void SetRandomState(std::uint64_t state)
{
	randomState = state ? state : 0x9e3779b97f4a7c15ULL;
}

std::uint64_t GetRandomState()
{
	return randomState;
}

void SetDeterministicRandom(std::uint64_t seed, std::uint64_t tick, std::uint64_t stream)
{
	SetRandomState(MixRandomSeed(seed ^ MixRandomSeed(tick) ^ MixRandomSeed(stream)));
}

std::uint32_t RandomNext()
{
	randomState += 0x9e3779b97f4a7c15ULL;
	std::uint64_t value = randomState;
	value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
	value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
	value ^= value >> 31;
	return static_cast<std::uint32_t>(value >> 32);
}

int RandomVal(std::uint64_t max)
{
	if (max == 0)
	{
		return 0;
	}

	return static_cast<int>(RandomNext() % max);
}

bool RandomPercent(int val)
{
	return RandomVal(1000) >= (1000 - (val * 10));
}

bool RandomPercentX10(int val)
{
	return RandomVal(1000) >= (1000 - val);
}

void Color::operator/=(int d)
{
	repeat(3)
		c[i] /= d;
}

void Color::operator+=(Color C)
{
	unsigned short tmpVal;

	repeat(3)
	{
		tmpVal = c[i];

		tmpVal += C.c[i];

		if (tmpVal > 255)
			tmpVal = 255;

		c[i] = static_cast<byte>(tmpVal);
	}
}

void Color::SetRandom()
{
	#ifdef PresetRandomColors
	{
		uint i = RandomVal(21);

		c[0] = presetColors[i][0];
		c[1] = presetColors[i][1];
		c[2] = presetColors[i][2];
	}
	#else
	{
		c[0] = RandomVal(256);
		c[1] = RandomVal(256);
		c[2] = RandomVal(256);
	}
	#endif

	change_vector[0] = RandomVal(256);
	change_vector[1] = RandomVal(256);
	change_vector[2] = RandomVal(256);

	NormalizeChangeVector();
}

void Color::RandomChange(const int strMin, const int strMax)
{
	if (RandomPercent(5))
	{
		change_vector[0] += RandomVal(20);
		change_vector[1] += RandomVal(20);
		change_vector[2] += RandomVal(20);

		NormalizeChangeVector();
	}

	repeat(3)
	{
		short& s = c[i];

		s += change_vector[i];

		if (s < 0)
		{
			s *= -1;
			change_vector[i] *= -1;
		}
		else if(s > 255)
		{
			s -= (s - 255) * 2;
			change_vector[i] *= -1;
		}
	}
}

void Color::NormalizeChangeVector()
{
	float magnitude = sqrt(
		(change_vector[0] * change_vector[0]) + 
		(change_vector[1] * change_vector[1]) + 
		(change_vector[2] * change_vector[2])) 
		/ 18.0f; // A little correction
	 
	repeat(3)
	{
		change_vector[i] /= magnitude;
	}
}

void Color::Set(int r, int g, int b)
{
	c[0] = r;
	c[1] = g;
	c[2] = b;
}

Color Color::GetRandomColor()
{
	Color toRet;

	toRet.SetRandom();

	return toRet;
}

Point::Point(int X, int Y) :x(X), y(Y) 
{}

Point::Point() :x(-1), y(-1) 
{}

void Point::Shift(int X, int Y) 
{
	x += X;
	y += Y;
}

void Point::Set(int X, int Y) 
{
	x = X;
	y = Y;
}

Point Point::operator+(Point toAdd)
{
	return {x + toAdd.x, y + toAdd.y};
}

bool Rect::IsInBounds(Point p)
{
	return ((p.x >= x) && (p.y >= y) && (p.x <= x + w) && (p.y <= y + h));
}

int RandomValRange(int min, int max)
{
	if (max <= min)
	{
		return min;
	}

	return min + RandomVal(static_cast<std::uint64_t>(max - min));
}

float RandomFloatInRange(float min, float max)
{ 
	return min + (static_cast<float>(RandomNext()) / static_cast<float>(UINT32_MAX)) * (max - min);
}


}
