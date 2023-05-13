#ifndef __LineSensor_included
#define __LineSensor_included

struct Car;

struct LineSensor {

  LineSensor() {};
  
  LineSensor(Car* car_, int x_, int y_, int size_) : car(car_), x(x_), y(y_), size(size_) {};

  Car* car;

  int getWorldX();
  int getWorldY();

  // local coordinates on the car, relative to center
  int x;
  int y;
  
  int size;
};

#endif
