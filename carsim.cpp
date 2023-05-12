
// [X] Read maze walls and viz
// 2. Add pathlines to maze
// 3. Create car object and equip with drive, line sensors, and ultrasonic sensor
  
#include <SDL.h>
#include <SDL_ttf.h>

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

  Cell(int i_, int j_) : i(i_), j(j_) {
    l = false;
    r = false;
    t = false;
    b = false;
  }
  
  int i,j;
  
  bool l;
  bool r;
  bool t;
  bool b;
};

struct Segment {
  Segment(double x0_, double y0_, double x1_, double y1_) :
    x0(x0_), y0(y0_), x1(x1_), y1(y1_) {}
  bool vertical;
  bool horizontal;
  double x0,y0;
  double x1,y1;
};

#define FONT_SIZE 24
#define CELL_SIZE 100
#define WALL_THICKNESS 2

class Maze {

public:
  int h,w;
  
  vector<Wall> hWalls;
  vector<Wall> vWalls;

  vector< vector<Cell> > cells;

  vector<Segment> pathsegs;

  // Take list of walls and fill out cell data structure
  void finalize() {
    
    for (int j = 0; j < h; j++) {
      vector<Cell> row;
      for (int i = 0; i < w; i++) {
	Cell c(i,j);
	row.push_back(c);
      }
      cells.push_back(row);
    }

    // Fill out cells data structure
    std::vector<Wall>::iterator it;
    for (auto it: hWalls) {
      if (it.j < h) {
	printf("marking %d,%d b\n",it.j,it.i);
	cells[it.j][it.i].t = true;
      }
      if (it.j > 0) {
	printf("marking %d,%d t\n",it.j-1,it.i);
	cells[it.j-1][it.i].b = true;
      }
    }
    for (auto it: vWalls) {
      if (it.i < w) {
	printf("marking %d,%d l\n",it.j,it.i);
	cells[it.j][it.i].l = true;
      }
      if (it.i > 0) {
	printf("marking %d,%d r\n",it.j,it.i-1);
	cells[it.j][it.i-1].r = true;
      }
    }

    // Create pathlines.
    // All pathlines extend from the center of a cell to any open direction.
    
    for (auto it: cells) {
      for (auto itr: it) {
	if (!itr.b) {
	  printf("adding pathseg\n");
	  Segment s((itr.i+0.5)*CELL_SIZE, (itr.j+0.5)*CELL_SIZE,
		    (itr.i+0.5)*CELL_SIZE, (itr.j+1)*CELL_SIZE);
	  s.vertical = true;
	  pathsegs.push_back(s);
	}
	if (!itr.t) {
	  printf("adding pathseg\n");
	  Segment s((itr.i+0.5)*CELL_SIZE, (itr.j+0.5)*CELL_SIZE,
		    (itr.i+0.5)*CELL_SIZE, itr.j*CELL_SIZE);
	  s.vertical = true;
	  pathsegs.push_back(s);
	}
	if (!itr.l) {
	  printf("adding pathseg\n");
	  Segment s((itr.i+0.5)*CELL_SIZE, (itr.j+0.5)*CELL_SIZE,
		    (itr.i+0.0)*CELL_SIZE, (itr.j+0.5)*CELL_SIZE);
	  s.horizontal = true;
	  pathsegs.push_back(s);
	}
	if (!itr.r) {
	  printf("adding pathseg\n");
	  Segment s((itr.i+0.5)*CELL_SIZE, (itr.j+0.5)*CELL_SIZE,
		    (itr.i+1.0)*CELL_SIZE, (itr.j+0.5)*CELL_SIZE);
	  s.horizontal = true;
	  pathsegs.push_back(s);
	}
      }
    }
    
  }

  void annotateCell(SDL_Renderer* renderer, TTF_Font* font, const Cell& cell, int x, int y) {
    
    SDL_Color textColor = {255, 255, 255, 255};  // White color for text
    std::string wallText = (cell.t ? string("T") : string("")) +
      (cell.b ? string("B") : string("")) +
      (cell.l ? string("L") : string("")) +
      (cell.r ? string("R") : string(""));
    
    SDL_Surface* surface = TTF_RenderText_Solid(font, wallText.c_str(), textColor);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    
    int textWidth, textHeight;
    SDL_QueryTexture(texture, NULL, NULL, &textWidth, &textHeight);
    
    SDL_Rect dst = {x * CELL_SIZE + CELL_SIZE / 2 - textWidth / 2, y * CELL_SIZE + CELL_SIZE / 2 - textHeight / 2, textWidth, textHeight};
    SDL_RenderCopy(renderer, texture, NULL, &dst);
    
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
  }
  
  void draw(SDL_Renderer* renderer, TTF_Font* font) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE); // Set color to white
    
    for (const Wall& wall : hWalls) {
      SDL_Rect rect = { wall.i * CELL_SIZE, wall.j * CELL_SIZE, CELL_SIZE, WALL_THICKNESS };
      SDL_RenderFillRect(renderer, &rect);
    }
    
    for (const Wall& wall : vWalls) {
      SDL_Rect rect = { wall.i * CELL_SIZE, wall.j * CELL_SIZE, WALL_THICKNESS, CELL_SIZE };
      SDL_RenderFillRect(renderer, &rect);
    }

    // Annotate each cell
    // for (int j = 0; j < h; j++) {
    //     for (int i = 0; i < w; i++) {
    //         annotateCell(renderer, font, cells[j][i], i, j);
    //     }
    // }

    // Draw pathlines
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE); // Set color to red
    for (auto it: pathsegs) {
      int hw = 5;
      if (it.vertical) {
	SDL_Rect rect = {it.x0-hw, it.y0, hw*2.0, it.y1-it.y0};
	SDL_RenderFillRect(renderer, &rect);
      }
      if (it.horizontal) {
	SDL_Rect rect = {it.x0-hw, it.y0-hw, it.x1-it.x0+hw*2.0, hw*2.0};
	SDL_RenderFillRect(renderer, &rect);
      }
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

  int maxj = 0;
  for (auto it: maze.vWalls) {
    if (it.j > maxj) maxj = it.j;
  }

  int maxi = 0;
  for (auto it: maze.hWalls) {
    if (it.i > maxi) maxi = it.i;
  }

  // Flip in j so 0 is at the bottom
  // for (auto it: maze.hWalls) {
  //   it.j = maxj - it.j;
  // }
  // for (auto it: maze.vWalls) {
  //   it.j = maxj - it.j;
  // }

  maze.w = maxi+1;
  maze.h = maxj+1;

  printf("h,w = %d,%d\n",maze.h,maze.w);

  maze.finalize();
  
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
    maze.draw(renderer, font);
    
    // Update the window
    SDL_RenderPresent(renderer);
  }
  
  // Cleanup and quit
  TTF_CloseFont(font);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  
}
