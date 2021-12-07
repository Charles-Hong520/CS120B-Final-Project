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
    for(unsigned char p = 0; p < 10; p++) asm("nop");
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

*/
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

// unsigned char i;
// unsigned char arr[6] = {'A', 'B', 'C', 'D', 'E',1};
// unsigned char arrsize = 6;
// enum State {start, press, release} state;
// int Tick(int state) {
//     switch(state) {
//         case start:
//         state = release;
//         // i=eeprom_r();
//         i=eeprom_read_byte(0x5F);
//         if(i>arrsize) i=0;
//         break;
//         case press:
//         if((~PINA&0x04)==0x04) {
//             state = press;
//         } else {
//             state = release;
//         }
//         break;
//         case release:
//         if((~PINA&0x04)==0x04) {
//             state = press;
//             i = (i+1)%arrsize;
//             eeprom_write_byte(0x5F,i);
// //some reason, so I just used the library
//             // eeprom_w(i);
//         }
//     }
//     return state;
// }

unsigned short x, y;
int JoystickTick(int state) {
    x=ADC_Channel(0x01);    
    y=ADC_Channel(0x00); y = 1023-y;

    //display values of x and y direction
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

    return state;
}
#define SER 0
#define RCLK 1
#define SRCLK 2
#define SET_BIT(p,i) ((p) |= (1 << (i)))
#define CLR_BIT(p,i) ((p) &= ~(1 << (i)))
#define GET_BIT(p,i) ((p) & (1 << (i)))
unsigned char smile[] = {129, 36, 36, 36, 0, 66, 60, 129};
unsigned char k = 0;
int LEDMatrixTick(int state) {
        unsigned char val = ~smile[k];
        unsigned char temp;

        //store column
        temp = 1<<k;
        for(unsigned char j = 0; j < 8; j++) {    
            //store value
            if(temp&1) SET_BIT(PORTB, SER);
            else CLR_BIT(PORTB,SER);

            //pulse serial clock
            SET_BIT(PORTB,SRCLK);
            CLR_BIT(PORTB,SRCLK);

            //shift temp over
            temp >>= 1;
        }

        //store row actual color
        temp = val;
        for(unsigned char j = 0; j < 8; j++) {

            //store value
            if(temp&1) SET_BIT(PORTB, SER);
            else CLR_BIT(PORTB,SER);

            //pulse serial clock
            SET_BIT(PORTB,SRCLK);
            CLR_BIT(PORTB,SRCLK);

            //shift value of val over
            temp >>= 1;
        }

        //pulse the latch
        SET_BIT(PORTB,RCLK);
        CLR_BIT(PORTB,RCLK);
    k++;
    if(k==8) k=0;
    return state;
}

int main(void) {
    DDRA = 0xF0; PORTA = 0x0F; 
    DDRB = 0xFF; PORTB = 0x00; 
    DDRC = 0xFF; PORTC = 0x00; 
    DDRD = 0xFF; PORTD = 0x00; 
    unsigned long GCDPeriod = 1;
    TimerOn();
    TimerSet(GCDPeriod);

    LCD_init();
    LCD_CreateCustom();
    LCD_ClearScreen();

    LCD_Cursor(17);
    LCD_WriteData(1);
    LCD_Cursor(18);
    LCD_WriteData(2);
    LCD_Cursor(19);
    LCD_WriteData(3);

    ADC_init();

    unsigned char i = 0;
    tasks[i].state = 0;
    tasks[i].period = 200;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &JoystickTick;
    i++;
    tasks[i].state = 0;
    tasks[i].period = 2;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &LEDMatrixTick;

    while(1) {
        for(i = 0; i < numTasks; i++) {
            if(tasks[i].elapsedTime >= tasks[i].period) {
                tasks[i].state = tasks[i].TickFct(tasks[i].state);
                tasks[i].elapsedTime = 0;
            }
            tasks[i].elapsedTime += GCDPeriod;
        }
        while(!TimerFlag);
        TimerFlag = 0;
    }
    return 1;
}
