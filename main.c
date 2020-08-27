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
 * While the COMM_SAMPLE_HOLD_BIT is pulled to high  it is checked 
 * whether the current ADC value is by SPACE_MARK_THRESHOLD greater 
 * then the held value. If it is greater, the COMM_RESULT_BIT is set 
 * to low, otherwise it is set to high.
 * 
 * If COMM_SAMPLE_HOLD_BIT goes to low the COMM_RESULT_BIT is set to
 * high.
 */



#define DEBUG


// ADC Channel 3: P1.3

#define DEBUG_PORT P1OUT
#define DEBUG_PORT_DIR P1DIR
#define DEBUG_IDLE_BIT BIT7          // P1.7
#define DEBUG_ADC_BIT BIT6           // P1.6

#define COMM_PORT_OUT P2OUT
#define COMM_PORT_IN P2IN
#define COMM_PORT_REN P2REN
#define COMM_PORT_DIR P2DIR
#define COMM_RESULT_BIT BIT2         // P2.2
#define COMM_SAMPLE_HOLD_BIT BIT3    // P2.3

// derived from the calculation in mbus-converter.ods
// assumes a 25Ohm shunt resistor and a 10mA swing
#define SPACE_MARK_THRESHOLD 100

// SPI debug codes
#define DBG_START      0xffffffff
#define DBG_HOLD       0x01000000
#define DBG_SAMPLE     0x02000000
#define DBG_RX_SLOPE   0x04000000


// 32bit-value to transmit via SPI
volatile uint32_t spiTxBuf = 0;

// ISR for SPI transmitter
__attribute__((interrupt(USCIAB0TX_VECTOR)))
void spiTxIsr(void) {
    static uint8_t oc = 0;
    oc++;
    if (oc < 4) {
        UCA0TXBUF = (uint8_t)((spiTxBuf >> (8 * oc)) & 0x00ff);
    } else {
        IE2 &= ~UCA0TXIE;
        oc = 0;
    }
}

void writeSpi(uint32_t m) {
    spiTxBuf = m;
    UCA0TXBUF = (uint8_t)(spiTxBuf & 0x00ff);
    IE2 |= UCA0TXIE;
}



// ISR to read and process result from adc
__attribute__((interrupt(ADC10_VECTOR)))
void adcIsr(void) {
    static uint16_t holdValue = 0;
    static bool holdFlag = false;

    uint16_t currentValue = ADC10MEM;

    if ((COMM_PORT_IN & COMM_SAMPLE_HOLD_BIT)) {
        if (! holdFlag) {
            holdValue = currentValue;
            holdFlag = true;
            writeSpi(DBG_HOLD | holdValue);
        } else {
            if (currentValue > (holdValue + SPACE_MARK_THRESHOLD)) {
                COMM_PORT_OUT &= ~COMM_RESULT_BIT;
                // writeSpi(DBG_RX_SLOPE | currentValue);
            } else {
                COMM_PORT_OUT |= COMM_RESULT_BIT;
            }
        }
    } else {
        if (holdFlag) {
            holdFlag = false;
            COMM_PORT_OUT |= COMM_RESULT_BIT;
            writeSpi(DBG_SAMPLE);
        }
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
    COMM_PORT_DIR &= ~COMM_SAMPLE_HOLD_BIT;
    COMM_PORT_REN |= COMM_SAMPLE_HOLD_BIT;
    COMM_PORT_OUT &= ~COMM_SAMPLE_HOLD_BIT;
    COMM_PORT_OUT |= COMM_RESULT_BIT;

    // adc
    ADC10CTL0 = SREF_1 | REFON | ADC10ON | ADC10IE | MSC;
    ADC10CTL1 = INCH_3 | CONSEQ_2;
    ADC10AE0 = BIT3;

    // spi
    UCA0CTL1 = UCSWRST | UCSSEL_2;
    UCA0CTL0 = UCMST | UCSYNC;
    UCA0BR0 = 8;
    UCA0BR1 = 0;
    P1SEL |= BIT1 | BIT2 | BIT4;
    P1SEL2 |= BIT1 | BIT2 | BIT4;

    UCA0CTL1 &= ~UCSWRST;

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

    writeSpi(DBG_START);

    while (1) {
        loop();
    }
}
