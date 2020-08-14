#include <msp430g2553.h>
#include <intrinsics.h>
#include <stdint.h>
#include <stdbool.h>


/*
 * DEBUG_IDLE_BIT is toggled each cycle through the idle loop.
 * DEBUG_ADC_BIT is toggled each time the ADC interrupt is fired.
 * 
 * When the COMM_SAMPLE_HOLD_BIT is pulled to high, the next
 * conversion result from the ADC is saved and held until the 
 * COMM_SAMPLE_HOLD_BIT goes to low again.
 * 
 * When the COMM_ENABLE_BIT is pulled to high and an earlier ADC value 
 * is currently held, it is checked whether the current ADC value is
 * by SPACE_MARK_THRESHOLD greater then the held value.
 * If it is greater, the COMM_RESULT_BIT is set to high, otherwise
 * it is set to low.
 * 
 * If either the COMM_ENABLE_BIT or the COMM_SAMPLE_HOLD_BIT goes to 
 * low the COMM_RESULT_BIT is set to low.
 */

#define DEBUG

#define DEBUG_PORT P1OUT
#define DEBUG_PORT_DIR P1DIR
#define DEBUG_IDLE_BIT BIT7
#define DEBUG_ADC_BIT BIT6

#define COMM_PORT_OUT P2OUT
#define COMM_PORT_IN P2IN
#define COMM_PORT_DIR P2DIR
#define COMM_SAMPLE_HOLD_BIT BIT3
#define COMM_ENABLE_BIT BIT4
#define COMM_RESULT_BIT BIT2

#define SPACE_MARK_THRESHOLD 100


// ISR to read and process result from adc
__attribute__((interrupt(ADC10_VECTOR)))
void adcIsr(void)
{
    static uint16_t holdValue = 0;
    static bool holdFlag = false;

    uint16_t currentValue = ADC10MEM;

    if ((COMM_PORT_IN & COMM_SAMPLE_HOLD_BIT)) {
        if (! holdFlag) {
            holdValue = currentValue;
            holdFlag = true;
        }
    } else {
        holdFlag = false;
    }

    if ((COMM_PORT_IN & COMM_ENABLE_BIT) && holdFlag) {
        if (currentValue > (holdValue + SPACE_MARK_THRESHOLD)) {
            COMM_PORT_OUT |= COMM_RESULT_BIT;
        } else {
            COMM_PORT_OUT &= ~COMM_RESULT_BIT;
        }
    } else {
        COMM_PORT_OUT &= ~COMM_RESULT_BIT;
    }

#ifdef DEBUG
    DEBUG_PORT ^= DEBUG_ADC_BIT;
#endif // DEBUG
}


void setup() {
    WDTCTL = WDTPW | WDTHOLD;

    __disable_interrupt();

    // highest possible system clock
    DCOCTL = DCO0 | DCO1 | DCO2;
    BCSCTL1 = XT2OFF | RSEL0 | RSEL1 | RSEL2 | RSEL3;
    BCSCTL2 = 0;
    BCSCTL3 = 0;

    // debug port configuration, obviously
#ifdef DEBUG    
	DEBUG_PORT_DIR |= DEBUG_IDLE_BIT | DEBUG_ADC_BIT;
	DEBUG_PORT &= ~(DEBUG_IDLE_BIT | DEBUG_ADC_BIT);
#endif // DEBUG

    // communication
    COMM_PORT_DIR |= COMM_RESULT_BIT;
    COMM_PORT_DIR &= ~(COMM_ENABLE_BIT | COMM_SAMPLE_HOLD_BIT);
    COMM_PORT_OUT &= COMM_RESULT_BIT;

    // adc
    ADC10CTL0 = SREF1 | REFON | ADC10ON | ADC10IE | MSC;
    ADC10CTL1 = INCH_3 | CONSEQ_2;
    ADC10AE0 = BIT3;

    __enable_interrupt();

    // start the adc
    ADC10CTL0 |= ENC | ADC10SC;    
}


void loop() {
#ifdef DEBUG    
    DEBUG_PORT ^= DEBUG_IDLE_BIT;
#endif // DEBUG
}


int main() {
    setup();

    while (1) {
        loop();
    }
}
