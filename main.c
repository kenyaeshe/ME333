#include "NU32.h"          // config bits, constants, funcs for startup and UART
#include "utilities.h"
#include "isense.h"
#include "currentcontrol.h"
// include other header files here

#define BUF_SIZE 200
#define NUMSAMPS 100

// initialize the mode as a global var
// static volatile mode Mode;
static volatile int PWM_percent; // use to change PWM input from user
static volatile unsigned int ADC_sample_counts;
static volatile int Waveform[NUMSAMPS];
static volatile int current_response[NUMSAMPS];
static volatile int StoringData = 0; // use to tell MATLAB when to plot the data for ITEST
static volatile int Ki_in, Kp_in;


// static volatile int Ki, Kp;


// ENCODER.C:
static int encoder_command(int read) { // send a command to the encoder chip
                                       // 0 = reset count to 32,768, 1 = return the count
  SPI4BUF = read;                      // send the command
  while (!SPI4STATbits.SPIRBF) { ; }   // wait for the response
  SPI4BUF;                             // garbage was transferred, ignore it
  SPI4BUF = 5;                         // write garbage, but the read will have the data
  while (!SPI4STATbits.SPIRBF) { ; }
  return SPI4BUF;
}

int encoder_counts(void) {
  encoder_command(1); 
  return encoder_command(1); // call twice to obtain the most recent result
}

int encoder_reset(void){
  return encoder_command(0);
}

double encoder_degrees(void){
  unsigned int num_counts;

  num_counts = encoder_counts();
  int diff = num_counts - 32768;

  return (360 * (diff/1340.0)); // ~1340 counts per rotation
}

void encoder_init(void) {
  // SPI initialization for reading from the decoder chip
  SPI4CON = 0;              // stop and reset SPI4
  SPI4BUF;                  // read to clear the rx receive buffer
  SPI4BRG = 0x4;            // bit rate to 8 MHz, SPI4BRG = 80000000/(2*desired)-1
  SPI4STATbits.SPIROV = 0;  // clear the overflow
  SPI4CONbits.MSTEN = 1;    // master mode
  SPI4CONbits.MSSEN = 1;    // slave select enable
  SPI4CONbits.MODE16 = 1;   // 16 bit mode
  SPI4CONbits.MODE32 = 0; 
  SPI4CONbits.SMP = 1;      // sample at the end of the clock
  SPI4CONbits.ON = 1;       // turn SPI on
}


// 5KHz Current control ISR
void __ISR(_TIMER_3_VECTOR,IPL6SOFT) currentcontrolISR(void){ //5KHz
  // mode 

  IFS0bits.T3IF = 0;                          // clear the interrupt flag
  Mode = get_mode();

  // create the reference waveform for PI control


  switch (Mode){ 
    case IDLE:
    { // IDLE mode
      // set_mode(I); // set mode to IDLE
      OC1RS = 0;             // 0% duty cycle (?)
      break; // dont forget to break!
    } // end case I

    case PWM:
    { // PWM mode
      int duty_cycle;
      
      if (PWM_percent < -100){
        duty_cycle = 4000;                                    // saturate duty cycle at -100% if given input > 100
        LATDbits.LATD1 = 0;                                     // make RD0 output low
      }

      else if ((PWM_percent >= - 100) && (PWM_percent < 0)) {
        duty_cycle = (int) ((abs(PWM_percent)/100.0)*4000);
        LATDbits.LATD1 = 0;                                     // make direction bit high
      }

      else if (PWM_percent == 0){
        set_mode(IDLE);                                            // set mode to idle

      }
      
      else if ((PWM_percent > 0) && (PWM_percent <= 100)) {
        duty_cycle = (int) ((PWM_percent/100.0)*4000);                 // note; 4000 = PR2+1 ; PWM_percent/100% * PR2+1 = OC1RS
        LATDbits.LATD1 = 1;             // make RD0 output low
      }

      else {
        duty_cycle = 4000;                                   // saturate duty cycle if given input > 100
        LATDbits.LATD1 = 1;                                  // make RD1 output low
      }
      OC1RS = duty_cycle;                                   // 25 % Duty cycle = OC1RS/(PR2+1) * 100%

      break;
    } // end case P

    case ITEST:
    {
      static int counter = 0;
      static int eprev = 0;
      static int Eint = 0;
      static int sample, e = 0;
      static float PI_current, PI_current_new;
      
        
        // create a 100Hz waveform with 100 samples 0 to 99
        if ((counter >= 0) && (counter <= 25)){
          Waveform[counter] = 200; // units of mA
            
        }
        else if ((counter > 25) && (counter <= 50)){
          Waveform[counter] = - 200; // units of mA 
        }
        else if ((counter > 50) && (counter <= 75)){
          Waveform[counter] = 200;    
        }
        else if ((counter > 75) && (counter < 99)){
          Waveform[counter] = -200; 
        }

        // create the response array
        sample = ADC_sample(1); // ADC_sample (1) returns the current in mA
        current_response[counter] = sample;
        
        // calculations for the PI control
        e = Waveform[counter] - sample;           // error = reference value - actual value
        
        Eint += e;                                       // error integral
        
        // anti-integrator windup code
        if (Eint > 1000)
        {
          Eint = 1000;
        }
        else if (Eint < - 1000){
          Eint = - 1000;
        }

        PI_current = Ki_in*e + Kp_in*Eint;                     // PI Control
        PI_current_new = PI_current;

        if (PI_current_new > 100){
          PI_current_new = 100;
          
        }
        else if (PI_current_new < - 100){
          PI_current_new = -100;
        }

        // change direction bit depending on PI_current value  
        if (PI_current_new > 0){
          LATDbits.LATD1 = 1; // set direction bit to be high
        }
        else if (PI_current_new < 0){
          PI_current_new = abs(PI_current_new);
          LATDbits.LATD1 = 0; // set direction bit to be low
        }

        OC1RS = (int) ((PI_current_new/100.0) * 4000);

        eprev = e;                                    // update the previous error
       
        counter++;

        if (counter == 99){
          set_mode(IDLE); 
          counter = 0;
          StoringData = 0;
        }
           
        break;
      } //end ITEST
      
    default:
    {
      set_mode(IDLE);
      break;
    }
  
  } // end switch

 

  

} // end ISR


// MAIN //
int main() 
{
  NU32_Startup(); // cache on, min flash wait, interrupts on, LED/button init, UART init
  NU32_LED1 = 1;  // turn off the LEDs
  NU32_LED2 = 1;  

  char buffer[BUF_SIZE];
  
  set_mode(IDLE);           // set mode to IDLE 

  // makeWaveform();           // create the 200Hz
  
  __builtin_disable_interrupts();
  // in future, initialize modules or peripherals here
    ADC_init();                 // initialize the ADC AN0 to manual sampling, automatic conversion
    encoder_init(); // initialize SPI4 - 6MHz baud, 16bit, samples on falling edge, automatic slave detect
    current_control_peripheral_init();
  __builtin_enable_interrupts();


  while(1)
  {
    NU32_ReadUART3(buffer,BUF_SIZE); // we expect the next character to be a menu command
    NU32_LED2 = 1;                   // clear the error LED
    switch (buffer[0]) {
      case 'a':
      { // read & return current in ADC Counts
        // sprintf(buffer, "The current in ADC counts is: %d.\r\n", ADC_sample(0));
        sprintf(buffer, "%d\n", ADC_sample(0));
        NU32_WriteUART3(buffer);
        break;
      }
      case 'b':
      { // read & return current in mA
        sprintf(buffer, "%d\n", ADC_sample(1));
        NU32_WriteUART3(buffer);
        break;
      }
      case 'c': // return current encoder count
      {
        sprintf(buffer, "%d\n", encoder_counts());
        NU32_WriteUART3(buffer); // send encoder count to client
        break;
      }
      case 'd': // return the encoder angle in degrees
      {
        sprintf(buffer, "%.1f\n", encoder_degrees());
        NU32_WriteUART3(buffer);
        break;
      }

      case 'e': // reset the encoder
      {
        encoder_reset();
        sprintf(buffer,"%d\n", encoder_degrees());
         NU32_WriteUART3(buffer);
        break;
      }
      case 'f': //set PWM (-100 to 100)
      {
        // prompt the user to insert desired PWM vaue
        sprintf(buffer,"What PWM vaue would you like [-100 to 100]?\r\n");
        NU32_WriteUART3(buffer);

        // read PWM value from user
        NU32_ReadUART3(buffer, BUF_SIZE);
        sscanf(buffer, "%d\n", &PWM_percent); // recall PWM_percent is a static volatile global int
        
        set_mode(PWM); // set mode to PWM 

        if ((PWM_percent < 0) && (PWM_percent >= -100)){
          // sprintf(buffer, "PWM has been set to %d percent clockwise.\r\n", abs(PWM_percent)); //return the absolute value of the PWM
          sprintf(buffer, "%d\n", abs(PWM_percent));
          NU32_WriteUART3(buffer);
        }
        else if ((PWM_percent > 0) && (PWM_percent <= 100)){
          // sprintf(buffer, "PWM has been set to %d percent counterclockwise.\r\n", PWM_percent);
          sprintf(buffer,"%d\n",PWM_percent); 
          NU32_WriteUART3(buffer);
        }
        else if (PWM_percent == 0){
          sprintf(buffer,"%d\n",PWM_percent);
          // NU32_WriteUART3("The motor has been unpowered.\r\n");
          NU32_WriteUART3(buffer);
        }
        
        break;
      }

      case 'g': {// set current gains
        // int Ki, Kp;
        // int Ki_in, Kp_in;

        NU32_WriteUART3("Enter your desired integral current gain Ki [recommended: __ ]: \r\n");
        NU32_ReadUART3(buffer, BUF_SIZE);
        sscanf(buffer, "%d", &Ki_in); // accept Ki, store in Ki stored in currentcontrol.h

        NU32_WriteUART3("Enter your desired position current gain Kp [recommended: __ ]: \r\n");
        NU32_ReadUART3(buffer, BUF_SIZE);
        sscanf(buffer, "%d", &Kp_in); // accept Kp
      
        // set_current_gains(Ki_in,Kp_in);
        break;
      } // end case g

      case 'h':{// get current gains
        sprintf(buffer, "%d %d\n", Ki_in, Kp_in); 
        NU32_WriteUART3(buffer);
        break;
        
      }
      case 'k':{ // test current gains
        int jj;
        StoringData = 1;      
        if (StoringData) {

          set_mode(ITEST);

        } // end if

          NU32_WriteUART3("100\r\n"); // send 100 samples to Matlab

          // send samples in pairs
          for (jj = 0; jj < NUMSAMPS; jj++){
              sprintf(buffer, "%d %d\r\n", Waveform[jj], current_response[jj]);
              NU32_WriteUART3(buffer);
            } // end for


        break;
      } // end case k

      case 'p': { // unpower the motor
        set_mode(IDLE); 
        NU32_WriteUART3("The motor has been unpowered.\r\n");
        break;
      }

      case 'q':
      { // handle q for quit. Later you may want to return to IDLE mode here.
        NU32_WriteUART3("Exiting...\r\n"); 
        set_mode(IDLE);
        break;
      }
      case 'r':
      { // return get mode value
        mode current_mode = get_mode();

        // sprintf(buffer, "The current mode is %d\r\n", current_mode);
        sprintf(buffer,"%d\n",current_mode);
        NU32_WriteUART3(buffer);
        break;
        
      }

      case 'x':
      {
        int a, b, c;
        NU32_ReadUART3(buffer, BUF_SIZE);
        sscanf(buffer, "%d\n", &a); // accept two integers a & b

        NU32_ReadUART3(buffer,BUF_SIZE);
        sscanf(buffer, "%d\n", &b);

        c = a + b;
      
        sprintf(buffer,"%d\r\n", c); // sum the integers & return the value
        NU32_WriteUART3(buffer);
        break;
        
      }

      default:
      {
        NU32_LED2 = 0;  // turn on LED2 to indicate an error
        break;
      }
    }
  }
  return 0;
}


