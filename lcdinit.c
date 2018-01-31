/*
 * lcdinit.c
 *
 *  Created on: Jan 31, 2018
 *      Author: nihit
 */
#include <stdint.h>
#include "tm4c123gh6pm.h"

#define A0           (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32 + 2*4)))  //PE2
#define CS_NOT       (*((volatile uint32_t *)(0x42000000 + (0x400053FC-0x40000000)*32 + 1*4))) //PB1


uint8_t pixelMap[1024];



void sendGraphicsLcdData(uint8_t data)
{
    CS_NOT = 0;                        // assert chip select
    __asm (" NOP");
    // allow line to settle
    __asm (" NOP");
    __asm (" NOP");
    __asm (" NOP");
    A0 = 1;                            // set A0 for data
    SSI2_DR_R = data;                  // write data
    while (SSI2_SR_R & SSI_SR_BSY)
        ;    // wait for transmission to stop
    CS_NOT = 1;                        // de-assert chip select
}


void sendGraphicsLcdCommand(uint8_t command)
{
    CS_NOT = 0;                        // assert chip select
    __asm (" NOP");
    // allow line to settle
    __asm (" NOP");
    __asm (" NOP");
    __asm (" NOP");
    A0 = 0;                            // clear A0 for commands
    SSI2_DR_R = command;               // write command
    while (SSI2_SR_R & SSI_SR_BSY)
        ;
    CS_NOT = 1;                        // de-assert chip select
}

void setGraphicsLcdPage(uint8_t page)
{
    sendGraphicsLcdCommand(0xB0 | page);
}

void setGraphicsLcdColumn(uint8_t x)
{
    sendGraphicsLcdCommand(0x10 | ((x >> 4) & 0x0F));
    sendGraphicsLcdCommand(0x00 | (x & 0x0F));
}

void refreshGraphicsLcd()
{
    uint8_t x, page;
    uint16_t i = 0;
    for (page = 0; page < 8; page++)
    {
        setGraphicsLcdPage(page);
        setGraphicsLcdColumn(0);
        for (x = 0; x < 128; x++)
            sendGraphicsLcdData(pixelMap[i++]);
    }
}


void clearGraphicsLcd()
{
    uint16_t i;
    // clear data memory pixel map
    for (i = 0; i < 1024; i++)
        pixelMap[i] = 0;
    // copy to display
    refreshGraphicsLcd();
}




void initGraphicsLcd()
{
    sendGraphicsLcdCommand(0x40); // set start line to 0
    sendGraphicsLcdCommand(0xA1); // reverse horizontal order
    sendGraphicsLcdCommand(0xC0); // normal vertical order
    sendGraphicsLcdCommand(0xA6); // normal pixel polarity
    sendGraphicsLcdCommand(0xA3); // set led bias to 1/9 (should be A2)
    sendGraphicsLcdCommand(0x2F); // turn on voltage booster and regulator
    sendGraphicsLcdCommand(0xF8); // set internal volt booster to 4x Vdd
    sendGraphicsLcdCommand(0x00);
    sendGraphicsLcdCommand(0x27); // set contrast
    sendGraphicsLcdCommand(0x81); // set LCD drive voltage
    sendGraphicsLcdCommand(0x04);
    sendGraphicsLcdCommand(0xAC); // no flashing indicator
    sendGraphicsLcdCommand(0x00);
    clearGraphicsLcd();           // clear display
    sendGraphicsLcdCommand(0xAF); // display on
}

