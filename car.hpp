
#include <SDL.h>

#include "linesensor.hpp"
#include "maze.hpp"

struct Car {

  LineSensor lsen;
  LineSensor msen;
  LineSensor rsen;

  double w = 40;
  double l = 45;
  
  Car(SDL_Renderer* renderer) {
    x = 0.5*CELL_SIZE;
    y = 3.5*CELL_SIZE;

    // These coordinates are relative to car center
    lsen.x = -0.3*w;
    lsen.y = -l/2.0;
    lsen.car = this;
    
    msen.x = 0;
    msen.y = -l/2.0;
    msen.car = this;

    rsen.x = +0.3*w;
    rsen.y = -l/2.0;
    rsen.car = this;
    
    createTexture(renderer);
  }

  void forward()
  {
    double u = 1.0;
    double dt = 0.2;
    
    x += u*sin(theta)*dt;
    y -= u*cos(theta)*dt;
  }

  void readlinesensors(Maze& maze, bool& l, bool& m, bool& r)
  {
    l = maze.detectPath(lsen.getWorldX(theta), lsen.getWorldY(theta));
    m = maze.detectPath(msen.getWorldX(theta), msen.getWorldY(theta));
    r = maze.detectPath(rsen.getWorldX(theta), rsen.getWorldY(theta));
  }

  void control(Maze& maze)
  {
    double steer_amount = 0.001*M_PI;
    
    bool l,m,r;
    readlinesensors(maze, l, m, r);
    printf("l=%d, m=%d, r=%d\n",l,m,r);
    
    forward();
    printf("theta = %f\n",theta);
    if (l && m && !r)  {
      printf("110: steer left\n");
      theta -= steer_amount;
    }
    if (l && !m && !r) {
      printf("100: steer left\n");
      theta -= steer_amount;
    }
    if (!l && m && r)  {
      printf("011: steer right\n");
      theta += steer_amount;
    }
    if (!l && !m && r) {
      printf("001: steer hard right\n");
      theta += steer_amount;
    }
    printf("theta now %f\n",theta);
  }

  double x,y;

  double r = static_cast<double>(rand()) / static_cast<double>(RAND_MAX);

  double theta_rand = 0.1*r*M_PI;
  double theta = 0 +theta_rand;

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
    SDL_SetRenderDrawColor(softwareRenderer, 100, 30, 100, 100); // black color
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

    // Draw the sensors independently of the rotation so that we can
    // see if our world coordinates are right.
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // red
    SDL_RenderDrawPoint(renderer, lsen.getWorldX(theta), lsen.getWorldY(theta));
    SDL_RenderDrawPoint(renderer, lsen.getWorldX(theta)+1, lsen.getWorldY(theta));
    SDL_RenderDrawPoint(renderer, lsen.getWorldX(theta), lsen.getWorldY(theta)+1);
    SDL_RenderDrawPoint(renderer, lsen.getWorldX(theta)-1, lsen.getWorldY(theta));
    SDL_RenderDrawPoint(renderer, lsen.getWorldX(theta), lsen.getWorldY(theta)-1);

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // green
    SDL_RenderDrawPoint(renderer, msen.getWorldX(theta), msen.getWorldY(theta));
    SDL_RenderDrawPoint(renderer, msen.getWorldX(theta)+1, msen.getWorldY(theta));
    SDL_RenderDrawPoint(renderer, msen.getWorldX(theta), msen.getWorldY(theta)+1);
    SDL_RenderDrawPoint(renderer, msen.getWorldX(theta)-1, msen.getWorldY(theta));
    SDL_RenderDrawPoint(renderer, msen.getWorldX(theta), msen.getWorldY(theta)-1);

    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // green
    SDL_RenderDrawPoint(renderer, rsen.getWorldX(theta), rsen.getWorldY(theta));
    SDL_RenderDrawPoint(renderer, rsen.getWorldX(theta)+1, rsen.getWorldY(theta));
    SDL_RenderDrawPoint(renderer, rsen.getWorldX(theta), rsen.getWorldY(theta)+1);
    SDL_RenderDrawPoint(renderer, rsen.getWorldX(theta)-1, rsen.getWorldY(theta));
    SDL_RenderDrawPoint(renderer, rsen.getWorldX(theta), rsen.getWorldY(theta)-1);
    
    // SDL_SetRenderDrawColor(softwareRenderer, 0, 255, 0, 255); // green
    // SDL_RenderDrawPoint(softwareRenderer, w/2.0 +msen.x, l/2.0 +msen.y);

    // SDL_SetRenderDrawColor(softwareRenderer, 0, 0, 255, 255); // blue
    // SDL_RenderDrawPoint(softwareRenderer, w/2.0 +rsen.x, l/2.0 +rsen.y);
    
  }

  ~Car() {
    SDL_DestroyTexture(texture);
  }
};
