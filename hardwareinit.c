/*
 * hardwareinit.c
 *
 *  Created on: Jan 31, 2018
 *      Author: nihit
 */

#include <stdint.h>
#include <stdbool.h>
#include "wait.h"

void setTimerMode()
{
    SYSCTL_RCGCWTIMER_R |= SYSCTL_RCGCWTIMER_R5;     // turn-on timer
    WTIMER5_CTL_R &= ~TIMER_CTL_TAEN;   // turn-off counter before reconfiguring
    WTIMER5_CFG_R = 4;                   // configure as 32-bit counter (A only)
    WTIMER5_TAMR_R = TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP | TIMER_TAMR_TACDIR; // configure for edge time mode, count up
    WTIMER5_CTL_R = TIMER_CTL_TAEVENT_POS; // measure time from positive edge to positive edge
    WTIMER5_IMR_R = TIMER_IMR_CAEIM;                 // turn-on interrupts
    WTIMER5_TAV_R = 0;                          // zero counter for first period
    //WTIMER5_CTL_R |= TIMER_CTL_TAEN;                 // turn-on counter
    NVIC_EN3_R |= 1 << (INT_WTIMER5A - 16 - 96); // turn-on interrupt 120 (WTIMER5A)
}


void initHw()
{

    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN
            | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);
    SYSCTL_GPIOHBCTL_R = 0;
    SYSCTL_RCGC2_R = SYSCTL_RCGC2_GPIOA | SYSCTL_RCGC2_GPIOB
            | SYSCTL_RCGC2_GPIOF | SYSCTL_RCGC2_GPIOE | SYSCTL_RCGC2_GPIOD
            | SYSCTL_RCGC2_GPIOC;

    SYSCTL_RCGC1_R = SYSCTL_RCGC1_COMP0;
    SYSCTL_RCGCACMP_R = SYSCTL_RCGCACMP_R0;

    // Configure LED and pushbutton pins
    GPIO_PORTF_DIR_R |= 0x0F; // bits 0,1,2,3 are outputs, other pins are inputs
    GPIO_PORTF_DR2R_R |= 0xFF; // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTF_DEN_R |= 0xFF;  // enable LEDs and pushbuttons
    GPIO_PORTF_PUR_R |= 0x11;  // enable internal pull-up for push button

    GPIO_PORTD_DIR_R |= 0x03;  // SETTING MEASLR AND MEASC AS OUTPUTS
    GPIO_PORTD_DR2R_R |= 0x03; // SET DRIVE STRENGTH TO 2mA
    GPIO_PORTD_DEN_R |= 0xFF;  // DIGITAL DATA ENABLE FOR MEASC MEASLR

    GPIO_PORTE_DIR_R |= 0x1A;  // SETTING (LOW/HIGH)SIDE,INTEGRATE AS OUTPUTS
    GPIO_PORTE_DR2R_R |= 0xFF; // DRIVE STRENGTH AS 2mA
    GPIO_PORTE_DEN_R |= 0xFF;  // DIGITAL DATA ENABLE

    // Configure A0 and ~CS for graphics LCD
    GPIO_PORTB_DIR_R |= 0x42;  // make bits 1 and 6 outputs
    GPIO_PORTB_DR2R_R |= 0x42; // set drive strength to 2mA
    GPIO_PORTB_DEN_R |= 0x42;  // enable bits 1 and 6 for digital

    GPIO_PORTE_DIR_R |= 0x04;  // make bits 1 and 6 outputs
    GPIO_PORTE_DR2R_R |= 0x04; // set drive strength to 2mA
    GPIO_PORTE_DEN_R |= 0x04;  // enable bits 1 and 6 for digital

    // Configure SSI2 pins for SPI configuration
    SYSCTL_RCGCSSI_R |= SYSCTL_RCGCSSI_R2;           // turn-on SSI2 clocking
    GPIO_PORTB_DIR_R |= 0x90;                       // make bits 4 and 7 outputs
    GPIO_PORTB_DR2R_R |= 0x90;                      // set drive strength to 2mA
    GPIO_PORTB_AFSEL_R |= 0x90; // select alternative functions for MOSI, SCLK pins
    GPIO_PORTB_PCTL_R = GPIO_PCTL_PB7_SSI2TX | GPIO_PCTL_PB4_SSI2CLK; // map alt fns to SSI2
    GPIO_PORTB_DEN_R |= 0x90;        // enable digital operation on TX, CLK pins
    GPIO_PORTB_PUR_R |= 0x10;                      // must be enabled when SPO=1

    // Configure the SSI2 as a SPI master, mode 3, 8bit operation, 1 MHz bit rate
    SSI2_CR1_R &= ~SSI_CR1_SSE;       // turn off SSI2 to allow re-configuration
    SSI2_CR1_R = 0;                                  // select master mode
    SSI2_CC_R = 0;                    // select system clock as the clock source
    SSI2_CPSR_R = 40;                  // set bit rate to 1 MHz (if SR=0 in CR0)
    SSI2_CR0_R = SSI_CR0_SPH | SSI_CR0_SPO | SSI_CR0_FRF_MOTO | SSI_CR0_DSS_8; // set SR=0, mode 3 (SPH=1, SPO=1), 8-bit
    SSI2_CR1_R |= SSI_CR1_SSE;                       // turn on SSI2

    GPIO_PORTC_DIR_R |= 0x00;        // SETTING COMPARATOR INPUT
    GPIO_PORTC_DEN_R &= ~0xC0;       //TURN OFF DIGTITAL DATA
    GPIO_PORTC_AMSEL_R |= 0xC0;      //TURN ON ANALOG DATA
    GPIO_PORTF_LOCK_R = 0x4C4F434B;  // UNLOCK PORTF FOR COMP OUTPUT
    GPIO_PORTF_CR_R = 0x00000001;    // COMMIT REGISTER
    GPIO_PORTF_DEN_R |= 0x01;       //TURN ON DIGITAL DATA FOR COMPARATOR OUTPUT
    GPIO_PORTC_AFSEL_R |= 0xC0;   //TURN ON ALTERNATE FUCNTION ON PORT C
    GPIO_PORTF_AFSEL_R |= 01;        //TURN ON ALTERNATE FUNC. FOR PORTF0
    GPIO_PORTF_PCTL_R |= GPIO_PCTL_PF0_C0O; //CHOOSE ALTERNATE FUNCTION
    COMP_ACREFCTL_R |= 0x0000020F;
    COMP_ACCTL0_R |= 0x00000402;
    waitMicrosecond(15);

    // Configure UART0 pins
    SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R0; // turn-on UART0, leave other uarts in same status
    GPIO_PORTA_DEN_R |= 3;                         // default, added for clarity
    GPIO_PORTA_AFSEL_R |= 3;                       // default, added for clarity
    GPIO_PORTA_PCTL_R = GPIO_PCTL_PA1_U0TX | GPIO_PCTL_PA0_U0RX;

    // Configure UART0 to 115200 baud, 8N1 format (must be 3 clocks from clock enable and config writes)
    UART0_CTL_R = 0;                 // turn-off UART0 to allow safe programming
    UART0_CC_R = UART_CC_CS_SYSCLK;                 // use system clock (40 MHz)
    UART0_IBRD_R = 21; // r = 40 MHz / (Nx115.2kHz), set floor(r)=21, where N=16
    UART0_FBRD_R = 45;                               // round(fract(r)*64)=45
    UART0_LCRH_R = UART_LCRH_WLEN_8 | UART_LCRH_FEN; // configure for 8N1 w/ 16-level FIFO
    UART0_CTL_R = UART_CTL_TXE | UART_CTL_RXE | UART_CTL_UARTEN; // enable TX, RX, and module

    // Configure AIN4 for Analog Input on PD3
    SYSCTL_RCGCADC_R |= 1;
    GPIO_PORTD_AFSEL_R |= 0x08;
    GPIO_PORTD_DEN_R &= ~0x08;          //Turn of digital operations.
    GPIO_PORTD_AMSEL_R |= 0x08;         //Turn on analog operations.
    ADC0_CC_R = ADC_CC_CS_SYSPLL;
    ADC0_ACTSS_R &= ~ADC_ACTSS_ASEN3;
    ADC0_EMUX_R = ADC_EMUX_EM3_PROCESSOR;
    ADC0_SSMUX3_R = 4;                           // Select first sample for AIN4
    ADC0_SSCTL3_R = ADC_SSCTL3_END0;             // mark first sample as the end
    ADC0_ACTSS_R |= ADC_ACTSS_ASEN3;                 // enable SS3 for operation

    // Configure FREQ_IN for frequency counter
    GPIO_PORTD_AFSEL_R |= 0x40;  // select alternative functions for FREQ_IN pin
    GPIO_PORTD_PCTL_R &= ~GPIO_PCTL_PD6_M;           // map alt fns to FREQ_IN
    GPIO_PORTD_PCTL_R |= GPIO_PCTL_PD6_WT5CCP0;
    GPIO_PORTD_DEN_R |= 0x40;                  // enable bit 6 for digital input

    // Configure Wide Timer 5 as counter
            setTimerMode();


    // Configure Timer 1 as the time base
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;       // turn-on timer
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;      // turn-off timer before reconfiguring
    TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;    // configure as 32-bit timer (A+B)
    TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD; // configure for periodic mode (count down)
    TIMER1_TAILR_R = 0x2625A00; // set load value to 40e6 for 1 Hz interrupt rate
    TIMER1_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts
    NVIC_EN0_R |= 1 << (INT_TIMER1A - 16);     // turn-on interrupt 37 (TIMER1A)
    TIMER1_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer

}

void setCounterMode()
{
    SYSCTL_RCGCWTIMER_R |= SYSCTL_RCGCWTIMER_R5;     // turn-on timer
    WTIMER5_CTL_R &= ~TIMER_CTL_TAEN;   // turn-off counter before reconfiguring
    WTIMER5_CFG_R = 4;                   // configure as 32-bit counter (A only)
    WTIMER5_TAMR_R = TIMER_TAMR_TAMR_CAP | TIMER_TAMR_TACDIR; // configure for edge count mode, count up
    WTIMER5_CTL_R = 0;                               //
    WTIMER5_IMR_R = 0;                               // turn-off interrupts
    WTIMER5_TAV_R = 0;                          // zero counter for first period
    WTIMER5_CTL_R |= TIMER_CTL_TAEN;                 // turn-on counter
    NVIC_EN3_R &= ~(1 << (INT_WTIMER5A - 16 - 96)); // turn-off interrupt 120 (WTIMER5A)
}




