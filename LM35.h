/*
 * LM35.h
 *
 * Created: 19/04/2026 10:54:45 PM
 *  Author: Alaa
 */ 

#ifndef LM35_H_
#define LM35_H_

#include <stdint.h>

#define TEMP_THRESHOLD 25    /* degrees Celsius */

void LM35_init(void);
uint8_t LM35_read(void);

#endif /* LM35_H_ */