// NAME : NIHIT BHAVSAR
//STUDENT ID : 1001391283
//PROJECT : IMPEDANCE METER WITH GRAPHICS LCD


#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "tm4c123gh6pm.h"
#include "hardwareinit.h"
#include "wait.h"
#include "lcdinit.h"
#include "parser.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>


#define RED_LED      (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 1*4)))
#define GREEN_LED    (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))
#define BLUE_LED    (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*4))) // ON BOARD BLUE

#define PUSH_BUTTON     (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 4*4)))
#define AUTOBUTTON    (*((volatile uint32_t *)(0x42000000 + (0x400053FC-0x40000000)*32 + 1*4))) //PB1

#define MEAS_LR     (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32 + 4*4)))  //PE4
#define MEAS_C    (*((volatile uint32_t *)(0x42000000 + (0x400073FC-0x40000000)*32 + 1*4)))    //PD1

#define HIGHSIDE_R    (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32 + 1*4)))  //PE1
#define LOWSIDE_R  (*((volatile uint32_t *)(0x42000000 + (0x400053FC-0x40000000)*32 + 6*4))) //PB6
#define INTEGRATE    (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32 + 3*4)))   //PE3
#define POSITIVE(n) ((n)<0 ? 0 - (n) : (n))

#define A0           (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32 + 2*4)))  //PE2
#define CS_NOT       (*((volatile uint32_t *)(0x42000000 + (0x400053FC-0x40000000)*32 + 1*4))) //PB1





// Set pixel arguments
#define CLEAR  0
#define SET    1
#define INVERT 2
//uint8_t counter = 0;
uint32_t frequency = 0;
float time = 0;
bool timeMode = true;
bool freqUpdate = false;
bool timeUpdate = false;
int8_t FLAG = 0;
uint8_t field_posi[10];
char field_type[10];
char ArmRxBuffer[81];
char ParsedString1[20];
char *strpin1;
char *strpin2;
char *volt;
int raw;
float instVoltage;
char voltbuffer[30];
float iirVolt = 0.0;
float alpha = 0.99;
bool firstUpdate = true;
float capVoltage = 0.0;
float resistance;
float capacitance;
uint32_t i = 0;
uint8_t pixelMap[1024];
uint16_t txtIndex = 0;






// Blocking function that returns only when SW1 is pressed
void waitPbPress()
{
    while (PUSH_BUTTON)
        ;

}





// Blocking function that writes data to the SPI bus and waits for the data to complete transmission







void drawGraphicsLcdPixel(uint8_t x, uint8_t y, uint8_t op)
{
    uint8_t data, mask, page;
    uint16_t index;

    // determine pixel map entry
    page = y >> 3;

    // determine pixel map index
    index = page << 7 | x;

    // generate mask
    mask = 1 << (y & 7);

    // read pixel map
    data = pixelMap[index];

    // apply operator (0 = clear, 1 = set, 2 = xor)
    switch (op)
    {
    case 0:
        data &= ~mask;
        break;
    case 1:
        data |= mask;
        break;
    case 2:
        data ^= mask;
        break;
    }

    // write to pixel map
    pixelMap[index] = data;

    // write to display
    setGraphicsLcdPage(page);
    setGraphicsLcdColumn(x);
    sendGraphicsLcdData(data);
}

void drawGraphicsLcdRectangle(uint8_t xul, uint8_t yul, uint8_t dx, uint8_t dy,
                              uint8_t op)
{
    uint8_t page, page_start, page_stop;
    uint8_t bit_index, bit_start, bit_stop;
    uint8_t mask, data;
    uint16_t index;
    uint8_t x;

    // determine pages for rectangle
    page_start = yul >> 3;
    page_stop = (yul + dy - 1) >> 3;

    // draw in pages from top to bottom within extent
    for (page = page_start; page <= page_stop; page++)
    {
        // calculate mask for this page
        if (page > page_start)
            bit_start = 0;
        else
            bit_start = yul & 7;
        if (page < page_stop)
            bit_stop = 7;
        else
            bit_stop = (yul + dy - 1) & 7;
        mask = 0;
        for (bit_index = bit_start; bit_index <= bit_stop; bit_index++)
            mask |= 1 << bit_index;

        // write page
        setGraphicsLcdPage(page);
        setGraphicsLcdColumn(xul);
        index = (page << 7) | xul;
        for (x = 0; x < dx; x++)
        {
            // read pixel map
            data = pixelMap[index];
            // apply operator (0 = clear, 1 = set, 2 = xor)
            switch (op)
            {
            case 0:
                data &= ~mask;
                break;
            case 1:
                data |= mask;
                break;
            case 2:
                data ^= mask;
                break;
            }
            // write to pixel map
            pixelMap[index++] = data;
            // write to display
            sendGraphicsLcdData(data);
        }
    }
}

void setGraphicsLcdTextPosition(uint8_t x, uint8_t page)
{
    txtIndex = (page << 7) + x;
    setGraphicsLcdPage(page);
    setGraphicsLcdColumn(x);
}

void putcGraphicsLcd(char c)
{
    uint8_t i, val;
    uint8_t uc;
    // convert to unsigned to access characters > 127
    uc = (uint8_t) c;
    for (i = 0; i < 5; i++)
    {
        val = charGen[uc - ' '][i];
        pixelMap[txtIndex++] = val;
        sendGraphicsLcdData(val);
    }
    pixelMap[txtIndex++] = 0;
    sendGraphicsLcdData(0);
}

void putsGraphicsLcd(char str[])
{
    uint8_t i = 0;
    while (str[i] != 0)
        putcGraphicsLcd(str[i++]);
}

int roundNo(float num)
{
    return num < 0 ? num - 0.5 : num + 0.5;
}

void putcUart0(char c)
{
    while (UART0_FR_R & UART_FR_TXFF)
        ;
    UART0_DR_R = c;
}

void putsUart0(char* str)
{
    uint8_t i;
    for (i = 0; i < strlen(str); i++)
        putcUart0(str[i]);
}

int16_t readAdc0Ss3()
{
    ADC0_PSSI_R |= ADC_PSSI_SS3;                     // set start bit
    while (ADC0_ACTSS_R & ADC_ACTSS_BUSY)
        ;           // wait until SS3 is not busy
    return ADC0_SSFIFO3_R;                    // get single result from the FIFO
}
void voltCalc()
{
    raw = readAdc0Ss3();
    instVoltage = (raw / 4096.0) * 3.3;
}
void voltage()
{
    clearGraphicsLcd();
    memset(strpin1, 0, 20);
    memset(strpin2, 0, 20);
    memset(volt, 0, 20);
    uint16_t i = 0;
    MEAS_C = 1;
    MEAS_LR = 0;
    LOWSIDE_R = 0;
    INTEGRATE = 0;
    waitMicrosecond(100000);

    for (i = 0; i < 15; i++)
    {
        raw = readAdc0Ss3();
        instVoltage = ((raw / 4096.0) * 3.3) - 0.02;
        if (i == 14)
        {
            putsUart0("   raw value : ");
            sprintf(voltbuffer, "%u", raw);
            putsUart0(voltbuffer);
            putsUart0("\r  Unfiltered Voltage : ");
            sprintf(voltbuffer, "%1.8f", instVoltage);
            putsUart0(voltbuffer);
        }
    }
}

void voltageAvg()
{
    memset(strpin1, 0, 20);
    memset(strpin2, 0, 20);
    memset(volt, 0, 20);
    uint16_t i = 0;
    MEAS_C = 1;
    MEAS_LR = 0;
    HIGHSIDE_R = 0;
    LOWSIDE_R = 0;
    INTEGRATE = 1;
    waitMicrosecond(100000);

    for (i = 0; i < 15; i++)
    {
        raw = readAdc0Ss3();
        instVoltage = ((raw / 4096.0) * 3.3) - 0.02;
        if (firstUpdate)
        {
            iirVolt = instVoltage;
            firstUpdate = false;
        }
        else
            iirVolt = iirVolt * alpha + instVoltage * (1 - alpha);
        if (i == 14)
        {
            //putsUart0("   raw value : ");
            sprintf(voltbuffer, "%u", raw);
            putsUart0(voltbuffer);
            putsUart0("\r   Unfiltered Voltage :");
            sprintf(voltbuffer, "%1.5f", instVoltage);
            putsUart0(voltbuffer);
            putsUart0("\r  Filtered Voltage : ");
            sprintf(voltbuffer, "%1.5f", iirVolt);
            putsUart0(voltbuffer);

        }

    }

}

void deintegrate()
{
    INTEGRATE = 1;
    HIGHSIDE_R = 0;
    LOWSIDE_R = 1;
    waitMicrosecond(500000);
}

void integrate()
{
    INTEGRATE = 1;
    LOWSIDE_R = 0;
    HIGHSIDE_R = 1;
}

void test()
{
    clearGraphicsLcd();
    resistance = 0.0;
    time = 0.0;
    MEAS_LR = 0;
    MEAS_C = 0;
    memset(voltbuffer, 30, 0);
    deintegrate();
    integrate();
    WTIMER5_CTL_R |= TIMER_CTL_TAEN;
    raw = readAdc0Ss3();
    waitMicrosecond(500000);
    time *= 0.000000025;
    resistance = (time * 1000000) / 1.375;
    resistance += 10300.0;
    capacitance = (time * 1.375 / 100000);
    capacitance -= 0.000001;
    sprintf(voltbuffer, "%f", resistance);
    putsUart0("\r  Resistance : ");
    putsUart0(voltbuffer);
    setGraphicsLcdTextPosition(1, 1);
    putsGraphicsLcd("resistance : ");
    putsGraphicsLcd(voltbuffer);
    putsUart0("   Ohms");
    putsUart0("\r  Capacitance :");
    sprintf(voltbuffer, "%f", capacitance);
    putsUart0(voltbuffer);
    setGraphicsLcdTextPosition(1, 2);
    putsGraphicsLcd("cap : ");
    putsGraphicsLcd(voltbuffer);
    putsUart0("  F");
    deintegrate();
}

void Timer1Isr()
{
    if (!timeMode)
    {
        frequency = WTIMER5_TAV_R;                   // read counter input
        WTIMER5_TAV_R = 0;                      // reset counter for next period
        freqUpdate = true;                           // set update flag
        GREEN_LED ^= 1;                              // status

    }
    TIMER1_ICR_R = TIMER_ICR_TATOCINT;               // clear interrupt flag
}

// Period timer service publishing latest time measurements every positive edge
void WideTimer5Isr()
{
    if (timeMode)
    {
        time = WTIMER5_TAV_R;                         // read counter input
        WTIMER5_CTL_R &= ~TIMER_CTL_TAEN;             // turn off timer
        WTIMER5_TAV_R = 0;                         // zero counter for next edge
        timeUpdate = true;                           // set update flag
        RED_LED = 1;                                 // status
        waitMicrosecond(500000);
        RED_LED = 0;

    }
    WTIMER5_ICR_R = TIMER_ICR_CAECINT;               // clear interrupt flag
}

void resis()

{
    clearGraphicsLcd();
    resistance = 0.0;
    time = 0.0;
    memset(voltbuffer, 30, 0);
    HIGHSIDE_R = 0;
    MEAS_LR = 0;
    MEAS_C = 0;
    INTEGRATE = 1;
    LOWSIDE_R = 1;
    waitMicrosecond(10000);
    LOWSIDE_R = 0;
    MEAS_LR = 1;
    WTIMER5_TAV_R = 0;
    WTIMER5_CTL_R |= TIMER_CTL_TAEN;       // START THE TIMER
    waitMicrosecond(500000);
    voltCalc();
    time *= 0.000000025;
    //time -= 0.000358000;
    if(time > 0.00002000 && time < 0.00005000)     // FOR 10 OHMS
    {
    resistance = (time * 1000000) / 1.375;
    resistance -= 14.0;
    }
    else
        resistance = (time * 1000000) / 1.375;

    sprintf(voltbuffer, "%f", resistance);
    putsUart0("\r  Resistance : ");
    putsUart0(voltbuffer);
    putsUart0("   Ohms");
    setGraphicsLcdTextPosition(1, 1);
    putsGraphicsLcd("resistance : ");
    setGraphicsLcdTextPosition(1, 2);
    putsGraphicsLcd(voltbuffer);
    time = 0.0;
}

void xtor()
{

    resis();

}

void cap()
{


    capacitance = 0.0;
    INTEGRATE = 0;
    MEAS_LR = 0;
    INTEGRATE = 0;
    MEAS_LR = 0;
    LOWSIDE_R = 1;
    MEAS_C = 1;
    clearGraphicsLcd();
    setGraphicsLcdTextPosition(1, 1);
    putsGraphicsLcd("Wait for 5 seconds to measure capacitance...");
    putsUart0("Wait for 5 seconds to measure capacitance...");
    waitMicrosecond(100000);
    LOWSIDE_R = 0;
    WTIMER5_CTL_R |= TIMER_CTL_TAEN;          // START THE TIMER
    HIGHSIDE_R = 1;
    waitMicrosecond(5000000);
    time /= 40000000;
    capacitance = time / (100000 * 1.40039);
    sprintf(voltbuffer, "%1.8f", capacitance);
    putsUart0("\r  Capacitance : ");
    putsUart0(voltbuffer);
    putsUart0("   Farads");
    setGraphicsLcdTextPosition(1, 1);
    putsGraphicsLcd("capacitance : ");
    setGraphicsLcdTextPosition(1, 2);
    putsGraphicsLcd(voltbuffer);
    time = 0.0;
}

void inductance()
{
    clearGraphicsLcd();
    float inductance;
    float esr;
    time = 0.0;
    MEAS_LR = 0;
    MEAS_C = 0;
    INTEGRATE = 0;
    HIGHSIDE_R = 0;
    LOWSIDE_R = 0;
    //WTIMER5_TAV_R = 0;
    waitMicrosecond(5000);
    WTIMER5_CTL_R |= TIMER_CTL_TAEN;
    MEAS_LR = 1;
    LOWSIDE_R = 1;
    waitMicrosecond(500000);
    voltCalc();
    esr = (112.53 / instVoltage) - 33;    //AFTER SOLVING FOR ESR FROM THE EQUATION WE GET 112.53
    //time /= 40000000;
    if (time > 500 && time < 1400)        //LOOP FOR 18uH
    {
        time /= 40;
        //time -= 6;
        inductance = time * (33 + esr);       // CORRECTING FOR ESR

    }
    else if (time > 30 && time < 100)        // LOOP FOR 1000uH
    {
        time /= 40;
        time -= 1.2;
        inductance = time * (33 + esr);
    }
    else
        inductance = time * (33 + esr);

    putsUart0("\r Inductance in uH : ");
    sprintf(voltbuffer, "%f", inductance);
    putsUart0(voltbuffer);
    setGraphicsLcdTextPosition(1, 1);
    putsGraphicsLcd("inductance : ");
    putsGraphicsLcd(voltbuffer);
    putsUart0(" uH");
    putsGraphicsLcd("uH");
    sprintf(voltbuffer, "%1.8f", esr);
    putsUart0("\r  ESR : ");
    putsUart0(voltbuffer);
    setGraphicsLcdTextPosition(1, 2);
    putsGraphicsLcd("ESR : ");
    putsGraphicsLcd(voltbuffer);
    putsUart0("  ohms ");
    putsGraphicsLcd("ohms");


}

void autotest()
{
    uint32_t array[80];


    int differential;
    HIGHSIDE_R = 0;
    MEAS_LR = 0;
    INTEGRATE = 0;
    LOWSIDE_R = 1;
    waitMicrosecond(50000);
    LOWSIDE_R = 0;
    HIGHSIDE_R = 1;
    MEAS_C = 1;
    //waitMicrosecond(100000);
    voltCalc();
    for (i = 0; i < 60; i++)             // GET RAW VALUES TO DIFFERENTIATE BETWEEN RESISTANCE AND CAPACITANCE
    {
        waitMicrosecond(1000);
        raw = readAdc0Ss3();
        array[i] = raw;

    }

    differential = abs(array[59] - array[4]);

    if (differential < 50)              // IF COMPONENT IS INDUCTANCE OR RESISTANCE
    {
        time = 0.0;
        MEAS_LR = 0;
        MEAS_C = 0;
        INTEGRATE = 0;
        HIGHSIDE_R = 0;
        LOWSIDE_R = 0;
        waitMicrosecond(5000);
        WTIMER5_TAV_R = 0;
        WTIMER5_CTL_R |= TIMER_CTL_TAEN;
        MEAS_LR = 1;
        LOWSIDE_R = 1;
        waitMicrosecond(500000);


        raw = readAdc0Ss3();
        instVoltage = (raw / 4096.0) * 3.3;

        if (instVoltage > 2.0)
        {
            inductance();
        }
        else if (instVoltage < 2.0)
        {
            resis();
        }
    }
    else if (differential > 50)                  //IF COMPONENT IS CAPACITANCE
    {
        time = 0.0;
        cap();
    }

}

void blinkled()
{
    GREEN_LED = 1;
    waitMicrosecond(500000);
    GREEN_LED = 0;
}

float getvalue(int x)     // THIS LOOP GETS VALUE ENTERED BY THE USER
{
    float k = atof(&ArmRxBuffer[field_posi[x]]);
    return k;
}

char* getstring(int y)    // THIS LOOP GETS CHARACTERS ENTERED BY THE USER FROM FIELD POSITION FROM PARSER
{
    char *j = &ArmRxBuffer[field_posi[y]];

    return j;

}

void getsUart0()
{
    getInput();
    ProcessString();
    parser();
}



void Reset()
{
    extern ResetISR();
    main();
}


void command()                         // PROCESSING INPUT FROM USER
{
    GREEN_LED = 0;
    RED_LED = 0;

    if (iscommand("inductance", 0) || iscommand("inductance", 2)
            || iscommand("ind", 0))
    {
        RED_LED = 1;
        float i1 = getvalue(1);
        float i2 = getvalue(2);
        inductance();
        RED_LED = 0;
    }

    else if (iscommand("resistance", 0) || iscommand("resistance", 2)
            || iscommand("res", 0))
    {
        GREEN_LED = 1;
        float r1 = getvalue(1);
        float r2 = getvalue(2);
        resis();
        GREEN_LED = 0;
    }
    else if (iscommand("capacitance", 0) || iscommand("capacitance", 2)
            || iscommand("cap", 0))
    {
        BLUE_LED = 1;
        float c1 = getvalue(1);
        float c2 = getvalue(2);
        cap();
        BLUE_LED = 0;
    }

    else if (iscommand("dut2", 0))
    {
        HIGHSIDE_R = 1;
        LOWSIDE_R = 1;
        INTEGRATE = 0;
        MEAS_C = 0;
        MEAS_LR = 0;
        raw = readAdc0Ss3();
        sprintf(voltbuffer, "%u", raw);
        putsUart0(voltbuffer);
    }

    else if (iscommand("io", 2))
    {
        char *strpin1 = getstring(1);
        char *strpin2 = getstring(2);

        if ((strcmp(strpin1, "meas_c")) == 0 && (strcmp(strpin2, "on")) == 0)
        {
            MEAS_C = 1;
            MEAS_LR = 0;
        }
        if ((strcmp(strpin1, "meas_c")) == 0 && (strcmp(strpin2, "off")) == 0)
        {
            MEAS_C = 0;
            MEAS_LR = 0;
        }
        if ((strcmp(strpin1, "meas_lr")) == 0 && (strcmp(strpin2, "on") == 0))
        {
            MEAS_LR = 1;
            MEAS_C = 0;
        }
        if ((strcmp(strpin1, "meas_lr")) == 0 && (strcmp(strpin2, "off") == 0))
        {
            MEAS_LR = 0;
            MEAS_C = 0;
        }
        if ((strcmp(strpin1, "highside_r")) == 0
                && (strcmp(strpin2, "on")) == 0)
        {
            HIGHSIDE_R = 1;
        }
        if ((strcmp(strpin1, "highside_r")) == 0
                && (strcmp(strpin2, "off")) == 0)
        {
            HIGHSIDE_R = 0;
        }
        if ((strcmp(strpin1, "lowside_r")) == 0 && (strcmp(strpin2, "on")) == 0)
        {
            LOWSIDE_R = 1;
        }
        if ((strcmp(strpin1, "lowside_r")) == 0
                && (strcmp(strpin2, "off")) == 0)
        {
            LOWSIDE_R = 0;
        }
        if ((strcmp(strpin1, "integrate")) == 0 && (strcmp(strpin2, "on")) == 0)
        {
            INTEGRATE = 1;
        }
        if ((strcmp(strpin1, "integrate")) == 0
                && (strcmp(strpin2, "off")) == 0)
        {
            INTEGRATE = 0;
        }
    }

    else if (iscommand("io", 1))
    {
        char *str1 = getstring(1);
        if (strcmp(str1, "status") == 0)
        {
            int f = MEAS_C;
            int g = MEAS_LR;
            int h = HIGHSIDE_R;
            int i = LOWSIDE_R;
            int j = INTEGRATE;
            putsUart0("\rMEAS_C = ");
            if (f == 1)
            {
                putsUart0("on");
            }
            else
                putsUart0("off");

            putsUart0("\rMEAS_LR = ");
            if (g == 1)
            {
                putsUart0("on");
            }
            else
                putsUart0("off");

            putsUart0("\rHIGHSIDE_R = ");
            if (h == 1)
            {
                putsUart0("on");
            }
            else
                putsUart0("off");

            putsUart0("\rLOWSIDE_R = ");
            if (i == 1)
            {
                putsUart0("on");
            }
            else
                putsUart0("off");

            putsUart0("\rINTEGRATE = ");
            if (j == 1)
            {
                putsUart0("on");
            }
            else
                putsUart0("off");

        }

        else if (strcmp(str1, "meas_c") == 0)
        {
            int a = MEAS_C;
            if (a == 1)
            {
                putsUart0("\r MEAS_C is ON");
            }
            else
                putsUart0("\r MEAS_C is OFF");
        }

        else if (strcmp(str1, "meas_lr") == 0)
        {
            int b = MEAS_LR;
            if (b == 1)
            {
                putsUart0("\rMEAS_LR is ON");
            }
            else
                putsUart0("\rMEAS_LR is OFF");
        }

        else if (strcmp(str1, "highside_r") == 0)
        {
            int c = HIGHSIDE_R;
            if (c == 1)
            {
                putsUart0("\rHIGHSIDE_R is ON");
            }
            else
                putsUart0("\rHIGHSIDE_R is OFF");
        }

        else if (strcmp(str1, "lowside_r") == 0)
        {
            int d = LOWSIDE_R;
            if (d == 1)
            {
                putsUart0("\rLOWSIDE_R is ON");
            }
            else
                putsUart0("\rLOWSIDE_R is OFF");
        }

        else if (strcmp(str1, "integrate") == 0)
        {
            int e = INTEGRATE;
            if (e == 1)
            {
                putsUart0("\rINTEGRATE is ON");
            }
            else
                putsUart0("\rINTEGRATE is OFF");
        }

        else if (strcmp(str1, "off") == 0)
        {
            HIGHSIDE_R = 0;
            LOWSIDE_R = 0;
            INTEGRATE = 0;
            MEAS_C = 0;
            MEAS_LR = 0;
            putsUart0("\rALL GPIO PINS ARE TURNED OFF");
        }
        else if (strcmp(str1, "on") == 0)
        {
            HIGHSIDE_R = 1;
            LOWSIDE_R = 1;
            INTEGRATE = 1;
            MEAS_C = 0;
            MEAS_LR = 1;
            putsUart0("\rALL GPIO PINS ARE TURNED ON EXCEPT MEAS_C");
        }

    }

    else if ((iscommand("voltage", 0)) || iscommand("voltage", 1))
    {
        char *volt1 = getstring(1);
        char *volt0 = getstring(0);
        if ((strcmp(volt0, "voltage") == 0) && (strcmp(volt1, "average") == 0))
        {
            voltageAvg();
        }
        else if (strcmp(volt0, "voltage") == 0)
        {
            voltage();
        }

    }

    else if (iscommand("reset", 0))
    {
        Reset();
        main();
        putsUart0("\r Reset Successful.");
    }

    else if (iscommand("refresh", 0))
    {
       // Refresh();
    }
    else if (iscommand("charge", 0))
    {
        integrate();
    }
    else if (iscommand("discharge", 0))
    {
        deintegrate();
    }
    else if (iscommand("test", 0))
    {
        test();
    }
    else if (iscommand("a", 0))
    {
        RED_LED = 1;
        BLUE_LED = 1;
        GREEN_LED = 1;
        autotest();
        waitMicrosecond(1000);
        RED_LED = 0;
        BLUE_LED = 0;
        GREEN_LED = 0;
    }

    else
        putsUart0("\rPlease enter a valid string");

}


void welcomestrings()
{

    putsUart0(
            "\r\n___________IMPEDANCE METER <V1.0>___________                                         ");
    putsUart0(
            "\r\n***************COMMAND KEY***************                                                                      X1,X2 -> RANGE \r\n");
    putsUart0(
            "\r RESISTANCE  /  RESISTANCE  X1  X2                                                   ");
    putsUart0(
            "\r INDUCTANCE  /  INDUCTANCE  X1  X2                                                   ");
    putsUart0(
            "\r CAPACITANCE /  CAPACITANCE X1  X2                                                   ");
    putsUart0(
            "\r VOLTAGE     /  VOLTAGE AVERAGE                                                   ");
    putsUart0(
            "\r TEST                                                                     ");
    putsUart0(
            "\r CHARGE      /  DISCHARGE                                                      ");
    putsUart0(
            "\r\n ***************IO COMMANDS***************                                                                      ");
    putsUart0(
            "\r IO    STATUS                                   --> READ STATUS OF ALL GPIO PINS     ");
    putsUart0(
            "\r IO    ON/OFF                                   --> TURN ALL PINS ON/OFF             ");
    putsUart0(
            "\r IO   'PIN NAME'                                --> READ STATUS OF INDIVIDUAL PIN    ");
    putsUart0(
            "\r IO   'PIN NAME'   ON/OFF                       --> TURN INDIVIDUAL PIN ON/OFF       ");

}

void rolling()
{


        setGraphicsLcdTextPosition(1, 1);
        putsGraphicsLcd("IMPEDANCE METER V2.0");
        setGraphicsLcdTextPosition(1, 3);
        putsGraphicsLcd("NAME : NIHIT BHAVSAR ");
        setGraphicsLcdTextPosition(1, 5);
        putsGraphicsLcd("University : UT Arlington ");



}

int main(void)

 {

    initHw();
    blinkled();
    initGraphicsLcd(); // Initialize hardware
    rolling();

    while (1)
    {

        putsUart0("\n");
        putsUart0("\r\nEnter Command => ");
        getsUart0();
        command();
        Refresh();
    }

}
