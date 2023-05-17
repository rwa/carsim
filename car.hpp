
#include <SDL.h>

#include <string>
#include <sstream>
using namespace std;

#include "linesensor.hpp"
#include "distsensor.hpp"
#include "maze.hpp"

#define DT 1.6

// Drive modes
#define FOLLOW_LINE 0
#define LEFT_TURN 1
#define RIGHT_TURN 2
#define MOVE_UP_FOR_LEFT 3
#define LOOK 4
#define CREEP_TO_WALL_THEN_RIGHT 5
#define CREEP_TO_WALL_THEN_LEFT 6
#define CREEP_TO_WALL_THEN_BACK 7
#define CREEP_BACK_THEN_LEFT 8
#define BACK_UP 9

// Adjustable parameters
#define FRONT_STOP_DIST 20
#define WALL_DIST 75

#define FORWARD_TIME 1.5/2
#define BACK_UP_TIME 1.85/2
#define MOVE_UP_FOR_LEFT_TIME 1.75/2
#define TURN_TIME 0.7

#define TURN_SPEED M_PI/100;

struct Car {
 
  LineSensor lsen;
  LineSensor msen;
  LineSensor rsen;

  DistSensor distsen;

  Ray lastray;

  double x,y;

  double r = static_cast<double>(rand()) / static_cast<double>(RAND_MAX);

  double theta_rand = 0.1*r*M_PI;
  // double theta = 3*M_PI/2 +theta_rand;
  double theta = 0 +theta_rand;

  SDL_Texture* texture;

  int mode;
  bool lt,md,rt;
  
  bool in_reverse = false;

  bool front_wall_detected = false;
  bool left_wall_detected = false;
  bool right_wall_detected = false;

  // timers for marking the start of timed moves
  uint32_t move_up_start_ticks = 0;
  uint32_t turning_start_ticks = 0;
  uint32_t forward_start_ticks = 0;
  uint32_t backup_start_ticks = 0;
  
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

  vector<Point> left_wall_backup_start;
  vector<Point> left_wall_backup_end;

  double w = 65;
  double l = 80;

  Car(SDL_Renderer* renderer) {

    // x = 4.5*CELL_SIZE;
    // y = 2.5*CELL_SIZE;
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

  void backward()
  {
    double u = -1.0;
    
    x += u*sin(theta)*DT;
    y -= u*cos(theta)*DT;
  }

  void forward()
  {
    double u = 1.0;
    
    x += u*sin(theta)*DT;
    y -= u*cos(theta)*DT;
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

  void creep_forward_to_front_wall(Maze& maze, int turn_mode)
  {
    forward();
    double dist = readdistancesensor(maze);
    
    if (dist < FRONT_STOP_DIST) {
      printf("done creep, mode to %d\n",turn_mode);
      mode = turn_mode;
      turning_start_ticks = SDL_GetTicks();
      if (turn_mode == BACK_UP) backup_start_ticks = SDL_GetTicks();  
      
    }
    
  }

  void creep_back_to_left_wall(Maze& maze)
  {
    backward();
    double dist = readdistancesensor(maze);

    if (dist < WALL_DIST) {
      printf("mode to MOVE UP FOR LEFT\n");
      mode = MOVE_UP_FOR_LEFT;
      move_up_start_ticks = SDL_GetTicks();
    }
    
  }

  void move_up_for_left()
  {
    forward();
    
    uint32_t cur_ticks = SDL_GetTicks();
    double elapsed_turning =  (cur_ticks -move_up_start_ticks) / 1000.0;
    
    if (elapsed_turning > MOVE_UP_FOR_LEFT_TIME) {
      printf("mode to LEFT TURN\n");
      mode = LEFT_TURN;
      turning_start_ticks = SDL_GetTicks();      
    }
  }
  
  void turn(int dir)
  {
    theta += dir*TURN_SPEED;
    
    uint32_t cur_ticks = SDL_GetTicks();
    double elapsed_turning =  (cur_ticks -turning_start_ticks) / 1000.0;
    
    if (elapsed_turning > TURN_TIME) {
      printf("mode to FOLLOW_LINE\n");
      mode = FOLLOW_LINE;
      forward_start_ticks = SDL_GetTicks();      

    }
  }

  void back_up()
  {
    backward();

    uint32_t cur_ticks = SDL_GetTicks();
    double elapsed_turning =  (cur_ticks -backup_start_ticks) / 1000.0;
    
    if (elapsed_turning > BACK_UP_TIME) {
      in_reverse = true;
      mode = LOOK;
    }
  }
  
  void follow_line(Maze& maze)
  {
    (void) mode;
    //printf("following line\n");
    
    double steer_amount = 0.005*M_PI;
    
    readlinesensors(maze, lt, md, rt);

    if (lt && md && !rt)  {
      theta -= steer_amount;
    }
    if (lt && !md && !rt) {
      theta -= steer_amount;
    }
    if (!lt && md && rt)  {
      theta += steer_amount;
    }
    if (!lt && !md && rt) {
      theta += steer_amount;
    }
    // if (!lt && md && !rt) {
    //   forward();
    // }
    // if (lt && !md && rt) {
    //   forward();
    // }
    // if (lt && md && rt) {
    //   forward();
    // }
    // if (!lt && !md && !rt) {
    //   forward();
    // }
    forward();

    uint32_t cur_ticks = SDL_GetTicks();
    double elapsed =  (cur_ticks -forward_start_ticks) / 1000.0;
    if (elapsed > FORWARD_TIME) {
      printf("END following line\n");
      // stop motors
      mode = LOOK;
    }
  }

  void sense_walls(Maze& maze)
  {
    // go front, left, right, then return to front

    // here you command FRONT and wait for the move
    bool facing_front;
    do {
      facing_front = distsen.faceFront();
    } while (!facing_front);

    double dist = readdistancesensor(maze);
    printf("front dist: %f\n",dist);

    front_wall_detected = false;
    if (dist < WALL_DIST) {
      front_wall_detected = true;
    }
    
    // here you command LEFT and wait for the move
    bool facing_left;
    do {
      facing_left = distsen.faceLeft();
    } while (!facing_left);

    dist = readdistancesensor(maze);
    printf("left dist: %f\n",dist);
	
    left_wall_detected = false;
    if (dist < WALL_DIST) {
      left_wall_detected = true;
    }

    // here you command RIGHT and wait for the move
    bool facing_right;
    do {
      facing_right = distsen.faceRight();
    } while (!facing_right);

    dist = readdistancesensor(maze);
    printf("right dist: %f\n",dist);

    right_wall_detected = false;
    if (dist < WALL_DIST) {
      right_wall_detected = true;
    }

      facing_front = distsen.faceFront();
  }

  void look_and_react(Maze& maze) {
    printf("LOOK mode\n");

    sense_walls(maze);
    
    printf("left wall? %d, front wall? %d, right wall? %d\n",
	   left_wall_detected, front_wall_detected, right_wall_detected);
    
    // When backing up, we take rights if available, then lefts, or
    // keep backing up if neither.
    if (in_reverse) {
      if (!right_wall_detected) {
	printf("mode to RIGHT TURN (from reverse)\n");
	mode = RIGHT_TURN;
	turning_start_ticks = SDL_GetTicks();
	in_reverse = false;
      }
      else if (!left_wall_detected) {
	printf("mode to LEFT TURN (from reverse)\n");
	mode = LEFT_TURN;
	turning_start_ticks = SDL_GetTicks();
	in_reverse = false;
      }
      else {
	printf("mode to BACK_UP...\n");
	mode = BACK_UP;
	backup_start_ticks = SDL_GetTicks();
      }
    }
    else {
      
      // front open but not left open - move forward
      if (left_wall_detected && !front_wall_detected) {
	printf("mode to FOLLOW_LINE\n");
	mode = FOLLOW_LINE;
	forward_start_ticks = SDL_GetTicks();      
      }
      // L-right: look forward, creep to wall, turn right
      else if (left_wall_detected && front_wall_detected && !right_wall_detected) {
	bool facing_front;
	do { facing_front = distsen.faceFront(); } while (!facing_front);
	
	printf("mode to CREEP TO WALL THEN RIGHT...\n");
	mode = CREEP_TO_WALL_THEN_RIGHT;
      }
      // left turn with front wall for positioning
      else if (front_wall_detected && !left_wall_detected) {
	bool facing_front;
	do { facing_front = distsen.faceFront(); } while (!facing_front);
	
	printf("mode to CREEP TO WALL THEN LEFT...\n");
	mode = CREEP_TO_WALL_THEN_LEFT;
      }
      // left turn without front wall for positioning
      else if (!front_wall_detected && !left_wall_detected) {
	
	bool facing_left;
	do { facing_left = distsen.faceLeft(); } while (!facing_left);
	
	printf("mode to CREEP BACK THEN LEFT...\n");
	mode = CREEP_BACK_THEN_LEFT;
      }
      // all closed: back up
      else if (front_wall_detected && right_wall_detected && left_wall_detected) {
	printf("mode to CREEP_TO_WEALL_THEN_BACK...\n");

	bool facing_front;
	do { facing_front = distsen.faceFront(); } while (!facing_front);
	
	mode = CREEP_TO_WALL_THEN_BACK;
      }
      else {
	printf("UNHANDLED CASE\n");
      }
      
    } // moving forward
  }
    
  void control(Maze& maze)
  {
    switch (mode) {
    case LOOK:
      look_and_react(maze);
    break;
    case FOLLOW_LINE:
      follow_line(maze);
      break;
    case LEFT_TURN:
      turn(-1);
      break;
    case RIGHT_TURN:
      turn(+1);
      break;
    case CREEP_TO_WALL_THEN_RIGHT:
      creep_forward_to_front_wall(maze, RIGHT_TURN);
      break;
    case CREEP_TO_WALL_THEN_LEFT:
      creep_forward_to_front_wall(maze, LEFT_TURN);
      break;
    case CREEP_TO_WALL_THEN_BACK:
      creep_forward_to_front_wall(maze, BACK_UP);
      break;
    case CREEP_BACK_THEN_LEFT:
      creep_back_to_left_wall(maze);
      break;
    case MOVE_UP_FOR_LEFT:
      move_up_for_left();
      break;
    case BACK_UP:
      back_up();
      break;
    }
  }

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
    if (mode == LEFT_TURN) mode1 = "left turn";
    if (mode == RIGHT_TURN) mode1 = "right turn";
    if (mode == MOVE_UP_FOR_LEFT) mode1 = "moving up for left";

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
      string dir(to_string(lt) + to_string(md) + to_string(rt));
      
      // if (distsen.last_rot == -1) dir = "left";
      // if (distsen.last_rot == 0) dir = "front";
      std::string segText = "lmr: " +dir;
      
      SDL_Surface* surface = TTF_RenderText_Solid(font, segText.c_str(), textColor);
      SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
      
      int textWidth, textHeight;
      SDL_QueryTexture(texture, NULL, NULL, &textWidth, &textHeight);
      
      SDL_Rect dst = { 650 - textWidth / 2, 300 - textHeight / 2, textWidth, textHeight};
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

    for (auto& it: left_wall_backup_start) {
      // yellow cross
      SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
      SDL_RenderDrawLine(renderer, it.x-4, it.y+4, it.x+4, it.y-4);
      SDL_RenderDrawLine(renderer, it.x-4, it.y-4, it.x+4, it.y+4);
    }
    for (auto& it: left_wall_backup_end) {
      // yellow plus
      SDL_SetRenderDrawColor(renderer, 255, 255, 250, 255); 
      SDL_RenderDrawLine(renderer, it.x-4, it.y, it.x+4, it.y);
      SDL_RenderDrawLine(renderer, it.x, it.y-4, it.x, it.y+4);
    }
    
    annotate(renderer, font, elapsed_time);
  }

  ~Car() {
    SDL_DestroyTexture(texture);
  }
};
