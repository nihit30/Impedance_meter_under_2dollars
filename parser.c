/*
 * parser.c
 *
 *  Created on: Jan 31, 2018
 *      Author: nihit
 */

#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>


char ArmRxBuffer[81];
uint8_t field_posi[10];
char field_type[10];
uint8_t counter = 0;



char getcUart0()
{
    while (UART0_FR_R & UART_FR_RXFE)
        ;
    return UART0_DR_R & 0xFF;
}

void Refresh()                    // SET ALL ARRAYS AND VARIABLES TO ZERO
{
    //memset(ParsedString1, 0, 10);
   // memset(strpin1, 0, 20);
   // memset(strpin2, 0, 20);
    memset(ArmRxBuffer, 0, 81);
    memset(field_posi, 0, 10);
    memset(field_type, 0, 10);
    counter = 0;

}

bool iscommand(char match[], int ArgC)     //COMPARE THE INPUT STRING WITH ARGUMENTS
{

    return ((strcmp(match, &ArmRxBuffer[field_posi[0]])) == 0
            && (ArgC == counter - 1)); //returns true if equal

}


void getInput()
{
    uint16_t i = 0;
    {
        // accepting characters from 32 to 127 including delimiters
        for (i = 0; i < 81;)
        {
            ArmRxBuffer[i] = getcUart0();

            if (ArmRxBuffer[i] == 8)              // If Backspace,decrement buffer variable by 1
            {
                if (i != 0)
                {

                    i -= 1;
                }
            }

            else if (ArmRxBuffer[i] == 13)        // Enter as NULL character
            {
                ArmRxBuffer[i] = '\0';

                break;
            }

            else if (ArmRxBuffer[i] > 31 && ArmRxBuffer[i] < 127) //Store all characters
            {

                i += 1;
                continue;
            }
        }
    }

}

void ProcessString()
{
    uint16_t i = 0;
    {
        for (i = 0; i < 81; i++)        // FOR 81 CHARACTERS
        {
            if ((ArmRxBuffer[i] >= 32 && ArmRxBuffer[i] <= 45) //converting delims to null
            || (ArmRxBuffer[i] >= 58 && ArmRxBuffer[i] <= 64)
                    || (ArmRxBuffer[i] >= 123 && ArmRxBuffer[i] <= 126))
            {

                ArmRxBuffer[i] = 0;
            }

            if (ArmRxBuffer[i] >= 'A' && ArmRxBuffer[i] <= 'Z') //coverting uppercase letters to lowercase

            {
                ArmRxBuffer[i] = tolower(ArmRxBuffer[i]);

            }

        }
    }
}

void parser()           // THIS LOOP COMPARES PRESENT AND FUTURE SAMPLES
{
    uint16_t i = 0;

    for (i = 0; i < 81; i++)
    {

        if (ArmRxBuffer[i] == 0 && ArmRxBuffer[i + 1] >= 'a'
                 && ArmRxBuffer[i + 1] <= 'z')
                 //if First character is NULL and then Alphabets

        {
            field_type[counter] = 'a';
            field_posi[counter] = i + 1;
            counter += 1;
        }

        if (ArmRxBuffer[i] >= 'a' && ArmRxBuffer[i] <= 'z' && counter == 0) // if first charac is Alphabet

        {
            field_type[counter] = 'a';
            field_posi[counter] = i;
            counter += 1;
        }

        if (ArmRxBuffer[i + 1] >= '0' && ArmRxBuffer[i + 1] <= '9' // if First charac is NULL and then numerics
        && ArmRxBuffer[i] == 0)
        {
            field_type[counter] = 'n';
            field_posi[counter] = i + 1;
            counter += 1;
        }

    }

}

