#ifndef __Maze_included
#define __Maze_included

#include <SDL_ttf.h>

#include <vector>
#include <string>
#include <optional>
#include <utility>
using namespace std;

#define FONT_SIZE 24
#define CELL_SIZE 100
#define WALL_THICKNESS 2

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

  Segment(int x0_, int y0_, int x1_, int y1_) :
    x0(x0_), y0(y0_), x1(x1_), y1(y1_) {

    id = id_count;
    id_count++;

    selected = false;
    
    vertical = false;
    horizontal = false;
    if (x0 == x1) vertical = true;
    if (y0 == y1) horizontal = true;

    if (vertical && horizontal) printf("problem!!\n");
    if (!vertical && !horizontal) printf("problem!!\n");
  }

  bool vertical;
  bool horizontal;

  bool selected;

  int x0,y0;
  int x1,y1;

  int hw = 5;

  int id = 0;
  static int id_count;

  void annotate(SDL_Renderer* renderer, TTF_Font* font) {

    SDL_Color textColor = {255, 255, 255, 255};  // White color for text
    std::string segText = to_string(id);
    
    SDL_Surface* surface = TTF_RenderText_Solid(font, segText.c_str(), textColor);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    
    int textWidth, textHeight;
    SDL_QueryTexture(texture, NULL, NULL, &textWidth, &textHeight);
    
    SDL_Rect dst = { (x0+x1)/2 - textWidth / 2, (y0+y1)/2 - textHeight / 2, textWidth, textHeight};
    SDL_RenderCopy(renderer, texture, NULL, &dst);
    
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
  }
  
  
  void draw(SDL_Renderer* renderer, TTF_Font* font) {

    if (vertical) {
      SDL_Rect rect = {x0-hw, y0, hw*2, y1-y0};
      SDL_SetRenderDrawColor(renderer, 255, 0, 0, 100); // Set color to red
      SDL_RenderFillRect(renderer, &rect);
      // if (selected) {
      // 	SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE); // Set color to white
      // 	SDL_RenderDrawRect(renderer, &rect);
      // }
    }
    if (horizontal) {
      SDL_Rect rect = {x0, y0-hw, x1-x0, hw*2};
      SDL_SetRenderDrawColor(renderer, 255, 0, 0, 100); // Set color to red
      SDL_RenderFillRect(renderer, &rect);
      // if (selected) {
      // 	SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE); // Set color to white
      // 	SDL_RenderDrawRect(renderer, &rect);
      // }
    }

    (void) font;
    //annotate(renderer, font);
  }
  
  bool intersects(int x, int y)
  {
    SDL_Point p;
    p.x = x;
    p.y = y;

    bool intersects = false;
    if (vertical) {
      SDL_Rect rect = {x0-hw, y0, hw*2, y1-y0};
      intersects = SDL_PointInRect(&p, &rect);
    }
    if (horizontal) {
      SDL_Rect rect = {x0, y0-hw, x1-x0, hw*2};
      intersects = SDL_PointInRect(&p, &rect);
    }

    if (intersects) {
      selected = true;
    }
    else {
      selected = false;
    }

    return intersects;
  }

  
};

struct Point {
  Point() {};
  Point(double x_, double y_) : x(x_), y(y_) {};
  double x;
  double y;
};

// Ray data structure.
struct Ray {
  Ray() {};
  Ray(Point o, Point dir) : origin(o), direction(dir) {};
  Point origin, direction;
};

// LineSeg data structure.
struct LineSeg {

  LineSeg(Point& p0, Point& p1) : start(p0), end(p1) {};
  Point start, end;
};

//std::optional<Point> RayLineSegIntersect(const Ray& ray, const LineSeg& segment);
std::optional<Point> getIntersection(const Ray& ray, const LineSeg& segment);

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
	cells[it.j][it.i].t = true;
      }
      if (it.j > 0) {
	cells[it.j-1][it.i].b = true;
      }
    }
    for (auto it: vWalls) {
      if (it.i < w) {
	cells[it.j][it.i].l = true;
      }
      if (it.i > 0) {
	cells[it.j][it.i-1].r = true;
      }
    }

    // Create pathlines.
    // All pathlines extend from the center of a cell to any open direction.
    
    for (auto it: cells) {
      for (auto itr: it) {
	if (!itr.b) {
	  Segment s((itr.i+0.5)*CELL_SIZE, (itr.j+0.5)*CELL_SIZE,
		    (itr.i+0.5)*CELL_SIZE, (itr.j+1)*CELL_SIZE);
	  pathsegs.push_back(s);
	}
	if (!itr.t) {
	  Segment s((itr.i+0.5)*CELL_SIZE, itr.j*CELL_SIZE,
		    (itr.i+0.5)*CELL_SIZE, (itr.j+0.5)*CELL_SIZE);
	  pathsegs.push_back(s);
	}
	if (!itr.l) {
	  Segment s((itr.i+0.0)*CELL_SIZE, (itr.j+0.5)*CELL_SIZE,
		    (itr.i+0.5)*CELL_SIZE, (itr.j+0.5)*CELL_SIZE);
	  pathsegs.push_back(s);
	}
	if (!itr.r) {
	  Segment s((itr.i+0.5)*CELL_SIZE, (itr.j+0.5)*CELL_SIZE,
		    (itr.i+1.0)*CELL_SIZE, (itr.j+0.5)*CELL_SIZE);
	  pathsegs.push_back(s);
	}
      }
    }
    
  }

  void addPath(double x0, double y0, double x1, double y1) {

    Segment s(x0,y0,x1,y1);
    pathsegs.push_back(s);
    
  }

  void annotateCell(SDL_Renderer* renderer, TTF_Font* font, const Cell& cell, int x, int y) {

    return;
    
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
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            annotateCell(renderer, font, cells[j][i], i, j);
        }
    }

    // Draw pathlines
    for (auto it: pathsegs) {
      it.draw(renderer, font);
      
      // int hw = 5;
      // if (it.vertical) {
      // 	SDL_Rect rect = {it.x0-hw, it.y0, hw*2, it.y1-it.y0};
      // 	SDL_RenderFillRect(renderer, &rect);
      // }
      // if (it.horizontal) {
      // 	SDL_Rect rect = {it.x0-hw, it.y0-hw, it.x1-it.x0+hw*2, hw*2};
      // 	SDL_RenderFillRect(renderer, &rect);
      // }
    }
  }

  bool detectPath(int x, int y)
  {
    bool intersection = false;
    for (auto& it: pathsegs) {
      if (it.intersects(x,y)) intersection = true;
    }
    return intersection;
  }

  double distanceToClosestWall(int x, int y, double dx, double dy, Point& minpt)
  {
    double mindist = 1e6;

    Point r0(x,y);
    Point r1(dx,dy);
    Ray r(r0,r1);
    
    // vertwalls
    for (auto& it: vWalls) {
      double x0 = it.i*CELL_SIZE;
      double x1 = x0;
      double y0 = it.j*CELL_SIZE;
      double y1 = y0+CELL_SIZE;

      Point p0(x0,y0);
      Point p1(x1,y1);
      LineSeg seg(p0,p1);

      std::optional<Point> pt = getIntersection(r, seg);
      if (pt) {

	double dx = pt->x -x;
	double dy = pt->y -y;
	double dist = sqrt(dx*dx +dy*dy);
	if (dist < mindist) {
	  mindist = dist;
	  minpt = *pt;
	}
      }
    }

    // horiz walls
    for (auto& it: hWalls) {
      double x0 = it.i*CELL_SIZE;
      double x1 = x0+CELL_SIZE;
      double y0 = it.j*CELL_SIZE;
      double y1 = y0;

      Point p0(x0,y0);
      Point p1(x1,y1);
      LineSeg seg(p0,p1);

      std::optional<Point> pt = getIntersection(r, seg);
      if (pt) {

	double dx = pt->x-x;
	double dy = pt->y-y;
	double dist = sqrt(dx*dx+dy*dy);
	if (dist < mindist) {
	  mindist = dist;
	  minpt = *pt;
	}
      }
    }
    
    return mindist;
  }
  
  void print() {
    
  }
};

#endif // included
