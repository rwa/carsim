
#include "linesensor.hpp"
#include "car.hpp"

int LineSensor::getWorldX(double theta)
{
  double xr = x*cos(theta)-y*sin(theta);

  return xr +car->x;
}

int LineSensor::getWorldY(double theta)
{
  double yr = x*sin(theta)+y*cos(theta);

  return yr +car->y;
}
