
#include <SDL.h>

#include "linesensor.hpp"
#include "maze.hpp"

struct Car {

  LineSensor lsen;
  LineSensor msen;
  LineSensor rsen;

  Car(SDL_Renderer* renderer) {
    x = 0.5*CELL_SIZE;
    y = 3.5*CELL_SIZE;

    // These coordinates are relative to car center
    int size = 6;
    lsen.x = -0.2*w;
    lsen.y = -l/2.0;
    lsen.size = size;
    lsen.car = this;
    
    msen.x = 0;
    msen.y = -l/2.0;
    msen.size = size;
    msen.car = this;

    rsen.x = +0.2*w;
    rsen.y = -l/2.0;
    rsen.size = size;
    rsen.car = this;
    
    createTexture(renderer);
  }

  void forward()
  {
    double u = 1.0;
    double dt = 0.1;
    
    x += u*sin(theta)*dt;
    y += u*cos(theta)*dt;
  }

  void readlinesensors(Maze& maze, bool& l, bool& m, bool& r)
  {
    l = maze.detectPath(lsen.getWorldX(), lsen.getWorldY());
    m = maze.detectPath(msen.getWorldX(), msen.getWorldY());
    r = maze.detectPath(rsen.getWorldX(), rsen.getWorldY());
  }

  void control(Maze& maze)
  {
    bool l,m,r;
    l = false;
    m = false;
    r = false;
    readlinesensors(maze, l, m, r);
    printf("l=%d, m=%d, r=%d\n",l,m,r);
    
    forward();
  }

  double w = 40;
  double l = 65;
  
  double x,y;

  double r = static_cast<double>(rand()) / static_cast<double>(RAND_MAX);

  double theta_rand = 0.0;
  // double theta_rand = 0.1*r*M_PI;

  //printf("theta_rand = %f\n",theta_rand);
  
  double theta = 0 +theta_rand;

  // locations of line sensors relative to center x,y
  double ss = 6; // sensor size
  double dxl = -0.2*w;
  double dxm = 0.0;
  double dxr = +0.2*w;

  double dyl = -l/2.0;
  double dym = -l/2.0;
  double dyr = -l/2.0;

  SDL_Texture* texture;

  void createTexture(SDL_Renderer* renderer) {

    // Create a surface of the desired size
    int width = w;
    int height = l;
    SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
    
    // Fill the surface with a color (white in this case)
    SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 155, 155, 255));
    
    // Draw the arrow
    SDL_Renderer* softwareRenderer = SDL_CreateSoftwareRenderer(surface);
    SDL_SetRenderDrawColor(softwareRenderer, 0, 0, 0, 255); // black color
    SDL_RenderDrawLine(softwareRenderer, width / 2, 10, 10, height - 10); // left edge
    SDL_RenderDrawLine(softwareRenderer, width / 2, 10, width - 10, height - 10); // right edge
    SDL_RenderDrawLine(softwareRenderer, 10, height - 10, width - 10, height - 10); // base

    // Draw the sensor locations as rgb dots
    SDL_SetRenderDrawColor(softwareRenderer, 255, 0, 0, 255); // red
    SDL_RenderDrawPoint(softwareRenderer, w/2.0 +lsen.x, l/2.0 +lsen.y);

    SDL_SetRenderDrawColor(softwareRenderer, 0, 255, 0, 255); // green
    SDL_RenderDrawPoint(softwareRenderer, w/2.0 +msen.x, l/2.0 +msen.y);

    SDL_SetRenderDrawColor(softwareRenderer, 0, 0, 255, 255); // blue
    SDL_RenderDrawPoint(softwareRenderer, w/2.0 +rsen.x, l/2.0 +rsen.y);
    
    // Update the surface with what's been drawn
    SDL_RenderPresent(softwareRenderer);
    
    // Convert the surface to a texture
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    
    // Free the resources
    SDL_DestroyRenderer(softwareRenderer);
    SDL_FreeSurface(surface);
  }

  void draw(SDL_Renderer* renderer) {

    // Draw the texture at x,y as the center, rotated by theta degrees
    SDL_Rect destRect;
    destRect.x = x-w/2.0;
    destRect.y = y-l/2.0;
    destRect.w = w;
    destRect.h = l;
    SDL_RenderCopyEx(renderer, texture, NULL, &destRect, theta*180./M_PI, NULL, SDL_FLIP_NONE);
  }

  ~Car() {
    SDL_DestroyTexture(texture);
  }
};
