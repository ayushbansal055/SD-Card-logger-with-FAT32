#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
/**
 * main.c
 */
uint8_t spi_txrx(uint8_t data) {
    // 1. Wait until the "Transmit FIFO Not Full" flag is set
    while((SSI0_SR_R & SSI_SR_TNF) == 0) {};

    // 2. Write the data to the Transmit FIFO
    SSI0_DR_R = data;

    // 3. Wait until the "Receive FIFO Not Empty" flag is set
    while((SSI0_SR_R & SSI_SR_RNE) == 0) {};

    // 4. Read and return the data from the Receive FIFO
    return SSI0_DR_R;
}

/**
 * @brief Sets the Chip Select (CS) pin (PA3) LOW
 */
void cs_low(void) {
    GPIO_PORTA_DATA_R &= ~(1<<3);
}

/**
 * @brief Sets the Chip Select (CS) pin (PA3) HIGH
 */
void cs_high(void) {
    GPIO_PORTA_DATA_R |= (1<<3);
}

void portf_init(void){
    SYSCTL_RCGC2_R |=   0x00000020;      // ENABLE CLOCK TO GPIOF
    GPIO_PORTF_LOCK_R = 0x4C4F434B;      // UNLOCK COMMIT REGISTER
    GPIO_PORTF_CR_R   = 0x1F;            // MAKE PORTF0 CONFIGURABLE
    GPIO_PORTF_DEN_R  = 0x1F;            // SET PORTF PINS 4 PIN
    GPIO_PORTF_DIR_R  = 0x0E;            // SET PORTF4 PIN AS INPUT USER SWITCH PIN
    GPIO_PORTF_PUR_R  = 0x11;            // PORTF4 IS PULLED UP
}

void ssi_init(void){
        SYSCTL_RCGC2_R |=   0x00000001;      // ENABLE CLOCK TO GPIOA
        SYSCTL_RCGCSSI_R |= 0x00000001;      // Enable clock for SSI0 peripheral

        while((SYSCTL_PRGPIO_R & 0x00000001) == 0){};  //get peripherals ready
        while((SYSCTL_PRSSI_R  & 0x00000001) == 0) {};

        GPIO_PORTA_AFSEL_R |= 0x34;          // controlled by the alternate hardware function
        GPIO_PORTA_DEN_R |= 0x34;

        GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R & ~0x00FF0F00) | 0x00220200;

        GPIO_PORTA_DIR_R |= (1<<3); // Set PA3 as an output
        GPIO_PORTA_DEN_R |= (1<<3); // Enable digital for PA3

        GPIO_PORTA_DATA_R |= (1<<3);

        // 4. Configure SSI0 Peripheral (as SPI Master)
        // -----------------------------------------------------------------
        // Disable SSI0 before configuration
        SSI0_CR1_R = 0;

        SSI0_CC_R = 0x0;            // Use system clock (usually 16MHz)

        // Set clock prescaler (CPSDVSR). Must be an even number from 2 to 254.

        // This is a safe speed for initialization.
        SSI0_CPSR_R = 5;             // Use 10 for a 1.6MHz clock (16MHz / 10 = 1.6MHz).

        // Configure SSI0:
        // SCR=0 (no clock division)
        // SPH=0 (data captured on first clock edge)
        // SPO=0 (clock is steady low)
        // FRF=0 (Freescale SPI Format)
        // DSS=7 (8-bit data)
        SSI0_CR0_R = 0x00000007;

        // Enable SSI0
        SSI0_CR1_R |= (1<<1); // SSI_CR1_SSE (Enable bit)
}

int main(void)
{
    portf_init();
    ssi_init();

    // --- SD Card Initialization Sequence ---
        uint8_t i;
        uint8_t response = 0xFF; // Variable to store the card's response

        // 1. "Wake up" the card.
        // Send at least 74 clock cycles with CS HIGH.
        // We do this by sending 10 bytes of 0xFF (80 cycles).
        cs_high();
        for(i = 0; i < 10; i++) {
            spi_txrx(0xFF);
        }

        // 2. Select the card
        cs_low();

        // 3. Send CMD0 (Go Idle State)
        // CMD0 packet structure:
        // [Byte 1]: Command      -> 0x40 (01 | 000000)
        // [Byte 2-5]: Argument   -> 0x00000000
        // [Byte 6]: CRC          -> 0x95 (This is the pre-calculated CRC for CMD0)
        spi_txrx(0x40);
        spi_txrx(0x00);
        spi_txrx(0x00);
        spi_txrx(0x00);
        spi_txrx(0x00);
        spi_txrx(0x95);

        // 4. Read the response.
        // The card will send 0xFF while busy. We wait for a non-0xFF byte.
        // We'll give it a timeout (e.g., 10 tries) in case no card is present.
        for(i = 0; i < 10; i++) {
            response = spi_txrx(0xFF);
            if(response != 0xFF) {
                break; // We got a real response!
            }
        }

        // 5. Deselect the card
        cs_high();

        // --- Check the result ---
        if(response == 0x01) {
            // SUCCESS! The card is in IDLE STATE.
            // You can now toggle an LED (e.g., on Port F)
            GPIO_PORTF_DATA_R = 0X02;
        } else {
            // FAILED!
            // The response was 0xFF (timeout) or some other error.
            // Check your wiring.
        }



        while(1) {
            // Loop forever
        }
}


