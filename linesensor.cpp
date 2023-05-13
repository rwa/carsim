
#include "linesensor.hpp"
#include "car.hpp"

int LineSensor::getWorldX()
{
  return x +car->x;
}

int LineSensor::getWorldY()
{
  return y +car->y;
}
