#include <stdio.h>
#include <SDL.h>
#include <SDL_image.h>
#include <iostream>

#define WIDTH 1280
#define HEIGHT 720

int main(int argc, char* argv[]) {

	// variable declarations
	SDL_Window* win = NULL;
	SDL_Renderer* renderer = NULL;
	SDL_Texture* img = NULL;

	// Initialize SDL.
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		return 1;

	// create the window and renderer
	// note that the renderer is accelerated
	win = SDL_CreateWindow("Image Loading", 0, 78, WIDTH, HEIGHT, 0);
	renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

	// load our image
	img = IMG_LoadTexture(renderer, "E:/AVR_programming_explore/VirtualLifeSimulator1/materials/image.jpg");
	if (img == nullptr) {
		std::cout << "IMG_LoadTexture Error: " << SDL_GetError() << "\n";
		return 1;
	}


	int w, h; // texture width & height
	SDL_QueryTexture(img, NULL, NULL, &w, &h); // get the width and height of the texture
	// put the location where we want the texture to be drawn into a rectangle
	// I'm also scaling the texture 2x simply by setting the width and height
	SDL_Rect texr; texr.x = 0; texr.y = 0; texr.w = w; texr.h = h;

	unsigned int lastUpdateTime = 0;

	// main loop
	while (1) {

		// event handling
		SDL_Event e;
		if (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT)
				break;
			else if (e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_ESCAPE)
				break;
		}

		// paint the image once every 30ms, i.e. 33 images per second
		if (lastUpdateTime + 30 < SDL_GetTicks()) {
			lastUpdateTime = SDL_GetTicks();

			// clear the screen
			SDL_RenderClear(renderer);
			// copy the texture to the rendering context
			SDL_RenderCopy(renderer, img, NULL, &texr);
			// flip the backbuffer
			// this means that everything that we prepared behind the screens is actually shown
			SDL_RenderPresent(renderer);
		}
	}

	SDL_DestroyTexture(img);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(win);

	return 0;
}