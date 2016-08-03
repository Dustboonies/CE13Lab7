// **** Include libraries here ****
// Standard libraries

//CMPE13 Support Library
#include "BOARD.h"
#include "Leds.h"
#include "Adc.h"
#include "Ascii.h"
#include "Oled.h"
#include "OledDriver.h"
#include "Buttons.h"

// Microchip libraries
#include <xc.h>
#include <plib.h>



// **** Set any macros or preprocessor directives here ****
// Set a macro for resetting the timers, makes the code a little clearer.
#define TIMER_2HZ_RESET() (TMR1 = 0)
#define DEFAULT_DISPLAY = ""
#define START() do { \
    LEDS_SET(0x01); \
    OledDrawString(DEFAULT_DISPLAY); \
} while (0)
#define RESET() do { \
    START(); \
} while (0)

#define BAKE_MODE 0
#define TOAST_MODE 1
#define BROIL_MODE 2
#define BAKE_STRING "Bake "
#define BROIL_STRING "Broil"
#define TOAST_STRING "Toast"
#define TIME_INPUT 1
#define TEMP_INPUT 0
#define CompareTime(x,y) ((x)-(y))
#define LONG_PRESS 5


// **** Declare any datatypes here ****

// **** Define any module-level, global, or external variables here ****

enum {
    D1_ON,
    D2_ON_LEFT,
    D3_ON_LEFT,
    D4_ON,
    D3_ON_RIGHT,
    D2_ON_RIGHT
} state = D1_ON;

enum {
    RESET,
    START,
    COUNTDOWN,
    PENDING_SELECTOR_CHANGE,
    PENDING_RESET
} Ostate = START;

typedef struct ovenState {
    int cTimeLeft;
    int CookTime;
    int temperature;
    int cookingMode;
    int btnPressCount;
    int inputSelect;
} ovenState;

ovenState OS;
char display[100];
int bFlag = 0;
int adcflag = 0;
uint16_t myadc = 0;
uint8_t bcheck = 0;
int increment;
int twoh = 0;
// Configuration Bit settings

//Functions
void setDisplay();

int main()
{
    BOARD_Init();

    // Configure Timer 1 using PBCLK as input. We configure it using a 1:256 prescalar, so each timer
    // tick is actually at F_PB / 256 Hz, so setting PR1 to F_PB / 256 / 2 yields a 0.5s timer.
    OpenTimer1(T1_ON | T1_SOURCE_INT | T1_PS_1_256, BOARD_GetPBClock() / 256 / 2);

    // Set up the timer interrupt with a medium priority of 4.
    INTClearFlag(INT_T1);
    INTSetVectorPriority(INT_TIMER_1_VECTOR, INT_PRIORITY_LEVEL_4);
    INTSetVectorSubPriority(INT_TIMER_1_VECTOR, INT_SUB_PRIORITY_LEVEL_0);
    INTEnable(INT_T1, INT_ENABLED);

    // Configure Timer 2 using PBCLK as input. We configure it using a 1:16 prescalar, so each timer
    // tick is actually at F_PB / 16 Hz, so setting PR2 to F_PB / 16 / 100 yields a .01s timer.
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_16, BOARD_GetPBClock() / 16 / 100);

    // Set up the timer interrupt with a medium priority of 4.
    INTClearFlag(INT_T2);
    INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_4);
    INTSetVectorSubPriority(INT_TIMER_2_VECTOR, INT_SUB_PRIORITY_LEVEL_0);
    INTEnable(INT_T2, INT_ENABLED);

    // Configure Timer 3 using PBCLK as input. We configure it using a 1:256 prescalar, so each timer
    // tick is actually at F_PB / 256 Hz, so setting PR3 to F_PB / 256 / 5 yields a .2s timer.
    OpenTimer3(T3_ON | T3_SOURCE_INT | T3_PS_1_256, BOARD_GetPBClock() / 256 / 5);

    // Set up the timer interrupt with a medium priority of 4.
    INTClearFlag(INT_T3);
    INTSetVectorPriority(INT_TIMER_3_VECTOR, INT_PRIORITY_LEVEL_4);
    INTSetVectorSubPriority(INT_TIMER_3_VECTOR, INT_SUB_PRIORITY_LEVEL_0);
    INTEnable(INT_T3, INT_ENABLED);

    /***************************************************************************************************
     * Your code goes in between this comment and the following one with asterisks.
     **************************************************************************************************/
    LEDS_INIT;
    OledInit();
    ButtonsInit();
    AdcInit();
    LEDS_SET(0x01);
    OledOn();
    int newinc;
    int ledconstant;
    OS.inputSelect = TEMP_INPUT;
    OS.CookTime = 61;
    OS.cookingMode = BAKE_MODE;
    OS.temperature = 350;
    setDisplay();
    int lpressflag = 0;
    while (1) {

        switch (Ostate) {
        case RESET:
            //set time to default, set temp to default, turn off oven heating, update display
            OS.CookTime = 61;
            OS.temperature = 350;
            LEDS_SET(0x00);
            setDisplay();
            Ostate = START;
            break;
        case START:
            //display oven, update loop to update temperature/timer
            setDisplay();
            if (adcflag == 1) {
                if (OS.inputSelect == TEMP_INPUT) {
                    OS.temperature = (int) (((myadc * 255) / 1043) + 300);
                } else if (OS.inputSelect == TIME_INPUT) {
                    OS.CookTime = myadc >> 2;
                }
                OS.cTimeLeft = OS.CookTime;
                setDisplay();
                adcflag = 0;
                Ostate = START;
            }
            if (bcheck != 0) {
                Ostate = PENDING_SELECTOR_CHANGE;
            }

            break;

        case COUNTDOWN:
            setDisplay();
            ledconstant = (int) ((OS.cTimeLeft * 8 / OS.CookTime) + 1);
            if (ledconstant == 8) {
                LEDS_SET(0xFF);
            } else if (ledconstant == 7) {
                LEDS_SET(0xFE);
            } else if (ledconstant == 6) {
                LEDS_SET(0xFC);
            } else if (ledconstant == 5) {
                LEDS_SET(0xF8);
            } else if (ledconstant == 4) {
                LEDS_SET(0xF0);
            } else if (ledconstant == 3) {
                LEDS_SET(0xE0);
            } else if (ledconstant == 2) {
                LEDS_SET(0xC0);
            } else if (ledconstant == 1) {
                LEDS_SET(0x80);
            } else if (ledconstant == 0) {
                LEDS_SET(0x00);
            }
            if (twoh > 1 && OS.cTimeLeft > 0) {
                OS.cTimeLeft -= 1;
                twoh = 0;
                Ostate = COUNTDOWN;
            } else if (OS.cTimeLeft == 0) {
                Ostate = RESET;
            }
            if (bcheck & BUTTON_EVENT_4DOWN) {
                newinc = increment;
                bcheck = 0;
                Ostate = PENDING_RESET;
            }
            break;
//        case LONG_PRESS_STATE:
//            newinc = increment;
//            if ((bcheck & BUTTON_EVENT_3UP) && (newinc - increment >= 5)) {
//                bcheck = 0;
//                lpressflag = 1;
//                Ostate = PENDING_SELECTOR_CHANGE;
//            } else if ((bcheck & BUTTON_EVENT_4UP) && (newinc - increment >= 5)) {
//                bcheck = 0;
//                lpressflag = 1;
//                Ostate = PENDING_SELECTOR_CHANGE;
//            } else if ((bcheck & BUTTON_EVENT_4UP) && (newinc - increment < 5)) {
//                bcheck = 0;
//                lpressflag = 0;
//                Ostate = PENDING_SELECTOR_CHANGE;
//            } else if ((bcheck & BUTTON_EVENT_3UP) && (newinc - increment < 5)) {
//                bcheck = 0;
//                lpressflag = 0;
//                Ostate = PENDING_SELECTOR_CHANGE;
//            } else {
//                Ostate = LONG_PRESS_STATE;
//            }
//            break;
        case PENDING_SELECTOR_CHANGE:
            setDisplay();
            if ((((bcheck & BUTTON_EVENT_2UP) == BUTTON_EVENT_2UP) && OS.inputSelect == TEMP_INPUT)&& OS.cookingMode == BAKE_MODE) {
                OS.inputSelect = TIME_INPUT;
                lpressflag = 0;
                setDisplay();
                bcheck = 0;
                Ostate = START;
            } else if ((bcheck & BUTTON_EVENT_4DOWN) == BUTTON_EVENT_4DOWN) {
                TIMER_2HZ_RESET();
                bcheck = 0;
                Ostate = COUNTDOWN;
            } else if (((bcheck & BUTTON_EVENT_2UP) == BUTTON_EVENT_2UP && OS.inputSelect == TIME_INPUT)&& OS.cookingMode == BAKE_MODE) {
                OS.inputSelect = TEMP_INPUT;
                lpressflag = 0;
                setDisplay();
                bcheck = 0;
                Ostate = START;
            } else if ((((bcheck & BUTTON_EVENT_3UP) == BUTTON_EVENT_3UP) && OS.cookingMode == BAKE_MODE)) {
                OS.cookingMode = TOAST_MODE;
                OS.inputSelect = TIME_INPUT;
                lpressflag = 0;
                setDisplay();
                bcheck = 0;
                increment = 0;
                Ostate = START;
            } else if ((((bcheck & BUTTON_EVENT_3UP) == BUTTON_EVENT_3UP) && OS.cookingMode == TOAST_MODE)) {
                OS.cookingMode = BROIL_MODE;
                OS.inputSelect = TIME_INPUT;
                lpressflag = 0;
                setDisplay();
                bcheck = 0;
                increment = 0;
                Ostate = START;
            } else if ((((bcheck & BUTTON_EVENT_3UP) == BUTTON_EVENT_3UP) && OS.cookingMode == BROIL_MODE)) {
                OS.cookingMode = BAKE_MODE;
                setDisplay();
                lpressflag = 0;
                bcheck = 0;
                increment = 0;
                Ostate = START;
            } else {
                Ostate = START;
            }
            break;
        case PENDING_RESET:
            setDisplay();
            if (twoh > 1 && OS.cTimeLeft > 0) {
                OS.cTimeLeft -= 1;
                twoh = 0;
                Ostate = PENDING_RESET;
            } else if (OS.cTimeLeft == 0) {
                Ostate = RESET;
            }
            if (bcheck & BUTTON_EVENT_4UP) {
                Ostate = RESET;
            }
            break;


        }

        /***************************************************************************************************
         * Your code goes in between this comment and the preceding one with asterisks
         **************************************************************************************************/
    }
    while (1);
}

void __ISR(_TIMER_1_VECTOR, ipl4auto) TimerInterrupt2Hz(void)
{
    // Clear the interrupt flag.
    IFS0CLR = 1 << 4;
    twoh++;
}

void __ISR(_TIMER_3_VECTOR, ipl4auto) TimerInterrupt5Hz(void)
{
    // Clear the interrupt flag.
    IFS0CLR = 1 << 12;
    increment += 1;
}

void __ISR(_TIMER_2_VECTOR, ipl4auto) TimerInterrupt100Hz(void)
{
    // Clear the interrupt flag.
    IFS0CLR = 1 << 8;
    bcheck = ButtonsCheckEvents();
    if (AdcChanged()) {
        myadc = AdcRead();
        adcflag = 1;
    }
}

void setDisplay()
{
    char ovenTopOn[5];
    char ovenBotOn[5];
    char *cookmode;
    char select1 = '>';
    char select2 = ' ';
    char *temperatureString;
    sprintf(ovenTopOn, "%c", 0x01);
    sprintf(ovenBotOn, "%c", 0x03);
    if (OS.cookingMode == BAKE_MODE) {
        cookmode = "Bake ";
        temperatureString = "Temp: ";
        if (OS.inputSelect == TEMP_INPUT) {
            select2 = '>';
            select1 = ' ';
        } else if (OS.inputSelect == TIME_INPUT) {
            select2 = ' ';
            select1 = '>';
        }
    } else if (OS.cookingMode == BROIL_MODE) {
        select1 = ' ';
        select2 = ' ';
        cookmode = "Broil";
    } else if (OS.cookingMode == TOAST_MODE) {
        select1 = ' ';
        select2 = ' ';
        cookmode = "Toast";
    }
    if (Ostate == COUNTDOWN && OS.cookingMode == BAKE_MODE) {
        sprintf(display, "%c%c%c%c%c%c%s%s%s\n%s%c%s%d%c%d\n%s%c%s%d%c%c\n%c%c%c%c%c%c%c", '|', 0x01, 0x01, 0x01, 0x01, 0x01, "|  Mode: ", cookmode, " ",
                "|     | ", select1, "Time: ", (int) OS.cTimeLeft / 60, ':', (int) OS.cTimeLeft % 60,
                "|-----| ", select2, "Temp: ", OS.temperature, 0xF8, 'F',
                '|', 0x03, 0x03, 0x03, 0x03, 0x03, '|');
    } else if (Ostate == COUNTDOWN && OS.cookingMode == TOAST_MODE){
        sprintf(display, "%c%c%c%c%c%c%s%s\n%s%c%s%-d%c%d\n%s\n%c%c%c%c%c%c%c", '|', 0x01, 0x01, 0x01, 0x01, 0x01, "|  Mode: ", cookmode,
                "|     | ", select1, "Time: ", (int) OS.cTimeLeft / 60, ':', (int) OS.cTimeLeft % 60,
                "|-----|             ",
                '|', 0x03, 0x03, 0x03, 0x03, 0x03, '|');
    } else if (Ostate == COUNTDOWN && OS.cookingMode == BROIL_MODE){
        sprintf(display, "%c%c%c%c%c%c%s%s\n%s%c%s%-d%c%d\n%s%c%s%d%c%c\n%c%c%c%c%c%c%c", '|', 0x01, 0x01, 0x01, 0x01, 0x01, "|  Mode: ", cookmode,
                "|     | ", select1, "Time: ", (int) OS.cTimeLeft / 60, ':', (int) OS.cTimeLeft % 60,
                "|-----| ", select2, "Temp: ", 500, 0xF8, 'F',
                '|', 0x03, 0x03, 0x03, 0x03, 0x03, '|');
    } else if (OS.cookingMode == BAKE_MODE) {
        sprintf(display, "%c%c%c%c%c%c%s%s\n%s%c%s%-d%c%d\n%s%c%s%d%c%c\n%c%c%c%c%c%c%c", '|', 0x02, 0x02, 0x02, 0x02, 0x02, "|  Mode: ", cookmode,
                "|     | ", select1, "Time: ", (int) OS.cTimeLeft / 60, ':', (int) OS.cTimeLeft % 60,
                "|-----| ", select2, "Temp: ", OS.temperature, 0xF8, 'F',
                '|', 0x04, 0x04, 0x04, 0x04, 0x04, '|');
    } else if (OS.cookingMode == TOAST_MODE) {
        sprintf(display, "%c%c%c%c%c%c%s%s\n%s%c%s%-d%c%d\n%s\n%c%c%c%c%c%c%c", '|', 0x02, 0x02, 0x02, 0x02, 0x02, "|  Mode: ", cookmode,
                "|     | ", select1, "Time: ", (int) OS.cTimeLeft / 60, ':', (int) OS.cTimeLeft % 60,
                "|-----|             ",
                '|', 0x04, 0x04, 0x04, 0x04, 0x04, '|');
    } else if (OS.cookingMode == BROIL_MODE) {
        sprintf(display, "%c%c%c%c%c%c%s%s\n%s%c%s%-d%c%d\n%s%c%s%d%c%c\n%c%c%c%c%c%c%c", '|', 0x02, 0x02, 0x02, 0x02, 0x02, "|  Mode: ", cookmode,
                "|     | ", select1, "Time: ", (int) OS.cTimeLeft / 60, ':', (int) OS.cTimeLeft % 60,
                "|-----| ", select2, "Temp: ", 500, 0xF8, 'F',
                '|', 0x04, 0x04, 0x04, 0x04, 0x04, '|');
    }
    OledDrawString(display);
    OledUpdate();
}