
#include <SDL.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
using namespace std;

struct Wall {
    int i;
    int j;
    Wall(int i, int j) : i(i), j(j) {}
};

struct Cell {
  bool l;
  bool r;
  bool t;
  bool b;
};

#define CELL_SIZE 100
#define WALL_THICKNESS 2

class Maze {

public:
  std::vector<Wall> hWalls;
  std::vector<Wall> vWalls;

  std::vector< vector<Cell> > cells;

  // Take list of walls and fill out cell data structure
  void mapToCells() {

    // std::vector<Wall>::iterator it;
    // for (auto it: hWalls) {
    //   it->i
    // }
  }

  void draw(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE); // Set color to white
    
    for (const Wall& wall : hWalls) {
      SDL_Rect rect = { wall.i * CELL_SIZE, wall.j * CELL_SIZE, CELL_SIZE, WALL_THICKNESS };
      SDL_RenderFillRect(renderer, &rect);
    }
    
    for (const Wall& wall : vWalls) {
      SDL_Rect rect = { wall.i * CELL_SIZE, wall.j * CELL_SIZE, WALL_THICKNESS, CELL_SIZE };
      SDL_RenderFillRect(renderer, &rect);
    }
  }
  
  void print() {
    
  }
};

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
    for (int x = 0; x < line.size(); x++) {
      i = x/2;
      j = y/2;
      if (line[x] == '-') {
	printf("adding hwall\n");
	maze.hWalls.push_back(Wall(i, j));
      } else if (line[x] == '|') {
	printf("adding vwall\n");
	maze.vWalls.push_back(Wall(i, j));
      }
    }
    y++;
  }

  // Flip in j so 0 is at the bottom
  int maxj = 0;
  for (auto it: maze.vWalls) {
    if (it.j > maxj) maxj = it.j;
  }

  for (auto it: maze.hWalls) {
    it.j = maxj - it.j;
  }
  for (auto it: maze.vWalls) {
    it.j = maxj - it.j;
  }
  
  return maze;
}
  
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

int main()
{

  Maze maze = parseMaze();

  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "Could not initialize SDL: " << SDL_GetError() << std::endl;
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
  
  // Main loop
  bool running = true;
  while (running) {
    // Event handling
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
	running = false;
      }
    }
    
    // Clear the screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    // Draw
    maze.draw(renderer);
    
    // Update the window
    SDL_RenderPresent(renderer);
  }
  
  // Cleanup and quit
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  
}
