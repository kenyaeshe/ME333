// Module for the 5KHz current control loop
#include "currentcontrol.h"



// volatile int Ki = current_gain_array[0];
// volatile int Kp = current_gain_array[1];
// volatile int Waveform[NUMSAMPS];

// initialize the peripherals: timer 3, timer 4, OC1, and digital output RD0

void current_control_peripheral_init(void){
    // Timer 3 ISR intialization (5KHz)
    IFS0bits.T3IF = 0;              // clear timer 3 interrupt flag
    T3CONbits.TCKPS = 0b000;        // give timer 3 a prescaler N = 1
    PR3 =  15999;                   // 5KHz ISR -> P = 15999    
    IPC3bits.T3IP = 6;              // interrupt priority 6
    IPC3bits.T3IS = 0;              // subpriority 0
    IEC0bits.T3IE = 1;              // enable timer 3 interrupt
    T3CONbits.ON = 1;               // enable timer 3

    // Timer 2 + OC1 PWM initialization (20KHz)
    T2CONbits.TCKPS = 0b000;        // prescaler of 0 (0b000)
    PR2 = 3999;                     // 50us = (PR2 + 1) * (N=1) * 12.5ns => 20KHz
    TMR2 = 0;                       // set starting timer 2 count to 0
    OC1CONbits.OCM = 0b110;         // PWM with fault pin disabled
    OC1CONbits.OCTSEL = 0;          // sets OC1 to use timer 2
    OC1RS = 1000;                   // 25 % Duty cycle = OC1RS/(PR2+1) * 100%
    OC1R = 1000;                       // 25% DC
    T2CONbits.ON = 1;               // turn on timer 2
    OC1CONbits.ON = 1;              // turn on OC1

    // RD0 initialization
    TRISDbits.TRISD0 = 0;           // make RD0 an output
    // LATDbits.LATD0 = 0;
    TRISDbits.TRISD1 = 0;           // make RD1 an output
    LATDbits.LATD1 = 0;             // make RD0 output low
    

} // end current control peripheral

void set_current_gains(int Ki_in, int Kp_in){

    // current_gain_array[0] = Ki;
    // current_gain_array[1] = Kp;
    // return current_gain_array;
    Ki = Ki_in;
    Kp = Kp_in;
    
} // set current gains


// void makeWaveform() {
//     int i = 0, center = 2000, A = 1000; 
//     // square wave, PR3 = 3999, center = (PR3+1)/2, A = (PR3+1)/4
//     for (i = 0; i < NUMSAMPS; i++) {
//         if( i < NUMSAMPS/2){
//             Waveform[i] = center + A;
//         }
//         else {
//             Waveform[i] = center - A;
//         }
        

//     }

// }

// int get_current_gains(void){
//     int current_gain_array[2];

//     current_gain_array[0] = Ki;
//     current_gain_array[1] = Kp;
//     // int array = [Ki, Kp];

//     return current_gain_array;

// }