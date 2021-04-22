#ifndef __display_h__
#define __display_h__

// ----------------------------------------------------------------------------
//   LCD functions
// ----------------------------------------------------------------------------
#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define BIT0 1
#define BIT1 2
#define BIT2 4
#define BIT3 8
#define BIT4 16
#define BIT5 32
#define BIT6 64
#define BIT7 128

#define U8 unsigned char
#define I8   signed char
#define U16 unsigned int
#define I16   signed int
#define I32   signed long 
#define U32 unsigned long 


void lcdInit(void);
void clearLcd(void);
void clearFullLcd(void);

void home1Lcd(void); //Set cursor at start of Line 1
void home2Lcd(void); //Set cursor at start of Line 2
void lcdWrite(U8 b);  //write one character
void lcdWrite9(U8 n); //Write one digit. Allowed are values from 0..9

void sendStringToLCD(U8 *str);     //Send immediately


#endif
//EOF

