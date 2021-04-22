
/*
 Realisiert mit AtMega8
 
 Version 1.0 vom 27.04.2012 
 
 
 Interner RC Oszillator mit 1MHz  (Teiler/8 eingeschaltet)

                     (Reset) PC6 1  28 PC5   U_PB3            ADC5
                       Hold  PD0 2  27 PC4   U_PB2            ADC4
                         TX  PD1 3  26 PC3   U_PB1            ADC3
                  LCD_Enable PD2 4  25 PC2   U_PB6            ADC2
                  LCD_RS     PD3 5  24 PC1   U_PB0            ADC1
                  LCD_Data4  PD4 6  23 PC0   Ux Messspannung  ADC0
                             VCC 7  22 GND
                             GND 8  21 AREF
                       R150  PB6 9  20 AVCC
                             PB7 10 19 PB5   (SCK)   R2M2
                   LCD_Data5 PD5 11 18 PB4   (MISO)  R330K
                   LCD_Data6 PD6 12 17 PB3   (MOSI)  R47K
                   LCD_Data7 PD7 13 16 PB2  R5K6 
                       R125  PB0 14 15 PB1  R820                                          

Spannungsversorgung: 5.0V (über LM78L05 oder TLE4267)

R125: R110+R15   Ähnliche Werte gehen auch. Der ATmega kann 40mA je Portpin. Das entspricht 125Ohm bei 5.0V
           

 I_R125 = Ix = (U_PB0 - Ux)/125
 
 Rx = Ux/Ix  = Ux *125 / (U_PB0 - Ux)
 
 
 Rx = Ux *125 / (U_PB0 - Ux)  
 U1=U_PB0
 
           
*/

#define F_CPU 8000000UL	/* CPU clock in Hertz */
#include <inttypes.h>
#include <avr/io.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "Display2x8.h"

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


//--------------------------------------------------------
// Global Var
//--------------------------------------------------------


U8    g_mask[7]={BIT5, 
                BIT4, 
                BIT3,
                BIT2,
                BIT1,
                BIT6,
                BIT0,
                };

float g_Rref[7]={2200000.0,  //0
                 330000.0,  
                  47000.0,   //2
                   5600.0,
                    820.0,   //4
                    150.0, 
                    125.0    //6
              };
U8 g_mux[7]={0,0,5,4,3,2,1};

U16 g_inaccuracy;
volatile U16 g_powerdown;

//--------------------------------------------------------
U16 getAdc(void){
//--------------------------------------------------------
U8  n;
U16 sum;
U16 a;
U16 b;
U16 cnt;

  a=9999;
  cnt=100;
  do{ //bis zu 100mal messen, bis 2x hintereinander der gleiche Wert rauskommt 
    b=a;
    ADCSRA |= (1<<ADSC);
    while (ADCSRA & (1<<ADSC))
    ;
    a=ADC;
  }while ((a!=b) && (--cnt));
  
  //Mittelwert über 32 Messwerte
  sum=0;
  for (n=0; n<32; n++){
    ADCSRA |= (1<<ADSC);
    while (ADCSRA & (1<<ADSC))
    ;
    sum+=ADC;
  }
  return sum; //0..32736
}

//--------------------------------------------------------
float measureRx(U8 n){  // n ist der Index für den Referenzwiderstand. Setzt g_inaccuracy.
//--------------------------------------------------------
U16 U0;
U16 Ux;
float U1;
float U2;
float Rx, R1,R2;
float Uxx;
float Ixx;


  if (n<7){
    //select reference resistor
    PORTB=g_mask[n]; //Entweder ein Portpin ist High (Ausgang) oder hochohmig (Eingang).
    DDRB =g_mask[n];   
 
    if (g_mux[n]>0){
      ADMUX = (1<<REFS0) | g_mux[n]; //Uref= Vcc. Channel mux[n]  (U0)
      _delay_ms(1); 
      U0 = getAdc();
    }else{ //Fuer die hohen Widerstandswerte (330K, 2.2M) spielt der Innenwiderstand des Ports keine Rolle mehr.
           //Als U0 wird daher die Referenzspannung angenommen, also 32*1023=32736
      U0 = 32736;     
    }
    ADMUX = (1<<REFS0); //Uref= Vcc. Channel ADC0 (Ux)
    _delay_ms(5);  //Gib dem S+H Kondensator Zeit zum laden
    Ux = getAdc();

    DDRB=0; //Referenzwiderstände abschalten (Strom sparen)
    PORTB=0;
  
    if (Ux >= 16368){  // 16*1023 (entspricht Uref/2)
      g_inaccuracy = Ux - 16368;
    }else{
      g_inaccuracy = 16368 - Ux;
    }
 
    if (U0!=Ux){ //Division durch Null vermeiden
      // Testwiderstand: Rx  Referenzwiderstand: Rref
      // Rx = Ux/Ix 
      //    Strom durch Rx und Rref ist gleich:
      // Ix=Irref
      // Irref = Urref /Rref
      //    mit  Urref=U0-Ux  Spannung über Referenzwiderstand
      //    gibt das
      //Irref = (U0-Ux) / Rref
      //    damit ist der Testwiderstand:
      //Rx = Ux /((U0-Ux) / Rref)
      //    etwas Umgeformt:
      //Rx = Ux * Rref / (U0 - Ux);
      Rx = Ux;
      Rx *= g_Rref[n];
      Rx /= U0 - Ux;
      return Rx; 
    }else{
      return 90000000.0; //90M - quasi unendlich
    }
  }else{//Spezialmessung mit 2 parallelen Referenzwiderständen
    PORTB=BIT0|BIT6; // 125+150 Ohm einschalten
    DDRB =BIT0|BIT6;
    
    ADMUX = (1<<REFS0) | 1; //Uref= Vcc. Channel 1 (U über 125R)
    _delay_ms(1); 
    U1 = getAdc();

    ADMUX = (1<<REFS0) | 2; //Uref= Vcc. Channel 2 (U über 150R)
    _delay_ms(1); 
    U2 = getAdc();

    ADMUX = (1<<REFS0); //Uref= Vcc. Channel ADC0 (Ux)
    _delay_ms(1);  //Gib dem S+H Kondensator Zeit zum laden
    Uxx = getAdc();

    DDRB=0; //Referenzwiderstände abschalten (Strom sparen)
    PORTB=0;
  
    if (Uxx >= 16368){  // 16*1023 (entspricht Uref/2)
      g_inaccuracy = Uxx - 16368;
    }else{
      g_inaccuracy = 16368 - Uxx;
    }
    R1=g_Rref[6]; //125R
    R2=g_Rref[5]; //150R


    Ixx = (U1-Uxx)/R1 + (U2-Uxx)/R2;  //Ixx = I1 + I2
    if (0!=Ixx){
      Rx = Uxx  / Ixx;
      return Rx; 
    }else{
      return 90000000.0; //90M - quasi unendlich
    }
  }
}

//--------------------------------------------------------
I32 rasterung(float r){
//--------------------------------------------------------
static U16 e24[24]={10, 11, 12, 13, 15, 16,
             18, 20, 22, 24, 27, 30,
             33, 36, 39, 43, 47, 51, 
             56, 62, 68, 75, 82, 91};
I32 m;
I32 val;
float diff;
float diffmin=10000000L;  //10M
I32 rr=0;
U8 dekade;
U8 i;

  m=1;
  for (dekade=1; dekade<7; dekade++){
    for (i=0; i<24; i++){
      val=e24[i] * m;
      diff = val - r;
      if (diff<0) diff=0-diff;
      if (diff < diffmin){
        diffmin=diff;
        rr=val;
      }
    }
    m*=10;
  }
  return rr;
}

//--------------------------------------------------------
void displayResult(float y){
//--------------------------------------------------------
U8  c;
U16 num;
U32 frac;
U16 l;
U32 n;
U8 leading0;
//U8 line[8];
U32 div;
U32 r;
U8 i;
U8 z;
U16 mult;
U8  dotpos;
float yy;
U8 cnt;

   if (y<10){
     yy = y*10;
     mult=10;
   }else{
     yy=y;
     mult=1;
   }

   r=rasterung(yy);


/*Typische Ausgaben:
    1R   3R3     9R1
   10R  47R    910R
    1K   1K2     9K1
   10K 220K    910K         
    1M   4M7     9M1
*/
  //Reduzieren auf 4 signifikante Stellen
  if (y<10000000){//10M
    g_powerdown=360; //ca. 10,5s
    if (r>=1000000){ //1M
      c='M';
      num =r/1000000;  //1..9M
      frac=r%1000000;  
      l=frac/100000;   //eine Nachkommastelle
    }else    
    if (r>=1000){ //1K
      c='K';
      num=r/1000;
      frac=r%1000;
      l=frac/100;    //eine Nachkommastelle
    }else
    if (1==mult){ 
      c='R';
      num=r;
      l=0;
    }else{//Mult ==10
      c='R';
      num=r/10;
      l=r%10;
    }
    home1Lcd();
    lcdWrite(' ');
   //-- schreibe Kurzform, z.B.  4K7 -------------
    n=num;
    leading0 = 1;
    div=100;
    for (i=0; i<3; i++){
      z=n/div;
      n%=div;
      div/=10;
      if ((z>0) || (0==leading0)){
        lcdWrite9(z);
        leading0=0;
      }else{
        lcdWrite(' ');
      }
    }
    lcdWrite(c);
    if (l>0){
      lcdWrite9(l);
    }else{
      lcdWrite(' ');
    }

    home2Lcd();
    

   //-- schreibe Messwert, z.B.  4697 -------------

    if (y<10){
      n=y*100;
      dotpos=5;
    }else
    if (y<100){
      n=y*10;
      dotpos=6;
    }else{
      n=y;
      dotpos=7;
    }
    leading0 = 1;
    div=1000000;
    cnt=0;
    for (i=0; i<7; i++){
      if (dotpos==i) lcdWrite(',');
      z=n/div;
      n%=div;
      div/=10;
      if ((z>0) || (0==leading0) ||(i>=(dotpos-1))){
        if (cnt++ < 4){ //Nur 4 signifikante Stellen
          lcdWrite9(z);
        }else{
          lcdWrite('0');
        }
        leading0=0;
      }else{
        if ((1==mult) || (i>1)) lcdWrite(' ');
      }
    }
    _delay_ms(150);
  }else{
    if (36==g_powerdown) lcdInit();
    clearLcd();
                        //12345678
    sendStringToLCD((U8*)"  ---");
    _delay_ms(60);
  }
}

//--------------------------------------------------------
ISR(TIMER0_OVF_vect){
//--------------------------------------------------------
  if (g_powerdown){
    g_powerdown--;
  }else{
    //PORTD|=BIT0; //HOLD auf H schaltet Spannungsregler ab
	PORTD &= ~(1 << PD0); //umgekehrte Logik
  }
}

//--------------------------------------------------------
int main(void){
//--------------------------------------------------------

float Rx;
float bestRx=99000111; //99M

U16 inaccuracy_min;
U8 i;

  DDRB= BIT0| BIT1 | BIT2 | BIT3 | BIT4 | BIT5 | BIT6;
  DDRC= 0;
  DDRD= BIT0| BIT1 | BIT2 | BIT3 | BIT4 | BIT5 | BIT6 | BIT7;

  //PORTD=0; //HOLD fuer den TLE4267 auf GND
  PORTD |= (1 << PD0);   //HOLD auf H schaltet


/*
  // ADC at 125KHz, 
  ADCSRA = (1<<ADEN)|(1<<ADPS1)|(1<<ADPS0);  // 1MHz/8
  ADCSRB = 0; //Free running 		
  ADMUX = (1<<REFS0)|14; //Uref= Vcc. Channel 14: interne 1.1V (Vbg)
  DIDR0 = BIT0|BIT1|BIT2|BIT3|BIT4|BIT5; //Digital inputs disabled
 



  TCCR0A = 0; 
  TCCR0B = (1<<CS02)|(1<<CS00); //1MHz/1024=976Hz
  TIMSK0 = (1<<TOIE0); //Timer overflow every 262ms (3.81Hz) 
 */

  // ADC at 125KHz, 
  ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1);  // 8MHz/64
  //ADCSRB = 0; //Free running 		
  SFIOR=(0<<ACME);
  ADMUX = (1<<REFS0)|14; //Uref= Vcc. Channel 14: 
  //DIDR0 = BIT0|BIT1|BIT2|BIT3|BIT4|BIT5; //Digital inputs disabled
 



  TCCR0 = 0; 
  TCCR0 = (1<<CS02)|(1<<CS00); //1MHz/1024=976Hz
  TIMSK = (1<<TOIE0); //Timer overflow every 262ms (3.81Hz) 
  
      
     
  g_powerdown=40; //ca. 10,5s
  g_powerdown=360; //ca. 10,5s
  
  lcdInit();
  sei();  //Interrupts an

  //Messe Versorgungsspannung bis mind. 4.8V erreicht
  // Bei 4,8V=Uref liefert der ADC 1,1/4,8*1023*32=7502 
  // Bei 4,9V=Uref liefert der ADC 1,1/4,9*1023*32=7348 
  // Bei 5.0V=Uref liefert der ADC 1,1/5,0*1023*32=7201
  
   if (getAdc() > 8870){
    sendStringToLCD((U8*)"Low Batt");
  }
  //Falls 4.8V nicht erreicht werden, schaltet der Timer nach 10s ab.
  while (getAdc() > 8870)
  ; //Do nothing  - wait for power

  lcdInit(); //Nochmal initialisieren, da jetzt erst die Versorgungssannung optimal ist


  /*
     Algorithmus:
     Der Prüfling zieht den Meßspannungseingang auf GND.
     Nacheinander werden 8 verschiedene Pull-Up Widerstände eingeschaltet
     und die Spannung über dem Prüfling und über dem Referenzwiderstand 32 
     mal gemessen. Das ist notwendig, da über dem  Innenwiderstand der 
     Ports z.B. bei 330R Testwiderstand glatt 200mV abfallen.
     
     Derjenige Meßwert der am nächsten an 512*32   (512=Uref/2) ist, müßte der genaueste 
     Messwert sein. Der wird dann angezeigt.


     Spannungsversorgung: 
     Ein Taster schaltet die Eingangsspannung auf  E2 (Inhibit), wodurch der TLE4267 startet.
     Der Prozessor legt E6 (Hold) mit PD0 auf GND, wodurch der TLE4267 eingeschaltet bleibt,
     auch wenn der Taster losgelassen wird.
     Nach 10s ohne Messung wird PD0 aug H gelegt, was den Spannungsregler abschaltet.
     Die Schaltung braucht im Betrieb (ohne Rx) ca. 4,2mA. 
     Mit ausgeschaltetem Spannungsregler unter 1µA.

  */

  while(1){
    inaccuracy_min = 0xffff;
    for (i=0; i<8; i++){
      Rx = measureRx(i);
      if (g_inaccuracy <= inaccuracy_min){
        inaccuracy_min = g_inaccuracy;
        bestRx = Rx;
      }
    }
    displayResult(bestRx);
  }//while(1)
}
