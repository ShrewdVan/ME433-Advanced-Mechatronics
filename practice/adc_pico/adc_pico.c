#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

#define GPIO_WATCH_PIN 22
#define ADC_WATCH_PIN 0

volatile float vol = 0;

// Function used to initilize the led
int pico_led_init(void);
// Function used to set the status of led
void pico_set_led(bool led_on);

int main()
{
    // Initialization blocks

    // ADC Initialization block
    adc_init();
    adc_gpio_init(ADC_WATCH_PIN);
    adc_select_input(ADC_WATCH_PIN);

    // LED Initialization block
    stdio_init_all();
    int rc = pico_led_init();
    hard_assert(rc == PICO_OK);

    // GPIO Initialization block
    gpio_init(GPIO_WATCH_PIN);
    gpio_set_dir(22, GPIO_IN);
    gpio_pull_down(22);

    // variable used to accept user's required sample number
    int sample_number;

    // Wait till the usb port is open
    while (!stdio_usb_connected()) {
        ;
    }
    printf("Prot Connected!\r\n");

    // Check whether the GPIO watch pin is in pull down input mode               
    int volatile check_GPIO = gpio_is_pulled_down(22); 
    printf("Is gpio pulled down? %d\r\n",check_GPIO);

    // Default case: Turn on the LED
    if (check_GPIO == 1) {
        pico_set_led(1);
    }else{
        ;
    }

    // Wait until user hits the button
    while (!gpio_get(22)){
        ;
    } 
    
    // If everything works as expected, turn off the LED
    pico_set_led(0);
    printf("Main procedure starts here\r\n");

    // Start the loop
    while (1) {
        // Ask user for the required sample number
        printf("Enter the number of sample:");
        scanf("%d", &sample_number);
        printf("\r\n");

        // Printing loop
        for (int i = 0; i < sample_number; i++){
            // Read the ADC count
            volatile uint16_t result = adc_read();
            // Map the ADC count to voltage, 12 bits, which is 4096 in decimal, from 0 to 3.3V
            vol = ((float)result / 4096) * 3.3;
            // print back the voltage
            printf("Current voltage is %.2f\r\n", vol);
            // Set the printing frequency to be 100 Hz
            sleep_ms(10);
        }

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