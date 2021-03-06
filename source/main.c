/*	Author: Charles Hong
 *  Partner(s) Name: N/A
 *	Lab Section: 022
 *	Assignment: Final Custom Project
 *	Exercise Description: Final Demo
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 *  
 *  Demo Link: https://drive.google.com/file/d/1gdwfyb-8gH2mDGFBX-W8k5T85cCem0cX/view?usp=sharing
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
unsigned char numTasks = 4;
task tasks[4];


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
unsigned char currMaze = 0;

unsigned char maze[4][8] = {
                        {1, 117, 20, 214, 16, 87, 81, 92},
                        {8, 106, 42, 238, 8, 226, 79, 0},
                        {0, 36, 36, 36, 0, 66, 60, 0},
                        {0, 36, 36, 36, 0, 66, 60, 0}
                    };

unsigned char customCharacter[3][8] = {
   {0x0E, 0x1F, 0x11, 0x11, 0x1F, 0x1F, 0x1F, 0x1B},
   {0x00, 0x0A, 0x0A, 0x00, 0x0E, 0x11, 0x00, 0x00},
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
signed char time = 0;
unsigned char px, py;
unsigned char game_started;
unsigned char highScore;
unsigned char stopTime;
void DisplayScore() {
    LCD_Cursor(29);
    LCD_WriteData(highScore/10 + '0');
    LCD_Cursor(30);
    LCD_WriteData(highScore%10 + '0');
}
enum GameState {g_start, waitForStart, inGame};
int gameTick(int state) {

    switch(state) {
        case g_start:
        state = waitForStart;
        game_started = 0;
        currMaze = 0;
        stopTime = 1;
        highScore = eeprom_read_byte(0x5F);
        if(highScore>20) highScore = 0; 
        LCD_DisplayString(1,"L:Start,R:Reset  High Score: ");
        DisplayScore();
        break;
        case waitForStart:
        if(~PINA&0x08) {
            state = inGame;
            game_started = 1;
            stopTime = 0;
        } else {
            state = waitForStart;
        }
        break;
        case inGame:
        if(~PINA&0x04) {
            state = waitForStart;
            game_started = 0;
            px=7;
            py=0;
            stopTime = 1;
            currMaze = 0;
            time = 20;
            LCD_DisplayString(1,"L:Start,R:Reset  High Score: ");
            DisplayScore();
        } else if(currMaze==2) {
            //win
            currMaze = 0;
            if(time > (signed char)highScore) {
                eeprom_write_byte(0x5F,time);
                highScore = time;
            }
            LCD_ClearScreen();
            

            stopTime = 1;
            game_started = 0;
            LCD_DisplayString(3,"You Win!!      High Score: ");
            DisplayScore();
            LCD_Cursor(1);
            LCD_WriteData(1);
            LCD_Cursor(2);
            LCD_WriteData(3);
            currMaze = 3;
        } else if(time<=0) {
            LCD_ClearScreen();
            LCD_DisplayString(2,"You Lose!      R:Reset");
            LCD_Cursor(1);
            LCD_WriteData(2);
            stopTime = 1;
            game_started = 0;
        } else if(px==0 && py==7) {
            currMaze++;
            px=7;
            py=0;
        }
        break;
    }
    return state;
}


enum TimeState {t_start, timing, stop};
int TimerTick(int state) {
    switch(state) {
        case t_start:
        time = 20;
        state = timing;
        break;
        case timing:
        if(game_started==0) {
            time = 20;
        } else if(stopTime==0) {
            time--;
        } else {
            state = stop;
        }
        break;
        case stop:
        if(stopTime) {
            state = stop;
        } else {
            time = 20;
        }
        break;
    }
    return state;
}

unsigned short x, y;
enum JoystickState {j_start, mid, up, down, left, right} jstate;
int JoystickTick(int state) {
    if(game_started) {
        x=ADC_Channel(0x01);    
        y=ADC_Channel(0x00);
    } else {
        x=y=500;
    }
    // display values of x and y direction
    // for(unsigned char j = 0; j < 4; j++) {
    //     LCD_Cursor(8-j);
    //     LCD_WriteData('0'+x%10);
    //     x/=10;
    // }
    // for(unsigned char j = 0; j < 4; j++) {
    //     LCD_Cursor(16-j);
    //     LCD_WriteData('0'+y%10);
    //     y/=10;
    // }
    // LCD_Cursor(32);
    switch(state) {
        case j_start:
        state = mid;
        px=7;
        py=0;
        break;
        case mid:
        if(x<200 && px<7) {
            unsigned char wall = maze[currMaze][py];
            if((wall&(1<<(px+1)))==0) {
                state = left;
                px++;
            }
        } else if(x>850 && px>0) {
            unsigned char wall = maze[currMaze][py];
            if((wall&(1<<(px-1)))==0) {
                state = right;
                px--;
            }
        } else if(y<200 && py>0) {
            unsigned char wall = maze[currMaze][py-1];
            if((wall&(1<<(px)))==0) {
                state = up;
                py--;
            }
        } else if(y>850 && py<7) {
            unsigned char wall = maze[currMaze][py+1];
            if((wall&(1<<(px)))==0) {
                state = down;
                py++;
            }
        } else state = mid;
        break;
        case left:
        if(x<200) state = left;
        else state = mid;
        break;
        case right:
        if(x>850) state = right;
        else state = mid;
        break;
        case up:
        if(y<200) state = up;
        else state = mid;
        break;
        case down:
        if(y>850) state = down;
        else state = mid;
        break;
    }
    return state;
}


#define SER 0
#define RCLK 1
#define SRCLK 2
#define SET_BIT(p,i) ((p) |= (1 << (i)))
#define CLR_BIT(p,i) ((p) &= ~(1 << (i)))
#define GET_BIT(p,i) ((p) & (1 << (i)))

unsigned char k = 0;
int LEDMatrixTick(int state) {
    unsigned char temp;
    //store red
    if(py==k) temp = (1<<(px));
    else temp = 0;
    temp = ~temp;
    for(unsigned char j = 0; j < 8; j++) {    
        //store value
        if(temp&0x01) SET_BIT(PORTB, SER);
        else CLR_BIT(PORTB,SER);

        //pulse serial clock
        SET_BIT(PORTB,SRCLK);
        CLR_BIT(PORTB,SRCLK);

        //shift temp over
        temp >>= 1;
    }

    //store green
    temp = ~maze[currMaze][k];
    for(unsigned char j = 0; j < 8; j++) {

        //store value
        if(temp&0x01) SET_BIT(PORTB, SER);
        else CLR_BIT(PORTB,SER);

        //pulse serial clock
        SET_BIT(PORTB,SRCLK);
        CLR_BIT(PORTB,SRCLK);

        //shift value of val over
        temp >>= 1;
    }

    PORTD = 1<<k; //store row
    //pulse the latch
    SET_BIT(PORTB,RCLK);
    CLR_BIT(PORTB,RCLK);
    //increment row
    k++;
    if(k==8) k=0;
    return state;
}

int main(void) {
    DDRA = 0xF0; PORTA = 0x0F; 
    DDRB = 0xFF; PORTB = 0x00; 
    DDRC = 0xFF; PORTC = 0x00; 
    DDRD = 0xFF; PORTD = 0x00; 
    unsigned long GCDPeriod = 2;
    TimerOn();
    TimerSet(GCDPeriod);

    LCD_init();
    LCD_CreateCustom();
    LCD_ClearScreen();


    ADC_init();

    unsigned char i = 0;
    tasks[i].state = g_start;
    tasks[i].period = 100;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &gameTick;
    i++;
    tasks[i].state = t_start;
    tasks[i].period = 1000;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &TimerTick;
    i++;
    tasks[i].state = j_start;
    tasks[i].period = 150;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &JoystickTick;
    i++;
    tasks[i].state = 0;
    tasks[i].period = GCDPeriod;
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
