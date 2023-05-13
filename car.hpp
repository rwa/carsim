
#include <SDL.h>

#include "linesensor.hpp"
#include "distsensor.hpp"
#include "maze.hpp"

#define FOLLOW_LINE 0
#define MAKING_LEFT_TURN 1
#define MAKING_RIGHT_TURN 2
#define MOVING_UP 3
#define CHECKING_LEFT 4
#define CHECKING_FRONT 5

#define MOVE_UP_TIME 0.75

struct Car {

  LineSensor lsen;
  LineSensor msen;
  LineSensor rsen;

  DistSensor distsen;

  Ray lastray;

  double w = 40;
  double l = 45;

  bool front_wall_detected = false;
  bool left_wall_detected = false;
  bool going_straight = false;
  int next_turn = 0; // -1 next turn left, +1 next turn right

  // moving up is based on a timer, start timer when initiating moving up
  uint32_t moving_up_start_ticks = 0;
  
  Car(SDL_Renderer* renderer) {

    x = 0.5*CELL_SIZE;
    y = 4.5*CELL_SIZE;

    // These coordinates are relative to car center
    lsen.x = -0.15*w;
    lsen.y = -l/2.0;
    lsen.car = this;
    
    msen.x = 0;
    msen.y = -l/2.0;
    msen.car = this;

    rsen.x = +0.15*w;
    rsen.y = -l/2.0;
    rsen.car = this;

    distsen.car = this;
    
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

  double readdistancesensor(Maze& maze)
  {
    int x = distsen.getWorldX(theta);
    int y = distsen.getWorldY(theta);
    double sensor_theta = distsen.getWorldTheta();

    double dx = cos(sensor_theta);
    double dy = sin(sensor_theta);

    Point p(0,0);
    double dist = maze.distanceToClosestWall(x, y, dx, dy, p);

    lastray.origin.x = x;
    lastray.origin.y = y;
    lastray.direction.x = p.x;
    lastray.direction.y = p.y;
    
    return dist;
  }

  void forward_clear_corner(int& mode)
  {
    (void) mode;
    
  }

  void turn_left(int& mode, Maze& maze)
  {
    (void) mode;
    double turn_speed = M_PI/200;
    theta -= turn_speed;

    bool l,m,r;
    readlinesensors(maze, l, m, r);
    if (!l && m && r) {
      mode = FOLLOW_LINE;
    }
    
  }

  void turn_right(int& mode, Maze& maze)
  {
    (void) mode;
    double turn_speed = M_PI/200;
    theta += turn_speed;

    bool l,m,r;
    readlinesensors(maze, l, m, r);
    if (l && m && !r) {
      mode = FOLLOW_LINE;
    }
  }
  
  void follow_line(int& mode, Maze& maze)
  {
    (void) mode;
    
    double steer_amount = 0.005*M_PI;
    
    bool l,m,r;
    readlinesensors(maze, l, m, r);
    printf("l=%d, m=%d, r=%d\n",l,m,r);

    going_straight = false;
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
      printf("001: steer right\n");
      theta += steer_amount;
    }
    if (!l && m && !r) {
      printf("010: forward\n");
      forward();
      going_straight = true;
    }
    if (l && m && r) {
      printf("111: forward?\n");
      forward();
    }
    if (!l && !m && !r) {
      printf("000: forward?\n");
      forward();
    }
    forward();
  }

  void move_up(int& mode, Maze& maze)
  {
    uint32_t cur_ticks = SDL_GetTicks();
    double elapsed_moving_up =  (cur_ticks - moving_up_start_ticks) / 1000.0; // Convert to secs

    if (elapsed_moving_up > MOVE_UP_TIME) {
      printf("MOVE UP TIMEOUT\n");
      if (next_turn == -1) mode = MAKING_LEFT_TURN;
      if (next_turn == +1) mode = MAKING_RIGHT_TURN;
      return;
    }
    
    forward();

    (void) maze;
    // bool l,m,r;
    // readlinesensors(maze, l, m, r);
    // if (!l && !m && !r) {
    //   if (next_turn == -1) mode = MAKING_LEFT_TURN;
    //   if (next_turn == +1) mode = MAKING_RIGHT_TURN;
    // }
  }

  void check_left(int& mode, Maze& maze, double elapsed_time)
  {
    if (!going_straight) return;
    
    (void) elapsed_time;
    printf("checking left\n");

    distsen.rot = -1;
    double dist = readdistancesensor(maze);
    printf("left dist = %f\n",dist);
    if (dist < 60) {
      left_wall_detected = true;
    }
    else {
      left_wall_detected = false;
    }

    printf("left wall detected? %d\n",left_wall_detected);
    if (left_wall_detected) {

      printf(" -- front wall detected? %d\n",front_wall_detected);
      if (front_wall_detected) {
	front_wall_detected = false;
	printf(" -- change mode to right turn\n");
	mode = MOVING_UP;
	moving_up_start_ticks = SDL_GetTicks();
	next_turn = +1;
	return;
      }
      else {
	printf(" -- change mode to follow line\n");
	mode = FOLLOW_LINE;
	return;
      }
    }
    else {
      mode = MOVING_UP;
      moving_up_start_ticks = SDL_GetTicks();
      next_turn = -1;
      return;
    }
  }

  void check_front(int& mode, Maze& maze, double elapsed_time)
  {
    if (!going_straight) return;

    printf("checking front\n");
    distsen.rot = 0;
    double dist = readdistancesensor(maze);
    printf("dist = %f\n",dist);

    if (dist < 60) {
      front_wall_detected = true;
    }
    else {
      front_wall_detected = false;
    }
      
    (void) maze;
    (void) mode;
    (void) elapsed_time;
  }
  
  void control(Maze& maze, double elapsed_time)
  {
    (void) elapsed_time;
    
    static int mode = FOLLOW_LINE;

    switch (mode) {
      
    case FOLLOW_LINE:
      printf("mode is follow line %d\n",mode);
      follow_line(mode, maze);
      check_left(mode, maze, elapsed_time);
      check_front(mode, maze, elapsed_time);
      break;
    case MAKING_LEFT_TURN:
      printf("mode is making left turn %d\n",mode);
      turn_left(mode, maze);
      break;
    case MAKING_RIGHT_TURN:
      printf("mode is making right turn %d\n",mode);
      turn_right(mode, maze);
      break;
    case MOVING_UP:
      printf("mode is moving up %d\n",mode);
      move_up(mode, maze);
      break;
    case CHECKING_LEFT:
      printf("mode is checking left %d\n",mode);
      check_left(mode, maze, elapsed_time);
      break;
    case CHECKING_FRONT:
      printf("mode is checking front %d\n",mode);
      check_front(mode, maze, elapsed_time);
      break;
    }
    
    // if (check_front) {
    //   if (distsen.rot == -1) distsen.rot = 0;
    // }
    // else {

    //   // check for left wall
    //   if (distsen.rot != -1) distsen.rot = -1;
    //   double dist = readdistancesensor(maze);

    //   dist < 75 ? left_wall_detected = true : left_wall_detected = false;

    //   if (!left_wall_detected) {
    // 	forward_clear_corner();
    // 	turn_left();
    //   }
      
      
    // double dist = readdistancesensor(maze);
    // printf("dist = %f\n",dist);

    // if (elapsed_time > 3.0) {
    //   distsen.rot = 0;
    // }
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
    // Do this outside of here so that we can check the rotation calc
    // SDL_SetRenderDrawColor(softwareRenderer, 255, 0, 0, 255); // red
    // SDL_RenderDrawPoint(softwareRenderer, w/2.0 +lsen.x, l/2.0 +lsen.y);

    // SDL_SetRenderDrawColor(softwareRenderer, 0, 255, 0, 255); // green
    // SDL_RenderDrawPoint(softwareRenderer, w/2.0 +msen.x, l/2.0 +msen.y);

    // SDL_SetRenderDrawColor(softwareRenderer, 0, 0, 255, 255); // blue
    // SDL_RenderDrawPoint(softwareRenderer, w/2.0 +rsen.x, l/2.0 +rsen.y);
    
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

    // Draw the location of the ultrasonic sensor
    int x = distsen.getWorldX(theta);
    int y = distsen.getWorldY(theta);
    SDL_SetRenderDrawColor(renderer, 100, 40, 100, 255); // color
    SDL_RenderDrawPoint(renderer, x, y);
    SDL_RenderDrawPoint(renderer, x-1, y);
    SDL_RenderDrawPoint(renderer, x+1, y);
    SDL_RenderDrawPoint(renderer, x, y-1);
    SDL_RenderDrawPoint(renderer, x, y+1);
    

    // Render the ray from ultrasonic sensor to wall
    SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255); // color
    Ray& r = lastray;
    SDL_RenderDrawLine(renderer, r.origin.x, r.origin.y, r.direction.x, r.direction.y);

    // Draw green cross on end of ray
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // green
    SDL_RenderDrawPoint(renderer, r.direction.x, r.direction.y);
    SDL_RenderDrawPoint(renderer, r.direction.x-1, r.direction.y);
    SDL_RenderDrawPoint(renderer, r.direction.x+1, r.direction.y);
    SDL_RenderDrawPoint(renderer, r.direction.x, r.direction.y-1);
    SDL_RenderDrawPoint(renderer, r.direction.x, r.direction.y+1);
    
  }

  ~Car() {
    SDL_DestroyTexture(texture);
  }
};
