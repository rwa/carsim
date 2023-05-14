
// [X] Read maze walls and viz
// [X] Add pathlines to maze
// 3. Create car object and equip with drive, line sensors, and ultrasonic sensor
  
#include <SDL.h>
#include <SDL_ttf.h>

#include <cstdlib>
#include <ctime>

#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
using namespace std;

#include "linesensor.hpp"
#include "car.hpp"

const int FPS = 60; // The desired frame rate
const int frameDelay = 1000 / FPS; // Time for one frame in milliseconds

uint32_t startTime;

Uint32 frameStart;
int frameTime;

double elapsed_time = 0;

bool isPaused = false;

Maze parseMaze()
{
  std::ifstream file("maze.txt");
  Maze maze;
  std::string line;
  int y = 0;

  // x,y are coords in the text file.
  // i,j are coords in maze logical space.
  int i = 0;
  int j = 0;
  while (std::getline(file, line)) {
    for (unsigned long x = 0; x < line.size(); x++) {
      i = x/2;
      j = y/2;
      if (line[x] == '-') {
	maze.hWalls.push_back(Wall(i, j));
      } else if (line[x] == '|') {
	maze.vWalls.push_back(Wall(i, j));
      }
    }
    y++;
  }

  int maxj = 0;
  for (auto it: maze.vWalls) {
    if (it.j > maxj) maxj = it.j;
  }

  int maxi = 0;
  for (auto it: maze.hWalls) {
    if (it.i > maxi) maxi = it.i;
  }

  maze.w = maxi+1;
  maze.h = maxj+1;

  maze.finalize();
  
  return maze;
}
  
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

int main()
{
  std::srand(std::time(0)); // Use current time as seed for random generator
  
  Maze maze = parseMaze();

  //maze.addPath(0,200,100,300);
  
  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "Could not initialize SDL: " << SDL_GetError() << std::endl;
    return 1;
  }

  // Initialize SDL_ttf
  if (TTF_Init() == -1) {
    std::cerr << "Could not initialize SDL_ttf: " << TTF_GetError() << std::endl;
    SDL_Quit();
    return 1;
  }
  
  // Create a window
  SDL_Window* window = SDL_CreateWindow("Car Sim", 
					SDL_WINDOWPOS_UNDEFINED, 
					SDL_WINDOWPOS_UNDEFINED, 
					WINDOW_WIDTH, 
					WINDOW_HEIGHT, 
					SDL_WINDOW_SHOWN);
  if (window == nullptr) {
    std::cerr << "Could not create window: " << SDL_GetError() << std::endl;
    SDL_Quit();
    return 1;
  }
  
  // Create a renderer
  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (renderer == nullptr) {
    std::cerr << "Could not create renderer: " << SDL_GetError() << std::endl;
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", FONT_SIZE);
  if (font == nullptr) {
    std::cerr << "Could not load font: " << TTF_GetError() << std::endl;
    // Clean up and quit
    // ...
  }

  Car car(renderer);

  startTime = SDL_GetTicks();
  
  // Main loop
  bool running = true;
  while (running) {

    frameStart = SDL_GetTicks();

    // Event handling
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
	running = false;
      }
      else if (event.type == SDL_KEYDOWN) {
	switch (event.key.keysym.sym) {
	case SDLK_SPACE:  // Press 'P' to pause/unpause the game
	  isPaused = !isPaused;
	  break;
	}
      }
    }

    if (isPaused) continue;
    
    // Clear the screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    // Draw
    maze.draw(renderer, font);
    car.draw(renderer, font, elapsed_time);

    // annotate(renderer, font);
    
    // Update the window
    SDL_RenderPresent(renderer);

    uint32_t currTime = SDL_GetTicks();
    elapsed_time = (currTime - startTime) / 1000.0; // Convert to seconds.
    
    // Control the car
    car.control(maze, elapsed_time);

    frameTime = SDL_GetTicks() - frameStart;

    if (frameDelay > frameTime) {
        SDL_Delay(frameDelay - frameTime);
    }
    
  }
  
  // Cleanup and quit
  TTF_CloseFont(font);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  
}
