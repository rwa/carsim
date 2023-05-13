
#include "distsensor.hpp"
#include "car.hpp"

#include <cmath>
using namespace std;

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

  
