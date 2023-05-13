#ifndef __LineSensor_included
#define __LineSensor_included

struct Car;

struct LineSensor {

  LineSensor() {};
  
  //LineSensor(Car* car_, int x_, int y_) : car(car_), x(x_), y(y_) {};

  Car* car;

  int getWorldX(double theta);
  int getWorldY(double theta);

  // local coordinates on the car, relative to center
  int x;
  int y;
  
};

#endif
