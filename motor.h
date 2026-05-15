/*
 * motor.h
 *
 * Created: 19/04/2026 11:01:54 PM
 *  Author: Alaa
 */ 


#ifndef MOTOR_H_
#define MOTOR_H_

#define MOTOR_IN1      PB2
#define MOTOR_IN2      PB3

 void motor_init(void);
  void motor_forward(void);
  void motor_stop(void);




#endif /* MOTOR_H_ */