#ifndef NU32__H_
#define NU32__H_

typedef enum mode {
    // IDLE,
    // PWM,
    // ITEST,
    // HOLD,
    // TRACK
    IDLE,
    PWM,
    ITEST,
    HOLD, 
    TRACK
    // F // "follow" ie TRACK

} mode;

// declare a variable Mode of type mode
volatile mode Mode;

void set_mode(mode new_mode);
mode get_mode(void);

#endif // for NU32__H_