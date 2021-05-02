// #ifndef NU32__H__
// #define NU32__H__
#include "NU32.h"

volatile int Ki, Kp;

// initialize the peripherals for the 5KHz current control loop
void current_control_peripheral_init(void);
void set_current_gains(int Ki, int Kp);
// int get_current_gains(void);

// #endif // NU32__H__