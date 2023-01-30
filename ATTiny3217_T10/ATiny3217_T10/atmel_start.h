#ifndef ATMEL_START_H_INCLUDED
#define ATMEL_START_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "include/driver_init.h"
#include "include/atmel_start_pins.h"

#include "touch.h"

/**
 * Initializes MCU, drivers and middleware in the project
 **/
void atmel_start_init(void);
void rgb_button(int i);
void write_data(char *buffer,int num_char, int position) ;
void read_data(void);
void touch_detected(int i);
void slider(int i);
void slider_status(int i);

#ifdef __cplusplus
}
#endif
#endif
