
#include "distsensor.hpp"
#include "car.hpp"

#include <cmath>
using namespace std;

double DistSensor::getWorldTheta()
{
  double theta1 = 0;
  if (pos == -1) theta1 = -M_PI/2;
  if (pos == 0) theta1 = 0;
  if (pos == 1) theta1 = +M_PI/2;

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
  
  double yr = x*cos(theta)-y*sin(theta);
  
  return yr +car->y;
}
    

int DistSensor::getLocalX() {

  return 0;
  
  if (pos == -1) {
      return -car->w/2;
  }
  if (pos == 0) {
    return 0;
  }
  if (pos == 1) {
    return car->w/2;
  }

  return -1000;
}

int DistSensor::getLocalY() {

  return 0;
  
  if (pos == -1) {
    return -car->l/2 +car->l/6;
  }
  if (pos == 0) {
    return 0;
  }
  if (pos == 1) {
    return -car->l/2 +car->l/6;
  }

  return -1000;
}

  
