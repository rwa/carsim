#ifndef __DistSensor_included
#define __DistSensor_included

struct Car;

struct DistSensor {

  DistSensor() {};
  
  Car* car;

  int getWorldX(double theta);
  int getWorldY(double theta);

  int getLocalX();
  int getLocalY();

  double getWorldTheta();

  void moveToFront();
  void moveToLeft();
  void moveToRight();

  int rot = -1;
  
  // local coordinates on the car, relative to center
  int x;
  int y;
  
};

#endif
