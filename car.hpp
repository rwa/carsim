
#include <SDL.h>

#include <string>
using namespace std;

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

  int mode;

  bool front_wall_detected = false;
  bool left_wall_detected = false;
  bool going_straight = true;
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

    mode = FOLLOW_LINE;
    
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

  void forward_clear_corner()
  {
    (void) mode;
    
  }

  void turn_left(Maze& maze)
  {
    (void) mode;
    double turn_speed = M_PI/200;
    theta -= turn_speed;

    bool l,m,r;
    readlinesensors(maze, l, m, r);
    if (!l && m && r) {
      printf("exit left turn - back to follow line\n");
      mode = FOLLOW_LINE;
    }
    
  }

  void turn_right(Maze& maze)
  {
    (void) mode;
    double turn_speed = M_PI/200;
    theta += turn_speed;

    bool l,m,r;
    readlinesensors(maze, l, m, r);
    if (l && m && !r) {
      printf("exit right turn - back to follow line\n");
      mode = FOLLOW_LINE;
    }
  }
  
  void follow_line(Maze& maze)
  {
    (void) mode;
    
    double steer_amount = 0.005*M_PI;
    
    bool l,m,r;
    readlinesensors(maze, l, m, r);

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

  void move_up(Maze& maze)
  {
    uint32_t cur_ticks = SDL_GetTicks();
    double elapsed_moving_up =  (cur_ticks - moving_up_start_ticks) / 1000.0; // Convert to secs
    
    if (elapsed_moving_up > MOVE_UP_TIME) {
      printf("exit moving up - entering next_turn %d\n",next_turn);
      if (next_turn == -1) mode = MAKING_LEFT_TURN;
      if (next_turn == +1) mode = MAKING_RIGHT_TURN;
      return;
    }
    
    forward();

    (void) maze;
  }

  // void check_left(Maze& maze, double elapsed_time)
  // {
  //   if (!going_straight) return;
    
  //   (void) elapsed_time;
  //   printf("checking left\n");

  //   distsen.rot = -1;
  //   double dist = readdistancesensor(maze);
  //   printf("left dist = %f\n",dist);
  //   if (dist < 60) {
  //     left_wall_detected = true;
  //   }
  //   else {
  //     left_wall_detected = false;
  //   }

  //   printf("left wall detected? %d\n",left_wall_detected);
  //   if (left_wall_detected) {

  //     printf(" -- front wall detected? %d\n",front_wall_detected);
  //     if (front_wall_detected) {
  // 	front_wall_detected = false;
  // 	printf(" -- change mode to right turn\n");
  // 	mode = MOVING_UP;
  // 	moving_up_start_ticks = SDL_GetTicks();
  // 	next_turn = +1;
  // 	return;
  //     }
  //     else {
  // 	printf(" -- change mode to follow line\n");
  // 	mode = FOLLOW_LINE;
  // 	return;
  //     }
  //   }
  //   else {
  //     mode = MOVING_UP;
  //     moving_up_start_ticks = SDL_GetTicks();
  //     next_turn = -1;
  //     return;
  //   }
  // }

  // void check_front(Maze& maze, double elapsed_time)
  // {
  //   if (!going_straight) return;

  //   printf("checking front\n");
  //   distsen.rot = 0;
  //   double dist = readdistancesensor(maze);
  //   printf("dist = %f\n",dist);

  //   if (dist < 60) {
  //     front_wall_detected = true;
  //   }
  //   else {
  //     front_wall_detected = false;
  //   }
      
  //   (void) maze;
  //   (void) mode;
  //   (void) elapsed_time;
  // }

  void sense_walls(Maze& maze)
  {
    static bool sense_left = true;

    // only take measurements when straight
    if (!going_straight) return;
    
    if (sense_left) {

      bool facing_left = distsen.faceLeft();

      if (facing_left) {

	double dist = readdistancesensor(maze);
	printf("left dist: %f\n",dist);
	
	left_wall_detected = false;
	if (dist < 60) {
	  left_wall_detected = true;
	}

	sense_left = false;
      }
    }

    // sense front
    if (!sense_left) {
      
      bool facing_front = distsen.faceFront();
      
      if (facing_front) {
	double dist = readdistancesensor(maze);
	printf("front dist: %f\n",dist);
	
	front_wall_detected = false;
	if (dist < 60) {
	  front_wall_detected = true;
	}
	
	sense_left = true;
      }
    }
  }
  
  void control(Maze& maze, double elapsed_time)
  {
    (void) elapsed_time;

    // update front_wall_detected and left_wall_detected
    sense_walls(maze);

    // update mode based on wall sensing
    if (left_wall_detected) {
      if (front_wall_detected) {
	if (mode == FOLLOW_LINE) {
	  mode = MOVING_UP;
	  moving_up_start_ticks = SDL_GetTicks();
	  next_turn = +1;
	}
      }
      else {
  	mode = FOLLOW_LINE;
      }
    }
    else {
      if (mode == FOLLOW_LINE) {
	mode = MOVING_UP;
	moving_up_start_ticks = SDL_GetTicks();
	next_turn = -1;
      }
    }
    
    switch (mode) {
      
    case FOLLOW_LINE:
      printf("mode is follow line %d\n",mode);
      follow_line(maze);
      break;
    case MAKING_LEFT_TURN:
      printf("mode is making left turn %d\n",mode);
      turn_left(maze);
      break;
    case MAKING_RIGHT_TURN:
      printf("mode is making right turn %d\n",mode);
      turn_right(maze);
      break;
    case MOVING_UP:
      printf("mode is moving up %d\n",mode);
      move_up(maze);
      break;
    }
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


  void annotate(SDL_Renderer* renderer, TTF_Font* font) {

    SDL_Color textColor = {255, 255, 255, 255};  // White color for text

    string mode1;
    if (mode == FOLLOW_LINE) mode1 = "follow line";
    if (mode == MAKING_LEFT_TURN) mode1 = "left turn";
    if (mode == MAKING_RIGHT_TURN) mode1 = "right turn";
    if (mode == MOVING_UP) mode1 = "moving up";
    if (mode == CHECKING_LEFT) mode1 = "check left";
    if (mode == CHECKING_FRONT) mode1 = "check front";

    {
      std::string segText = "mode: " +mode1;
      
      SDL_Surface* surface = TTF_RenderText_Solid(font, segText.c_str(), textColor);
      SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
      
      int textWidth, textHeight;
      SDL_QueryTexture(texture, NULL, NULL, &textWidth, &textHeight);
      
      SDL_Rect dst = { 650 - textWidth / 2, 100 - textHeight / 2, textWidth, textHeight};
      SDL_RenderCopy(renderer, texture, NULL, &dst);

      SDL_FreeSurface(surface);
      SDL_DestroyTexture(texture);
    }

    {
      std::string segText = "front detect: " +to_string(front_wall_detected);
      
      SDL_Surface* surface = TTF_RenderText_Solid(font, segText.c_str(), textColor);
      SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
      
      int textWidth, textHeight;
      SDL_QueryTexture(texture, NULL, NULL, &textWidth, &textHeight);
      
      SDL_Rect dst = { 650 - textWidth / 2, 150 - textHeight / 2, textWidth, textHeight};
      SDL_RenderCopy(renderer, texture, NULL, &dst);
      SDL_FreeSurface(surface);
      SDL_DestroyTexture(texture);
    }

    {
      std::string segText = "left detect: " +to_string(left_wall_detected);
      
      SDL_Surface* surface = TTF_RenderText_Solid(font, segText.c_str(), textColor);
      SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
      
      int textWidth, textHeight;
      SDL_QueryTexture(texture, NULL, NULL, &textWidth, &textHeight);
      
      SDL_Rect dst = { 650 - textWidth / 2, 200 - textHeight / 2, textWidth, textHeight};
      SDL_RenderCopy(renderer, texture, NULL, &dst);
      SDL_FreeSurface(surface);
      SDL_DestroyTexture(texture);
    }

    {
      string dir;
      if (distsen.last_rot == -1) dir = "left";
      if (distsen.last_rot == 0) dir = "front";
      std::string segText = "last dist: " +dir;
      
      SDL_Surface* surface = TTF_RenderText_Solid(font, segText.c_str(), textColor);
      SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
      
      int textWidth, textHeight;
      SDL_QueryTexture(texture, NULL, NULL, &textWidth, &textHeight);
      
      SDL_Rect dst = { 650 - textWidth / 2, 250 - textHeight / 2, textWidth, textHeight};
      SDL_RenderCopy(renderer, texture, NULL, &dst);
      SDL_FreeSurface(surface);
      SDL_DestroyTexture(texture);
    }
    
  }
  

  void draw(SDL_Renderer* renderer, TTF_Font* font) {

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

    annotate(renderer, font);
  }

  ~Car() {
    SDL_DestroyTexture(texture);
  }
};
