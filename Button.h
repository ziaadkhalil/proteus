/*
 * Button.h
 *
 * Created: 17/04/2026 5:21:14 PM
 *  Author: Alaa
 */ 


#ifndef BUTTON_H_
#define BUTTON_H_

#include <stdint.h>

#define BTN_PIN PB5

void BUTTON_init(void);
uint8_t BUTTON_read(void);



#endif /* BUTTON_H_ */