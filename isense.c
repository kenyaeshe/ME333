#include "isense.h"

// ADC initialization: manual sampling & conversion
// talked to Dan - could do the ADC either way but i will do automatic sampling & conversion
// take the average of about 5 samples to mitigate noise

void ADC_init(void){
  AD1PCFGbits.PCFG0 = 0;                    // AN0 is an adc pin
  AD1CON1bits.SSRC = 0b111;                 // sets to automatic conversion
  AD1CON3bits.ADCS = 2;                     // ADC clock period is Tad = 2*(ADCS+1)*Tpb = 2*3*12.5ns = 75ns                         
  AD1CON3bits.SAMC = 20;                    // set sampling time to 2 Tad
  AD1CON1bits.ON = 1;                       // enable the ADC pin AN0 
}

// automatic sampling, automatic conversion
int ADC_sample(int num){ 
    int ADC_count_total = 0, avg_ADC_count; // had previously defined as unsigned int
    int ii;

    for (ii = 0; ii < 5; ii++){
        AD1CHSbits.CH0SA = 0;                     // sets AN0 as positive input to MUX A
       
        AD1CON1bits.SAMP = 1;                   // begin manual sampling

        while (!AD1CON1bits.DONE){
        ;                                   // wait for conversion process to finish
        }

        ADC_count_total += ADC1BUF0;            // add the current ADC count to that obtained from sampling
    }
   
    avg_ADC_count = ADC_count_total/5;      // find the average and return

    if (num == 0){
        return avg_ADC_count;                   // return average ADC count
    }
    else if (num == 1){
        return (int) ((avg_ADC_count * 3.84) - 1931.30);
    }

    
   
}

// return the current in mA
// least squares fit is: y = 3.84x + -1931.30
// double ADC_current(unsigned int ADC_counts){
    
//     // return (((double) ADC_sample() *3.84) - 1931.30);
//     return ((ADC_counts * 3.84) - 1931.30);
//     // return ((avg_ADC_count*3.84) - 1931.30);
//     // return value;
   
// }

 