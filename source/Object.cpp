#include "Bot.h"

#include <algorithm>
#include <cmath>


uint Object::currentFrame = 0;
Field* Object::static_pField = NULL;
Object*** Object::static_pCells = NULL;

const Rect Object::image_rect = { 0, 0, FieldCellSize, FieldCellSize };



Object::Object(int X, int Y) :x(X), y(Y), type(abstract)
{
	energy = 0;

	//Set up pointers on field class and cells array
	pField = static_pField;
	pCells = (Object* (*)[FieldCellsWidth][FieldCellsHeight])static_pCells;
}


void Object::CalcScreenX()
{
	screenX = x - Field::renderX;

	if (screenX < 0)
	{
		screenX += FieldCellsWidth;
	}
}

void Object::CalcObjectRect()
{
	double scale = Field::GetViewScale();
	int left = FieldX + (int)std::floor(screenX * FieldCellSize * scale - Field::viewX);
	int top = FieldY + (int)std::floor(y * FieldCellSize * scale - Field::viewY);
	int right = FieldX + (int)std::ceil((screenX + 1) * FieldCellSize * scale - Field::viewX);
	int bottom = FieldY + (int)std::ceil((y + 1) * FieldCellSize * scale - Field::viewY);

	object_rect = { left, top, std::max(1, right - left), std::max(1, bottom - top) };
}

void Object::CalcObjectRectShrinked(int shrink)
{
	CalcObjectRect();

	int scaledShrink = (int)std::round(shrink * Field::GetViewScale());
	scaledShrink = std::min(scaledShrink, (object_rect.w - 1) / 2);
	scaledShrink = std::min(scaledShrink, (object_rect.h - 1) / 2);

	object_rect.x += scaledShrink;
	object_rect.y += scaledShrink;
	object_rect.w -= 2 * scaledShrink;
	object_rect.h -= 2 * scaledShrink;
}

void Object::draw()
{
	CalcScreenX();	
	CalcObjectRect();

	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	SDL_RenderFillRect(renderer, &object_rect);
}

void Object::drawEnergy() { draw(); };
void Object::drawPredators() { draw(); };

int Object::tick()
{
	//if already made a move on this turn
	if (currentFrame == lastTickFrame)
		return 2;

	++lifetime;
	lastTickFrame = currentFrame;

	return 0;
}


uint Object::GetLifetime()
{
	return lifetime;
}

void Object::SetLifetime(uint val)
{
	lifetime = val;
}

void Object::SetPointers(Field* field, Object*** cells)
{
	static_pField = field;
	static_pCells = cells;
}
