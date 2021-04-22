// ----------------------------------------------------------------------------
//   LCD functions - 4 bit version (2 zeilig)
// ----------------------------------------------------------------------------
#include "Display2x8.h"



#define  LCD_ENABLE    {PORTD |= BIT2;}
#define  LCD_DISABLE   {PORTD &= ~BIT2;} 
#define  LCD_DATA      PORTD
#define  LCD_RS_ON     PORTD |=  BIT3
#define  LCD_RS_OFF    PORTD &= ~BIT3

// ---------------------------------------------------------------------------
/*! Send nibble to display.
    Depending on the value of LCD_RS the byte is written to display(LCD_RS=1) or register (LCD_RS=0)

   \param b is the nibble to be send. Only bits 4..7 are used
*/// -------------------------------------------------------------------------
void lcdWriteNibble(U8 b) { //b: only Bits 4..7 are used
      asm volatile ("nop"); // 1 NOP bei 8MHz braucht etwa  125ns.  t_AS: 40ns min 
      LCD_ENABLE;
      LCD_DATA = (LCD_DATA & 0x0F) | (b & 0xf0); 
      _delay_us(4); //t_DSW: 80ns min.
      LCD_DISABLE;
      //t_H: 10ns min
      //t_CYC 500ns min
      _delay_us(2);
}

/*! Send byte to display.
    Depending on the value of LCD_RS the byte is written to display(LCD_RS=1) or register (LCD_RS=0)

   \param b is the byte to be send.
   \note the BUSY-Flag isn't checked (since we can't read from display with R/W hard-wired to GND)
*/// -------------------------------------------------------------------------
void lcdWrite(U8 b) { 
  lcdWriteNibble(b);
  lcdWriteNibble(b<<4);
  //Delay >37µs  
  _delay_us(50);
  // CAUTION: we do not check the BUSY-Flag (since we can't read from display)
  // CAUTION: so for HOME and CLEAR commands the user has to add 2ms delay!
}

// ---------------------------------------------------------------------------
void lcdWrite9(U8 n){ //Write one digit. Allowed are values from 0..9
  lcdWrite(48+n);
}

// ---------------------------------------------------------------------------
void cmd2Lcd(U8 cmd) {
  LCD_RS_OFF;
  _delay_us(80);
  lcdWrite(cmd);
  LCD_RS_ON;
  _delay_us(2050);
}
// ---------------------------------------------------------------------------
/*! Clear LCD and set cursor home
   (HD 44780 based displays only)
*/// -------------------------------------------------------------------------

void clearLcd(void) {
  cmd2Lcd(0x01);
}


// ---------------------------------------------------------------------------
/*! Set cursor home in line 1
   (HD 44780 based displays only)
*/// -------------------------------------------------------------------------
void home1Lcd(void) {
  cmd2Lcd(2);
}
// ---------------------------------------------------------------------------
/*! Set cursor at start of Line 2
   (HD 44780 based displays only)
*/// -------------------------------------------------------------------------
void home2Lcd(void) {
  cmd2Lcd(192);
}


// ---------------------------------------------------------------------------
/*! Initialize display (2 Lines, 4 Bit, no shift, increment)
   (HD 44780 based displays only)
*/// -------------------------------------------------------------------------
void lcdInit(void) {

  LCD_RS_OFF;
  _delay_us(300);
  LCD_ENABLE;

  //Delay >15ms - passierte eigentlich bereits in cldInit
  _delay_ms(20);
  
  lcdWriteNibble(0x30); // DB5=1 DB4=1        # see BATRON-Specification for HD44780, page 58
  _delay_ms(15);    // wait more than 4.1ms

  lcdWriteNibble(0x30); // DB5=1 DB4=1
  _delay_ms(15);    // wait more than 100us

  lcdWriteNibble(0x30); // DB5=1 DB4=1
  _delay_ms(15);    // wait more than 100us


  lcdWrite(0x28); //2 Lines, 4Bit Data,   0 0 1 DL  N F * *  DL:0=4Bit, 1=8Bit  n:0=1Zeile, 1:2Zeilen  F:0=5x7, 1=5x11 Pixel
  _delay_us(1000);    //just for paranoia
  lcdWrite(0x28); //2 Lines, 4Bit Data,  
  _delay_us(1000);    //just for paranoia

  lcdWrite(0x08); // Display off
  _delay_us(1000);    //just for paranoia  

  lcdWrite(0x0c); // Display on, Cursor + Blink off
  _delay_us(1000);    //just for paranoia  
/*
  lcdWrite(0x01); // Display clear
  _delay_ms(3);
*/
  lcdWrite(0x06); // Entry mode; no shift
  _delay_us(2000);
  
  LCD_RS_ON;
}

//----- String sofort senden ----------------------------------------------------------
void sendStringToLCD(U8 *str)
{
  U8 i;

  //Zeichen fuer Zeichen senden (ohne abschliessende '\0')
  i = 0;
  while (str[i] !=0)
  {
    lcdWrite(str[i]);
    i++;
    // mehr als 8 Zeichen werden nie an das Display gesendet
    if (i > 7){
      break;
    }
  }
}
