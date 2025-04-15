#include <stdio.h>
#include <math.h> 
#include "pico/stdlib.h"
#include "hardware/spi.h"

// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19
#define GPIO_WATCH_PIN 20

// Variable used to send the data to the Dac
uint8_t data[2];
// Length of the data
int len = 2;
// Counter for A sinwave, and B triangle wave
int counter_A = 1, counter_B = 1;


// Function used to initilize the led
int pico_led_init(void);
// Function used to set the status of led
void pico_set_led(bool led_on);

// Function used to manipulate the CSn pin
static inline void cs_select() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PIN_CS, 0);  // Active low
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PIN_CS, 1);  // Active high
    asm volatile("nop \n nop \n nop");
}

// Function used to Write Data to the Dac
void WriteDac();
// Function including updating calculation and format transformation for channel A
void Update_Data_A();
// Function including updating calculation and format transformation for channel B
void Update_Data_B();


// Main Function starts here
int main()
{

    // LED Initialization block
    stdio_init_all();

    int rc = pico_led_init();
    hard_assert(rc == PICO_OK);

    // GPIO Initialization block
    gpio_init(GPIO_WATCH_PIN);
    gpio_set_dir(20, GPIO_OUT);

    // SPI initialisation. This example will use SPI at 1MHz.
    spi_init(SPI_PORT, 1000*1000);
    gpio_set_function(PIN_MISO,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_CS,   GPIO_FUNC_SIO);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    

    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    // Check whether the port is open
    while (!stdio_usb_connected()) {
        ;
    }
    sleep_ms(100);
    printf("Start\r\n");

    // Drag down the LDAC pin to enable the auto-filled to the channel register
    gpio_put(GPIO_WATCH_PIN, 0);
    // Turn on the led as a flag of "Everything works fine"
    pico_set_led(1);

    // Infinite while loop
    while (true) {
        Update_Data_A();
        WriteDac();
        counter_A ++;
        printf("Channel A completes Writing\r\n");

        Update_Data_B();
        WriteDac();
        counter_B ++;
        printf("Channel B completes Writing\r\n");
        // Set the frequency to be 100 times of the sin signal wave frequency and 200 times of the triangle wave frequency
        // Therefore the delay for each iteration should be 1/200 = 5 ms
        sleep_ms(5);
        
    }
}




int pico_led_init(void) {
    // A device like Pico that uses a GPIO for the LED will define PICO_DEFAULT_LED_PIN
    // so we can use normal GPIO functionality to turn the led on and off
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return PICO_OK;
}

// Turn the led on or off
void pico_set_led(bool led_on) {
    // Just set the GPIO on or off
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);
}

void WriteDac(){
    cs_select();
    spi_write_blocking(SPI_PORT, data, len); 
    cs_deselect();
}

// Function used to calculate the sin wave and related data updating manipulation
void Update_Data_A(){
    // There are 200 sampling points from 0 to 1. Therefore 100 points for each period
    // Once the counter hits the boundary, reset it to the origin
    if (counter_A == 101){
        counter_A = 1;
    }
    // Update block for the channel A
    uint16_t number_A = 512 + 511*sin(4*M_PI*(float)counter_A/200);
    // Take the bits 10 - 7 from the result which is in a format of uint16_t, and set it to be the least 4 bits of the data[0]
    uint8_t number_A_1 = (number_A >> 6) & 0x0F;
    // Take the bits 6 - 0 from the result which is in a format of uint16_t, and set it to be the most 6 bits of the data[1]
    uint8_t number_A_2 = (number_A & 0x3F) << 2;
    data[0] = number_A_1;
    // Edit the 4 most significant bits to meet the format of channel A
    data[0] |= (0b0111<<4);
    data[1] = number_A_2;
}

void Update_Data_B(){
    // There are 200 sampling points from 0 to 1. 200 for each period
    // Once the counter hits the boundary, reset it to the origin
    if (counter_B == 201){
        counter_B = 1;
    }

    // update block for the channel B
    uint16_t number_B;
    // At the first half, increase the output voltage from 0 to 3.3 with 100 points
    if (counter_B < 100){
        number_B = (uint16_t)(10.23f * counter_B);
    }else{
    // At the second half, decrease the output voltage from 3.3 to 0 with 100 points    
        number_B = (uint16_t)(-10.23f * (counter_B - 100) + 1023);
    } 
    // Take the bits 10 - 7 from the result which is in a format of uint16_t, and set it to be the least 4 bits of the data[0]
    uint8_t number_B_1 = (number_B >> 6) & 0x0F;
    // Take the bits 6 - 0 from the result which is in a format of uint16_t, and set it to be the most 6 bits of the data[1]
    uint8_t number_B_2 = (number_B & 0x3F) << 2;
    data[0] = number_B_1;
    // Edit the 4 most significant bits to meet the format of channel B
    data[0] |= (0b1111<<4);
    data[1] = number_B_2;

}

    

