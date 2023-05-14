
#include <SDL.h>

#include <string>
#include <sstream>
using namespace std;

#include "linesensor.hpp"
#include "distsensor.hpp"
#include "maze.hpp"

// Drive modes
#define FOLLOW_LINE 0
#define MAKING_LEFT_TURN 1
#define MAKING_RIGHT_TURN 2
#define MOVING_UP 3
#define MAKING_UTURN 4

// Adjustable constants
#define MOVE_UP_TIME_LEFT_TURN 1.5
#define MOVE_UP_TIME_RIGHT_TURN 1.0
#define MOVE_UP_TIME_UTURN 1.2
#define TURN_RIGHT_TIME 0.5
#define TURN_LEFT_TIME 0.75
#define UTURN_TIME 1.5
#define LEFT_LOCK_TIME 2.2
#define FRONT_LOCK_TIME 2.0

#define TURN_SPEED M_PI/100;

#define FRONT_DIST 50
#define LEFT_DIST 75
#define RIGHT_DIST 75

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
  bool right_wall_detected = false;

  bool going_straight = true;
  int next_turn = 0; // -1 next turn left, +1 next turn right, +2 u-turn

  // after a left turn, lock out another one for a bit
  bool left_turn_lockout = false;
  uint32_t left_lockout_start_ticks = 0;

  // if we're far from a front wall, wait to measure again
  bool front_lockout = false;
  uint32_t front_lockout_start_ticks = 0;
  
  // moving up is based on a timer, start timer when initiating moving up
  uint32_t moving_up_start_ticks = 0;

  uint32_t turning_start_ticks = 0;
  
  // Diagnostics
  vector<Point> move_up_for_left_turn;
  vector<Point> start_left_turn;
  vector<Point> end_left_turn;

  vector<Point> move_up_for_right_turn;
  vector<Point> start_right_turn;
  vector<Point> end_right_turn;

  vector<Point> move_up_for_uturn;
  vector<Point> start_uturn;
  vector<Point> end_uturn;
  
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
    double dt = 0.3;
    
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
    (void) maze;
    
    uint32_t cur_ticks = SDL_GetTicks();
    double elapsed_turning =  (cur_ticks -turning_start_ticks) / 1000.0;
    
    (void) mode;
    theta -= TURN_SPEED;

    if (elapsed_turning > TURN_LEFT_TIME) {
      mode = FOLLOW_LINE;

      left_turn_lockout = true;
      left_lockout_start_ticks = SDL_GetTicks();
      
      // Diagnostic
      Point p(x,y);
      end_left_turn.push_back(p);
    }
    
    left_wall_detected = true;
    going_straight = false;
  }

  void turn_right(Maze& maze)
  {
    (void) maze;
    
    uint32_t cur_ticks = SDL_GetTicks();
    double elapsed_turning =  (cur_ticks -turning_start_ticks) / 1000.0;
    
    (void) mode;
    theta += TURN_SPEED;

    if (elapsed_turning > TURN_RIGHT_TIME) {
      mode = FOLLOW_LINE;

      // Diagnostic
      Point p(x,y);
      end_right_turn.push_back(p);
      
    }
    
    // bool l,m,r;
    // readlinesensors(maze, l, m, r);
    // if (l && m && !r) {
    //   printf("exit right turn - back to follow line\n");
    //   mode = FOLLOW_LINE;

    //   // Diagnostic
    //   Point p(x,y);
    //   end_right_turn.push_back(p);
      
    // }

    going_straight = false;
  }


  void make_uturn(Maze& maze)
  {
    (void) maze;
    
    uint32_t cur_ticks = SDL_GetTicks();
    double elapsed_turning =  (cur_ticks -turning_start_ticks) / 1000.0;
    
    (void) mode;
    theta -= TURN_SPEED;

    if (elapsed_turning > UTURN_TIME) {
      mode = FOLLOW_LINE;

      // Diagnostic
      Point p(x,y);
      end_uturn.push_back(p);
    }
    
    going_straight = false;
  }
  
  void follow_line(Maze& maze)
  {
    (void) mode;
    
    double steer_amount = 0.005*M_PI;
    
    bool l,m,r;
    readlinesensors(maze, l, m, r);
    //printf("%d %d %d\n",l,m,r);
    
    going_straight = false;
    if (l && m && !r)  {
      //printf("110: steer left\n");
      theta -= steer_amount;
    }
    if (l && !m && !r) {
      //printf("100: steer left\n");
      theta -= steer_amount;
    }
    if (!l && m && r)  {
      //printf("011: steer right\n");
      theta += steer_amount;
    }
    if (!l && !m && r) {
      //printf("001: steer right\n");
      theta += steer_amount;
    }
    if (!l && m && !r) {
      //printf("010: forward\n");
      forward();
      going_straight = true;
    }
    if (l && m && r) {
      //printf("111: forward?\n");
      forward();
    }
    if (!l && !m && !r) {
      //printf("000: forward?\n");
      forward();
    }
    forward();
  }

  void move_up(Maze& maze)
  {
    uint32_t cur_ticks = SDL_GetTicks();
    double elapsed_moving_up =  (cur_ticks - moving_up_start_ticks) / 1000.0; // Convert to secs

    if (next_turn == -1 && elapsed_moving_up > MOVE_UP_TIME_LEFT_TURN) {
      mode = MAKING_LEFT_TURN;

      turning_start_ticks = SDL_GetTicks();

      // Diagnostic
      Point p(x,y);
      start_left_turn.push_back(p);
      
      return;
    }
    if (next_turn == +1 && elapsed_moving_up > MOVE_UP_TIME_RIGHT_TURN) {
      mode = MAKING_RIGHT_TURN;

      turning_start_ticks = SDL_GetTicks();
      
      // Diagnostic
      Point p(x,y);
      start_right_turn.push_back(p);
      
      return;
    }
    if (next_turn == +2 && elapsed_moving_up > MOVE_UP_TIME_UTURN) {
      mode = MAKING_UTURN;

      turning_start_ticks = SDL_GetTicks();
      
      // Diagnostic
      Point p(x,y);
      start_uturn.push_back(p);
      
      return;
    }
    
    forward();

    (void) maze;
  }

  void sense_walls(Maze& maze)
  {
    static bool sense_left = true;

    // don't measure when turning or moving up
    if (mode != FOLLOW_LINE) return;

    // Update front lockout flag
    uint32_t cur_ticks = SDL_GetTicks();
    double elapsed_lockout =  (cur_ticks -front_lockout_start_ticks) / 1000.0;
    if (elapsed_lockout > FRONT_LOCK_TIME) {
      front_lockout = false;
    }

    if (front_lockout) sense_left = true;
    
    if (sense_left) {

      bool facing_left = distsen.faceLeft();

      if (facing_left) {

	double dist = readdistancesensor(maze);
	//printf("left dist: %f\n",dist);
	
	left_wall_detected = false;
	if (dist < LEFT_DIST) {
	  left_wall_detected = true;
	}

	sense_left = false;
      }
    }

    // sense front
    if (!sense_left) {
      
      bool facing_front = distsen.faceFront();

      bool close_to_front = false;
      bool far_from_front = false;
      
      if (facing_front) {
	double dist = readdistancesensor(maze);
	printf("front dist: %f\n",dist);

	// If we're closing in on a front wall, keep measuring front
	double close_tol = 100;
	if (dist < close_tol) close_to_front = true;

	// If we're far from a wall, we can measure side for awhile
	double far_tol = 200;
	if (dist > far_tol) far_from_front = true;

	if (far_from_front) {
	  front_lockout = true;
	  front_lockout_start_ticks = SDL_GetTicks();
	}

	front_wall_detected = false;
	if (dist < FRONT_DIST) {
	  printf("FRONT WALL DETECT\n");
	  front_wall_detected = true;
	  close_to_front = false;
	}
	
	if (!close_to_front) sense_left = true;
      }
    }
  }

  bool sense_right_wall(Maze& maze)
  {
    bool facing_right;

    do {
      facing_right = distsen.faceRight();
    } while (!facing_right);

    double dist = readdistancesensor(maze);
    printf("right dist: %f\n",dist);

    if (dist < RIGHT_DIST) {
      printf("RIGHT WALL DETECT\n");
      return true;
    }
      
    return false;
  }

  void sense_left_and_right(Maze& maze)
  {
    bool facing;

    do {
      facing = distsen.faceRight();
    } while (!facing);

    double dist = readdistancesensor(maze);
    printf("right dist: %f\n",dist);

    if (dist < RIGHT_DIST) {
      right_wall_detected = true;
    }

    do {
      facing = distsen.faceLeft();
    } while (!facing);

    dist = readdistancesensor(maze);
    printf("left dist: %f\n",dist);

    if (dist < LEFT_DIST) {
      left_wall_detected = true;
    }
  }
  
  
  void control(Maze& maze, double elapsed_time)
  {
    (void) elapsed_time;

    // update front_wall_detected and left_wall_detected
    sense_walls(maze);

    // we were probably zooming on the front wall and may have missed
    // left detections.  look both ways.
    if (front_wall_detected) {
      sense_left_and_right(maze);
    }

    // Update left lockout flag
    uint32_t cur_ticks = SDL_GetTicks();
    double elapsed_lockout =  (cur_ticks -left_lockout_start_ticks) / 1000.0;
    if (elapsed_lockout > LEFT_LOCK_TIME) {
      left_turn_lockout = false;
    }
	
    // update mode based on wall sensing
    if (mode == FOLLOW_LINE) {
      if (left_wall_detected) {
	if (front_wall_detected) {
	  front_wall_detected = false;

	  bool right_wall_detected = sense_right_wall(maze);
	  if (right_wall_detected) {
	    printf("START MOVING UP FOR UTURN\n");
	    mode = MOVING_UP;
	    moving_up_start_ticks = SDL_GetTicks();
	    next_turn = +2;

	    // Diagnostic
	    Point p(x,y);
	    move_up_for_uturn.push_back(p);
	  }
	  else {
	    printf("START MOVING UP FOR RIGHT TURN\n");
	    mode = MOVING_UP;
	    moving_up_start_ticks = SDL_GetTicks();
	    next_turn = +1;
	    
	    // Diagnostic
	    Point p(x,y);
	    move_up_for_right_turn.push_back(p);
	  } // no right wall
	}
      }
      else {

	bool l,m,r;
	readlinesensors(maze, l, m, r);
	if (l && m && !left_turn_lockout) {
	  printf("START MOVING UP FOR LEFT TURN\n");
	  mode = MOVING_UP;
	  moving_up_start_ticks = SDL_GetTicks();
	  next_turn = -1;
	  front_wall_detected = false;
	  
	  // Diagnostic
	  Point p(x,y);
	  move_up_for_left_turn.push_back(p);
	}
      }
    }
    
    switch (mode) {
      
    case FOLLOW_LINE:
      //printf("mode is follow line %d\n",mode);
      follow_line(maze);
      break;
    case MAKING_LEFT_TURN:
      //printf("mode is making left turn %d\n",mode);
      turn_left(maze);
      break;
    case MAKING_RIGHT_TURN:
      //printf("mode is making right turn %d\n",mode);
      turn_right(maze);
      break;
    case MOVING_UP:
      // printf("mode is moving up %d\n",mode);
      move_up(maze);
      break;
    case MAKING_UTURN:
      //printf("mode is making right turn %d\n",mode);
      make_uturn(maze);
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


  void annotate(SDL_Renderer* renderer, TTF_Font* font, double elapsed_time) {

    SDL_Color textColor = {255, 255, 255, 255};  // White color for text

    string mode1;
    if (mode == FOLLOW_LINE) mode1 = "follow line";
    if (mode == MAKING_LEFT_TURN) mode1 = "left turn";
    if (mode == MAKING_RIGHT_TURN) mode1 = "right turn";
    if (mode == MOVING_UP) mode1 = "moving up";

    {
      std::ostringstream timestr;
      timestr.precision(1);
      timestr << std::fixed << elapsed_time;
      std::string segText = "elapsed time: " +timestr.str();
      
      SDL_Surface* surface = TTF_RenderText_Solid(font, segText.c_str(), textColor);
      SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
      
      int textWidth, textHeight;
      SDL_QueryTexture(texture, NULL, NULL, &textWidth, &textHeight);
      
      SDL_Rect dst = { 650 - textWidth / 2, 50 - textHeight / 2, textWidth, textHeight};
      SDL_RenderCopy(renderer, texture, NULL, &dst);

      SDL_FreeSurface(surface);
      SDL_DestroyTexture(texture);
    }

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
    {
      std::string segText = "going straight: " +to_string(going_straight);
      
      SDL_Surface* surface = TTF_RenderText_Solid(font, segText.c_str(), textColor);
      SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
      
      int textWidth, textHeight;
      SDL_QueryTexture(texture, NULL, NULL, &textWidth, &textHeight);
      
      SDL_Rect dst = { 650 - textWidth / 2, 300 - textHeight / 2, textWidth, textHeight};
      SDL_RenderCopy(renderer, texture, NULL, &dst);
      SDL_FreeSurface(surface);
      SDL_DestroyTexture(texture);
    }
    {
      std::string segText = "left lockout: " +to_string(left_turn_lockout);
      
      SDL_Surface* surface = TTF_RenderText_Solid(font, segText.c_str(), textColor);
      SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
      
      int textWidth, textHeight;
      SDL_QueryTexture(texture, NULL, NULL, &textWidth, &textHeight);
      
      SDL_Rect dst = { 650 - textWidth / 2, 350 - textHeight / 2, textWidth, textHeight};
      SDL_RenderCopy(renderer, texture, NULL, &dst);
      SDL_FreeSurface(surface);
      SDL_DestroyTexture(texture);
    }
    
    {
      std::string segText = "front sense lockout: " +to_string(front_lockout);
      
      SDL_Surface* surface = TTF_RenderText_Solid(font, segText.c_str(), textColor);
      SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
      
      int textWidth, textHeight;
      SDL_QueryTexture(texture, NULL, NULL, &textWidth, &textHeight);
      
      SDL_Rect dst = { 650 - textWidth / 2, 400 - textHeight / 2, textWidth, textHeight};
      SDL_RenderCopy(renderer, texture, NULL, &dst);
      SDL_FreeSurface(surface);
      SDL_DestroyTexture(texture);
    }
    
  }
  

  void draw(SDL_Renderer* renderer, TTF_Font* font, double elapsed_time) {

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

    // Diagnostics
    for (auto& it: move_up_for_right_turn) {
      // green cross
      SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
      SDL_RenderDrawLine(renderer, it.x-4, it.y+4, it.x+4, it.y-4);
      SDL_RenderDrawLine(renderer, it.x-4, it.y-4, it.x+4, it.y+4);
    }
    for (auto& it: start_right_turn) {
      // green plus
      SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); 
      SDL_RenderDrawLine(renderer, it.x-4, it.y, it.x+4, it.y);
      SDL_RenderDrawLine(renderer, it.x, it.y-4, it.x, it.y+4);
    }
    // for (auto& it: end_right_turn) {
    //   // green diamond
    //   SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); 
    //   SDL_RenderDrawLine(renderer, it.x-4, it.y, it.x, it.y-4);
    //   SDL_RenderDrawLine(renderer, it.x, it.y-4, it.x+4, it.y);
    //   SDL_RenderDrawLine(renderer, it.x+4, it.y, it.x, it.y+4);
    //   SDL_RenderDrawLine(renderer, it.x, it.y+4, it.x-4, it.y);
    // }
    
    for (auto& it: move_up_for_left_turn) {
      // blue cross
      SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // blue
      SDL_RenderDrawLine(renderer, it.x-4, it.y+4, it.x+4, it.y-4);
      SDL_RenderDrawLine(renderer, it.x-4, it.y-4, it.x+4, it.y+4);
    }
    for (auto& it: start_left_turn) {
      // blue plus
      SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); 
      SDL_RenderDrawLine(renderer, it.x-4, it.y, it.x+4, it.y);
      SDL_RenderDrawLine(renderer, it.x, it.y-4, it.x, it.y+4);
    }

    for (auto& it: move_up_for_uturn) {
      // white cross
      SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
      SDL_RenderDrawLine(renderer, it.x-4, it.y+4, it.x+4, it.y-4);
      SDL_RenderDrawLine(renderer, it.x-4, it.y-4, it.x+4, it.y+4);
    }
    
    annotate(renderer, font, elapsed_time);
  }

  ~Car() {
    SDL_DestroyTexture(texture);
  }
};
