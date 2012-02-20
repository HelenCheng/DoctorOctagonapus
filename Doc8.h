#ifndef DOC8_H_
#define DOC8_H_

#include "WPILib.h"
#include "CANDrive.h"
#include "Lift.h"
#include "Shooter.h"
#include "RampDescend.h"
//#include "Windows.h"

#define ENCOUNT_SHOOTER 360
#define ENCOUNT_DRIVE 250

#define SHOOTER_TOP_SPEED_HIGH 1000.0
#define SHOOTER_TOP_SPEED_MED 255.0
#define SHOOTER_TOP_SPEED_LOW 255.0
#define SHOOTER_BOTTOM_SPEED_HIGH -2650.0
#define SHOOTER_BOTTOM_SPEED_MED -2270.0
#define SHOOTER_BOTTOM_SPEED_LOW -1400.0
#define SHOOTER_BOTTOM_SPEED_IDLE -1000.0  /*IDLE SPEED*/

#define SHOOTER_SPEED_OFF 0.0
#define SHOOTER_MAX_SPEED 2700.0

#define JOYSTICK_PILOT_PORT 1
#define JOYSTICK_COPILOT_PORT 2

#define KINECT_LEFT 1
#define KINECT_RIGHT 2

#define CAN_DRIVE_LEFT_FRONT 3
#define CAN_DRIVE_LEFT_BACK 2

#define CAN_DRIVE_RIGHT_FRONT 5
#define CAN_DRIVE_RIGHT_BACK 4

#define CAN_SHOOTER_TOP 7
#define CAN_SHOOTER_BOTTOM 6

#define CAN_LIFT_TOWER 8
#define CAN_RAMP_DESCENDER 9

#define KP	0.250
#define KI	0.015
#define KD	0.020

#endif