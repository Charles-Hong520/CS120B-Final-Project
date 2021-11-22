/*	Author: Charles Hong
 *  Partner(s) Name: N/A
 *	Lab Section: 022
 *	Assignment: Final Custom Project
 *	Exercise Description: 2 Axis Joystick
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 *  
 *  Demo Link: 
 */

#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#include "io.h"
#endif
volatile unsigned char TimerFlag = 0; //TimerISR sets it to 1, programmer sets it to 0
unsigned long _avr_timer_M = 1; //start count from here, down to 0. Default 1ms
unsigned long _avr_timer_cntcurr = 0; //current internal count of 1ms ticks

void TimerOn() {
    TCCR1B = 0x0B;
    OCR1A = 125;
    TIMSK1 = 0x02;
    TCNT1  = 0;
    _avr_timer_cntcurr = _avr_timer_M;
    SREG |= 0x80;
}

void TimerOff() {
    TCCR1B = 0x00;
}

void TimerISR() {
    TimerFlag = 1;
}

ISR(TIMER1_COMPA_vect) {
    _avr_timer_cntcurr--;
    if(_avr_timer_cntcurr == 0) {
        TimerISR();
        _avr_timer_cntcurr = _avr_timer_M;
    }
}

void TimerSet(unsigned long M) {
    _avr_timer_M = M;
    _avr_timer_cntcurr = _avr_timer_M;
}
unsigned char GetBit(unsigned char port, unsigned char number) 
{
    return ( port & (0x01 << number) );
}
void ADC_init() {
    ADCSRA |= (1<<ADEN) | (1<<ADSC) | (1<<ADATE);
    //ADEN: setting this bit enables analog to digital conversion
    //ADSC: setting this bit starts the first conversion
    //ADATE:    setting this bit enables auto-triggering. Since we are
    //          in Free Running Mode, a new conversion will trigger 
    //          whenever the previous conversion completes.
}

typedef struct _task{
    // Tasks should have members that include: state, period,
    //a measurement of elapsed time, and a function pointer.
    signed   char state;        //Task's current state
    unsigned long period;       //Task period
    unsigned long elapsedTime;  //Time elapsed since last task tick
    int (*TickFct)(int);        //Task tick function
} task;
unsigned char numTasks = 2;
task tasks[2];
#define EEWE 1
#define EEMWE 2
#define EERE 0
void eeprom_w(unsigned char val) {
    while(EECR & (1<<EEWE));
    EEAR = 0x5F;
    EEDR=val;
    EECR |= (1<<EEMWE);
    EECR |= (1<<EEWE);
}

unsigned char eeprom_r() {
    unsigned char s;
    while(EECR & (1<<EEWE));
    EEAR = 0x5F;
    EECR |= (1<<EERE);
    s=EEDR;
    return s;
}

unsigned char customCharacter[3][8] = {
   {0x0E, 0x1F, 0x11, 0x11, 0x1F, 0x1F, 0x1F, 0x1B},
   {0x00, 0x0A, 0x0A, 0x00, 0x11, 0x0E, 0x00, 0x00},
   {0x03, 0x06, 0x0C, 0x0F, 0x1E, 0x06, 0x0C, 0x18}
};
void LCD_CreateCustom() {
    for(unsigned z = 1; z <= 3; z++) {
        LCD_WriteCommand(0x40+z*8);
        for(unsigned char k = 0; k < 8; k++) {
            LCD_WriteData(customCharacter[z-1][k]);
        }
    }
    LCD_WriteCommand(0x80);
}

unsigned char e;
unsigned char i;
unsigned char arr[6] = {'A', 'B', 'C', 'D', 'E',1};
unsigned char arrsize = 6;
enum State {start, press, release} state;
void Tick() {
    switch(state) {
        case start:
        state = release;
        i=eeprom_read_byte(0x5F);
        if(i>arrsize) i=0;
        break;
        case press:
        if((~PINA&0x01)==0x01) {
            state = press;
        } else {
            state = release;
        }
        break;
        case release:
        if((~PINA&0x01)==0x01) {
            state = press;
            i = (i+1)%arrsize;
            eeprom_write_byte(0x5F,i);
        }
    }

}

int main(void) {
    DDRA = 0x00; PORTA = 0xFF; 
    DDRB = 0xFF; PORTB = 0x00; 
    DDRC = 0xFF; PORTC = 0x00; 
    DDRD = 0xFF; PORTD = 0x00; 

    TimerOn();
    TimerSet(200);

    LCD_init();
    LCD_CreateCustom();
    LCD_ClearScreen();

    LCD_Cursor(17);
    LCD_WriteData(1);

    LCD_Cursor(18);
    LCD_WriteData(2);
    LCD_Cursor(19);
    LCD_WriteData(3);
    while(1) {
        Tick();
        LCD_Cursor(1);
        LCD_WriteData(arr[i]);
        while(!TimerFlag);
        TimerFlag = 0;
    }
    return 1;
}
