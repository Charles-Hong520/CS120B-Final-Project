/*	Author: Charles Hong
 *  Partner(s) Name: N/A
 *	Lab Section: 022
 *	Assignment: Final Custom Project
 *	Exercise Description: Joystick
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 *  
 *  Demo Link: https://drive.google.com/file/d/1cbfGRmzglm8k5zVxBTdjddRwTSf8hf61/view?usp=sharing
 */

#include <avr/io.h>
// #include <avr/eeprom.h>
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

unsigned short ADC_Channel(unsigned char ch) {
    ADMUX = ch;
    for(unsigned char p = 0; p < 30; p++) asm("nop");
    return ADC;
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


/*

my own EEPROM read and write function doesn't work for
some reason, so I just used the library

*/
void eeprom_w(unsigned char val) {
    while(EECR & (1<<EEPE));
    EEAR = 0x5F;
    EEDR=val;
    EECR |= (1<<EEMPE);
    EECR |= (1<<EEPE);
}

unsigned char eeprom_r() {

    while(EECR & (1<<EEPE));
    EEAR = 0x5F;
    EECR |= (1<<EERE);
    return EEDR;
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
        // i=eeprom_r();
        i=eeprom_read_byte(0x5F);
        if(i>arrsize) i=0;
        break;
        case press:
        if((~PINA&0x04)==0x04) {
            state = press;
        } else {
            state = release;
        }
        break;
        case release:
        if((~PINA&0x04)==0x04) {
            state = press;
            i = (i+1)%arrsize;
            eeprom_write_byte(0x5F,i);
//some reason, so I just used the library
            // eeprom_w(i);
        }
    }

}

unsigned short x, y;
void TickJ() {
    x=ADC_Channel(0x01);
    
    y=ADC_Channel(0x00);
    unsigned char tmpB = 0;
    if(x<384) tmpB |= 1<<1;
    else if(x>640) tmpB |= 1<<0;
    if(y<384) tmpB |= 1<<3;
    else if(y>640) tmpB |= 1<<2;

    for(unsigned char j = 0; j < 4; j++) {
        LCD_Cursor(8-j);
        LCD_WriteData('0'+x%10);
        x/=10;
    }
    for(unsigned char j = 0; j < 4; j++) {
        LCD_Cursor(16-j);
        LCD_WriteData('0'+y%10);
        y/=10;
    }
    LCD_Cursor(32);

    PORTB = tmpB;
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

    LCD_Cursor(3);
    LCD_WriteData('X');
    LCD_Cursor(4);
    LCD_WriteData(':');

    LCD_Cursor(11);
    LCD_WriteData('Y');
    LCD_Cursor(12);
    LCD_WriteData(':');
    ADC_init();
    while(1) {
        Tick();
        LCD_Cursor(1);
        LCD_WriteData(arr[i]);
        TickJ();
        while(!TimerFlag);
        TimerFlag = 0;
    }
    return 1;
}
