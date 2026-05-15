/*
 * UART.h
 *
 * Created: 17/04/2026 5:19:15 PM
 *  Author: Alaa
 */ 


#ifndef UART_H_
#define UART_H_
#define F_CPU 8000000UL  


#include <stdint.h>

void UART_init(uint32_t baud);
void UART_sendChar(char c);
void UART_sendString(const char *str);
void uart_print_number(uint8_t num);
#endif /* UART_H_ */