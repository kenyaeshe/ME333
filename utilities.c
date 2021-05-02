#include "utilities.h"

// volatile mode Mode;

void set_mode(mode new_mode){
    Mode = new_mode;
}

mode get_mode(void){
    return Mode;
}
