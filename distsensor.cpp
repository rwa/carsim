
#include "distsensor.hpp"
#include "car.hpp"

#include <cmath>
using namespace std;

bool DistSensor::faceLeft() {

  static uint32_t start_ticks;
  
  if (rot == -1) return true;

  if (rot == 0 || rot == 1) {
    start_ticks = SDL_GetTicks();
  }
  
  rot = -2; // in transition to left

  uint32_t cur_ticks = SDL_GetTicks();
  double elapsed_time = (cur_ticks - start_ticks) / 1000.0; // Convert to secs

  if (elapsed_time > 1.0) {
    rot = -1;
    last_rot = -1;
    return true;
  }

  return false;
}

bool DistSensor::faceFront() {

  static uint32_t start_ticks;
  
  if (rot == 0) return true;

  if (rot == -1 || rot == 1) {
    start_ticks = SDL_GetTicks();
  }
  
  rot = +2; // in transition to front

  uint32_t cur_ticks = SDL_GetTicks();
  double elapsed_time = (cur_ticks - start_ticks) / 1000.0; // Convert to secs

  if (elapsed_time > 1.0) {
    rot = 0;
    last_rot = 0;
    return true;
  }

  return false;
}

double DistSensor::getWorldTheta()
{
  double theta1 = 0;
  if (rot == -1) theta1 = -M_PI/2;
  if (rot == 0) theta1 = 0;
  if (rot == 1) theta1 = +M_PI/2;

  theta1 -= M_PI/2;

  return car->theta +theta1;
}

int DistSensor::getWorldX(double theta) {

  double x = getLocalX();
  double y = getLocalY();

  double xr = x*cos(theta)-y*sin(theta);
  
  return xr +car->x;
}

int DistSensor::getWorldY(double theta) {

  double x = getLocalX();
  double y = getLocalY();

  double yr = x*sin(theta)+y*cos(theta);
  
  return yr +car->y;
}
    

int DistSensor::getLocalX() {

  return 0;
}

int DistSensor::getLocalY() {

  return -car->l/2;
}

  
