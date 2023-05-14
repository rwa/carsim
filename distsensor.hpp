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

  bool faceLeft();
  bool faceRight();
  bool faceFront();

  // -1: left, 0: forward, -2: in transition
  int rot = -1;
  int last_rot = -1;
  
  // local coordinates on the car, relative to center
  int x;
  int y;
  
};

#endif
